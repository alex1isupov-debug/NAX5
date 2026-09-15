#pragma once

#include "nax5/nax5apilane.h"
#include "nax5/nax5autherror.h"
#include "nax5/nax5authparser.h"
#include "nax5/connection/nax5connectionparser.h"
#include "nax5/session/nax5sessionparser.h"

#include <QList>
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
    quint64 fetchConnection(const QString &session_token, const QString &public_session_id);
    quint64 markConnected(const QString &session_token, const QString &public_session_id);
    quint64 failSession(const QString &session_token, const QString &public_session_id);
    quint64 endSession(const QString &session_token, const QString &public_session_id);
    quint64 operatorProvision(const QString &session_token, const QString &console_code, const QByteArray &body);
    quint64 operatorActivate(const QString &session_token, const QString &console_code);
    quint64 operatorTestConnection(const QString &session_token, const QString &console_code);
    quint64 operatorTestResult(const QString &session_token, const QString &console_code, const QByteArray &body);
    quint64 postClientReport(const QString &session_token, const QByteArray &body);
    void abortLane(Nax5ApiLane lane);
    void abortAll();
    bool hasLane(Nax5ApiLane lane) const;

signals:
    void loginFinished(quint64 request_id, const Nax5LoginParseResult &result);
    void meFinished(quint64 request_id, const Nax5MeParseResult &result);
    void logoutFinished(quint64 request_id);
    void reserveFinished(quint64 request_id, const Nax5SessionParseResult &result);
    void currentFinished(quint64 request_id, const Nax5SessionParseResult &result);
    void cancelFinished(quint64 request_id, const Nax5SessionParseResult &result);
    void connectionFinished(quint64 request_id, const Nax5ConnectionParseResult &result);
    void connectedFinished(quint64 request_id, const Nax5SessionParseResult &result);
    void failFinished(quint64 request_id, const Nax5SessionParseResult &result);
    void endFinished(quint64 request_id, const Nax5SessionParseResult &result);
    void operatorFinished(quint64 request_id, const Nax5ConnectionParseResult &result);
    void clientReportFinished(quint64 request_id, int http_status);

private:
    struct InFlight
    {
        quint64 request_id;
        Nax5ApiLane lane;
        QPointer<QNetworkReply> reply;
    };

    QNetworkReply *sendJson(Nax5ApiLane lane, quint64 request_id, const QString &method, const QString &path, const QByteArray &body, const QString &session_token, const QByteArray &idempotency_key = QByteArray());
    void finishLogin(quint64 request_id, QNetworkReply *reply);
    void finishMe(quint64 request_id, QNetworkReply *reply);
    void finishLogout(quint64 request_id, QNetworkReply *reply);
    void finishReserve(quint64 request_id, QNetworkReply *reply);
    void finishCurrent(quint64 request_id, QNetworkReply *reply);
    void finishCancel(quint64 request_id, QNetworkReply *reply);
    void finishConnection(quint64 request_id, QNetworkReply *reply);
    void finishConnected(quint64 request_id, QNetworkReply *reply);
    void finishFail(quint64 request_id, QNetworkReply *reply);
    void finishEnd(quint64 request_id, QNetworkReply *reply);
    void finishOperator(quint64 request_id, QNetworkReply *reply);
    void finishClientReport(quint64 request_id, QNetworkReply *reply);
    Nax5SessionParseResult finishSessionNetwork(QNetworkReply *reply, bool *used_body);
    bool completeLive(quint64 request_id);
    void abortInFlight(InFlight &item);
    static bool isNoNetwork(QNetworkReply *reply);

    QNetworkAccessManager *network;
    QList<InFlight> in_flight;
    quint64 next_request_id;
};
