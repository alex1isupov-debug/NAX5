#pragma once

#include <QByteArray>
#include <QString>

enum Nax5OperatorHostError
{
    Nax5OperatorHostOk = 0,
    Nax5OperatorHostNoSelection,
    Nax5OperatorHostNotRegistered,
    Nax5OperatorHostMissingConsoleCode
};

struct Nax5SyntheticRegisteredHost
{
    QString nickname;
    QByteArray regist_key;
};

Nax5OperatorHostError nax5ValidateProvisionSelection(int selected_index, bool server_valid, bool registered, const QString &console_code);
const Nax5SyntheticRegisteredHost *nax5PickSyntheticRegisteredHost(const Nax5SyntheticRegisteredHost *hosts, int host_count, int selected_index);
QString nax5OperatorHostErrorText(Nax5OperatorHostError error);
