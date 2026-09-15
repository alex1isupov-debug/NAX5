#include "nax5/nax5telemetry.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QtGlobal>
#include <cstdio>

static int g_failed = 0;

static void expect(bool condition, const char *name)
{
    if (condition)
        std::printf("ok %s\n", name);
    else
    {
        std::fprintf(stderr, "FAIL %s\n", name);
        g_failed++;
    }
}

static void test_payload_has_version_and_sha()
{
    const QByteArray body = nax5BuildClientEventBatch(QStringLiteral("APP_STARTED"));
    const QJsonObject root = QJsonDocument::fromJson(body).object();
    const QJsonObject installation = root.value(QStringLiteral("installation")).toObject();
    expect(!installation.value(QStringLiteral("client_version")).toString().isEmpty(), "client version present");
    expect(!installation.value(QStringLiteral("client_sha")).toString().isEmpty(), "client sha present");
    expect(!installation.value(QStringLiteral("installation_id")).toString().isEmpty(), "installation id present");
}

static void test_payload_no_secrets()
{
    const QByteArray body = nax5BuildClientEventBatch(QStringLiteral("PLAY_REQUESTED"));
    expect(!nax5ClientEventPayloadContainsSecrets(body), "payload has no secret patterns");
    expect(!body.contains("regist_key"), "no regist_key");
    expect(!body.contains("X-Session-Token"), "no session token");
}

int main()
{
    test_payload_has_version_and_sha();
    test_payload_no_secrets();
    return g_failed == 0 ? 0 : 1;
}
