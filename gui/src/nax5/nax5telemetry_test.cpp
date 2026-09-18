#include "nax5/nax5telemetry.h"

#include "nax5/nax5clientreport.h"
#include "nax5/nax5processlog.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QtGlobal>
#include <cstdio>

QString GetLogBaseDir()
{
    return QString();
}

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
    const QJsonObject event = root.value(QStringLiteral("events")).toArray().at(0).toObject();
    expect(installation.value(QStringLiteral("client_version")).toString() == QStringLiteral("testver"), "installation version is NAX5 product version");
    expect(installation.value(QStringLiteral("client_version")).toString() != QStringLiteral("1.10.0"), "installation version is not chiaki");
    expect(installation.value(QStringLiteral("client_sha")).toString() == QStringLiteral("testsha"), "installation sha is git short hash");
    expect(event.value(QStringLiteral("client_version")).toString() == QStringLiteral("testver"), "event version is NAX5 product version");
    expect(event.value(QStringLiteral("client_sha")).toString() == QStringLiteral("testsha"), "event sha is git short hash");
    expect(!installation.value(QStringLiteral("installation_id")).toString().isEmpty(), "installation id present");
}

static void test_report_has_product_version()
{
    const QByteArray body = nax5BuildClientReportJson(Nax5ClientReportKindQuit);
    const QJsonObject root = QJsonDocument::fromJson(body).object();
    expect(root.value(QStringLiteral("client_version")).toString() == QStringLiteral("testver"), "report version is NAX5 product version");
    expect(root.value(QStringLiteral("client_sha")).toString() == QStringLiteral("testsha"), "report sha is git short hash");
    expect(nax5ClientVersion() == QStringLiteral("testver"), "nax5ClientVersion is product version");
    expect(nax5BuildInfoText().contains(QStringLiteral("client_version=testver")), "build info has product version");
    expect(!nax5BuildInfoText().contains(QStringLiteral("client_version=1.10.0")), "build info version is not chiaki");
}

static void test_report_zip_is_archive()
{
    const QByteArray zip = nax5BuildClientReportZipBytes(Nax5ClientReportKindQuit);
    expect(zip.startsWith("PK\x03\x04"), "zip archive magic");
    expect(zip.contains("BUILD-INFO"), "zip contains BUILD-INFO");
    expect(zip.contains("client_version=testver"), "zip build info has product version");
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
    test_report_has_product_version();
    test_report_zip_is_archive();
    test_payload_no_secrets();
    return g_failed == 0 ? 0 : 1;
}
