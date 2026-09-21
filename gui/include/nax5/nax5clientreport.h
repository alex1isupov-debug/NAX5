#pragma once

#include "nax5/nax5processlog.h"

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
    Nax5ClientReportKindManual,
    Nax5ClientReportKindAutomatic
};

QString nax5ClientReportKindName(Nax5ClientReportKind kind);
Nax5ClientReportKind nax5ClientReportKindFromName(const QString &name);
bool nax5ClientReportUsesArchive(Nax5ClientReportKind kind);
int nax5ClientReportMaxArchiveBytes();
QByteArray nax5TailBytes(const QByteArray &data, int max_bytes);
QList<QPair<QString, QByteArray>> nax5FitReportFiles(const QList<QPair<QString, QByteArray>> &files, int max_bytes);
QByteArray nax5ZipBytes(const QList<QPair<QString, QByteArray>> &files);
QByteArray nax5BuildClientReportJson(Nax5ClientReportKind kind, const QString &session_id = QString());
QByteArray nax5BuildClientReportZipBytes(Nax5ClientReportKind kind = Nax5ClientReportKindManual, const Nax5BuildInfoSnapshot &snapshot = Nax5BuildInfoSnapshot());
QString nax5WriteClientReportZip(const Nax5BuildInfoSnapshot &snapshot = Nax5BuildInfoSnapshot());
QString nax5PendingClientReportPath();
QString nax5PendingClientReportKindPath();
Nax5ClientReportKind nax5PendingClientReportKind();
bool nax5WritePendingClientReportZip(Nax5ClientReportKind kind, const Nax5BuildInfoSnapshot &snapshot = Nax5BuildInfoSnapshot());
bool nax5ClearPendingClientReport();
bool nax5WriteStoredZip(const QString &zip_path, const QList<QPair<QString, QByteArray>> &files);
