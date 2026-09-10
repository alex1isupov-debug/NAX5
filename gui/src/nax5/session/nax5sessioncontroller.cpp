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
    , test_result_request_id(0)
    , ignore_cancel_result(false)
    , stream_was_connected(false)
    , operator_test_active(false)
{
    lease_timer->setSingleShot(true);
    connect(lease_timer, &QTimer::timeout, this, &Nax5SessionController::syncCurrent);
    connect(api, &Nax5ApiClient::reserveFinished, this, &Nax5SessionController::onReserveFinished);
    connect(api, &Nax5ApiClient::currentFinished, this, &Nax5SessionController::onCurrentFinished);
    connect(api, &Nax5ApiClient::cancelFinished, this, &Nax5SessionController::onCancelFinished);
    connect(api, &Nax5ApiClient::connectionFinished, this, &Nax5SessionController::onConnectionFinished);
    connect(api, &Nax5ApiClient::operatorFinished, this, [this](quint64 request_id, const Nax5ConnectionParseResult &result) {
        const bool silent = request_id == test_result_request_id;
        if (silent)
            test_result_request_id = 0;
        if (result.error != Nax5SessionErrorNone)
        {
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
    if (auth)
        connect(auth, &Nax5AuthController::stateChanged, this, &Nax5SessionController::onAuthStateChanged);
    if (backend)
        connect(backend, &QmlBackend::sessionChanged, this, &Nax5SessionController::onChiakiSessionChanged);
    connect(qApp, &QCoreApplication::aboutToQuit, this, &Nax5SessionController::cancelBestEffort);
}

Nax5SessionController::~Nax5SessionController()
{
    ignore_cancel_result = true;
    discardMaterial();
    cancelBestEffort();
}

bool Nax5SessionController::playEnabled() const
{
    return auth && auth->authenticated() && nax5SessionCanStartPlay(session_state);
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
    test_result_request_id = 0;
    idempotency_key.clear();
    operator_console_code.clear();
    ignore_cancel_result = false;
    stream_was_connected = false;
    operator_test_active = false;
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

    bumpGeneration();
    idempotency_key = QUuid::createUuid().toString(QUuid::WithoutBraces);
    setError(Nax5SessionErrorNone);
    setStatusText(QStringLiteral("Ищем свободную консоль..."));
    setState(nax5SessionReduce(session_state, Nax5GameSessionActionPlayClicked));
    reserve_request_id = api->reserveSession(auth->sessionToken(), idempotency_key);
}

void Nax5SessionController::release()
{
    if (!releaseEnabled())
        return;
    if (session_state == Nax5GameSessionStateReserving)
    {
        api->abortAll();
        resetLocal();
        return;
    }
    if (session_id.isEmpty() || !auth)
        return;
    bumpGeneration();
    reserve_request_id = 0;
    current_request_id = 0;
    connection_request_id = 0;
    setError(Nax5SessionErrorNone);
    setStatusText(QStringLiteral("Освобождаем консоль..."));
    setState(nax5SessionReduce(session_state, Nax5GameSessionActionReleaseClicked));
    ignore_cancel_result = false;
    if (session_state == Nax5GameSessionStateEnding)
        cancel_request_id = api->endSession(auth->sessionToken(), session_id);
    else
        cancel_request_id = api->cancelSession(auth->sessionToken(), session_id);
    if (backend && backend->qmlSession())
        backend->stopSession(false);
}

void Nax5SessionController::cancelBestEffort()
{
    bumpGeneration();
    reserve_request_id = 0;
    current_request_id = 0;
    connection_request_id = 0;
    if (!auth || session_id.isEmpty())
    {
        if (session_state == Nax5GameSessionStateReserving || session_state == Nax5GameSessionStateFetchingConnection)
            api->abortAll();
        return;
    }
    const QString token = auth->sessionToken();
    if (token.isEmpty())
        return;
    ignore_cancel_result = true;
    if (session_state == Nax5GameSessionStateActive)
        api->endSession(token, session_id);
    else if (session_state == Nax5GameSessionStateConnecting)
        api->failSession(token, session_id);
    else
        api->cancelSession(token, session_id);
}

void Nax5SessionController::releaseAndLogout()
{
    cancelBestEffort();
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
    if (request_id != reserve_request_id)
        return;
    reserve_request_id = 0;

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
    if (request_id != current_request_id)
        return;
    current_request_id = 0;
    if (!auth || !auth->authenticated())
        return;
    if (result.error != Nax5SessionErrorNone)
        return;
    if (!result.has_session)
    {
        if (operator_test_active || session_state == Nax5GameSessionStateConnecting || session_state == Nax5GameSessionStateActive)
            return;
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
    setState(nax5SessionReduce(session_state, Nax5GameSessionActionSyncedOccupied));
}

void Nax5SessionController::onCancelFinished(quint64 request_id, const Nax5SessionParseResult &result)
{
    if (ignore_cancel_result)
        return;
    if (request_id != cancel_request_id)
        return;
    cancel_request_id = 0;
    discardMaterial();
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
    if (request_id != connection_request_id)
        return;
    connection_request_id = 0;
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
    if (!operator_test && !result.material.session_id.isEmpty() && result.material.session_id != session_id)
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
        return;
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
        api->markConnected(auth->sessionToken(), session_id);
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
    if (!auth || session_id.isEmpty())
        return;
    ignore_cancel_result = true;
    api->failSession(auth->sessionToken(), session_id);
}

void Nax5SessionController::reportEnd()
{
    if (!auth || session_id.isEmpty())
        return;
    ignore_cancel_result = true;
    api->endSession(auth->sessionToken(), session_id);
}

void Nax5SessionController::reportOperatorTestResult(bool passed)
{
    if (!auth || !Nax5Runtime::operatorMode() || operator_console_code.isEmpty())
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
    Nax5Runtime::setLastOperatorConsoleCode(console_code);
    setError(Nax5SessionErrorNone);
    QJsonObject body;
    body.insert(QStringLiteral("version"), Nax5ConnectionContractVersion);
    body.insert(QStringLiteral("target"), target);
    body.insert(QStringLiteral("nickname"), nickname);
    body.insert(QStringLiteral("registKey"), QString::fromLatin1(regist_key.toBase64()));
    body.insert(QStringLiteral("morning"), QString::fromLatin1(morning.toBase64()));
    body.insert(QStringLiteral("consolePin"), console_pin);
    api->operatorProvision(auth->sessionToken(), console_code, QJsonDocument(body).toJson(QJsonDocument::Compact));
}

void Nax5SessionController::operatorActivate(const QString &console_code)
{
    if (!auth || !Nax5Runtime::operatorMode() || console_code.trimmed().isEmpty())
        return;
    Nax5Runtime::setLastOperatorConsoleCode(console_code);
    setError(Nax5SessionErrorNone);
    api->operatorActivate(auth->sessionToken(), console_code);
}

void Nax5SessionController::operatorTest(const QString &console_code)
{
    if (!auth || !Nax5Runtime::operatorMode() || !nax5SessionCanStartPlay(session_state) || console_code.trimmed().isEmpty())
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
