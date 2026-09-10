#include "nax5/session/nax5sessionstate.h"

Nax5GameSessionState nax5SessionReduce(Nax5GameSessionState current, Nax5GameSessionAction action)
{
    switch (action) {
    case Nax5GameSessionActionPlayClicked:
        if (!nax5SessionCanStartPlay(current))
            return current;
        return Nax5GameSessionStateReserving;
    case Nax5GameSessionActionReserveSucceeded:
        return Nax5GameSessionStateFetchingConnection;
    case Nax5GameSessionActionSyncedOccupied:
        if (current == Nax5GameSessionStateConnecting || current == Nax5GameSessionStateActive)
            return current;
        if (current == Nax5GameSessionStateFetchingConnection)
            return current;
        return Nax5GameSessionStateFetchingConnection;
    case Nax5GameSessionActionConnectionReceived:
        if (current != Nax5GameSessionStateFetchingConnection && current != Nax5GameSessionStateConnecting)
            return current;
        return Nax5GameSessionStateConnecting;
    case Nax5GameSessionActionStreamConnected:
        if (current != Nax5GameSessionStateConnecting)
            return current;
        return Nax5GameSessionStateActive;
    case Nax5GameSessionActionStreamEnded:
    case Nax5GameSessionActionCancelSucceeded:
    case Nax5GameSessionActionSyncedEmpty:
    case Nax5GameSessionActionReset:
        return Nax5GameSessionStateIdle;
    case Nax5GameSessionActionReserveNoCapacity:
    case Nax5GameSessionActionReserveDenied:
        return Nax5GameSessionStateIdle;
    case Nax5GameSessionActionReserveFailed:
    case Nax5GameSessionActionConnectionFailed:
    case Nax5GameSessionActionStreamFailed:
        return Nax5GameSessionStateError;
    case Nax5GameSessionActionReleaseClicked:
        if (!nax5SessionCanRelease(current))
            return current;
        if (current == Nax5GameSessionStateActive || current == Nax5GameSessionStateConnecting)
            return Nax5GameSessionStateEnding;
        return Nax5GameSessionStateCancelling;
    case Nax5GameSessionActionCancelFailed:
        if (current == Nax5GameSessionStateCancelling)
            return Nax5GameSessionStateFetchingConnection;
        if (current == Nax5GameSessionStateEnding)
            return Nax5GameSessionStateActive;
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
    return current == Nax5GameSessionStateFetchingConnection
        || current == Nax5GameSessionStateConnecting
        || current == Nax5GameSessionStateActive
        || current == Nax5GameSessionStateReserving;
}

bool nax5SessionPlayBusy(Nax5GameSessionState current)
{
    return current == Nax5GameSessionStateReserving
        || current == Nax5GameSessionStateFetchingConnection
        || current == Nax5GameSessionStateConnecting
        || current == Nax5GameSessionStateEnding
        || current == Nax5GameSessionStateCancelling;
}

bool nax5SessionHasAssignment(Nax5GameSessionState current)
{
    return current == Nax5GameSessionStateFetchingConnection
        || current == Nax5GameSessionStateConnecting
        || current == Nax5GameSessionStateActive
        || current == Nax5GameSessionStateEnding
        || current == Nax5GameSessionStateCancelling;
}
