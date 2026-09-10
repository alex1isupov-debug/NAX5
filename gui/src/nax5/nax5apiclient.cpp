#include "nax5/nax5apiclient.h"
#include "nax5/nax5apiconfig.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

Q_LOGGING_CATEGORY(nax5Api, "nax5.api")

Nax5ApiClient::Nax5ApiClient(QObject *parent)
    : QObject(parent)
    , network(new QNetworkAccessManager(this))
    , next_request_id(1)
    , active_request_id(0)
    , active_kind(RequestNone)
{
}

Nax5ApiClient::~Nax5ApiClient()
{
    abortAll();
}

void Nax5ApiClient::abortAll()
{
    if (!active_reply)
        return;
    QNetworkReply *reply = active_reply.data();
    active_reply.clear();
    active_request_id = 0;
    active_kind = RequestNone;
    reply->abort();
    reply->deleteLater();
}

QNetworkReply *Nax5ApiClient::sendJson(const QString &method, const QString &path, const QByteArray &body, const QString &session_token, const QByteArray &idempotency_key)
{
    abortAll();

    QString config_error;
    const QUrl base = Nax5ApiConfig::baseUrl(&config_error);
    if (!base.isValid())
        return nullptr;

    QUrl url = base.resolved(QUrl(path));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setHeader(QNetworkRequest::UserAgentHeader, Nax5ApiConfig::userAgent());
    request.setTransferTimeout(Nax5ApiConfig::requestTimeoutMs());
    if (!session_token.isEmpty())
        request.setRawHeader("X-Session-Token", session_token.toUtf8());
    if (!idempotency_key.isEmpty())
        request.setRawHeader("Idempotency-Key", idempotency_key);

    QNetworkReply *reply = nullptr;
    if (method == QLatin1String("POST"))
        reply = network->post(request, body);
    else if (method == QLatin1String("DELETE"))
        reply = network->sendCustomRequest(request, "DELETE", body.isEmpty() ? QByteArray("{}") : body);
    else
        reply = network->get(request);

    qCInfo(nax5Api) << method << path;
    return reply;
}

bool Nax5ApiClient::isNoNetwork(QNetworkReply *reply)
{
    const QNetworkReply::NetworkError error = reply->error();
    return error == QNetworkReply::HostNotFoundError
        || error == QNetworkReply::ConnectionRefusedError
        || error == QNetworkReply::RemoteHostClosedError
        || error == QNetworkReply::TemporaryNetworkFailureError
        || error == QNetworkReply::NetworkSessionFailedError
        || error == QNetworkReply::UnknownNetworkError
        || error == QNetworkReply::TimeoutError
        || error == QNetworkReply::OperationCanceledError;
}

quint64 Nax5ApiClient::login(const QString &email, const QString &password)
{
    const quint64 request_id = next_request_id++;
    QJsonObject payload;
    payload.insert(QStringLiteral("email"), email);
    payload.insert(QStringLiteral("password"), password);
    QByteArray body = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    payload.insert(QStringLiteral("password"), QString());

    QNetworkReply *reply = sendJson(QStringLiteral("POST"), QStringLiteral("/_allauth/app/v1/auth/login"), body, QString());
    body.fill(' ');
    if (!reply)
    {
        Nax5LoginParseResult result;
        result.error = Nax5AuthErrorServerError;
        emit loginFinished(request_id, result);
        return request_id;
    }

    active_reply = reply;
    active_request_id = request_id;
    active_kind = RequestLogin;
    connect(reply, &QNetworkReply::finished, this, [this, request_id, reply]() {
        finishLogin(request_id, reply);
    });
    return request_id;
}

quint64 Nax5ApiClient::fetchMe(const QString &session_token)
{
    const quint64 request_id = next_request_id++;
    QNetworkReply *reply = sendJson(QStringLiteral("GET"), QStringLiteral("/api/v1/auth/me/"), QByteArray(), session_token);
    if (!reply)
    {
        emit meFinished(request_id, Nax5MeParseResult());
        return request_id;
    }
    active_reply = reply;
    active_request_id = request_id;
    active_kind = RequestMe;
    connect(reply, &QNetworkReply::finished, this, [this, request_id, reply]() {
        finishMe(request_id, reply);
    });
    return request_id;
}

