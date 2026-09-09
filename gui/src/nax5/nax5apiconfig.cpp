#include "nax5/nax5apiconfig.h"

#include <QProcessEnvironment>
#include <QUrl>

QString Nax5ApiConfig::defaultBaseUrl()
{
    return QStringLiteral("https://cloudgta6.com");
}

QUrl Nax5ApiConfig::baseUrl(QString *error_message)
{
    QString configured = QProcessEnvironment::systemEnvironment().value(QStringLiteral("NAX5_API_BASE_URL")).trimmed();
    if (configured.endsWith(QLatin1Char('/')))
        configured.chop(1);
    if (configured.isEmpty())
        configured = defaultBaseUrl();

    const QUrl url(configured);
    if (!url.isValid() || url.host().isEmpty()) {
        if (error_message)
            *error_message = QStringLiteral("Invalid NAX5 API base URL");
        return QUrl();
    }

    const QString scheme = url.scheme().toLower();
    const QString host = url.host().toLower();
    const bool loopback = host == QLatin1String("127.0.0.1")
        || host == QLatin1String("localhost")
        || host == QLatin1String("::1");
    if (scheme == QLatin1String("https"))
        return url;
    if (scheme == QLatin1String("http") && loopback)
        return url;

    if (error_message) {
        *error_message = loopback
            ? QStringLiteral("Local NAX5 API URL must use http or https")
            : QStringLiteral("Production NAX5 API URL must use HTTPS");
    }
    return QUrl();
}

QString Nax5ApiConfig::userAgent()
{
#ifdef Q_OS_WIN
    return QStringLiteral("NAX5/0.2 Windows");
#else
    return QStringLiteral("NAX5/0.2");
#endif
}

int Nax5ApiConfig::requestTimeoutMs()
{
    return 15000;
}
