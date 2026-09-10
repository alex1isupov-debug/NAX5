#include "nax5/connection/nax5connectionmaterial.h"
#include "nax5/connection/nax5connectionparser.h"
#include "nax5/nax5runtime.h"
#include "nax5/session/nax5sessionstate.h"

#include <QByteArray>
#include <QString>
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

static QByteArray b64(const QByteArray &raw)
{
    return raw.toBase64();
}

static QByteArray sixteen(char fill)
{
    return QByteArray(Nax5RegistKeySize, fill);
}

static QByteArray connectionBody(const QByteArray &regist, const QByteArray &morning, const QString &host = QStringLiteral("203.0.113.10"), int version = 1, int target = 1000100)
{
    return QStringLiteral(
        "{\"session\":{\"id\":\"11111111-1111-4111-8111-111111111111\",\"status\":\"CONNECTING\"},"
        "\"connection\":{\"version\":%1,\"target\":%2,\"host\":\"%3\",\"nickname\":\"PS5-439\","
        "\"registKey\":\"%4\",\"morning\":\"%5\",\"consolePin\":\"\"}}")
        .arg(version)
        .arg(target)
        .arg(host)
        .arg(QString::fromLatin1(b64(regist)))
        .arg(QString::fromLatin1(b64(morning)))
        .toUtf8();
}

static void test_valid_base64_keys()
{
    const QByteArray regist = sixteen('A');
    const QByteArray morning = sixteen('B');
    const Nax5ConnectionParseResult result = nax5ParseConnectionResponse(200, connectionBody(regist, morning));
    expect(result.error == Nax5SessionErrorNone, "valid parse");
    expect(result.has_material, "has material");
    expect(result.material.regist_key == regist, "regist key");
    expect(result.material.morning == morning, "morning");
    expect(result.material.host == QStringLiteral("203.0.113.10"), "host");
    expect(result.material.target == 1000100, "target");
    expect(result.material.session_status == QStringLiteral("CONNECTING"), "status");
}

static void test_invalid_base64_and_length()
{
    QByteArray bad = "{\"session\":{\"id\":\"11111111-1111-4111-8111-111111111111\",\"status\":\"CONNECTING\"},"
        "\"connection\":{\"version\":1,\"target\":1000100,\"host\":\"203.0.113.10\","
        "\"registKey\":\"@@@\",\"morning\":\"QkJCQkJCQkJCQkJCQkJCQg==\",\"consolePin\":\"\"}}";
    expect(nax5ParseConnectionResponse(200, bad).error == Nax5SessionErrorInvalidResponse, "invalid base64");

    const QByteArray short_key = QByteArray(8, 'A').toBase64();
    const QByteArray morning = sixteen('B').toBase64();
    const QByteArray short_body = QStringLiteral(
        "{\"session\":{\"id\":\"11111111-1111-4111-8111-111111111111\",\"status\":\"CONNECTING\"},"
        "\"connection\":{\"version\":1,\"target\":1000100,\"host\":\"203.0.113.10\","
        "\"registKey\":\"%1\",\"morning\":\"%2\",\"consolePin\":\"\"}}")
        .arg(QString::fromLatin1(short_key), QString::fromLatin1(morning)).toUtf8();
    expect(nax5ParseConnectionResponse(200, short_body).error == Nax5SessionErrorInvalidResponse, "wrong length");
}

static void test_missing_and_unsupported()
{
    expect(nax5ParseConnectionResponse(200, "{\"session\":{\"id\":\"x\",\"status\":\"CONNECTING\"},\"connection\":{\"version\":1,\"target\":1000100,\"registKey\":\"QQ==\",\"morning\":\"QQ==\"}}").error == Nax5SessionErrorInvalidResponse, "missing host");
    const QByteArray regist = sixteen('A');
    const QByteArray morning = sixteen('B');
    expect(nax5ParseConnectionResponse(200, connectionBody(regist, morning, QStringLiteral("203.0.113.10"), 2)).error == Nax5SessionErrorInvalidResponse, "unsupported version");
    expect(nax5ParseConnectionResponse(200, connectionBody(regist, morning, QStringLiteral("203.0.113.10"), 1, 800)).error == Nax5SessionErrorInvalidResponse, "unsupported target");
}

