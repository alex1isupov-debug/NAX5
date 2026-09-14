#include "nax5/session/nax5sessioncontroller.h"

#include "nax5/connection/nax5connectionparser.h"
#include "nax5/connection/nax5transienthost.h"
#include "nax5/nax5apiclient.h"
#include "nax5/nax5authcontroller.h"
#include "nax5/nax5runtime.h"
#include "nax5/session/nax5sessionparser.h"
#include "qmlbackend.h"
#include "streamsession.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUuid>

Nax5SessionController::Nax5SessionController(Nax5AuthController *auth, QmlBackend *backend, QObject *parent)
    : QObject(parent)
    , auth(auth)
    , backend(backend)
    , api(new Nax5ApiClient(this))
    , lease_timer(new QTimer(this))
    , session_state(Nax5GameSessionStateIdle)
    , generation(0)
    , stream_generation(0)
    , reserve_request_id(0)
    , current_request_id(0)
    , cancel_request_id(0)
    , connection_request_id(0)
    , connected_request_id(0)
    , test_result_request_id(0)
    , operator_request_id(0)
    , ignore_cancel_result(false)
    , stream_was_connected(false)
    , operator_test_active(false)
    , shutdown_started(false)
    , awaiting_abort_current(false)
    , pending_start_stream(false)
{
    pending_terminal.mutation = Nax5TerminalMutationNone;
    pending_terminal.generation = 0;
    pending_terminal.request_id = 0;
    pending_terminal.attempts = 0;
    pending_terminal.silent = true;
    lease_timer->setSingleShot(true);
    connect(lease_timer, &QTimer::timeout, this, &Nax5SessionController::syncCurrent);
    connect(api, &Nax5ApiClient::reserveFinished, this, &Nax5SessionController::onReserveFinished);
    connect(api, &Nax5ApiClient::currentFinished, this, &Nax5SessionController::onCurrentFinished);
    connect(api, &Nax5ApiClient::cancelFinished, this, &Nax5SessionController::onCancelFinished);
    connect(api, &Nax5ApiClient::connectionFinished, this, &Nax5SessionController::onConnectionFinished);
    connect(api, &Nax5ApiClient::connectedFinished, this, &Nax5SessionController::onConnectedFinished);
    connect(api, &Nax5ApiClient::failFinished, this, &Nax5SessionController::onFailFinished);
    connect(api, &Nax5ApiClient::endFinished, this, &Nax5SessionController::onEndFinished);
    connect(api, &Nax5ApiClient::operatorFinished, this, [this](quint64 request_id, const Nax5ConnectionParseResult &result) {
        if (request_id == operator_request_id)
            operator_request_id = 0;
        const bool silent = request_id == test_result_request_id;
        if (silent)
            test_result_request_id = 0;
        if (result.error != Nax5SessionErrorNone)
        {
            if (logoutIfUnauthenticated(result.error))
                return;
            if (!silent || (session_state != Nax5GameSessionStateConnecting && session_state != Nax5GameSessionStateActive))
            {
                setError(result.error);
                setStatusText(errorText(result.error));
            }
            return;
        }
        if (!silent)
        {
            setError(Nax5SessionErrorNone);
            setStatusText(QStringLiteral("Оператор: готово"));
        }
    });
    if (this->auth)
        connect(this->auth, &Nax5AuthController::stateChanged, this, &Nax5SessionController::onAuthStateChanged);
    if (this->backend)
        connect(this->backend, &QmlBackend::sessionChanged, this, &Nax5SessionController::onChiakiSessionChanged);
    connect(qApp, &QCoreApplication::aboutToQuit, this, &Nax5SessionController::prepareShutdown);
}

Nax5SessionController::~Nax5SessionController()
{
    disconnect(qApp, &QCoreApplication::aboutToQuit, this, &Nax5SessionController::prepareShutdown);
    ignore_cancel_result = true;
    pending_terminal.mutation = Nax5TerminalMutationNone;
    discardMaterial();
}

bool Nax5SessionController::playEnabled() const
{
    if (!auth || !auth->authenticated() || !nax5SessionCanStartPlay(session_state))
        return false;
    if (nax5SessionTerminalBlocksPlay(pending_terminal.mutation))
        return false;
    if (streamSessionAlive())
        return false;
    return true;
}

