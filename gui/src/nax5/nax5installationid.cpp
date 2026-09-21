#include "nax5/nax5telemetry.h"

#include "nax5/nax5runtime.h"

#include <QSettings>
#include <QUuid>

namespace {

QString settingsKey()
{
    return QStringLiteral("telemetry/installation_id");
}

} // namespace

QString nax5InstallationId()
{
    QSettings settings(QSettings::defaultFormat(), QSettings::UserScope,
        Nax5Runtime::settingsOrganizationName(), Nax5Runtime::settingsApplicationName());
    const QString existing = settings.value(settingsKey()).toString();
    if (!existing.isEmpty())
        return existing;
    const QString generated = QUuid::createUuid().toString(QUuid::WithoutBraces);
    settings.setValue(settingsKey(), generated);
    return generated;
}
