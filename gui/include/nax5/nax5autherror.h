#pragma once

#include <QString>

enum Nax5AuthError
{
    Nax5AuthErrorNone = 0,
    Nax5AuthErrorInvalidCredentials,
    Nax5AuthErrorEmailNotVerified,
    Nax5AuthErrorRateLimited,
    Nax5AuthErrorNetworkError,
    Nax5AuthErrorServerError,
    Nax5AuthErrorInvalidResponse
};

QString nax5AuthErrorMessage(Nax5AuthError error);