quint64 Nax5ApiClient::logout(const QString &session_token)
{
    const quint64 request_id = next_request_id++;
    QNetworkReply *reply = sendJson(QStringLiteral("DELETE"), QStringLiteral("/_allauth/app/v1/auth/session"), QByteArray(), session_token);
    if (!reply)
    {
        emit logoutFinished(request_id);
        return request_id;
    }
    active_reply = reply;
    active_request_id = request_id;
    active_kind = RequestLogout;
    connect(reply, &QNetworkReply::finished, this, [this, request_id, reply]() {
        finishLogout(request_id, reply);
    });
    return request_id;
}

void Nax5ApiClient::finishLogin(quint64 request_id, QNetworkReply *reply)
{
    if (active_request_id != request_id)
    {
        reply->deleteLater();
        return;
    }
    active_reply.clear();
    active_request_id = 0;
    active_kind = RequestNone;

    Nax5LoginParseResult result;
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError && status == 0)
    {
        result.error = nax5MapNetworkFailure(status, reply->error() == QNetworkReply::OperationCanceledError || reply->error() == QNetworkReply::TimeoutError, isNoNetwork(reply));
        qCWarning(nax5Api) << "login failed: network";
        emit loginFinished(request_id, result);
        reply->deleteLater();
        return;
    }

    result = nax5ParseLoginResponse(status, reply->readAll());
    if (result.error != Nax5AuthErrorNone)
        qCWarning(nax5Api) << "login failed:" << result.error;
    emit loginFinished(request_id, result);
    reply->deleteLater();
}

void Nax5ApiClient::finishMe(quint64 request_id, QNetworkReply *reply)
{
    if (active_request_id != request_id)
    {
        reply->deleteLater();
        return;
    }
    active_reply.clear();
    active_request_id = 0;
    active_kind = RequestNone;

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    Nax5MeParseResult result;
    if (reply->error() != QNetworkReply::NoError && status == 0)
        result.network_failure = true;
    else
        result = nax5ParseMeResponse(status, reply->readAll());
    emit meFinished(request_id, result);
    reply->deleteLater();
}

void Nax5ApiClient::finishLogout(quint64 request_id, QNetworkReply *reply)
{
    if (active_request_id != request_id)
    {
        reply->deleteLater();
        return;
    }
    active_reply.clear();
    active_request_id = 0;
    active_kind = RequestNone;
    emit logoutFinished(request_id);
    reply->deleteLater();
}

quint64 Nax5ApiClient::reserveSession(const QString &session_token, const QString &idempotency_key)
{
    const quint64 request_id = next_request_id++;
    QNetworkReply *reply = sendJson(
        QStringLiteral("POST"),
        QStringLiteral("/api/v1/sessions/reserve/"),
        QByteArray("{}"),
        session_token,
        idempotency_key.toUtf8());
    if (!reply)
    {
        Nax5SessionParseResult result;
        result.error = Nax5SessionErrorServerError;
        emit reserveFinished(request_id, result);
        return request_id;
    }
    active_reply = reply;
    active_request_id = request_id;
    active_kind = RequestReserve;
    connect(reply, &QNetworkReply::finished, this, [this, request_id, reply]() {
        finishReserve(request_id, reply);
    });
    return request_id;
}

quint64 Nax5ApiClient::fetchCurrentSession(const QString &session_token)
{
    const quint64 request_id = next_request_id++;
    QNetworkReply *reply = sendJson(QStringLiteral("GET"), QStringLiteral("/api/v1/sessions/current/"), QByteArray(), session_token);
    if (!reply)
    {
        emit currentFinished(request_id, Nax5SessionParseResult());
        return request_id;
    }
    active_reply = reply;
    active_request_id = request_id;
    active_kind = RequestCurrent;
    connect(reply, &QNetworkReply::finished, this, [this, request_id, reply]() {
        finishCurrent(request_id, reply);
    });
    return request_id;
}