bool Nax5SessionController::streamSessionAlive() const
{
    return backend && backend->qmlSession();
}

QString Nax5SessionController::liveToken() const
{
    if (!auth)
        return QString();
    return auth->sessionToken();
}

quint64 Nax5SessionController::bumpGeneration()
{
    return ++generation;
}

void Nax5SessionController::setState(Nax5GameSessionState next)
{
    if (session_state == next)
        return;
    session_state = next;
    emit stateChanged();
}

void Nax5SessionController::setStatusText(const QString &text)
{
    if (status_text == text)
        return;
    status_text = text;
    emit statusTextChanged();
}

void Nax5SessionController::setError(Nax5SessionError error)
{
    const QString message = errorText(error);
    if (error_message == message)
        return;
    error_message = message;
    emit errorMessageChanged();
}

QString Nax5SessionController::errorText(Nax5SessionError error) const
{
    return nax5SessionErrorMessage(error, Nax5Runtime::operatorMode());
}

bool Nax5SessionController::isOperatorTest() const
{
    return operator_test_active || material.session_id == QLatin1String("operator-test");
}

void Nax5SessionController::clearAssignment()
{
    session_id.clear();
    console_code.clear();
    console_region.clear();
    lease_timer->stop();
    emit assignmentChanged();
}

void Nax5SessionController::discardMaterial()
{
    material.clear();
}

void Nax5SessionController::applyAssignment(const Nax5SessionParseResult &result)
{
    session_id = result.session.id;
    console_code = result.console.code;
    console_region = result.console.region;
    emit assignmentChanged();
    scheduleLeaseSync(result.session.lease_expires_at);
}

void Nax5SessionController::resetLocal()
{
    bumpGeneration();
    reserve_request_id = 0;
    current_request_id = 0;
    cancel_request_id = 0;
    connection_request_id = 0;
    connected_request_id = 0;
    test_result_request_id = 0;
    operator_request_id = 0;
    pending_terminal.mutation = Nax5TerminalMutationNone;
    pending_terminal.request_id = 0;
    pending_terminal.attempts = 0;
    idempotency_key.clear();
    operator_console_code.clear();
    ignore_cancel_result = false;
    stream_was_connected = false;
    operator_test_active = false;
    awaiting_abort_current = false;
    pending_start_stream = false;
    discardMaterial();
    clearAssignment();
    setError(Nax5SessionErrorNone);
    setStatusText(QString());
    setState(nax5SessionReduce(session_state, Nax5GameSessionActionReset));
}

void Nax5SessionController::scheduleLeaseSync(const QString &lease_expires_at)
{
    lease_timer->stop();
    if (lease_expires_at.isEmpty())
        return;
    const QDateTime expires = QDateTime::fromString(lease_expires_at, Qt::ISODate);
    if (!expires.isValid())
        return;
    const int msec = static_cast<int>(QDateTime::currentDateTimeUtc().msecsTo(expires.toUTC()));
    if (msec <= 0)
        QTimer::singleShot(0, this, &Nax5SessionController::syncCurrent);
    else
        lease_timer->start(msec + 250);
}

void Nax5SessionController::syncCurrent()
{
    if (!auth || !auth->authenticated())
        return;
    if (session_state != Nax5GameSessionStateIdle && session_state != Nax5GameSessionStateError)
        return;
    current_request_id = api->fetchCurrentSession(auth->sessionToken());
}

void Nax5SessionController::onAuthStateChanged()
{
    emit stateChanged();
    if (auth && auth->authenticated())
    {
        syncCurrent();
        return;
    }
    resetLocal();
}

void Nax5SessionController::play()
{
    if (!playEnabled())
        return;
    if (api->hasLane(Nax5ApiLaneReserve))
        return;
    if (nax5SessionTerminalBlocksPlay(pending_terminal.mutation))
        return;
    if (streamSessionAlive())
        return;

    bumpGeneration();
    idempotency_key = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ignore_cancel_result = false;
    setError(Nax5SessionErrorNone);
    setStatusText(QStringLiteral("Ищем свободную консоль..."));
    setState(nax5SessionReduce(session_state, Nax5GameSessionActionPlayClicked));
    reserve_request_id = api->reserveSession(liveToken(), idempotency_key);
}

