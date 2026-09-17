#pragma once

#include <QString>

bool nax5AuthRememberEnabled();
void nax5AuthSetRememberEnabled(bool enabled);

QString nax5AuthSavedEmail();
QString nax5AuthSavedPassword();
QString nax5AuthSavedSessionToken();

void nax5AuthSaveCredentials(const QString &email, const QString &password, const QString &session_token);
void nax5AuthClearCredentials();
