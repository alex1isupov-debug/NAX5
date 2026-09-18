#include "nax5/nax5telemetry.h"

#include "nax5/nax5processlog.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUuid>

namespace {

QJsonObject installationObject()
{
    QJsonObject installation;
    installation.insert(QStringLiteral("installation_id"), nax5InstallationId());
    installation.insert(QStringLiteral("client_version"), nax5ClientVersion());
    installation.insert(QStringLiteral("client_sha"), nax5ClientSha());
    installation.insert(QStringLiteral("platform"), QStringLiteral("windows"));
#if defined(Q_OS_WIN)
    installation.insert(QStringLiteral("os_version"), QStringLiteral("windows"));
#else
    installation.insert(QStringLiteral("os_version"), QString());
#endif
    installation.insert(QStringLiteral("locale"), QStringLiteral("ru-RU"));
    return installation;
}

QVector<QJsonObject> g_pending_events;

} // namespace

QByteArray nax5BuildClientEventBatch(const QString &event_type, const QString &session_public_id)
{
    QJsonObject event;
    event.insert(QStringLiteral("event_id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    event.insert(QStringLiteral("event_type"), event_type);
    event.insert(QStringLiteral("client_version"), nax5ClientVersion());
    event.insert(QStringLiteral("client_sha"), nax5ClientSha());
    if (!session_public_id.isEmpty())
        event.insert(QStringLiteral("session_public_id"), session_public_id);

    QJsonObject root;
    root.insert(QStringLiteral("installation"), installationObject());
    QJsonArray events;
    events.append(event);
    root.insert(QStringLiteral("events"), events);
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
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
    QJsonObject event;
    event.insert(QStringLiteral("event_id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    event.insert(QStringLiteral("event_type"), event_type);
    event.insert(QStringLiteral("client_version"), nax5ClientVersion());
    event.insert(QStringLiteral("client_sha"), nax5ClientSha());
    if (!session_public_id.isEmpty())
        event.insert(QStringLiteral("session_public_id"), session_public_id);
    g_pending_events.append(event);
}

void nax5FlushClientEvents(const QString &session_token, QObject *api_client)
{
    Q_UNUSED(session_token);
    Q_UNUSED(api_client);
    g_pending_events.clear();
}