void Nax5SessionController::release()
{
    if (!releaseEnabled())
        return;
    if (session_state == Nax5GameSessionStateReserving)
    {
        api->abortLane(Nax5ApiLaneReserve);
        reserve_request_id = 0;
        awaiting_abort_current = true;
        ignore_cancel_result = false;
        setError(Nax5SessionErrorNone);
        setStatusText(QStringLiteral("Освобождаем консоль..."));
        setState(nax5SessionReduce(session_state, Nax5GameSessionActionReleaseClicked));
        if (auth && auth->authenticated())
            current_request_id = api->fetchCurrentSession(auth->sessionToken());
        return;
    }
    if (session_id.isEmpty() || !auth)
        return;
    const Nax5TerminalMutation mutation = nax5ReleaseMutation(session_state, stream_was_connected);
    bumpGeneration();
    reserve_request_id = 0;
    current_request_id = 0;
    connection_request_id = 0;
    setError(Nax5SessionErrorNone);
    setStatusText(QStringLiteral("Освобождаем консоль..."));
    setState(nax5SessionReduce(session_state, Nax5GameSessionActionReleaseClicked));
    ignore_cancel_result = false;
    dispatchTerminal(mutation, false);
    if (streamSessionAlive())
        backend->stopSession(false);
}

void Nax5SessionController::dispatchTerminal(Nax5TerminalMutation mutation, bool silent)
{
    if (mutation == Nax5TerminalMutationNone)
        return;
    pending_terminal.mutation = mutation;
    pending_terminal.token = liveToken();
    pending_terminal.session_id = session_id;
    pending_terminal.generation = generation;
    pending_terminal.attempts = 0;
    pending_terminal.silent = silent;
    pending_terminal.request_id = 0;
    if (pending_terminal.token.isEmpty() || pending_terminal.session_id.isEmpty())
        return;
    sendPendingTerminal();
}

void Nax5SessionController::sendPendingTerminal()
{
    if (pending_terminal.mutation == Nax5TerminalMutationNone)
        return;
    if (pending_terminal.token.isEmpty() || pending_terminal.session_id.isEmpty())
        return;
    pending_terminal.attempts++;
    switch (pending_terminal.mutation) {
    case Nax5TerminalMutationEnd:
        pending_terminal.request_id = api->endSession(pending_terminal.token, pending_terminal.session_id);
        cancel_request_id = pending_terminal.request_id;
        break;
    case Nax5TerminalMutationFail:
        pending_terminal.request_id = api->failSession(pending_terminal.token, pending_terminal.session_id);
        cancel_request_id = pending_terminal.request_id;
        break;
    case Nax5TerminalMutationCancel:
        pending_terminal.request_id = api->cancelSession(pending_terminal.token, pending_terminal.session_id);
        cancel_request_id = pending_terminal.request_id;
        break;
    case Nax5TerminalMutationNone:
        break;
    }
}

bool Nax5SessionController::retryTerminalIfNeeded(const Nax5SessionParseResult &result)
{
    if (shutdown_started)
        return false;
    if (pending_terminal.mutation == Nax5TerminalMutationNone)
        return false;
    if (pending_terminal.generation != generation)
        return false;
    if (result.error != Nax5SessionErrorNetworkError)
        return false;
    if (pending_terminal.attempts >= nax5TerminalRetryLimit())
        return false;
    QTimer::singleShot(200, this, &Nax5SessionController::sendPendingTerminal);
    return true;
}

void Nax5SessionController::handleTerminalFinished(quint64 request_id, const Nax5SessionParseResult &result, Nax5TerminalMutation mutation)
{
    if (!nax5AcceptAsync(pending_terminal.generation, generation, pending_terminal.request_id, request_id)
        && request_id != cancel_request_id)
        return;
    if (pending_terminal.mutation != mutation && pending_terminal.mutation != Nax5TerminalMutationNone)
        return;
    if (retryTerminalIfNeeded(result))
        return;
    pending_terminal.request_id = 0;
    const bool silent = ignore_cancel_result || pending_terminal.silent || shutdown_started;
    pending_terminal.mutation = Nax5TerminalMutationNone;
    pending_terminal.attempts = 0;
    if (logoutIfUnauthenticated(result.error))
        return;
    if (silent)
        ignore_cancel_result = false;
    emit stateChanged();
    if (silent)
        return;
    onCancelFinished(request_id, result);
}

