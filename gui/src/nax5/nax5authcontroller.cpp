#include "nax5/nax5authcontroller.h"
#include "nax5/nax5apiclient.h"
#include "nax5/nax5authparser.h"

Nax5AuthController::Nax5AuthController(QObject *parent)
    : QObject(parent)
    , api(new Nax5ApiClient(this))
    , auth_state(Nax5AuthStateUnauthenticated)
    , user_id(0)
    , email_verified(false)
    , login_request_id(0)
    , me_request_id(0)
    , logout_request_id(0)
{
    connect(api, &Nax5ApiClient::loginFinished, this, &Nax5AuthController::onLoginFinished);
    connect(api, &Nax5ApiClient::meFinished, this, &Nax5AuthController::onMeFinished);
    connect(api, &Nax5ApiClient::logoutFinished, this, &Nax5AuthController::onLogoutFinished);
}

Nax5AuthController::~Nax5AuthController()
{
    clearSessionToken();
    api->abortAll();
}

void Nax5AuthController::setState(Nax5AuthState next)
{
    if (auth_state == next)
        return;
    auth_state = next;
    emit stateChanged();
}

void Nax5AuthController::setError(Nax5AuthError error)
{
    const QString message = nax5AuthErrorMessage(error);
    if (error_message == message)
        return;
    error_message = message;
    emit errorMessageChanged();
}

void Nax5AuthController::clearAccount()
{
    user_id = 0;
    account_email.clear();
    account_city.clear();
    access_status.clear();
    email_verified = false;
    emit accountChanged();
}

void Nax5AuthController::clearSessionToken()
{
    if (!session_token.isEmpty())
        session_token.fill(QLatin1Char(' '));
    session_token.clear();
}

void Nax5AuthController::login(const QString &email, const QString &password)
{
    if (!nax5AuthCanStartLogin(auth_state))
        return;

    setError(Nax5AuthErrorNone);
    setState(nax5AuthReduce(auth_state, Nax5AuthActionLoginClicked));
    login_request_id = api->login(email, password);
}

void Nax5AuthController::logout()
{
    login_request_id = 0;
    me_request_id = 0;
    const QString token = session_token;
    clearSessionToken();
    clearAccount();
    setError(Nax5AuthErrorNone);
    setState(nax5AuthReduce(auth_state, Nax5AuthActionLogout));
    if (!token.isEmpty())
        logout_request_id = api->logout(token);
}

void Nax5AuthController::onLoginFinished(quint64 request_id, const Nax5LoginParseResult &result)
{
    if (request_id != login_request_id)
        return;
    login_request_id = 0;
    if (result.error != Nax5AuthErrorNone)
    {
        clearSessionToken();
        setError(result.error);
        setState(nax5AuthReduce(auth_state, Nax5AuthActionLoginFailed));
        return;
    }
    session_token = result.session_token;
    me_request_id = api->fetchMe(session_token);
}

void Nax5AuthController::onMeFinished(quint64 request_id, const Nax5MeParseResult &result)
{
    if (request_id != me_request_id)
        return;
    me_request_id = 0;
    if (!result.ok)
    {
        clearSessionToken();
        clearAccount();
        setError(result.network_failure ? Nax5AuthErrorNetworkError : Nax5AuthErrorInvalidResponse);
        setState(nax5AuthReduce(auth_state, Nax5AuthActionLoginFailed));
        return;
    }
    user_id = result.user_id;
    account_email = result.email;
    account_city = result.city;
    access_status = result.access_status;
    email_verified = result.email_verified;
    emit accountChanged();
    setError(Nax5AuthErrorNone);
    setState(nax5AuthReduce(auth_state, Nax5AuthActionLoginSucceeded));
}

void Nax5AuthController::onLogoutFinished(quint64 request_id)
{
    if (request_id != logout_request_id)
        return;
    logout_request_id = 0;
}
