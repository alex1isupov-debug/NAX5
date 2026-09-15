#pragma once

#include <QByteArray>
#include <QString>

class QObject;

QString nax5InstallationId();
QByteArray nax5BuildClientEventBatch(const QString &event_type, const QString &session_public_id = QString());
bool nax5ClientEventPayloadContainsSecrets(const QByteArray &json);
void nax5QueueClientEvent(const QString &event_type, const QString &session_public_id = QString());
void nax5FlushClientEvents(const QString &session_token, QObject *api_client);