quint64 Nax5ApiClient::cancelSession(const QString &session_token, const QString &public_session_id)
{
    const quint64 request_id = next_request_id++;
    const QString path = QStringLiteral("/api/v1/sessions/%1/cancel/").arg(public_session_id);
    QNetworkReply *reply = sendJson(QStringLiteral("POST"), path, QByteArray("{}"), session_token);
    if (!reply)
    {
        Nax5SessionParseResult result;
        result.error = Nax5SessionErrorServerError;
        emit cancelFinished(request_id, result);
        return request_id;
    }
    active_reply = reply;
    active_request_id = request_id;
    active_kind = RequestCancel;
    connect(reply, &QNetworkReply::finished, this, [this, request_id, reply]() {
        finishCancel(request_id, reply);
    });
    return request_id;
}

Nax5SessionParseResult Nax5ApiClient::finishSessionNetwork(QNetworkReply *reply, bool *used_body)
{
    Nax5SessionParseResult result;
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError && status == 0)
    {
        result.error = nax5MapSessionHttpError(status, reply->error() == QNetworkReply::OperationCanceledError || reply->error() == QNetworkReply::TimeoutError, isNoNetwork(reply));
        *used_body = false;
        return result;
    }
    *used_body = true;
    result.error = Nax5SessionErrorNone;
    return result;
}

void Nax5ApiClient::finishReserve(quint64 request_id, QNetworkReply *reply)
{
    if (active_request_id != request_id)
    {
        reply->deleteLater();
        return;
    }
    active_reply.clear();
    active_request_id = 0;
    active_kind = RequestNone;

    bool used_body = false;
    Nax5SessionParseResult result = finishSessionNetwork(reply, &used_body);
    if (used_body)
        result = nax5ParseReserveResponse(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), reply->readAll());
    if (result.error != Nax5SessionErrorNone)
        qCWarning(nax5Api) << "reserve failed:" << result.error;
    else
        qCInfo(nax5Api) << "reserve ok" << result.session.id << result.console.code;
    emit reserveFinished(request_id, result);
    reply->deleteLater();
}

void Nax5ApiClient::finishCurrent(quint64 request_id, QNetworkReply *reply)
{
    if (active_request_id != request_id)
    {
        reply->deleteLater();
        return;
    }
    active_reply.clear();
    active_request_id = 0;
    active_kind = RequestNone;

    bool used_body = false;
    Nax5SessionParseResult result = finishSessionNetwork(reply, &used_body);
    if (used_body)
        result = nax5ParseCurrentResponse(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), reply->readAll());
    emit currentFinished(request_id, result);
    reply->deleteLater();
}

void Nax5ApiClient::finishCancel(quint64 request_id, QNetworkReply *reply)
{
    if (active_request_id != request_id)
    {
        reply->deleteLater();
        return;
    }
    active_reply.clear();
    active_request_id = 0;
    active_kind = RequestNone;

    bool used_body = false;
    Nax5SessionParseResult result = finishSessionNetwork(reply, &used_body);
    if (used_body)
        result = nax5ParseCancelResponse(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), reply->readAll());
    if (result.error == Nax5SessionErrorNone)
        qCInfo(nax5Api) << "cancel ok" << result.session.id;
    else
        qCWarning(nax5Api) << "cancel failed:" << result.error;
    emit cancelFinished(request_id, result);
    reply->deleteLater();
}

quint64 Nax5ApiClient::fetchConnection(const QString &session_token, const QString &public_session_id)
{
    const quint64 request_id = next_request_id++;
    const QString path = QStringLiteral("/api/v1/sessions/%1/connection/").arg(public_session_id);
    QNetworkReply *reply = sendJson(QStringLiteral("POST"), path, QByteArray("{}"), session_token);
    if (!reply)
    {
        Nax5ConnectionParseResult result;
        result.error = Nax5SessionErrorServerError;
        emit connectionFinished(request_id, result);
        return request_id;
    }
    active_reply = reply;
    active_request_id = request_id;
    active_kind = RequestConnection;
    connect(reply, &QNetworkReply::finished, this, [this, request_id, reply]() {
        finishConnection(request_id, reply);
    });
    return request_id;
}

