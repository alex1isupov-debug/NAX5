#include "nax5/nax5clientreport.h"

#include "nax5/nax5processlog.h"
#include "nax5/nax5runtime.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QPair>
#include <QStandardPaths>

namespace {

const int kTailBytes = 96 * 1024;
const int kMaxJsonBytes = 256 * 1024;

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

} // namespace

QString nax5ClientReportKindName(Nax5ClientReportKind kind)
{
    switch (kind) {
    case Nax5ClientReportKindError:
        return QStringLiteral("error");
    case Nax5ClientReportKindReserveFail:
        return QStringLiteral("reserve-fail");
    case Nax5ClientReportKindLogout:
        return QStringLiteral("logout");
    case Nax5ClientReportKindManual:
        return QStringLiteral("manual");
    case Nax5ClientReportKindQuit:
        break;
    }
    return QStringLiteral("quit");
}

QByteArray nax5BuildClientReportJson(Nax5ClientReportKind kind)
{
    QJsonObject root;
    root.insert(QStringLiteral("client_sha"), nax5ClientSha());
    root.insert(QStringLiteral("operator_mode"), Nax5Runtime::operatorMode());
    root.insert(QStringLiteral("kind"), nax5ClientReportKindName(kind));
    root.insert(QStringLiteral("nax5_log_tail"), nax5ProcessLogTail(kTailBytes));
    const QString chiaki_tail = nax5ChiakiSessionLogTail(kTailBytes);
    if (!chiaki_tail.isEmpty())
        root.insert(QStringLiteral("chiaki_session_log_tail"), chiaki_tail);
    QByteArray body = QJsonDocument(root).toJson(QJsonDocument::Compact);
    if (body.size() > kMaxJsonBytes)
        body = body.left(kMaxJsonBytes);
    return body;
}

bool nax5WriteStoredZip(const QString &zip_path, const QList<QPair<QString, QByteArray>> &files)
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

    QFile zip(zip_path);
    if (!zip.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    zip.write(local);
    zip.write(central);
    zip.write(end);
    return true;
}

QString nax5WriteClientReportZip()
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

    QList<QPair<QString, QByteArray>> files;
    files.append(qMakePair(QStringLiteral("BUILD-INFO"), nax5BuildInfoText().toUtf8()));
    const QString nax5_path = nax5ProcessLogPath();
    if (!nax5_path.isEmpty())
        files.append(qMakePair(QFileInfo(nax5_path).fileName(), fileBytes(nax5_path)));
    const QStringList chiaki_paths = nax5RecentChiakiSessionLogPaths(5);
    for (const QString &chiaki_path : chiaki_paths)
    {
        if (chiaki_path.isEmpty())
            continue;
        files.append(qMakePair(QFileInfo(chiaki_path).fileName(), fileBytes(chiaki_path)));
    }
    if (!nax5WriteStoredZip(zip_path, files))
        return QString();
    return zip_path;
}
