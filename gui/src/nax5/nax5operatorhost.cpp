#include "nax5/nax5operatorhost.h"

Nax5OperatorHostError nax5ValidateProvisionSelection(int selected_index, bool server_valid, bool registered, const QString &console_code)
{
    if (selected_index < 0 || !server_valid)
        return Nax5OperatorHostNoSelection;
    if (!registered)
        return Nax5OperatorHostNotRegistered;
    if (console_code.trimmed().isEmpty())
        return Nax5OperatorHostMissingConsoleCode;
    return Nax5OperatorHostOk;
}

const Nax5SyntheticRegisteredHost *nax5PickSyntheticRegisteredHost(const Nax5SyntheticRegisteredHost *hosts, int host_count, int selected_index)
{
    if (!hosts || selected_index < 0 || selected_index >= host_count)
        return nullptr;
    return &hosts[selected_index];
}

QString nax5OperatorHostErrorText(Nax5OperatorHostError error)
{
    switch (error) {
    case Nax5OperatorHostNoSelection:
        return QStringLiteral("Select a registered PS5 in the console list, then Provision.");
    case Nax5OperatorHostNotRegistered:
        return QStringLiteral("Register the selected console in Operator Mode first.");
    case Nax5OperatorHostMissingConsoleCode:
        return QStringLiteral("Enter the backend console code before provisioning.");
    case Nax5OperatorHostOk:
        break;
    }
    return QString();
}