static void test_http_errors()
{
    expect(nax5ParseConnectionResponse(401, "{\"code\":\"UNAUTHENTICATED\"}").error == Nax5SessionErrorUnauthenticated, "401");
    expect(nax5ParseConnectionResponse(403, "{\"code\":\"SESSION_NOT_OWNED\"}").error == Nax5SessionErrorForbidden, "owner mismatch");
    expect(nax5ParseConnectionResponse(404, "{\"code\":\"SESSION_NOT_FOUND\"}").error == Nax5SessionErrorNotFound, "missing session");
    expect(nax5ParseConnectionResponse(409, "{\"code\":\"INVALID_STATE\"}").error == Nax5SessionErrorInvalidConnectionMaterial, "expired or terminal");
    expect(nax5ParseConnectionResponse(0, "x").error == Nax5SessionErrorNetworkError, "timeout");
    expect(nax5ParseConnectionResponse(200, "not-json").error == Nax5SessionErrorInvalidResponse, "malformed json");
}

static void test_cleanup_and_duplicate_state()
{
    Nax5ConnectionMaterial material;
    material.regist_key = sixteen('A');
    material.morning = sixteen('B');
    material.host = QStringLiteral("203.0.113.10");
    material.clear();
    expect(nax5ConnectionMaterialIsEmpty(material), "cleared");
    expect(nax5SessionReduce(Nax5GameSessionStateConnecting, Nax5GameSessionActionConnectionReceived) == Nax5GameSessionStateConnecting, "duplicate connecting");
    expect(nax5SessionReduce(Nax5GameSessionStateIdle, Nax5GameSessionActionConnectionReceived) == Nax5GameSessionStateIdle, "stale connection");
    expect(nax5SessionReduce(Nax5GameSessionStateFetchingConnection, Nax5GameSessionActionReset) == Nax5GameSessionStateIdle, "logout cleanup");
}

static void test_decode_and_operator_test_payload()
{
    QByteArray out;
    expect(!nax5DecodeRpKey(QString(), Nax5RegistKeySize, &out), "empty key");
    expect(!nax5DecodeRpKey(QStringLiteral("not-base64"), Nax5RegistKeySize, &out), "not base64");
    expect(!nax5DecodeRpKey(QString::fromLatin1(QByteArray("deadbeefdeadbeef").toHex()), Nax5RegistKeySize, &out), "hex rejected");
    expect(nax5DecodeRpKey(QString::fromLatin1(sixteen('S').toBase64()), Nax5RegistKeySize, &out), "sentinel decode");
    expect(out == sixteen('S'), "sentinel bytes");
    expect(nax5TargetIsSupportedDirectPs5(1000000), "ps5 unknown");
    expect(nax5TargetIsSupportedDirectPs5(1000100), "ps5 1");
    expect(!nax5TargetIsSupportedDirectPs5(800), "ps4 rejected");

    const QByteArray body = QStringLiteral(
        "{\"session\":{\"id\":\"operator-test\",\"status\":\"CONNECTING\"},"
        "\"connection\":{\"version\":1,\"target\":1000100,\"host\":\"203.0.113.10\",\"nickname\":\"Lab\","
        "\"registKey\":\"%1\",\"morning\":\"%2\",\"consolePin\":\"\"}}")
        .arg(QString::fromLatin1(b64(sixteen('A'))), QString::fromLatin1(b64(sixteen('B')))).toUtf8();
    const Nax5ConnectionParseResult result = nax5ParseConnectionResponse(200, body);
    expect(result.error == Nax5SessionErrorNone, "operator-test parse");
    expect(result.material.session_id == QStringLiteral("operator-test"), "operator-test id");
}

static void test_operator_namespace()
{
    qunsetenv("NAX5_OPERATOR_MODE");
    expect(Nax5Runtime::settingsOrganizationName() == QStringLiteral("NAX5"), "org");
    expect(Nax5Runtime::settingsApplicationName() == QStringLiteral("NAX5"), "product app");
    qputenv("NAX5_OPERATOR_MODE", "1");
    expect(Nax5Runtime::settingsApplicationName() == QStringLiteral("NAX5-Operator"), "operator app");
    qunsetenv("NAX5_OPERATOR_MODE");
}

int main()
{
    test_valid_base64_keys();
    test_invalid_base64_and_length();
    test_missing_and_unsupported();
    test_http_errors();
    test_cleanup_and_duplicate_state();
    test_decode_and_operator_test_payload();
    test_operator_namespace();
    if (g_failed)
    {
        std::fprintf(stderr, "%d NAX5 connection tests failed\n", g_failed);
        return 1;
    }
    std::printf("All NAX5 connection tests passed\n");
    return 0;
}
