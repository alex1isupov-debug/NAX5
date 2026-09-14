#pragma once

#include "nax5/session/nax5sessionerror.h"

enum Nax5GameSessionState
{
    Nax5GameSessionStateIdle = 0,
    Nax5GameSessionStateReserving,
    Nax5GameSessionStateFetchingConnection,
    Nax5GameSessionStateConnecting,
    Nax5GameSessionStateActive,
    Nax5GameSessionStateEnding,
    Nax5GameSessionStateCancelling,
    Nax5GameSessionStateError
};

enum Nax5GameSessionAction
{
    Nax5GameSessionActionPlayClicked = 0,
    Nax5GameSessionActionReserveSucceeded,
    Nax5GameSessionActionReserveNoCapacity,
    Nax5GameSessionActionReserveDenied,
    Nax5GameSessionActionReserveFailed,
    Nax5GameSessionActionConnectionReceived,
    Nax5GameSessionActionConnectionFailed,
    Nax5GameSessionActionStreamConnected,
    Nax5GameSessionActionStreamEnded,
    Nax5GameSessionActionStreamFailed,
    Nax5GameSessionActionSyncedOccupied,
    Nax5GameSessionActionSyncedEmpty,
    Nax5GameSessionActionReleaseClicked,
    Nax5GameSessionActionCancelSucceeded,
    Nax5GameSessionActionCancelFailed,
    Nax5GameSessionActionReset
};

Nax5GameSessionState nax5SessionReduce(Nax5GameSessionState current, Nax5GameSessionAction action);
bool nax5SessionCanStartPlay(Nax5GameSessionState current);
bool nax5SessionCanRelease(Nax5GameSessionState current);
bool nax5SessionPlayBusy(Nax5GameSessionState current);
bool nax5SessionHasAssignment(Nax5GameSessionState current);
bool nax5SessionShouldFetchOnSyncedOccupied(Nax5GameSessionState current);
bool nax5SessionShouldResetLocalAfterAbortCurrent(bool current_has_session);
bool nax5SessionCanCreateStream(bool stream_session_alive);