quint64 Nax5ApiClient::markConnected(const QString &session_token, const QString &public_session_id)
{
    const quint64 request_id = next_request_id++;
    const QString path = QStringLiteral("/api/v1/sessions/%1/connected/").arg(public_session_id);
    QNetworkReply *reply = sendJson(QStringLiteral("POST"), path, QByteArray("{}"), session_token);
    if (!reply)
    {
        Nax5SessionParseResult result;
        result.error = Nax5SessionErrorServerError;
        emit connectedFinished(request_id, result);
        return request_id;
    }
    active_reply = reply;
    active_request_id = request_id;
    active_kind = RequestConnected;
    connect(reply, &QNetworkReply::finished, this, [this, request_id, reply]() {
        finishConnected(request_id, reply);
    });
    return request_id;
}

quint64 Nax5ApiClient::failSession(const QString &session_token, const QString &public_session_id)
{
    const quint64 request_id = next_request_id++;
    const QString path = QStringLiteral("/api/v1/sessions/%1/fail/").arg(public_session_id);
    QNetworkReply *reply = sendJson(QStringLiteral("POST"), path, QByteArray("{}"), session_token);
    if (!reply)
    {
        Nax5SessionParseResult result;
        result.error = Nax5SessionErrorServerError;
        emit failFinished(request_id, result);
        return request_id;
    }
    active_reply = reply;
    active_request_id = request_id;
    active_kind = RequestFail;
    connect(reply, &QNetworkReply::finished, this, [this, request_id, reply]() {
        finishFail(request_id, reply);
    });
    return request_id;
}

quint64 Nax5ApiClient::endSession(const QString &session_token, const QString &public_session_id)
{
    const quint64 request_id = next_request_id++;
    const QString path = QStringLiteral("/api/v1/sessions/%1/end/").arg(public_session_id);
    QNetworkReply *reply = sendJson(QStringLiteral("POST"), path, QByteArray("{}"), session_token);
    if (!reply)
    {
        Nax5SessionParseResult result;
        result.error = Nax5SessionErrorServerError;
        emit endFinished(request_id, result);
        return request_id;
    }
    active_reply = reply;
    active_request_id = request_id;
    active_kind = RequestEnd;
    connect(reply, &QNetworkReply::finished, this, [this, request_id, reply]() {
        finishEnd(request_id, reply);
    });
    return request_id;
}

quint64 Nax5ApiClient::operatorProvision(const QString &session_token, const QString &console_code, const QByteArray &body)
{
    const quint64 request_id = next_request_id++;
    const QString path = QStringLiteral("/api/v1/operator/consoles/%1/provision/").arg(console_code);
    QNetworkReply *reply = sendJson(QStringLiteral("POST"), path, body, session_token);
    if (!reply)
    {
        Nax5ConnectionParseResult result;
        result.error = Nax5SessionErrorServerError;
        emit operatorFinished(request_id, result);
        return request_id;
    }
    active_reply = reply;
    active_request_id = request_id;
    active_kind = RequestOperator;
    connect(reply, &QNetworkReply::finished, this, [this, request_id, reply]() {
        finishOperator(request_id, reply);
    });
    return request_id;
}

quint64 Nax5ApiClient::operatorActivate(const QString &session_token, const QString &console_code)
{
    const quint64 request_id = next_request_id++;
    const QString path = QStringLiteral("/api/v1/operator/consoles/%1/activate/").arg(console_code);
    QNetworkReply *reply = sendJson(QStringLiteral("POST"), path, QByteArray("{}"), session_token);
    if (!reply)
    {
        Nax5ConnectionParseResult result;
        result.error = Nax5SessionErrorServerError;
        emit operatorFinished(request_id, result);
        return request_id;
    }
    active_reply = reply;
    active_request_id = request_id;
    active_kind = RequestOperator;
    connect(reply, &QNetworkReply::finished, this, [this, request_id, reply]() {
        finishOperator(request_id, reply);
    });
    return request_id;
}

