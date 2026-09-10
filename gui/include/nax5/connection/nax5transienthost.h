#pragma once

#include "nax5/connection/nax5connectionmaterial.h"

class Settings;
struct StreamSessionConnectInfo;

bool nax5FillStreamSessionConnectInfo(
    Settings *settings,
    const Nax5ConnectionMaterial &material,
    StreamSessionConnectInfo *out);
