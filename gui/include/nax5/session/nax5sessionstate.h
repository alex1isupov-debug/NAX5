#pragma once

#include "nax5/session/nax5sessionerror.h"

enum Nax5GameSessionState
{
    Nax5GameSessionStateIdle = 0,
    Nax5GameSessionStateReserving,
    Nax5GameSessionStateReserved,
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
