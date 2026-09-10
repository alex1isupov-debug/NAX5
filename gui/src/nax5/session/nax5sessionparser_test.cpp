#include "nax5/nax5apiconfig.h"
#include "nax5/session/nax5sessionerror.h"
#include "nax5/session/nax5sessionparser.h"
#include "nax5/session/nax5sessionstate.h"

#include <QString>
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

static const char *kReserved =
    "{\"session\":{\"id\":\"11111111-1111-4111-8111-111111111111\",\"status\":\"RESERVED\","
    "\"reservedAt\":\"2026-09-09T19:00:00Z\",\"leaseExpiresAt\":\"2026-09-09T19:02:00Z\"},"
    "\"console\":{\"code\":\"PS5-439\",\"region\":\"Moscow\"}}";

static void test_reserve_success()
{
    const Nax5SessionParseResult result = nax5ParseReserveResponse(200, kReserved);
    expect(result.error == Nax5SessionErrorNone, "reserve success");
    expect(result.has_session, "reserve has session");
    expect(result.has_console, "reserve has console");
    expect(result.session.id == QStringLiteral("11111111-1111-4111-8111-111111111111"), "reserve id");
    expect(result.session.status == QStringLiteral("RESERVED"), "reserve status");
    expect(result.console.code == QStringLiteral("PS5-439"), "reserve console");
    expect(result.console.region == QStringLiteral("Moscow"), "reserve region");
}

static void test_no_capacity()
{
    const char *body = "{\"code\":\"NO_CAPACITY\",\"message\":\"No console is currently available.\"}";
    const Nax5SessionParseResult result = nax5ParseReserveResponse(409, body);
    expect(result.error == Nax5SessionErrorNoCapacity, "no capacity");
    expect(!result.has_session, "no capacity has no session");
}

static void test_eligibility_denied()
{
    const char *body = "{\"code\":\"USER_NOT_ELIGIBLE\",\"message\":\"denied\"}";
    expect(nax5ParseReserveResponse(403, body).error == Nax5SessionErrorUserNotEligible, "eligibility");
}

static void test_unauthenticated()
{
    expect(nax5ParseReserveResponse(401, "{\"code\":\"UNAUTHENTICATED\"}").error == Nax5SessionErrorUnauthenticated, "401");
}

static void test_conflict_and_rate_limit_and_server()
{
    expect(nax5ParseReserveResponse(409, "{\"code\":\"ACTIVE_SESSION_EXISTS\",\"session\":{\"id\":\"11111111-1111-4111-8111-111111111111\",\"status\":\"RESERVED\",\"reservedAt\":\"2026-09-09T19:00:00Z\",\"leaseExpiresAt\":\"2026-09-09T19:02:00Z\"},\"console\":{\"code\":\"PS5-439\",\"region\":\"Moscow\"}}").error == Nax5SessionErrorActiveSessionExists, "active exists");
    expect(nax5ParseReserveResponse(429, "{\"status\":429}").error == Nax5SessionErrorRateLimited, "429");
    expect(nax5ParseReserveResponse(500, "internal").error == Nax5SessionErrorServerError, "500");
}

static void test_malformed_and_timeout()
{
    expect(nax5ParseReserveResponse(200, "not-json").error == Nax5SessionErrorInvalidResponse, "malformed");
    expect(nax5ParseReserveResponse(200, "{\"session\":{}}").error == Nax5SessionErrorInvalidResponse, "incomplete");
    expect(nax5MapSessionHttpError(0, true, false) == Nax5SessionErrorNetworkError, "timeout");
    expect(nax5MapSessionHttpError(0, false, true) == Nax5SessionErrorNetworkError, "offline");
}

static void test_current_none_and_present()
{
    const Nax5SessionParseResult empty = nax5ParseCurrentResponse(200, "{\"session\":null}");
    expect(empty.error == Nax5SessionErrorNone, "current none error");
    expect(!empty.has_session, "current none");
    const Nax5SessionParseResult present = nax5ParseCurrentResponse(200, kReserved);
    expect(present.error == Nax5SessionErrorNone, "current present");
    expect(present.console.code == QStringLiteral("PS5-439"), "current console");
}

static void test_cancel_success_and_failure()
{
    const char *cancelled =
        "{\"session\":{\"id\":\"11111111-1111-4111-8111-111111111111\",\"status\":\"CANCELLED\","
        "\"reservedAt\":\"2026-09-09T19:00:00Z\",\"leaseExpiresAt\":null},"
        "\"console\":{\"code\":\"PS5-439\",\"region\":\"Moscow\"}}";
    const Nax5SessionParseResult ok = nax5ParseCancelResponse(200, cancelled);
    expect(ok.error == Nax5SessionErrorNone, "cancel ok");
    expect(ok.session.status == QStringLiteral("CANCELLED"), "cancel status");
    expect(nax5ParseCancelResponse(403, "{\"code\":\"SESSION_NOT_OWNED\"}").error == Nax5SessionErrorForbidden, "cancel forbidden");
}

