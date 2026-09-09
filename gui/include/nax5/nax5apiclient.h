#pragma once

#include "nax5/nax5autherror.h"
#include "nax5/nax5authparser.h"
#include "nax5/session/nax5sessionparser.h"

#include <QObject>
#include <QPointer>

class QNetworkAccessManager;
class QNetworkReply;

class Nax5ApiClient : public QObject
{
    Q_OBJECT

public:
    explicit Nax5ApiClient(QObject *parent = nullptr);
    ~Nax5ApiClient();

    quint64 login(const QString &email, const QString &password);
    quint64 fetchMe(const QString &session_token);
    quint64 logout(const QString &session_token);
    quint64 reserveSession(const QString &session_token, const QString &idempotency_key);
    quint64 fetchCurrentSession(const QString &session_token);
    quint64 cancelSession(const QString &session_token, const QString &public_session_id);
    void abortAll();

signals:
    void loginFinished(quint64 request_id, const Nax5LoginParseResult &result);
    void meFinished(quint64 request_id, const Nax5MeParseResult &result);
    void logoutFinished(quint64 request_id);
    void reserveFinished(quint64 request_id, const Nax5SessionParseResult &result);
    void currentFinished(quint64 request_id, const Nax5SessionParseResult &result);
    void cancelFinished(quint64 request_id, const Nax5SessionParseResult &result);

private:
    enum RequestKind
    {
        RequestNone,
        RequestLogin,
        RequestMe,
        RequestLogout,
        RequestReserve,
        RequestCurrent,
        RequestCancel
    };

    QNetworkReply *sendJson(const QString &method, const QString &path, const QByteArray &body, const QString &session_token, const QByteArray &idempotency_key = QByteArray());
    void finishLogin(quint64 request_id, QNetworkReply *reply);
    void finishMe(quint64 request_id, QNetworkReply *reply);
    void finishLogout(quint64 request_id, QNetworkReply *reply);
    void finishReserve(quint64 request_id, QNetworkReply *reply);
    void finishCurrent(quint64 request_id, QNetworkReply *reply);
    void finishCancel(quint64 request_id, QNetworkReply *reply);
    Nax5SessionParseResult finishSessionNetwork(QNetworkReply *reply, bool *used_body);
    static bool isNoNetwork(QNetworkReply *reply);

    QNetworkAccessManager *network;
    QPointer<QNetworkReply> active_reply;
    quint64 next_request_id;
    quint64 active_request_id;
    RequestKind active_kind;
};
