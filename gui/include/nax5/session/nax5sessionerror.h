#pragma once

#include <QString>

enum Nax5SessionError
{
    Nax5SessionErrorNone = 0,
    Nax5SessionErrorNoCapacity,
    Nax5SessionErrorUserNotEligible,
    Nax5SessionErrorActiveSessionExists,
    Nax5SessionErrorUnauthenticated,
    Nax5SessionErrorNetworkError,
    Nax5SessionErrorRateLimited,
    Nax5SessionErrorServerError,
    Nax5SessionErrorInvalidResponse,
    Nax5SessionErrorForbidden,
    Nax5SessionErrorNotFound
};

QString nax5SessionErrorMessage(Nax5SessionError error);
