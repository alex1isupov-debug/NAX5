#include "nax5/nax5authstate.h"

Nax5AuthState nax5AuthReduce(Nax5AuthState current, Nax5AuthAction action)
{
    switch (action) {
    case Nax5AuthActionLoginClicked:
        if (current == Nax5AuthStateAuthenticating)
            return current;
        return Nax5AuthStateAuthenticating;
    case Nax5AuthActionLoginSucceeded:
        return Nax5AuthStateAuthenticated;
    case Nax5AuthActionLoginFailed:
        return Nax5AuthStateUnauthenticated;
    case Nax5AuthActionLogout:
        return Nax5AuthStateUnauthenticated;
    }
    return current;
}

bool nax5AuthCanStartLogin(Nax5AuthState current)
{
    return current != Nax5AuthStateAuthenticating;
}

bool nax5AuthAllowsRemotePlay(Nax5AuthState current)
{
    return current == Nax5AuthStateAuthenticated;
}