bool Nax5SessionController::logoutIfUnauthenticated(Nax5SessionError error)
{
    if (error != Nax5SessionErrorUnauthenticated)
        return false;
    if (auth)
        auth->logout();
    return true;
}

void Nax5SessionController::prepareShutdown()
{
    if (shutdown_started)
        return;
    shutdown_started = true;
    ignore_cancel_result = true;
    const Nax5TerminalMutation mutation = nax5ShutdownMutation(session_state, stream_was_connected);
    if (mutation == Nax5TerminalMutationNone || session_id.isEmpty() || liveToken().isEmpty())
    {
        if (session_state == Nax5GameSessionStateReserving)
            api->abortLane(Nax5ApiLaneReserve);
        bumpGeneration();
        return;
    }
    dispatchTerminal(mutation, true);
    bumpGeneration();
    reserve_request_id = 0;
    current_request_id = 0;
    connection_request_id = 0;
    connected_request_id = 0;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(api, &Nax5ApiClient::endFinished, &loop, &QEventLoop::quit);
    QObject::connect(api, &Nax5ApiClient::failFinished, &loop, &QEventLoop::quit);
    QObject::connect(api, &Nax5ApiClient::cancelFinished, &loop, &QEventLoop::quit);
    timer.start(nax5ShutdownGraceMs());
    loop.exec();
}

void Nax5SessionController::releaseAndLogout()
{
    prepareShutdown();
    resetLocal();
    if (auth)
        auth->logout();
}

void Nax5SessionController::fetchConnection()
{
    if (!auth || session_id.isEmpty())
        return;
    setStatusText(QStringLiteral("Подключаемся..."));
    connection_request_id = api->fetchConnection(auth->sessionToken(), session_id);
}

void Nax5SessionController::startStream()
{
    if (!backend)
        return;
    if (session_state != Nax5GameSessionStateFetchingConnection && session_state != Nax5GameSessionStateConnecting)
    {
        pending_start_stream = false;
        return;
    }
    if (!nax5SessionCanCreateStream(streamSessionAlive()))
    {
        pending_start_stream = true;
        return;
    }
    pending_start_stream = false;
    StreamSessionConnectInfo info;
    if (!nax5FillStreamSessionConnectInfo(backend->chiakiSettings(), material, &info))
    {
        discardMaterial();
        setError(Nax5SessionErrorInvalidConnectionMaterial);
        setStatusText(errorText(Nax5SessionErrorInvalidConnectionMaterial));
        setState(nax5SessionReduce(session_state, Nax5GameSessionActionConnectionFailed));
        if (isOperatorTest())
            reportOperatorTestResult(false);
        else
            reportFail();
        return;
    }
    backend->createSession(info);
    stream_generation = generation;
}

void Nax5SessionController::onReserveFinished(quint64 request_id, const Nax5SessionParseResult &result)
{
    if (!nax5AcceptAsync(generation, generation, reserve_request_id, request_id))
        return;
    reserve_request_id = 0;

    if (logoutIfUnauthenticated(result.error))
        return;

    if (result.error == Nax5SessionErrorNone && result.has_session)
    {
        applyAssignment(result);
        idempotency_key.clear();
        setError(Nax5SessionErrorNone);
        setState(nax5SessionReduce(session_state, Nax5GameSessionActionReserveSucceeded));
        fetchConnection();
        return;
    }
    if (result.error == Nax5SessionErrorActiveSessionExists && result.has_session)
    {
        applyAssignment(result);
        idempotency_key.clear();
        setError(Nax5SessionErrorNone);
        setState(nax5SessionReduce(session_state, Nax5GameSessionActionSyncedOccupied));
        fetchConnection();
        return;
    }

    idempotency_key.clear();
    clearAssignment();
    setError(result.error);
    if (result.error == Nax5SessionErrorNoCapacity || result.error == Nax5SessionErrorUserNotEligible || result.error == Nax5SessionErrorUnauthenticated)
    {
        setStatusText(errorText(result.error));
        setState(nax5SessionReduce(session_state, result.error == Nax5SessionErrorNoCapacity ? Nax5GameSessionActionReserveNoCapacity : Nax5GameSessionActionReserveDenied));
        return;
    }
    setStatusText(errorText(result.error));
    setState(nax5SessionReduce(session_state, Nax5GameSessionActionReserveFailed));
}

