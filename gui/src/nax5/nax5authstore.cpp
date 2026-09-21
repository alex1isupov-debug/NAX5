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
    return QSettings(QSettings::defaultFormat(), QSettings::UserScope,
        Nax5Runtime::settingsOrganizationName(), Nax5Runtime::settingsApplicationName());
}

void removeLegacySecrets(QSettings &settings)
{
    settings.remove(kPasswordKey);
    settings.remove(kSessionTokenKey);
}

} // namespace

bool nax5AuthRememberEnabled()
{
    QSettings settings = authSettings();
    removeLegacySecrets(settings);
    return settings.value(kRememberKey, false).toBool();
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
    QSettings settings = authSettings();
    removeLegacySecrets(settings);
    return {};
}

QString nax5AuthSavedSessionToken()
{
    QSettings settings = authSettings();
    removeLegacySecrets(settings);
    return {};
}

void nax5AuthSaveCredentials(const QString &email, const QString &password, const QString &session_token)
{
    Q_UNUSED(password);
    Q_UNUSED(session_token);
    QSettings settings = authSettings();
    settings.setValue(kRememberKey, true);
    settings.setValue(kEmailKey, email.trimmed());
    removeLegacySecrets(settings);
}

void nax5AuthClearCredentials()
{
    QSettings settings = authSettings();
    settings.remove(kRememberKey);
    settings.remove(kEmailKey);
    settings.remove(kPasswordKey);
    settings.remove(kSessionTokenKey);
}
