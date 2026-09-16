#include "nax5/session/nax5sessionlifecycle.h"

#include <QString>
#include <QStringList>

Nax5TerminalMutation nax5ShutdownMutation(Nax5GameSessionState state, bool stream_was_connected)
{
    if (stream_was_connected || state == Nax5GameSessionStateActive || state == Nax5GameSessionStateEnding)
        return Nax5TerminalMutationEnd;
    if (state == Nax5GameSessionStateConnecting)
        return Nax5TerminalMutationFail;
    if (state == Nax5GameSessionStateReserving
        || state == Nax5GameSessionStateFetchingConnection
        || state == Nax5GameSessionStateCancelling)
        return Nax5TerminalMutationCancel;
    return Nax5TerminalMutationNone;
}

Nax5TerminalMutation nax5StreamQuitMutation(bool stream_was_connected, bool operator_test)
{
    if (operator_test)
        return Nax5TerminalMutationNone;
    if (stream_was_connected)
        return Nax5TerminalMutationEnd;
    return Nax5TerminalMutationFail;
}

Nax5TerminalMutation nax5ReleaseMutation(Nax5GameSessionState state, bool stream_was_connected)
{
    if (state == Nax5GameSessionStateActive || stream_was_connected)
        return Nax5TerminalMutationEnd;
    if (state == Nax5GameSessionStateConnecting)
        return Nax5TerminalMutationFail;
    if (state == Nax5GameSessionStateReserving
        || state == Nax5GameSessionStateFetchingConnection
        || state == Nax5GameSessionStateCancelling)
        return Nax5TerminalMutationCancel;
    return Nax5TerminalMutationNone;
}

bool nax5SessionTerminalBlocksPlay(Nax5TerminalMutation mutation)
{
    return mutation == Nax5TerminalMutationEnd || mutation == Nax5TerminalMutationFail;
}

bool nax5SessionNeedsQuitRelease(Nax5GameSessionState state)
{
    return state == Nax5GameSessionStateReserving
        || state == Nax5GameSessionStateFetchingConnection
        || state == Nax5GameSessionStateConnecting
        || state == Nax5GameSessionStateActive
        || state == Nax5GameSessionStateEnding
        || state == Nax5GameSessionStateCancelling;
}

bool nax5ShutdownNeedsCurrentSync(Nax5GameSessionState state)
{
    return state == Nax5GameSessionStateReserving;
}

bool nax5ShouldSendTerminalBeforeUnauthLogout(bool has_session_id, bool has_token, Nax5TerminalMutation mutation)
{
    return has_session_id && has_token && mutation != Nax5TerminalMutationNone;
}

bool nax5PlayEligibilityOk(bool authenticated, bool email_verified, const QString &access_status)
{
    return authenticated && email_verified && access_status == QLatin1String("ACTIVE");
}

bool nax5AcceptAsync(quint64 live_generation, quint64 event_generation, quint64 live_request_id, quint64 event_request_id)
{
    return event_request_id != 0
        && live_request_id == event_request_id
        && live_generation == event_generation;
}

bool nax5AcceptSessionIdentity(const QString &live_session_id, const QString &event_session_id)
{
    if (event_session_id.isEmpty())
        return false;
    return live_session_id == event_session_id;
}

int nax5TerminalRetryLimit()
{
    return 2;
}

int nax5ShutdownGraceMs()
{
    return 400;
}

bool nax5MaySleepConsole(bool operator_mode)
{
    return operator_mode;
}

bool nax5MayResumeAfterOsSleep(bool operator_mode)
{
    return operator_mode;
}

bool nax5ShowsChiakiQuitDialog(bool operator_mode)
{
    return operator_mode;
}

bool nax5OperatorHostConnectAllowed(bool operator_mode)
{
    return operator_mode;
}

bool nax5StreamConnectedOnNewGeneration()
{
    return false;
}

bool nax5StreamFirstFrameSeenOnNewGeneration()
{
    return false;
}

bool nax5ShouldRetryMarkConnected(Nax5GameSessionState state, bool shutdown_started, Nax5SessionError error)
{
    if (shutdown_started || error != Nax5SessionErrorNetworkError)
        return false;
    return state == Nax5GameSessionStateActive || state == Nax5GameSessionStateConnecting;
}

bool nax5ProductShouldWakeupBeforeCreateSession()
{
    return true;
}

int nax5ProductWakeupSendsPerStartStream()
{
    return 1;
}

bool nax5OperatorConnectShouldWakeup(bool discovered, bool standby)
{
    return discovered && standby;
}

QString nax5WakeupHostKindName(const QString &host)
{
    const QString trimmed = host.trimmed();
    if (trimmed.isEmpty())
        return QStringLiteral("unknown");
    if (trimmed.contains(QLatin1Char(':')))
    {
        const QString lower = trimmed.toLower();
        if (lower == QLatin1String("::1")
            || lower.startsWith(QLatin1String("fe80:"))
            || lower.startsWith(QLatin1String("fc"))
            || lower.startsWith(QLatin1String("fd")))
            return QStringLiteral("lan");
        return QStringLiteral("wan");
    }
    const QStringList parts = trimmed.split(QLatin1Char('.'));
    if (parts.size() != 4)
        return QStringLiteral("wan");
    int octet[4];
    for (int i = 0; i < 4; ++i)
    {
        bool ok = false;
        octet[i] = parts.at(i).toInt(&ok);
        if (!ok || octet[i] < 0 || octet[i] > 255)
            return QStringLiteral("wan");
    }
    if (octet[0] == 10
        || octet[0] == 127
        || (octet[0] == 192 && octet[1] == 168)
        || (octet[0] == 172 && octet[1] >= 16 && octet[1] <= 31)
        || (octet[0] == 169 && octet[1] == 254))
        return QStringLiteral("lan");
    return QStringLiteral("wan");
}

Nax5MaterialWakeup nax5MaterialWakeupCall(const QString &host, const QByteArray &regist_key, bool ps5)
{
    Nax5MaterialWakeup call;
    call.host = host.trimmed();
    call.regist_key = regist_key;
    call.ps5 = ps5;
    call.ready = !call.host.isEmpty() && !call.regist_key.isEmpty();
    return call;
}