void Nax5SessionController::onCurrentFinished(quint64 request_id, const Nax5SessionParseResult &result)
{
    if (!nax5AcceptAsync(generation, generation, current_request_id, request_id))
        return;
    current_request_id = 0;
    if (!auth || !auth->authenticated())
        return;
    if (logoutIfUnauthenticated(result.error))
        return;
    if (result.error != Nax5SessionErrorNone)
    {
        if (awaiting_abort_current)
        {
            awaiting_abort_current = false;
            setError(result.error);
            setStatusText(errorText(result.error));
            setState(Nax5GameSessionStateError);
        }
        return;
    }
    if (!result.has_session)
    {
        if (operator_test_active || session_state == Nax5GameSessionStateConnecting || session_state == Nax5GameSessionStateActive)
            return;
        if (awaiting_abort_current)
        {
            awaiting_abort_current = false;
            if (nax5SessionShouldResetLocalAfterAbortCurrent(false))
                resetLocal();
            return;
        }
        discardMaterial();
        clearAssignment();
        if (nax5SessionHasAssignment(session_state) || session_state == Nax5GameSessionStateCancelling)
        {
            setStatusText(QString());
            setError(Nax5SessionErrorNone);
            setState(nax5SessionReduce(session_state, Nax5GameSessionActionSyncedEmpty));
        }
        return;
    }
    if (operator_test_active || session_state == Nax5GameSessionStateConnecting || session_state == Nax5GameSessionStateActive)
        return;
    applyAssignment(result);
    setError(Nax5SessionErrorNone);
    if (awaiting_abort_current)
    {
        awaiting_abort_current = false;
        if (session_state != Nax5GameSessionStateCancelling)
            setState(nax5SessionReduce(session_state, Nax5GameSessionActionReleaseClicked));
        dispatchTerminal(Nax5TerminalMutationCancel, false);
        return;
    }
    const Nax5GameSessionState previous = session_state;
    setState(nax5SessionReduce(session_state, Nax5GameSessionActionSyncedOccupied));
    if (nax5SessionShouldFetchOnSyncedOccupied(previous))
        fetchConnection();
}

void Nax5SessionController::onCancelFinished(quint64 request_id, const Nax5SessionParseResult &result)
{
    if (ignore_cancel_result || shutdown_started)
        return;
    if (!nax5AcceptAsync(generation, generation, cancel_request_id, request_id)
        && request_id != pending_terminal.request_id)
        return;
    if (retryTerminalIfNeeded(result))
        return;
    cancel_request_id = 0;
    pending_terminal.mutation = Nax5TerminalMutationNone;
    discardMaterial();
    if (logoutIfUnauthenticated(result.error))
        return;
    if (result.error == Nax5SessionErrorNone || result.error == Nax5SessionErrorNotFound)
    {
        clearAssignment();
        setError(Nax5SessionErrorNone);
        setStatusText(QString());
        setState(nax5SessionReduce(session_state, Nax5GameSessionActionCancelSucceeded));
        return;
    }
    setError(result.error);
    setStatusText(errorText(result.error));
    setState(nax5SessionReduce(session_state, Nax5GameSessionActionCancelFailed));
}

