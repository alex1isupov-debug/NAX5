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

QNetworkReply *Nax5ApiClient::sendJson(const QString &method, const QString &path, const QByteArray &body, const QString &session_token)
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
