#include "nax5/session/nax5sessionerror.h"

QString nax5SessionErrorMessage(Nax5SessionError error)
{
    switch (error) {
    case Nax5SessionErrorNoCapacity:
        return QStringLiteral("Все консоли сейчас заняты.\nПопробуйте немного позже.");
    case Nax5SessionErrorUserNotEligible:
        return QStringLiteral("Этот аккаунт пока не может получить консоль.");
    case Nax5SessionErrorActiveSessionExists:
        return QStringLiteral("У вас уже есть выделенная консоль.");
    case Nax5SessionErrorUnauthenticated:
        return QStringLiteral("Сессия входа истекла. Войдите снова.");
    case Nax5SessionErrorNetworkError:
        return QStringLiteral("Нет подключения к интернету.");
    case Nax5SessionErrorRateLimited:
        return QStringLiteral("Слишком много попыток. Попробуйте позже.");
    case Nax5SessionErrorServerError:
        return QStringLiteral("Сервис временно недоступен.");
    case Nax5SessionErrorInvalidResponse:
        return QStringLiteral("Сервис временно недоступен.");
    case Nax5SessionErrorForbidden:
        return QStringLiteral("Нельзя освободить чужую сессию.");
    case Nax5SessionErrorNotFound:
        return QStringLiteral("Игровая сессия не найдена.");
    case Nax5SessionErrorNone:
        return QString();
    }
    return QStringLiteral("Сервис временно недоступен.");
}
