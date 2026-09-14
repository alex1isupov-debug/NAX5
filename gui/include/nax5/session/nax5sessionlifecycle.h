#pragma once

#include "nax5/session/nax5sessionstate.h"

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
bool nax5AcceptAsync(quint64 live_generation, quint64 event_generation, quint64 live_request_id, quint64 event_request_id);
bool nax5AcceptSessionIdentity(const QString &live_session_id, const QString &event_session_id);
int nax5TerminalRetryLimit();
int nax5ShutdownGraceMs();
bool nax5MaySleepConsole(bool operator_mode);
bool nax5MayResumeAfterOsSleep(bool operator_mode);
bool nax5ShowsChiakiQuitDialog(bool operator_mode);
bool nax5OperatorHostConnectAllowed(bool operator_mode);
