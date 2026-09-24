#pragma once

#include "nax5/session/nax5sessionerror.h"
#include "nax5/session/nax5sessionstate.h"

#include <QByteArray>
#include <QString>
#include <QtGlobal>

enum Nax5TerminalMutation
{
    Nax5TerminalMutationNone = 0,
    Nax5TerminalMutationCancel,
    Nax5TerminalMutationFail,
    Nax5TerminalMutationEnd
};

Nax5TerminalMutation nax5ShutdownMutation(Nax5GameSessionState state, bool stream_was_connected);
Nax5TerminalMutation nax5StreamQuitMutation(bool stream_was_connected, bool operator_test);
Nax5TerminalMutation nax5ReleaseMutation(Nax5GameSessionState state, bool stream_was_connected);
bool nax5SessionTerminalBlocksPlay(Nax5TerminalMutation mutation);
bool nax5SessionNeedsQuitRelease(Nax5GameSessionState state);
bool nax5ShutdownNeedsCurrentSync(Nax5GameSessionState state);
bool nax5ShouldSendTerminalBeforeUnauthLogout(bool has_session_id, bool has_token, Nax5TerminalMutation mutation);
bool nax5PlayEligibilityOk(bool authenticated, bool email_verified, const QString &access_status);
bool nax5AcceptAsync(quint64 live_generation, quint64 event_generation, quint64 live_request_id, quint64 event_request_id);
bool nax5AcceptSessionIdentity(const QString &live_session_id, const QString &event_session_id);
int nax5TerminalRetryLimit();
// Network failures and 5xx (e.g. a backend deadlock) leave the session occupied; retry them.
bool nax5TerminalShouldRetry(Nax5SessionError error);
int nax5TerminalRetryDelayMs(int attempt);
int nax5ShutdownGraceMs();
int nax5ShutdownReportGraceMs();
bool nax5MaySleepConsole(bool operator_mode);
bool nax5MayResumeAfterOsSleep(bool operator_mode);
bool nax5ShowsChiakiQuitDialog(bool operator_mode);
bool nax5OperatorHostConnectAllowed(bool operator_mode);
bool nax5StreamConnectedOnNewGeneration();
bool nax5StreamFirstFrameSeenOnNewGeneration();
bool nax5ShouldRetryMarkConnected(Nax5GameSessionState state, bool shutdown_started, Nax5SessionError error);
bool nax5ProductShouldWakeupBeforeCreateSession();
int nax5ProductWakeupSendsPerStartStream();
bool nax5OperatorConnectShouldWakeup(bool discovered, bool standby);
QString nax5WakeupHostKindName(const QString &host);

struct Nax5MaterialWakeup
{
    QString host;
    QByteArray regist_key;
    bool ps5;
    bool ready;

    Nax5MaterialWakeup()
        : ps5(false)
        , ready(false)
    {
    }
};

Nax5MaterialWakeup nax5MaterialWakeupCall(const QString &host, const QByteArray &regist_key, bool ps5);
