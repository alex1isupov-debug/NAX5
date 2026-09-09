#pragma once

#include "nax5/nax5autherror.h"
#include "nax5/nax5authparser.h"

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
    void abortAll();

signals:
    void loginFinished(quint64 request_id, const Nax5LoginParseResult &result);
    void meFinished(quint64 request_id, const Nax5MeParseResult &result);
    void logoutFinished(quint64 request_id);

private:
    enum RequestKind
    {
        RequestNone,
        RequestLogin,
        RequestMe,
        RequestLogout
    };

    QNetworkReply *sendJson(const QString &method, const QString &path, const QByteArray &body, const QString &session_token);
    void finishLogin(quint64 request_id, QNetworkReply *reply);
    void finishMe(quint64 request_id, QNetworkReply *reply);
    void finishLogout(quint64 request_id, QNetworkReply *reply);
    static bool isNoNetwork(QNetworkReply *reply);

    QNetworkAccessManager *network;
    QPointer<QNetworkReply> active_reply;
    quint64 next_request_id;
    quint64 active_request_id;
    RequestKind active_kind;
};
