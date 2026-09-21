#include "nax5/nax5clientreport.h"

#include "nax5/nax5processlog.h"
#include "nax5/nax5runtime.h"
#include "sessionlog.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QLoggingCategory>
#include <QPair>
#include <QStandardPaths>

namespace {

Q_LOGGING_CATEGORY(nax5ReportLog, "nax5.report")

const int kJsonTailBytes = 96 * 1024;
const int kMaxJsonBytes = 256 * 1024;
const int kMaxArchiveBytes = 1536 * 1024;
const int kNax5ArchiveTailBytes = 256 * 1024;
const int kPreviousChiakiTailBytes = 128 * 1024;

void appendU16(QByteArray &out, quint16 value)
{
    out.append(static_cast<char>(value & 0xff));
    out.append(static_cast<char>((value >> 8) & 0xff));
}

void appendU32(QByteArray &out, quint32 value)
{
    out.append(static_cast<char>(value & 0xff));
    out.append(static_cast<char>((value >> 8) & 0xff));
    out.append(static_cast<char>((value >> 16) & 0xff));
    out.append(static_cast<char>((value >> 24) & 0xff));
}

quint32 crc32Ieee(const QByteArray &data)
{
    quint32 crc = 0xffffffffu;
    for (int i = 0; i < data.size(); ++i)
    {
        crc ^= static_cast<quint8>(data.at(i));
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

QByteArray fileBytes(const QString &path)
{
    if (path.isEmpty())
        return QByteArray();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QByteArray();
    return file.readAll();
}

int zipStoredSize(const QList<QPair<QString, QByteArray>> &files)
{
    int total = 22;
    for (const auto &entry : files)
    {
        const int name_size = entry.first.toUtf8().size();
        total += 30 + 46 + (2 * name_size) + entry.second.size();
    }
    return total;
}

QList<QPair<QString, QByteArray>> reportFiles(Nax5ClientReportKind kind, const Nax5BuildInfoSnapshot &snapshot, bool bounded)
{
    QList<QPair<QString, QByteArray>> files;
    QByteArray build_info = nax5BuildInfoText(snapshot).toUtf8();
    build_info += QStringLiteral("report_kind=%1\n").arg(nax5ClientReportKindName(kind)).toUtf8();
    build_info += QStringLiteral("archive_bounded=%1\n").arg(bounded ? QStringLiteral("true") : QStringLiteral("false")).toUtf8();
    files.append(qMakePair(QStringLiteral("BUILD-INFO"), build_info));

    const QString nax5_path = nax5ProcessLogPath();
    const QStringList chiaki_paths = nax5RecentChiakiSessionLogPaths(bounded ? 2 : 5);

    if (bounded)
    {
        if (!chiaki_paths.isEmpty())
        {
            files.append(qMakePair(
                QFileInfo(chiaki_paths.first()).fileName(),
                nax5ReadFileTailBytes(chiaki_paths.first(), kMaxArchiveBytes)));
        }
        if (!nax5_path.isEmpty())
        {
            files.append(qMakePair(
                QFileInfo(nax5_path).fileName(),
                nax5ReadFileTailBytes(nax5_path, kNax5ArchiveTailBytes)));
        }
        if (chiaki_paths.size() > 1)
        {
            files.append(qMakePair(
                QFileInfo(chiaki_paths.at(1)).fileName(),
                nax5ReadFileTailBytes(chiaki_paths.at(1), kPreviousChiakiTailBytes)));
        }
        return nax5FitReportFiles(files, kMaxArchiveBytes);
    }

    if (!nax5_path.isEmpty())
        files.append(qMakePair(QFileInfo(nax5_path).fileName(), fileBytes(nax5_path)));
    for (const QString &chiaki_path : chiaki_paths)
    {
        if (chiaki_path.isEmpty())
            continue;
        files.append(qMakePair(QFileInfo(chiaki_path).fileName(), fileBytes(chiaki_path)));
    }
    return files;
}

QByteArray buildZipBytes(const QList<QPair<QString, QByteArray>> &files)
{
    QByteArray local;
    QByteArray central;
    quint32 offset = 0;
    for (const auto &entry : files)
    {
        const QByteArray name = entry.first.toUtf8();
        const QByteArray data = entry.second;
        const quint32 crc = crc32Ieee(data);
        const quint32 size = static_cast<quint32>(data.size());

        QByteArray header;
        header.append("PK\x03\x04");
        appendU16(header, 20);
        appendU16(header, 0);
        appendU16(header, 0);
        appendU16(header, 0);
        appendU16(header, 0);
        appendU32(header, crc);
        appendU32(header, size);
        appendU32(header, size);
        appendU16(header, static_cast<quint16>(name.size()));
        appendU16(header, 0);
        header.append(name);
        header.append(data);
        local.append(header);

        QByteArray dir;
        dir.append("PK\x01\x02");
        appendU16(dir, 20);
        appendU16(dir, 20);
        appendU16(dir, 0);
        appendU16(dir, 0);
        appendU16(dir, 0);
        appendU16(dir, 0);
        appendU32(dir, crc);
        appendU32(dir, size);
        appendU32(dir, size);
        appendU16(dir, static_cast<quint16>(name.size()));
        appendU16(dir, 0);
        appendU16(dir, 0);
        appendU16(dir, 0);
        appendU16(dir, 0);
        appendU32(dir, 0);
        appendU32(dir, offset);
        dir.append(name);
        central.append(dir);
        offset += static_cast<quint32>(header.size());
    }

    QByteArray end;
    end.append("PK\x05\x06");
    appendU16(end, 0);
    appendU16(end, 0);
    appendU16(end, static_cast<quint16>(files.size()));
    appendU16(end, static_cast<quint16>(files.size()));
    appendU32(end, static_cast<quint32>(central.size()));
    appendU32(end, static_cast<quint32>(local.size()));
    appendU16(end, 0);

    QByteArray zip;
    zip.append(local);
    zip.append(central);
    zip.append(end);
    return zip;
}

} // namespace

QString nax5ClientReportKindName(Nax5ClientReportKind kind)
{
    switch (kind) {
    case Nax5ClientReportKindError:
        return QStringLiteral("error");
    case Nax5ClientReportKindCrash:
        return QStringLiteral("crash");
    case Nax5ClientReportKindReserveFail:
        return QStringLiteral("reserve-fail");
    case Nax5ClientReportKindLogout:
        return QStringLiteral("logout");
    case Nax5ClientReportKindManual:
        return QStringLiteral("manual");
    case Nax5ClientReportKindAutomatic:
        return QStringLiteral("automatic");
    case Nax5ClientReportKindQuit:
        break;
    }
    return QStringLiteral("quit");
}

Nax5ClientReportKind nax5ClientReportKindFromName(const QString &name)
{
    if (name == QLatin1String("error"))
        return Nax5ClientReportKindError;
    if (name == QLatin1String("crash"))
        return Nax5ClientReportKindCrash;
    if (name == QLatin1String("reserve-fail"))
        return Nax5ClientReportKindReserveFail;
    if (name == QLatin1String("logout"))
        return Nax5ClientReportKindLogout;
    if (name == QLatin1String("manual"))
        return Nax5ClientReportKindManual;
    if (name == QLatin1String("automatic"))
        return Nax5ClientReportKindAutomatic;
    return Nax5ClientReportKindQuit;
}

bool nax5ClientReportUsesArchive(Nax5ClientReportKind kind)
{
    return kind != Nax5ClientReportKindLogout && kind != Nax5ClientReportKindReserveFail;
}

int nax5ClientReportMaxArchiveBytes()
{
    return kMaxArchiveBytes;
}

QByteArray nax5TailBytes(const QByteArray &data, int max_bytes)
{
    if (max_bytes <= 0)
        return QByteArray();
    if (data.size() <= max_bytes)
        return data;
    QByteArray tail = data.right(max_bytes);
    const int newline = tail.indexOf('\n');
    if (newline >= 0 && newline + 1 < tail.size())
        tail = tail.mid(newline + 1);
    return tail;
}

QList<QPair<QString, QByteArray>> nax5FitReportFiles(const QList<QPair<QString, QByteArray>> &files, int max_bytes)
{
    QList<QPair<QString, QByteArray>> out = files;
    auto shrink_index = [&out]() {
        for (int i = out.size() - 1; i >= 0; --i)
        {
            if (out[i].first == QLatin1String("BUILD-INFO"))
                continue;
            if (out[i].second.size() > 512)
                return i;
        }
        return -1;
    };
    while (!out.isEmpty() && zipStoredSize(out) > max_bytes)
    {
        const int i = shrink_index();
        if (i < 0)
            break;
        const int over = zipStoredSize(out) - max_bytes;
        int new_size = out[i].second.size() - over - 16;
        if (new_size < 512)
        {
            out.removeAt(i);
            continue;
        }
        out[i].second = nax5TailBytes(out[i].second, new_size);
    }
    return out;
}

QByteArray nax5ZipBytes(const QList<QPair<QString, QByteArray>> &files)
{
    return buildZipBytes(files);
}

QByteArray nax5BuildClientReportJson(Nax5ClientReportKind kind, const QString &session_id)
{
    QString nax5_tail = nax5ProcessLogTail(kJsonTailBytes);
    QString chiaki_tail = nax5ChiakiSessionLogTail(kJsonTailBytes);
    auto build = [&]() {
        QJsonObject root;
        root.insert(QStringLiteral("client_version"), nax5ClientVersion());
        root.insert(QStringLiteral("client_sha"), nax5ClientSha());
        root.insert(QStringLiteral("operator_mode"), Nax5Runtime::operatorMode());
        root.insert(QStringLiteral("kind"), nax5ClientReportKindName(kind));
        if (!session_id.isEmpty())
            root.insert(QStringLiteral("session_id"), session_id);
        root.insert(QStringLiteral("nax5_log_tail"), nax5_tail);
        if (!chiaki_tail.isEmpty())
            root.insert(QStringLiteral("chiaki_session_log_tail"), chiaki_tail);
        return QJsonDocument(root).toJson(QJsonDocument::Compact);
    };
    QByteArray body = build();
    while (body.size() > kMaxJsonBytes)
    {
        if (chiaki_tail.size() > 2048)
            chiaki_tail = chiaki_tail.right(chiaki_tail.size() / 2);
        else if (nax5_tail.size() > 2048)
            nax5_tail = nax5_tail.right(nax5_tail.size() / 2);
        else if (!chiaki_tail.isEmpty())
            chiaki_tail.clear();
        else
            nax5_tail = nax5_tail.right(qMax(0, nax5_tail.size() - (body.size() - kMaxJsonBytes + 64)));
        body = build();
        if (chiaki_tail.isEmpty() && nax5_tail.size() <= 64)
            break;
    }
    return body;
}

QByteArray nax5BuildClientReportZipBytes(Nax5ClientReportKind kind, const Nax5BuildInfoSnapshot &snapshot)
{
    return buildZipBytes(reportFiles(kind, snapshot, true));
}

bool nax5WriteStoredZip(const QString &zip_path, const QList<QPair<QString, QByteArray>> &files)
{
    const QByteArray zip = buildZipBytes(files);
    if (zip.isEmpty())
        return false;
    QFile file(zip_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return file.write(zip) == zip.size();
}

QString nax5PendingClientReportPath()
{
    const QString dir_str = GetLogBaseDir();
    if (dir_str.isEmpty())
        return QString();
    return QDir(dir_str).filePath(QStringLiteral("pending-client-report.zip"));
}

QString nax5PendingClientReportKindPath()
{
    const QString dir_str = GetLogBaseDir();
    if (dir_str.isEmpty())
        return QString();
    return QDir(dir_str).filePath(QStringLiteral("pending-client-report.kind"));
}

Nax5ClientReportKind nax5PendingClientReportKind()
{
    const QString path = nax5PendingClientReportKindPath();
    if (path.isEmpty() || !QFile::exists(path))
        return Nax5ClientReportKindCrash;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return Nax5ClientReportKindCrash;
    return nax5ClientReportKindFromName(QString::fromUtf8(file.readAll()).trimmed());
}

bool nax5WritePendingClientReportZip(Nax5ClientReportKind kind, const Nax5BuildInfoSnapshot &snapshot)
{
    const QString path = nax5PendingClientReportPath();
    if (path.isEmpty())
        return false;
    const bool ok = nax5WriteStoredZip(path, reportFiles(kind, snapshot, true));
    if (ok)
    {
        const QString kind_path = nax5PendingClientReportKindPath();
        if (!kind_path.isEmpty())
        {
            QFile kind_file(kind_path);
            if (kind_file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
                kind_file.write(nax5ClientReportKindName(kind).toUtf8());
        }
        qCWarning(nax5ReportLog) << "pending client report saved" << path;
    }
    return ok;
}

bool nax5ClearPendingClientReport()
{
    const QString zip_path = nax5PendingClientReportPath();
    const QString kind_path = nax5PendingClientReportKindPath();
    bool removed = false;
    if (!zip_path.isEmpty() && QFile::exists(zip_path))
        removed = QFile::remove(zip_path) || removed;
    if (!kind_path.isEmpty() && QFile::exists(kind_path))
        removed = QFile::remove(kind_path) || removed;
    return removed;
}

QString nax5WriteClientReportZip(const Nax5BuildInfoSnapshot &snapshot)
{
    QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (desktop.isEmpty())
        desktop = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (desktop.isEmpty())
        desktop = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    if (desktop.isEmpty())
        return QString();
    QDir().mkpath(desktop);
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss"));
    const QString zip_path = QDir(desktop).filePath(QStringLiteral("NAX5-report-%1.zip").arg(stamp));
    if (!nax5WriteStoredZip(zip_path, reportFiles(Nax5ClientReportKindManual, snapshot, false)))
        return QString();
    return zip_path;
}
