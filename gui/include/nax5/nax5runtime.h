#pragma once

#include <QString>

class Nax5Runtime
{
public:
    static bool operatorMode();
    static QString settingsApplicationName();
    static QString settingsOrganizationName();
    static QString lastOperatorConsoleCode();
    static void setLastOperatorConsoleCode(const QString &code);
};