static void test_state_transitions_and_double_click()
{
    expect(nax5SessionCanStartPlay(Nax5GameSessionStateIdle), "idle can play");
    expect(nax5SessionReduce(Nax5GameSessionStateIdle, Nax5GameSessionActionPlayClicked) == Nax5GameSessionStateReserving, "play");
    expect(!nax5SessionCanStartPlay(Nax5GameSessionStateReserving), "busy cannot play");
    expect(nax5SessionReduce(Nax5GameSessionStateReserving, Nax5GameSessionActionPlayClicked) == Nax5GameSessionStateReserving, "double click ignored");
    expect(nax5SessionReduce(Nax5GameSessionStateReserving, Nax5GameSessionActionReserveSucceeded) == Nax5GameSessionStateFetchingConnection, "fetching");
    expect(nax5SessionReduce(Nax5GameSessionStateFetchingConnection, Nax5GameSessionActionConnectionReceived) == Nax5GameSessionStateConnecting, "connecting");
    expect(nax5SessionReduce(Nax5GameSessionStateConnecting, Nax5GameSessionActionStreamConnected) == Nax5GameSessionStateActive, "active");
    expect(nax5SessionReduce(Nax5GameSessionStateIdle, Nax5GameSessionActionStreamConnected) == Nax5GameSessionStateIdle, "stale connected after cancel");
    expect(nax5SessionReduce(Nax5GameSessionStateReserving, Nax5GameSessionActionReserveNoCapacity) == Nax5GameSessionStateIdle, "no capacity idle");
    expect(nax5SessionReduce(Nax5GameSessionStateReserving, Nax5GameSessionActionReserveFailed) == Nax5GameSessionStateError, "error");
    expect(nax5SessionReduce(Nax5GameSessionStateConnecting, Nax5GameSessionActionStreamFailed) == Nax5GameSessionStateError, "failed stream retryable");
    expect(nax5SessionCanStartPlay(Nax5GameSessionStateError), "retry from error");
    expect(nax5SessionReduce(Nax5GameSessionStateFetchingConnection, Nax5GameSessionActionReleaseClicked) == Nax5GameSessionStateCancelling, "release");
    expect(nax5SessionReduce(Nax5GameSessionStateCancelling, Nax5GameSessionActionCancelSucceeded) == Nax5GameSessionStateIdle, "cancelled idle");
    expect(nax5SessionReduce(Nax5GameSessionStateFetchingConnection, Nax5GameSessionActionSyncedEmpty) == Nax5GameSessionStateIdle, "stale idle");
    expect(nax5SessionReduce(Nax5GameSessionStateConnecting, Nax5GameSessionActionConnectionReceived) == Nax5GameSessionStateConnecting, "connection retry stays connecting");
    expect(nax5SessionReduce(Nax5GameSessionStateConnecting, Nax5GameSessionActionReleaseClicked) == Nax5GameSessionStateEnding, "connecting release ends");
    expect(nax5SessionReduce(Nax5GameSessionStateActive, Nax5GameSessionActionReleaseClicked) == Nax5GameSessionStateEnding, "active release ends");
    expect(nax5SessionReduce(Nax5GameSessionStateIdle, Nax5GameSessionActionConnectionReceived) == Nax5GameSessionStateIdle, "stale connection after logout");
}

static void test_user_facing_errors_and_no_leak()
{
    const QString no_capacity = nax5SessionErrorMessage(Nax5SessionErrorNoCapacity);
    expect(no_capacity.contains(QStringLiteral("заняты")), "capacity message");
    expect(!no_capacity.contains(QStringLiteral("HTTP")), "no http");
    expect(!no_capacity.contains(QStringLiteral("409")), "no status code");
    expect(!nax5SessionPayloadLooksLeaky(kReserved), "clean payload");
    expect(nax5SessionPayloadLooksLeaky("{\"public_host\":\"1.2.3.4\",\"regist_key\":\"aa\"}"), "leaky payload");
    expect(nax5SessionErrorMessage(Nax5SessionErrorInvalidConnectionMaterial) == QStringLiteral("Не удалось подключиться к консоли."), "product generic");
    expect(nax5SessionErrorMessage(Nax5SessionErrorHostNotConfigured) == QStringLiteral("Не удалось подключиться к консоли."), "product host generic");
    expect(nax5SessionErrorMessage(Nax5SessionErrorHostNotConfigured, true) == QStringLiteral("Test failed: console host not configured"), "operator host");
    expect(nax5SessionErrorMessage(Nax5SessionErrorHostUnreachable, true) == QStringLiteral("Test failed: PS5 unreachable"), "operator unreachable");
    expect(nax5SessionErrorMessage(Nax5SessionErrorInvalidConnectionMaterial, true).contains(QStringLiteral("authentication")), "operator auth");
    expect(nax5SessionErrorMessage(Nax5SessionErrorConnectionTimeout, true) == QStringLiteral("Test failed: timeout"), "operator timeout");
    expect(!nax5SessionErrorMessage(Nax5SessionErrorInvalidConnectionMaterial, true).contains(QStringLiteral("regist")), "no regist");
    expect(!nax5SessionErrorMessage(Nax5SessionErrorNone, true).contains(QStringLiteral("Test failed")), "cleared error");
}

static void test_user_agent_version()
{
    expect(Nax5ApiConfig::userAgent().startsWith(QStringLiteral("NAX5/0.4")), "user agent 0.4");
}

int main()
{
    test_reserve_success();
    test_no_capacity();
    test_eligibility_denied();
    test_unauthenticated();
    test_conflict_and_rate_limit_and_server();
    test_malformed_and_timeout();
    test_current_none_and_present();
    test_cancel_success_and_failure();
    test_state_transitions_and_double_click();
    test_user_facing_errors_and_no_leak();
    test_user_agent_version();
    if (g_failed)
    {
        std::fprintf(stderr, "%d NAX5 session tests failed\n", g_failed);
        return 1;
    }
    std::printf("All NAX5 session tests passed\n");
    return 0;
}
