#include "nax5/nax5telemetry.h"

#include "nax5/nax5processlog.h"

#include <QByteArray>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QMetaType>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QRegularExpression>
#include <QUuid>
#include <QVector>
#include <QtGlobal>

namespace {

const int kMaxBatchEvents = 25;
QMutex g_events_mutex;
QVector<QJsonObject> g_pending_events;

QJsonObject installationObject()
{
    QJsonObject installation;
    installation.insert(QStringLiteral("installation_id"), nax5InstallationId());
    installation.insert(QStringLiteral("client_version"), nax5ClientVersion());
    installation.insert(QStringLiteral("client_sha"), nax5ClientSha());
    installation.insert(QStringLiteral("platform"), nax5OsPlatform());
    installation.insert(QStringLiteral("os_version"), nax5OsBuild());
    installation.insert(QStringLiteral("locale"), nax5LocaleName());
    return installation;
}

QJsonObject eventObject(const QString &event_type, const QString &session_public_id)
{
    QJsonObject event;
    event.insert(QStringLiteral("event_id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    event.insert(QStringLiteral("event_type"), event_type);
    event.insert(QStringLiteral("occurred_at_client"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    event.insert(QStringLiteral("client_version"), nax5ClientVersion());
    event.insert(QStringLiteral("client_sha"), nax5ClientSha());
    if (!session_public_id.isEmpty())
        event.insert(QStringLiteral("session_public_id"), session_public_id);
    return event;
}

QByteArray batchBytes(const QVector<QJsonObject> &events)
{
    QJsonObject root;
    root.insert(QStringLiteral("installation"), installationObject());
    QJsonArray array;
    for (const QJsonObject &event : events)
        array.append(event);
    root.insert(QStringLiteral("events"), array);
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

} // namespace

QByteArray nax5BuildClientEventBatch(const QString &event_type, const QString &session_public_id)
{
    QVector<QJsonObject> events;
    events.append(eventObject(event_type, session_public_id));
    return batchBytes(events);
}

QByteArray nax5TakeQueuedClientEventBatch()
{
    QVector<QJsonObject> taken;
    {
        QMutexLocker lock(&g_events_mutex);
        if (g_pending_events.isEmpty())
            return QByteArray();
        const int count = qMin(kMaxBatchEvents, g_pending_events.size());
        taken.reserve(count);
        for (int i = 0; i < count; ++i)
            taken.append(g_pending_events.takeFirst());
    }
    return batchBytes(taken);
}

int nax5QueuedClientEventCount()
{
    QMutexLocker lock(&g_events_mutex);
    return g_pending_events.size();
}

bool nax5ClientEventPayloadContainsSecrets(const QByteArray &json)
{
    static const QRegularExpression forbidden(
        QStringLiteral("(password|session[_-]?token|x-session-token|regist[_-]?key|morning|authorization:)"),
        QRegularExpression::CaseInsensitiveOption);
    return forbidden.match(QString::fromUtf8(json)).hasMatch();
}

void nax5QueueClientEvent(const QString &event_type, const QString &session_public_id)
{
    nax5QueueClientEvent(event_type, session_public_id, QJsonObject());
}

void nax5QueueClientEvent(const QString &event_type, const QString &session_public_id, const QJsonObject &metadata)
{
    QMutexLocker lock(&g_events_mutex);
    auto event = eventObject(event_type, session_public_id);
    if (!metadata.isEmpty())
        event.insert(QStringLiteral("metadata"), metadata);
    g_pending_events.append(event);
    // The full session log retains the event even if the separate analytics POST fails.
    nax5ProcessLogWrite("nax5.event", QString::fromUtf8(QJsonDocument(event).toJson(QJsonDocument::Compact)));
}

void nax5FlushClientEvents(const QString &session_token, QObject *api_client)
{
    if (!api_client || session_token.isEmpty())
        return;
    while (nax5QueuedClientEventCount() > 0)
    {
        const QByteArray body = nax5TakeQueuedClientEventBatch();
        if (body.isEmpty())
            break;
        if (nax5ClientEventPayloadContainsSecrets(body))
            continue;
        const bool invoked = QMetaObject::invokeMethod(
            api_client,
            "submitClientEvents",
            Qt::DirectConnection,
            Q_ARG(QString, session_token),
            Q_ARG(QByteArray, body));
        if (!invoked)
            qWarning("nax5: failed to submit client events batch");
    }
}
