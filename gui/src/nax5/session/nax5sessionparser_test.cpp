#include "nax5/nax5apiconfig.h"
#include "nax5/nax5apilane.h"
#include "nax5/nax5operatorhost.h"
#include "nax5/session/nax5sessionerror.h"
#include "nax5/session/nax5sessionlifecycle.h"
#include "nax5/session/nax5sessionparser.h"
#include "nax5/session/nax5sessionstate.h"

#include <QObject>
#include <QPointer>
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

static void test_request_lanes_do_not_abort_unrelated()
{
    expect(nax5ApiLaneAllowsConcurrent(Nax5ApiLaneTerminal), "terminal concurrent");
    expect(!nax5ApiLaneAllowsConcurrent(Nax5ApiLaneReserve), "reserve exclusive");
    expect(!nax5ApiShouldAbortExisting(Nax5ApiLaneConnection, Nax5ApiLaneTerminal), "end does not abort connection");
    expect(!nax5ApiShouldAbortExisting(Nax5ApiLaneTerminal, Nax5ApiLaneTerminal), "connected does not abort end");
    expect(nax5ApiShouldAbortExisting(Nax5ApiLaneReserve, Nax5ApiLaneReserve), "second reserve replaces first");
    expect(!nax5ApiShouldAbortExisting(Nax5ApiLaneQuery, Nax5ApiLaneReserve), "query does not abort reserve");
    expect(!nax5ApiShouldAbortExisting(Nax5ApiLaneOperator, Nax5ApiLaneTerminal), "operator does not abort terminal");
}

static void test_shutdown_and_stale_lifecycle()
{
    expect(nax5ShutdownMutation(Nax5GameSessionStateFetchingConnection, false) == Nax5TerminalMutationCancel, "reserved cancel");
    expect(nax5ShutdownMutation(Nax5GameSessionStateConnecting, false) == Nax5TerminalMutationFail, "connecting fail");
    expect(nax5ShutdownMutation(Nax5GameSessionStateActive, false) == Nax5TerminalMutationEnd, "active end");
    expect(nax5ShutdownMutation(Nax5GameSessionStateConnecting, true) == Nax5TerminalMutationEnd, "connected stream end");
    expect(nax5StreamQuitMutation(true, false) == Nax5TerminalMutationEnd, "quit after connected");
    expect(nax5StreamQuitMutation(false, false) == Nax5TerminalMutationFail, "quit before connected");
    expect(nax5StreamQuitMutation(true, true) == Nax5TerminalMutationNone, "operator test no product end");
    expect(nax5AcceptAsync(2, 2, 9, 9), "same generation request");
    expect(!nax5AcceptAsync(3, 2, 9, 9), "stale generation ignored");
    expect(!nax5AcceptAsync(2, 2, 10, 9), "stale request ignored");
    expect(!nax5AcceptAsync(2, 2, 0, 9), "cleared request ignored");
    expect(nax5AcceptSessionIdentity(QStringLiteral("session-a"), QStringLiteral("session-a")), "same session");
    expect(!nax5AcceptSessionIdentity(QStringLiteral("session-b"), QStringLiteral("session-a")), "session b ignores a");
    expect(nax5TerminalRetryLimit() >= 2, "bounded retries");
    expect(nax5ShutdownGraceMs() > 0 && nax5ShutdownGraceMs() <= 1000, "short shutdown window");
}

static void test_logout_and_connected_end_races()
{
    expect(nax5SessionReduce(Nax5GameSessionStateReserving, Nax5GameSessionActionReset) == Nax5GameSessionStateIdle, "logout during reserve");
    expect(nax5SessionReduce(Nax5GameSessionStateFetchingConnection, Nax5GameSessionActionReset) == Nax5GameSessionStateIdle, "logout during connection");
    expect(nax5SessionReduce(Nax5GameSessionStateConnecting, Nax5GameSessionActionReleaseClicked) == Nax5GameSessionStateEnding, "connected then end");
    expect(nax5SessionReduce(Nax5GameSessionStateEnding, Nax5GameSessionActionCancelSucceeded) == Nax5GameSessionStateIdle, "end ack idle");
    expect(nax5SessionReduce(Nax5GameSessionStateFetchingConnection, Nax5GameSessionActionReleaseClicked) == Nax5GameSessionStateCancelling, "cancel during connection");
    expect(!nax5SessionCanStartPlay(Nax5GameSessionStateReserving), "double play ignored");
    expect(!nax5SessionCanRelease(Nax5GameSessionStateCancelling), "double cancel ignored");
    expect(!nax5SessionCanRelease(Nax5GameSessionStateEnding), "end already pending");
}

static void test_operator_multi_host_selection()
{
    Nax5SyntheticRegisteredHost hosts[2];
    hosts[0].nickname = QStringLiteral("PS5-ONE");
    hosts[0].regist_key = QByteArray("key-one");
    hosts[1].nickname = QStringLiteral("PS5-TWO");
    hosts[1].regist_key = QByteArray("key-two");
    expect(nax5PickSyntheticRegisteredHost(hosts, 2, 1)->nickname == QStringLiteral("PS5-TWO"), "selected second host");
    expect(nax5PickSyntheticRegisteredHost(hosts, 2, 1)->regist_key != hosts[0].regist_key, "not index 0 fallback");
    expect(nax5PickSyntheticRegisteredHost(hosts, 2, 0)->nickname == QStringLiteral("PS5-ONE"), "index 0 only if selected");
    expect(nax5PickSyntheticRegisteredHost(hosts, 2, -1) == nullptr, "reject missing selection");
    expect(nax5PickSyntheticRegisteredHost(hosts, 2, 2) == nullptr, "reject oob");
    expect(nax5ValidateProvisionSelection(-1, false, false, QStringLiteral("PS5-439")) == Nax5OperatorHostNoSelection, "no host");
    expect(nax5ValidateProvisionSelection(1, true, false, QStringLiteral("PS5-439")) == Nax5OperatorHostNotRegistered, "unregistered");
    expect(nax5ValidateProvisionSelection(1, true, true, QString()) == Nax5OperatorHostMissingConsoleCode, "missing code");
    expect(nax5ValidateProvisionSelection(1, true, true, QStringLiteral("PS5-439")) == Nax5OperatorHostOk, "explicit host and code");
}

static void test_shutdown_lifetime_qpointer()
{
    QObject *owner = new QObject();
    QObject *dependency = new QObject(owner);
    QObject *controller = new QObject(dependency);
    QPointer<QObject> dependency_ptr = dependency;
    QPointer<QObject> controller_ptr = controller;
    delete owner;
    expect(controller_ptr.isNull(), "controller cannot outlive parent dependency");
    expect(dependency_ptr.isNull(), "dependency destroyed with owner");
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
    test_request_lanes_do_not_abort_unrelated();
    test_shutdown_and_stale_lifecycle();
    test_logout_and_connected_end_races();
    test_operator_multi_host_selection();
    test_shutdown_lifetime_qpointer();
    if (g_failed)
    {
        std::fprintf(stderr, "%d NAX5 session tests failed\n", g_failed);
        return 1;
    }
    std::printf("All NAX5 session tests passed\n");
    return 0;
}
