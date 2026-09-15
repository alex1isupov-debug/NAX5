#pragma once

#include <QByteArray>
#include <QList>
#include <QPair>
#include <QString>

enum Nax5ClientReportKind
{
    Nax5ClientReportKindQuit = 0,
    Nax5ClientReportKindError,
    Nax5ClientReportKindReserveFail,
    Nax5ClientReportKindLogout,
    Nax5ClientReportKindManual
};

QString nax5ClientReportKindName(Nax5ClientReportKind kind);
QByteArray nax5BuildClientReportJson(Nax5ClientReportKind kind);
QString nax5WriteClientReportZip();
bool nax5WriteStoredZip(const QString &zip_path, const QList<QPair<QString, QByteArray>> &files);