quint64 Nax5ApiClient::operatorTestConnection(const QString &session_token, const QString &console_code)
{
    const quint64 request_id = next_request_id++;
    const QString path = QStringLiteral("/api/v1/operator/consoles/%1/test-connection/").arg(console_code);
    QNetworkReply *reply = sendJson(QStringLiteral("POST"), path, QByteArray("{}"), session_token);
    if (!reply)
    {
        Nax5ConnectionParseResult result;
        result.error = Nax5SessionErrorServerError;
        emit connectionFinished(request_id, result);
        return request_id;
    }
    active_reply = reply;
    active_request_id = request_id;
    active_kind = RequestConnection;
    connect(reply, &QNetworkReply::finished, this, [this, request_id, reply]() {
        finishConnection(request_id, reply);
    });
    return request_id;
}

quint64 Nax5ApiClient::operatorTestResult(const QString &session_token, const QString &console_code, const QByteArray &body)
{
    const quint64 request_id = next_request_id++;
    const QString path = QStringLiteral("/api/v1/operator/consoles/%1/test-result/").arg(console_code);
    QNetworkReply *reply = sendJson(QStringLiteral("POST"), path, body, session_token);
    if (!reply)
    {
        Nax5ConnectionParseResult result;
        result.error = Nax5SessionErrorServerError;
        emit operatorFinished(request_id, result);
        return request_id;
    }
    active_reply = reply;
    active_request_id = request_id;
    active_kind = RequestOperator;
    connect(reply, &QNetworkReply::finished, this, [this, request_id, reply]() {
        finishOperator(request_id, reply);
    });
    return request_id;
}

void Nax5ApiClient::finishConnection(quint64 request_id, QNetworkReply *reply)
{
    if (active_request_id != request_id)
    {
        reply->deleteLater();
        return;
    }
    active_reply.clear();
    active_request_id = 0;
    active_kind = RequestNone;
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();
    Nax5ConnectionParseResult result;
    if (reply->error() != QNetworkReply::NoError && status == 0)
        result.error = Nax5SessionErrorNetworkError;
    else
        result = nax5ParseConnectionResponse(status, body);
    qCInfo(nax5Api) << "connection" << status;
    emit connectionFinished(request_id, result);
    reply->deleteLater();
}

void Nax5ApiClient::finishConnected(quint64 request_id, QNetworkReply *reply)
{
    if (active_request_id != request_id)
    {
        reply->deleteLater();
        return;
    }
    active_reply.clear();
    active_request_id = 0;
    active_kind = RequestNone;
    bool used_body = false;
    Nax5SessionParseResult result = finishSessionNetwork(reply, &used_body);
    if (used_body)
        result = nax5ParseCurrentResponse(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), reply->readAll());
    emit connectedFinished(request_id, result);
    reply->deleteLater();
}

void Nax5ApiClient::finishFail(quint64 request_id, QNetworkReply *reply)
{
    if (active_request_id != request_id)
    {
        reply->deleteLater();
        return;
    }
    active_reply.clear();
    active_request_id = 0;
    active_kind = RequestNone;
    bool used_body = false;
    Nax5SessionParseResult result = finishSessionNetwork(reply, &used_body);
    if (used_body)
        result = nax5ParseCancelResponse(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), reply->readAll());
    emit failFinished(request_id, result);
    reply->deleteLater();
}

void Nax5ApiClient::finishEnd(quint64 request_id, QNetworkReply *reply)
{
    if (active_request_id != request_id)
    {
        reply->deleteLater();
        return;
    }
    active_reply.clear();
    active_request_id = 0;
    active_kind = RequestNone;
    bool used_body = false;
    Nax5SessionParseResult result = finishSessionNetwork(reply, &used_body);
    if (used_body)
        result = nax5ParseCancelResponse(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), reply->readAll());
    emit endFinished(request_id, result);
    reply->deleteLater();
}

void Nax5ApiClient::finishOperator(quint64 request_id, QNetworkReply *reply)
{
    if (active_request_id != request_id)
    {
        reply->deleteLater();
        return;
    }
    active_reply.clear();
    active_request_id = 0;
    active_kind = RequestNone;
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();
    Nax5ConnectionParseResult result = nax5ParseOperatorProvisionResponse(status, body);
    qCInfo(nax5Api) << "operator" << status;
    emit operatorFinished(request_id, result);
    reply->deleteLater();
}
