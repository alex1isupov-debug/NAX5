#pragma once

#include <QByteArray>
#include <QList>
#include <QPair>
#include <QString>

enum Nax5ClientReportKind
{
    Nax5ClientReportKindQuit = 0,
    Nax5ClientReportKindError,
    Nax5ClientReportKindCrash,
    Nax5ClientReportKindReserveFail,
    Nax5ClientReportKindLogout,
    Nax5ClientReportKindManual
};

QString nax5ClientReportKindName(Nax5ClientReportKind kind);
QByteArray nax5BuildClientReportJson(Nax5ClientReportKind kind, const QString &session_id = QString());
QByteArray nax5BuildClientReportZipBytes(Nax5ClientReportKind kind = Nax5ClientReportKindManual);
QString nax5WriteClientReportZip();
QString nax5PendingClientReportPath();
bool nax5WritePendingClientReportZip(Nax5ClientReportKind kind);
bool nax5ClearPendingClientReport();
bool nax5WriteStoredZip(const QString &zip_path, const QList<QPair<QString, QByteArray>> &files);
