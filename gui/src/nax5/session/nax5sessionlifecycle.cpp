#include "nax5/session/nax5sessionlifecycle.h"

#include <QString>

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
