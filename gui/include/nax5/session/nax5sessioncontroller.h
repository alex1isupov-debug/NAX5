#pragma once

#include "nax5/connection/nax5connectionmaterial.h"
#include "nax5/session/nax5sessionerror.h"
#include "nax5/session/nax5sessionlifecycle.h"
#include "nax5/session/nax5sessionstate.h"

#include <chiaki/session.h>

#include <QObject>
#include <QPointer>
#include <QString>

class Nax5ApiClient;
class Nax5AuthController;
class QmlBackend;
class StreamSession;
struct Nax5ConnectionParseResult;
struct Nax5SessionParseResult;
class QTimer;

class Nax5SessionController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool reserving READ reserving NOTIFY stateChanged)
    Q_PROPERTY(bool reserved READ reserved NOTIFY stateChanged)
    Q_PROPERTY(bool cancelling READ cancelling NOTIFY stateChanged)
    Q_PROPERTY(bool playEnabled READ playEnabled NOTIFY stateChanged)
    Q_PROPERTY(bool releaseEnabled READ releaseEnabled NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString consoleCode READ consoleCode NOTIFY assignmentChanged)
    Q_PROPERTY(QString consoleRegion READ consoleRegion NOTIFY assignmentChanged)
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY assignmentChanged)

public:
    explicit Nax5SessionController(Nax5AuthController *auth, QmlBackend *backend, QObject *parent = nullptr);
    ~Nax5SessionController() override;

    int state() const { return static_cast<int>(session_state); }
    bool reserving() const { return session_state == Nax5GameSessionStateReserving; }
    bool reserved() const { return nax5SessionHasAssignment(session_state); }
    bool cancelling() const
    {
        return session_state == Nax5GameSessionStateCancelling || session_state == Nax5GameSessionStateEnding;
    }
    bool playEnabled() const;
    bool releaseEnabled() const { return nax5SessionCanRelease(session_state); }
    QString statusText() const { return status_text; }
    QString errorMessage() const { return error_message; }
    QString consoleCode() const { return console_code; }
    QString consoleRegion() const { return console_region; }
    QString sessionId() const { return session_id; }

    Q_INVOKABLE void play();
    Q_INVOKABLE void release();
    Q_INVOKABLE void releaseAndLogout();
    Q_INVOKABLE void operatorActivate(const QString &console_code);
    Q_INVOKABLE void operatorTest(const QString &console_code);
    void provisionFromFields(const QString &console_code, int target, const QByteArray &regist_key, const QByteArray &morning, const QString &console_pin, const QString &nickname);
    void prepareShutdown();

signals:
    void stateChanged();
    void statusTextChanged();
    void errorMessageChanged();
    void assignmentChanged();

private:
    struct PendingTerminal
    {
        Nax5TerminalMutation mutation;
        QString token;
        QString session_id;
        quint64 generation;
        quint64 request_id;
        int attempts;
        bool silent;
    };

    void setState(Nax5GameSessionState next);
    void setStatusText(const QString &text);
    void setError(Nax5SessionError error);
    void clearAssignment();
    void discardMaterial();
    void applyAssignment(const Nax5SessionParseResult &result);
    void resetLocal();
    QString errorText(Nax5SessionError error) const;
    bool isOperatorTest() const;
    void reportOperatorTestResult(bool passed);
    void syncCurrent();
    void scheduleLeaseSync(const QString &lease_expires_at);
    void onAuthStateChanged();
    void onReserveFinished(quint64 request_id, const Nax5SessionParseResult &result);
    void onCurrentFinished(quint64 request_id, const Nax5SessionParseResult &result);
    void onCancelFinished(quint64 request_id, const Nax5SessionParseResult &result);
    void onConnectionFinished(quint64 request_id, const Nax5ConnectionParseResult &result);
    void onConnectedFinished(quint64 request_id, const Nax5SessionParseResult &result);
    void onFailFinished(quint64 request_id, const Nax5SessionParseResult &result);
    void onEndFinished(quint64 request_id, const Nax5SessionParseResult &result);
    void fetchConnection();
    void startStream();
    void onChiakiSessionChanged(StreamSession *session);
    void onStreamConnected();
    void onStreamQuit(ChiakiQuitReason reason, const QString &reason_str);
    void reportFail();
    void reportEnd();
    void dispatchTerminal(Nax5TerminalMutation mutation, bool silent);
    void sendPendingTerminal();
    void handleTerminalFinished(quint64 request_id, const Nax5SessionParseResult &result, Nax5TerminalMutation mutation);
    bool retryTerminalIfNeeded(const Nax5SessionParseResult &result);
    bool logoutIfUnauthenticated(Nax5SessionError error);
    bool streamSessionAlive() const;
    quint64 bumpGeneration();
    QString liveToken() const;

    QPointer<Nax5AuthController> auth;
    QPointer<QmlBackend> backend;
    Nax5ApiClient *api;
    QTimer *lease_timer;
    Nax5GameSessionState session_state;
    QString status_text;
    QString error_message;
    QString console_code;
    QString console_region;
    QString session_id;
    QString operator_console_code;
    QString idempotency_key;
    Nax5ConnectionMaterial material;
    PendingTerminal pending_terminal;
    quint64 generation;
    quint64 stream_generation;
    quint64 reserve_request_id;
    quint64 current_request_id;
    quint64 cancel_request_id;
    quint64 connection_request_id;
    quint64 connected_request_id;
    quint64 test_result_request_id;
    quint64 operator_request_id;
    bool ignore_cancel_result;
    bool stream_was_connected;
    bool operator_test_active;
    bool shutdown_started;
    bool awaiting_abort_current;
    bool pending_start_stream;
};
