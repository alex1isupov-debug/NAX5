#pragma once

#include <QByteArray>
#include <QString>

class QObject;
class QJsonObject;

QString nax5InstallationId();
QByteArray nax5BuildClientEventBatch(const QString &event_type, const QString &session_public_id = QString());
QByteArray nax5TakeQueuedClientEventBatch();
int nax5QueuedClientEventCount();
bool nax5ClientEventPayloadContainsSecrets(const QByteArray &json);
void nax5QueueClientEvent(const QString &event_type, const QString &session_public_id = QString());
// Metadata keys must be on the backend allowlist (client_telemetry/schemas.py); values are sent as strings.
void nax5QueueClientEvent(const QString &event_type, const QString &session_public_id, const QJsonObject &metadata);
void nax5FlushClientEvents(const QString &session_token, QObject *api_client);
