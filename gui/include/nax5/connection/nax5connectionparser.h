#pragma once

#include "nax5/connection/nax5connectionmaterial.h"
#include "nax5/session/nax5sessionerror.h"

#include <QByteArray>
#include <QString>

struct Nax5ConnectionParseResult
{
    Nax5SessionError error;
    Nax5ConnectionMaterial material;
    bool has_material;

    Nax5ConnectionParseResult()
        : error(Nax5SessionErrorInvalidResponse)
        , has_material(false)
    {
    }
};

Nax5ConnectionParseResult nax5ParseConnectionResponse(int http_status, const QByteArray &body);
Nax5ConnectionParseResult nax5ParseOperatorProvisionResponse(int http_status, const QByteArray &body);
bool nax5DecodeRpKey(const QString &base64, int expected_size, QByteArray *out);
bool nax5TargetIsSupportedDirectPs5(int target);