void Nax5SessionController::onConnectionFinished(quint64 request_id, const Nax5ConnectionParseResult &result)
{
    if (!nax5AcceptAsync(generation, generation, connection_request_id, request_id))
        return;
    connection_request_id = 0;
    if (logoutIfUnauthenticated(result.error))
        return;
    if (session_state != Nax5GameSessionStateFetchingConnection && session_state != Nax5GameSessionStateConnecting)
    {
        if (result.has_material)
        {
            Nax5ConnectionMaterial stale = result.material;
            stale.clear();
        }
        return;
    }
    if (result.error != Nax5SessionErrorNone || !result.has_material)
    {
        discardMaterial();
        setError(result.error == Nax5SessionErrorNone ? Nax5SessionErrorInvalidConnectionMaterial : result.error);
        setStatusText(errorText(result.error == Nax5SessionErrorNone ? Nax5SessionErrorInvalidConnectionMaterial : result.error));
        setState(nax5SessionReduce(session_state, Nax5GameSessionActionConnectionFailed));
        if (isOperatorTest() && result.has_material)
            reportOperatorTestResult(false);
        else if (!isOperatorTest())
            reportFail();
        operator_test_active = false;
        return;
    }
    const bool operator_test = result.material.session_id == QLatin1String("operator-test");
    if (!operator_test && !nax5AcceptSessionIdentity(session_id, result.material.session_id) && !result.material.session_id.isEmpty())
    {
        discardMaterial();
        setError(Nax5SessionErrorInvalidConnectionMaterial);
        setStatusText(errorText(Nax5SessionErrorInvalidConnectionMaterial));
        setState(nax5SessionReduce(session_state, Nax5GameSessionActionConnectionFailed));
        reportFail();
        return;
    }
    operator_test_active = operator_test;
    if (operator_test)
        session_id.clear();
    material = result.material;
    setError(Nax5SessionErrorNone);
    setStatusText(QStringLiteral("Подключаемся..."));
    setState(nax5SessionReduce(session_state, Nax5GameSessionActionConnectionReceived));
    startStream();
}

void Nax5SessionController::onChiakiSessionChanged(StreamSession *session)
{
    if (!session)
    {
        emit stateChanged();
        if (pending_start_stream)
            startStream();
        return;
    }
    connect(session, &StreamSession::ConnectedChanged, this, &Nax5SessionController::onStreamConnected, Qt::UniqueConnection);
    connect(session, &StreamSession::SessionQuit, this, &Nax5SessionController::onStreamQuit, Qt::UniqueConnection);
}

void Nax5SessionController::onStreamConnected()
{
    if (stream_generation != generation)
        return;
    if (!backend || !backend->qmlSession() || !backend->qmlSession()->GetConnected())
        return;
    if (session_state != Nax5GameSessionStateConnecting)
        return;
    stream_was_connected = true;
    setState(nax5SessionReduce(session_state, Nax5GameSessionActionStreamConnected));
    setStatusText(QStringLiteral("Игра"));
    if (isOperatorTest())
    {
        reportOperatorTestResult(true);
        return;
    }
    if (auth && !session_id.isEmpty())
        connected_request_id = api->markConnected(auth->sessionToken(), session_id);
}

void Nax5SessionController::onConnectedFinished(quint64 request_id, const Nax5SessionParseResult &result)
{
    if (!nax5AcceptAsync(generation, generation, connected_request_id, request_id))
        return;
    connected_request_id = 0;
    if (logoutIfUnauthenticated(result.error))
        return;
    if (result.error == Nax5SessionErrorNetworkError && !shutdown_started && session_state == Nax5GameSessionStateActive && auth && !session_id.isEmpty())
        connected_request_id = api->markConnected(auth->sessionToken(), session_id);
}

static Nax5SessionError errorFromQuitReason(ChiakiQuitReason reason)
{
    switch (reason) {
    case CHIAKI_QUIT_REASON_SESSION_REQUEST_CONNECTION_REFUSED:
    case CHIAKI_QUIT_REASON_CTRL_CONNECT_FAILED:
    case CHIAKI_QUIT_REASON_CTRL_CONNECTION_REFUSED:
        return Nax5SessionErrorHostUnreachable;
    case CHIAKI_QUIT_REASON_SESSION_REQUEST_UNKNOWN:
    case CHIAKI_QUIT_REASON_CTRL_UNKNOWN:
    case CHIAKI_QUIT_REASON_STREAM_CONNECTION_UNKNOWN:
        return Nax5SessionErrorConnectionTimeout;
    default:
        return Nax5SessionErrorInvalidConnectionMaterial;
    }
}

