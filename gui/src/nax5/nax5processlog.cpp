#include "nax5/nax5processlog.h"

#include "nax5/nax5runtime.h"
#include "sessionlog.h"

#include <algorithm>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QPair>
#include <QRegularExpression>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

#ifndef CHIAKI_VERSION
#define CHIAKI_VERSION "unknown"
#endif
#ifndef NAX5_VERSION
#define NAX5_VERSION "unknown"
#endif
#ifndef NAX5_CLIENT_SHA
#define NAX5_CLIENT_SHA "unknown"
#endif

namespace {

const int kKeepLogFiles = 5;
QMutex g_mutex;
QString g_path;
bool g_started = false;

QString dateStamp()
{
    static const QString format = QStringLiteral("yyyy-MM-dd_HH-mm-ss-zzzzzz");
    return QDateTime::currentDateTime().toString(format);
}

void rotateNax5Logs(const QDir &dir)
{
    static const QRegularExpression name_re(QStringLiteral("^nax5_(.*)\\.log$"));
    const QStringList existing = dir.entryList(QStringList() << QStringLiteral("nax5_*.log"), QDir::Files);
    QVector<QPair<QString, QDateTime>> dated;
    dated.reserve(existing.size());
    for (const QString &filename : existing)
    {
        QDateTime date;
        const QRegularExpressionMatch match = name_re.match(filename);
        if (match.hasMatch())
            date = QDateTime::fromString(match.captured(1), QStringLiteral("yyyy-MM-dd_HH-mm-ss-zzzzzz"));
        dated.append(qMakePair(filename, date));
    }
    std::sort(dated.begin(), dated.end(), [](const QPair<QString, QDateTime> &a, const QPair<QString, QDateTime> &b) {
        return a.second > b.second;
    });
    for (int i = kKeepLogFiles; i < dated.size(); ++i)
    {
        if (!dated[i].second.isValid())
            break;
        QDir(dir).remove(dated[i].first);
    }
}

QString readTail(const QString &path, int max_bytes)
{
    if (path.isEmpty() || max_bytes <= 0)
        return QString();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QString();
    const qint64 size = file.size();
    if (size > max_bytes)
        file.seek(size - max_bytes);
    QByteArray bytes = file.readAll();
    if (size > max_bytes)
    {
        const int newline = bytes.indexOf('\n');
        if (newline >= 0 && newline + 1 < bytes.size())
            bytes = bytes.mid(newline + 1);
    }
    return QString::fromUtf8(bytes);
}

} // namespace

QString nax5SanitizeProcessLogLine(const QString &line)
{
    QString sanitized = line;
    static const QRegularExpression secret_re(
        QStringLiteral("(password|passwd|session[_-]?token|x-session-token|regist[_-]?key|morning|console[_-]?pin|\\bpin\\b)([\"'\\s:=]+)([^\\s,;\"']+)"),
        QRegularExpression::CaseInsensitiveOption);
    sanitized.replace(secret_re, QStringLiteral("\\1\\2<redacted>"));
    sanitized.replace(QRegularExpression(QStringLiteral("X-Session-Token:\\s*\\S+"), QRegularExpression::CaseInsensitiveOption),
                      QStringLiteral("X-Session-Token: <redacted>"));
    return sanitized;
}

QString nax5ClientVersion()
{
    return QStringLiteral(NAX5_VERSION);
}

QString nax5ClientSha()
{
    return QStringLiteral(NAX5_CLIENT_SHA);
}

QString nax5BuildInfoText()
{
    QString text;
    text += QStringLiteral("client_version=%1\n").arg(nax5ClientVersion());
    text += QStringLiteral("client_sha=%1\n").arg(nax5ClientSha());
    text += QStringLiteral("chiaki_version=%1\n").arg(QStringLiteral(CHIAKI_VERSION));
    text += QStringLiteral("operator_mode=%1\n").arg(Nax5Runtime::operatorMode() ? QStringLiteral("true") : QStringLiteral("false"));
    return text;
}

void nax5ProcessLogStart()
{
    QMutexLocker lock(&g_mutex);
    if (g_started)
        return;
    const QString dir_str = GetLogBaseDir();
    if (dir_str.isEmpty())
        return;
    QDir dir(dir_str);
    rotateNax5Logs(dir);
    g_path = dir.absoluteFilePath(QStringLiteral("nax5_%1.log").arg(dateStamp()));
    QFile file(g_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        g_path.clear();
        return;
    }
    const QString header = nax5SanitizeProcessLogLine(
        QStringLiteral("[%1] nax5 process log start version=%2 sha=%3 operator=%4\n")
            .arg(QDateTime::currentDateTime().toString(Qt::ISODate),
                 nax5ClientVersion(),
                 nax5ClientSha(),
                 Nax5Runtime::operatorMode() ? QStringLiteral("true") : QStringLiteral("false")));
    file.write(header.toUtf8());
    file.flush();
    g_started = true;
}

void nax5ProcessLogWrite(const char *category, const QString &message)
{
    const QString cat = QString::fromUtf8(category ? category : "");
    if (!cat.startsWith(QLatin1String("nax5")))
        return;
    QMutexLocker lock(&g_mutex);
    if (!g_started || g_path.isEmpty())
        return;
    QFile file(g_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;
    const QString line = nax5SanitizeProcessLogLine(
        QStringLiteral("[%1] [%2] %3\n")
            .arg(QDateTime::currentDateTime().toString(Qt::ISODate), cat, message));
    file.write(line.toUtf8());
    file.flush();
}

QString nax5ProcessLogPath()
{
    QMutexLocker lock(&g_mutex);
    return g_path;
}

QString nax5ProcessLogTail(int max_bytes)
{
    QString path;
    {
        QMutexLocker lock(&g_mutex);
        path = g_path;
    }
    return nax5SanitizeProcessLogLine(readTail(path, max_bytes));
}

QStringList nax5RecentChiakiSessionLogPaths(int max_files)
{
    QStringList paths;
    if (max_files <= 0)
        return paths;
    const QString dir_str = GetLogBaseDir();
    if (dir_str.isEmpty())
        return paths;
    QDir dir(dir_str);
    const QStringList files = dir.entryList(QStringList() << QStringLiteral("chiaki_session_*.log"), QDir::Files, QDir::Time);
    const int limit = qMin(max_files, files.size());
    for (int i = 0; i < limit; ++i)
        paths.append(dir.absoluteFilePath(files.at(i)));
    return paths;
}

QString nax5LatestChiakiSessionLogPath()
{
    const QStringList paths = nax5RecentChiakiSessionLogPaths(1);
    return paths.isEmpty() ? QString() : paths.first();
}

QString nax5ChiakiSessionLogTail(int max_bytes)
{
    return nax5SanitizeProcessLogLine(readTail(nax5LatestChiakiSessionLogPath(), max_bytes));
}
