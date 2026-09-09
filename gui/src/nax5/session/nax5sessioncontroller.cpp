#include "nax5/session/nax5sessioncontroller.h"
#include "nax5/nax5apiclient.h"
#include "nax5/nax5authcontroller.h"
#include "nax5/session/nax5sessionparser.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QTimer>
#include <QUuid>

Nax5SessionController::Nax5SessionController(Nax5AuthController *auth, QObject *parent)
    : QObject(parent)
    , auth(auth)
    , api(new Nax5ApiClient(this))
    , lease_timer(new QTimer(this))
    , session_state(Nax5GameSessionStateIdle)
    , reserve_request_id(0)
    , current_request_id(0)
    , cancel_request_id(0)
    , ignore_cancel_result(false)
{
    lease_timer->setSingleShot(true);
    connect(lease_timer, &QTimer::timeout, this, &Nax5SessionController::syncCurrent);
    connect(api, &Nax5ApiClient::reserveFinished, this, &Nax5SessionController::onReserveFinished);
    connect(api, &Nax5ApiClient::currentFinished, this, &Nax5SessionController::onCurrentFinished);
    connect(api, &Nax5ApiClient::cancelFinished, this, &Nax5SessionController::onCancelFinished);
    if (auth)
        connect(auth, &Nax5AuthController::stateChanged, this, &Nax5SessionController::onAuthStateChanged);
    connect(qApp, &QCoreApplication::aboutToQuit, this, &Nax5SessionController::cancelBestEffort);
}

Nax5SessionController::~Nax5SessionController()
{
    ignore_cancel_result = true;
    cancelBestEffort();
}

bool Nax5SessionController::playEnabled() const
{
    return auth && auth->authenticated() && nax5SessionCanStartPlay(session_state);
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
    const QString message = nax5SessionErrorMessage(error);
    if (error_message == message)
        return;
    error_message = message;
    emit errorMessageChanged();
}

void Nax5SessionController::clearAssignment()
{
    session_id.clear();
    console_code.clear();
    console_region.clear();
    lease_timer->stop();
    emit assignmentChanged();
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
    reserve_request_id = 0;
    current_request_id = 0;
    cancel_request_id = 0;
    idempotency_key.clear();
    ignore_cancel_result = false;
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

    idempotency_key = QUuid::createUuid().toString(QUuid::WithoutBraces);
    setError(Nax5SessionErrorNone);
    setStatusText(QStringLiteral("Ищем свободную консоль..."));
    setState(nax5SessionReduce(session_state, Nax5GameSessionActionPlayClicked));
    reserve_request_id = api->reserveSession(auth->sessionToken(), idempotency_key);
}

void Nax5SessionController::release()
{
    if (!releaseEnabled() || session_id.isEmpty() || !auth)
        return;
    setError(Nax5SessionErrorNone);
    setStatusText(QStringLiteral("Освобождаем консоль..."));
    setState(nax5SessionReduce(session_state, Nax5GameSessionActionReleaseClicked));
    ignore_cancel_result = false;
    cancel_request_id = api->cancelSession(auth->sessionToken(), session_id);
}

void Nax5SessionController::cancelBestEffort()
{
    if (!auth || session_id.isEmpty())
    {
        if (session_state == Nax5GameSessionStateReserving)
            api->abortAll();
        return;
    }
    const QString token = auth->sessionToken();
    if (token.isEmpty())
        return;
    ignore_cancel_result = true;
    api->cancelSession(token, session_id);
}

void Nax5SessionController::releaseAndLogout()
{
    cancelBestEffort();
    resetLocal();
    if (auth)
        auth->logout();
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
        setStatusText(QStringLiteral("Консоль выделена"));
        setState(nax5SessionReduce(session_state, Nax5GameSessionActionReserveSucceeded));
        return;
    }
    if (result.error == Nax5SessionErrorActiveSessionExists && result.has_session)
    {
        applyAssignment(result);
        idempotency_key.clear();
        setError(Nax5SessionErrorNone);
        setStatusText(QStringLiteral("Консоль выделена"));
        setState(nax5SessionReduce(session_state, Nax5GameSessionActionSyncedOccupied));
        return;
    }

    idempotency_key.clear();
    clearAssignment();
    setError(result.error);
    if (result.error == Nax5SessionErrorNoCapacity || result.error == Nax5SessionErrorUserNotEligible || result.error == Nax5SessionErrorUnauthenticated)
    {
        setStatusText(nax5SessionErrorMessage(result.error));
        setState(nax5SessionReduce(session_state, result.error == Nax5SessionErrorNoCapacity ? Nax5GameSessionActionReserveNoCapacity : Nax5GameSessionActionReserveDenied));
        return;
    }
    setStatusText(nax5SessionErrorMessage(result.error));
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
        clearAssignment();
        if (session_state == Nax5GameSessionStateReserved || session_state == Nax5GameSessionStateCancelling)
        {
            setStatusText(QString());
            setError(Nax5SessionErrorNone);
            setState(nax5SessionReduce(session_state, Nax5GameSessionActionSyncedEmpty));
        }
        return;
    }
    applyAssignment(result);
    setError(Nax5SessionErrorNone);
    setStatusText(QStringLiteral("Консоль выделена"));
    setState(nax5SessionReduce(session_state, Nax5GameSessionActionSyncedOccupied));
}

void Nax5SessionController::onCancelFinished(quint64 request_id, const Nax5SessionParseResult &result)
{
    if (ignore_cancel_result)
        return;
    if (request_id != cancel_request_id)
        return;
    cancel_request_id = 0;
    if (result.error == Nax5SessionErrorNone || result.error == Nax5SessionErrorNotFound)
    {
        clearAssignment();
        setError(Nax5SessionErrorNone);
        setStatusText(QString());
        setState(nax5SessionReduce(session_state, Nax5GameSessionActionCancelSucceeded));
        return;
    }
    setError(result.error);
    setStatusText(nax5SessionErrorMessage(result.error));
    setState(nax5SessionReduce(session_state, Nax5GameSessionActionCancelFailed));
}
