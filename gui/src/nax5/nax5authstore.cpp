#include "nax5/nax5authstore.h"

#include "nax5/nax5runtime.h"

#include <QSettings>

namespace {

constexpr auto kRememberKey = "auth/remember";
constexpr auto kEmailKey = "auth/email";
constexpr auto kPasswordKey = "auth/password";
constexpr auto kSessionTokenKey = "auth/session_token";

QSettings authSettings()
{
    return QSettings(Nax5Runtime::settingsOrganizationName(), Nax5Runtime::settingsApplicationName());
}

QString encodeSecret(const QString &value)
{
    return QString::fromUtf8(value.toUtf8().toBase64());
}

QString decodeSecret(const QString &value)
{
    return QString::fromUtf8(QByteArray::fromBase64(value.toUtf8()));
}

} // namespace

bool nax5AuthRememberEnabled()
{
    return authSettings().value(kRememberKey, false).toBool();
}

void nax5AuthSetRememberEnabled(bool enabled)
{
    authSettings().setValue(kRememberKey, enabled);
}

QString nax5AuthSavedEmail()
{
    return authSettings().value(kEmailKey).toString();
}

QString nax5AuthSavedPassword()
{
    return decodeSecret(authSettings().value(kPasswordKey).toString());
}

QString nax5AuthSavedSessionToken()
{
    return decodeSecret(authSettings().value(kSessionTokenKey).toString());
}

void nax5AuthSaveCredentials(const QString &email, const QString &password, const QString &session_token)
{
    QSettings settings = authSettings();
    settings.setValue(kRememberKey, true);
    settings.setValue(kEmailKey, email.trimmed());
    settings.setValue(kPasswordKey, encodeSecret(password));
    settings.setValue(kSessionTokenKey, encodeSecret(session_token));
}

void nax5AuthClearCredentials()
{
    QSettings settings = authSettings();
    settings.remove(kRememberKey);
    settings.remove(kEmailKey);
    settings.remove(kPasswordKey);
    settings.remove(kSessionTokenKey);
}