void Nax5SessionController::onStreamQuit(ChiakiQuitReason reason, const QString &reason_str)
{
    Q_UNUSED(reason_str);
    if (stream_generation != generation)
        return;
    pending_start_stream = false;
    discardMaterial();
    if (session_state == Nax5GameSessionStateIdle
        || session_state == Nax5GameSessionStateError
        || session_state == Nax5GameSessionStateCancelling
        || session_state == Nax5GameSessionStateEnding)
        return;
    if (stream_was_connected)
    {
        if (!isOperatorTest())
            reportEnd();
        setState(nax5SessionReduce(session_state, Nax5GameSessionActionStreamEnded));
        setStatusText(QString());
        operator_test_active = false;
        clearAssignment();
        return;
    }
    const Nax5SessionError error = errorFromQuitReason(reason);
    setError(error);
    setStatusText(errorText(error));
    setState(nax5SessionReduce(session_state, Nax5GameSessionActionStreamFailed));
    if (isOperatorTest())
        reportOperatorTestResult(false);
    else
        reportFail();
    operator_test_active = false;
    clearAssignment();
}

void Nax5SessionController::reportFail()
{
    ignore_cancel_result = true;
    dispatchTerminal(Nax5TerminalMutationFail, true);
}

void Nax5SessionController::reportEnd()
{
    ignore_cancel_result = true;
    dispatchTerminal(Nax5TerminalMutationEnd, true);
}

void Nax5SessionController::onFailFinished(quint64 request_id, const Nax5SessionParseResult &result)
{
    handleTerminalFinished(request_id, result, Nax5TerminalMutationFail);
}

void Nax5SessionController::onEndFinished(quint64 request_id, const Nax5SessionParseResult &result)
{
    handleTerminalFinished(request_id, result, Nax5TerminalMutationEnd);
}

void Nax5SessionController::reportOperatorTestResult(bool passed)
{
    if (!auth || !Nax5Runtime::operatorMode() || operator_console_code.isEmpty())
        return;
    if (api->hasLane(Nax5ApiLaneOperator) && test_result_request_id != 0)
        return;
    QJsonObject body;
    body.insert(QStringLiteral("passed"), passed);
    test_result_request_id = api->operatorTestResult(
        auth->sessionToken(),
        operator_console_code,
        QJsonDocument(body).toJson(QJsonDocument::Compact));
}

void Nax5SessionController::provisionFromFields(const QString &console_code, int target, const QByteArray &regist_key, const QByteArray &morning, const QString &console_pin, const QString &nickname)
{
    if (!auth || !Nax5Runtime::operatorMode() || console_code.trimmed().isEmpty())
        return;
    if (api->hasLane(Nax5ApiLaneOperator))
        return;
    Nax5Runtime::setLastOperatorConsoleCode(console_code);
    setError(Nax5SessionErrorNone);
    QJsonObject body;
    body.insert(QStringLiteral("version"), Nax5ConnectionContractVersion);
    body.insert(QStringLiteral("target"), target);
    body.insert(QStringLiteral("nickname"), nickname);
    body.insert(QStringLiteral("registKey"), QString::fromLatin1(regist_key.toBase64()));
    body.insert(QStringLiteral("morning"), QString::fromLatin1(morning.toBase64()));
    body.insert(QStringLiteral("consolePin"), console_pin);
    operator_request_id = api->operatorProvision(auth->sessionToken(), console_code, QJsonDocument(body).toJson(QJsonDocument::Compact));
}

void Nax5SessionController::operatorActivate(const QString &console_code)
{
    if (!auth || !Nax5Runtime::operatorMode() || console_code.trimmed().isEmpty())
        return;
    if (api->hasLane(Nax5ApiLaneOperator))
        return;
    Nax5Runtime::setLastOperatorConsoleCode(console_code);
    setError(Nax5SessionErrorNone);
    operator_request_id = api->operatorActivate(auth->sessionToken(), console_code);
}

void Nax5SessionController::operatorTest(const QString &console_code)
{
    if (!auth || !Nax5Runtime::operatorMode() || !nax5SessionCanStartPlay(session_state) || console_code.trimmed().isEmpty())
        return;
    if (api->hasLane(Nax5ApiLaneConnection))
        return;
    bumpGeneration();
    operator_console_code = console_code.trimmed();
    operator_test_active = true;
    stream_was_connected = false;
    session_id.clear();
    Nax5Runtime::setLastOperatorConsoleCode(operator_console_code);
    setError(Nax5SessionErrorNone);
    setStatusText(QStringLiteral("Проверяем подключение..."));
    setState(Nax5GameSessionStateFetchingConnection);
    connection_request_id = api->operatorTestConnection(auth->sessionToken(), operator_console_code);
}
