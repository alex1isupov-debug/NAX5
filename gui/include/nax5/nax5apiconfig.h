#pragma once

#include <QString>
#include <QUrl>

class Nax5ApiConfig
{
public:
    static QString defaultBaseUrl();
    static QUrl baseUrl(QString *error_message = nullptr);
    static QString userAgent();
    static int requestTimeoutMs();
};
