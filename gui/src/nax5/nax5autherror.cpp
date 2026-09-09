#include "nax5/nax5autherror.h"

QString nax5AuthErrorMessage(Nax5AuthError error)
{
    switch (error) {
    case Nax5AuthErrorInvalidCredentials:
        return QStringLiteral("Неверный email или пароль.");
    case Nax5AuthErrorEmailNotVerified:
        return QStringLiteral("Подтвердите email перед входом.");
    case Nax5AuthErrorRateLimited:
        return QStringLiteral("Слишком много попыток. Попробуйте позже.");
    case Nax5AuthErrorNetworkError:
        return QStringLiteral("Нет подключения к интернету.");
    case Nax5AuthErrorServerError:
        return QStringLiteral("Сервис временно недоступен.");
    case Nax5AuthErrorInvalidResponse:
        return QStringLiteral("Сервис временно недоступен.");
    case Nax5AuthErrorNone:
        return QString();
    }
    return QStringLiteral("Сервис временно недоступен.");
}
