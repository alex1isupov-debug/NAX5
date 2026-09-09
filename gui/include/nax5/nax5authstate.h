#pragma once

#include "nax5/nax5autherror.h"

#include <QString>

enum Nax5AuthState
{
    Nax5AuthStateUnauthenticated = 0,
    Nax5AuthStateAuthenticating,
    Nax5AuthStateAuthenticated
};

enum Nax5AuthAction
{
    Nax5AuthActionLoginClicked = 0,
    Nax5AuthActionLoginSucceeded,
    Nax5AuthActionLoginFailed,
    Nax5AuthActionLogout
};

Nax5AuthState nax5AuthReduce(Nax5AuthState current, Nax5AuthAction action);
bool nax5AuthCanStartLogin(Nax5AuthState current);
