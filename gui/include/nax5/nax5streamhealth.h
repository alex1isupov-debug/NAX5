#pragma once

#include "nax5/nax5pathprobe.h"

#include <QJsonObject>
#include <QVector>
#include <QtGlobal>

// Aggregates 1 Hz stream samples into the STREAM_HEALTH telemetry event sent
// every kWindowSeconds during play, so lag is visible on the server live.
// Keys must match ALLOWED_METADATA_KEYS in nax5-backend client_telemetry/schemas.py.

struct Nax5StreamSecond
{
    double packet_loss = 0;     // chiaki rolling fraction (0..1)
    int frames_lost_total = 0;  // cumulative receiver frames lost
    qint64 bitrate_kbps = -1;
    int render_dropped = 0;     // renderer drops in the last second
    double queue_depth = 0;     // render queue depth EMA
};

class Nax5StreamHealth
{
public:
    static constexpr int kWindowSeconds = 20;

    void reset();
    // Returns true when a full window is ready to be taken.
    bool add(const Nax5StreamSecond &second);
    QJsonObject take(const Nax5PathWindow &path);
    int size() const { return int(seconds.size()); }

private:
    QVector<Nax5StreamSecond> seconds;
    int frames_lost_base = -1;
};
