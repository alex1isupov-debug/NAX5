#include "nax5/session/nax5sessionstate.h"

Nax5GameSessionState nax5SessionReduce(Nax5GameSessionState current, Nax5GameSessionAction action)
{
    switch (action) {
    case Nax5GameSessionActionPlayClicked:
        if (!nax5SessionCanStartPlay(current))
            return current;
        return Nax5GameSessionStateReserving;
    case Nax5GameSessionActionReserveSucceeded:
    case Nax5GameSessionActionSyncedOccupied:
        return Nax5GameSessionStateReserved;
    case Nax5GameSessionActionReserveNoCapacity:
    case Nax5GameSessionActionReserveDenied:
    case Nax5GameSessionActionSyncedEmpty:
    case Nax5GameSessionActionCancelSucceeded:
    case Nax5GameSessionActionReset:
        return Nax5GameSessionStateIdle;
    case Nax5GameSessionActionReserveFailed:
        return Nax5GameSessionStateError;
    case Nax5GameSessionActionReleaseClicked:
        if (current != Nax5GameSessionStateReserved)
            return current;
        return Nax5GameSessionStateCancelling;
    case Nax5GameSessionActionCancelFailed:
        if (current == Nax5GameSessionStateCancelling)
            return Nax5GameSessionStateReserved;
        return current;
    }
    return current;
}

bool nax5SessionCanStartPlay(Nax5GameSessionState current)
{
    return current == Nax5GameSessionStateIdle || current == Nax5GameSessionStateError;
}

bool nax5SessionCanRelease(Nax5GameSessionState current)
{
    return current == Nax5GameSessionStateReserved;
}

bool nax5SessionPlayBusy(Nax5GameSessionState current)
{
    return current == Nax5GameSessionStateReserving || current == Nax5GameSessionStateCancelling;
}
