#pragma once

#include "nax5/session/nax5sessionerror.h"

#include <QByteArray>
#include <QString>
#include <QtGlobal>

struct Nax5AssignedConsole
{
    QString code;
    QString region;
};

struct Nax5SessionInfo
{
    QString id;
    QString status;
    QString reserved_at;
    QString lease_expires_at;
};

struct Nax5SessionParseResult
{
    Nax5SessionError error;
    bool has_session;
    bool has_console;
    Nax5SessionInfo session;
    Nax5AssignedConsole console;

    Nax5SessionParseResult()
        : error(Nax5SessionErrorInvalidResponse)
        , has_session(false)
        , has_console(false)
    {
    }
};

Nax5SessionError nax5MapSessionHttpError(int http_status, bool timed_out, bool no_network);
Nax5SessionParseResult nax5ParseReserveResponse(int http_status, const QByteArray &body);
Nax5SessionParseResult nax5ParseCurrentResponse(int http_status, const QByteArray &body);
Nax5SessionParseResult nax5ParseCancelResponse(int http_status, const QByteArray &body);
bool nax5SessionPayloadLooksLeaky(const QByteArray &body);
