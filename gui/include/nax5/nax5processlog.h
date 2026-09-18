#pragma once

#include <QString>
#include <QStringList>

void nax5ProcessLogStart();
void nax5ProcessLogWrite(const char *category, const QString &message);
QString nax5ProcessLogPath();
QString nax5ProcessLogTail(int max_bytes);
QString nax5LatestChiakiSessionLogPath();
QStringList nax5RecentChiakiSessionLogPaths(int max_files);
QString nax5ChiakiSessionLogTail(int max_bytes);
QString nax5SanitizeProcessLogLine(const QString &line);
QString nax5ClientVersion();
QString nax5ClientSha();
QString nax5BuildInfoText();
