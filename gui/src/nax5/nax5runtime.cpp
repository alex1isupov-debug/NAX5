#include "nax5/nax5runtime.h"

#include <QByteArray>

bool Nax5Runtime::operatorMode()
{
#ifdef NAX5_OPERATOR_BUILD
    return true;
#else
    const QByteArray value = qgetenv("NAX5_OPERATOR_MODE").trimmed();
    return value == "1" || value.compare("true", Qt::CaseInsensitive) == 0;
#endif
}

QString Nax5Runtime::settingsApplicationName()
{
    return operatorMode() ? QStringLiteral("NAX5-Operator") : QStringLiteral("NAX5");
}

QString Nax5Runtime::settingsOrganizationName()
{
    return QStringLiteral("NAX5");
}
