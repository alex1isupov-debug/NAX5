#include "nax5/connection/nax5connectionmaterial.h"

void Nax5ConnectionMaterial::clear()
{
    version = 0;
    target = 0;
    host.clear();
    nickname.clear();
    if (!regist_key.isEmpty())
        regist_key.fill(' ');
    regist_key.clear();
    if (!morning.isEmpty())
        morning.fill(' ');
    morning.clear();
    console_pin.fill(QLatin1Char(' '));
    console_pin.clear();
    session_id.clear();
    session_status.clear();
}

bool nax5ConnectionMaterialIsEmpty(const Nax5ConnectionMaterial &material)
{
    return material.regist_key.isEmpty()
        && material.morning.isEmpty()
        && material.host.isEmpty();
}
