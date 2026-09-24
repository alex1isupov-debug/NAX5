#pragma once

#include <QString>
#include <QtGlobal>
#include <memory>

struct Nax5NetworkHint
{
    QString adapter_type = QStringLiteral("unknown");
    QString adapter_name;
    qint64 link_speed_mbps = -1;
    bool active = false;
};

// Wine/Proton ships IP Helper stubs that abort the process when called,
// so all IP Helper diagnostics are skipped there.
bool nax5RunningUnderWine();

Nax5NetworkHint nax5QueryNetworkHint();
QString nax5NetworkHintText(const Nax5NetworkHint &hint);

struct Nax5NetworkDiagnosticsSummary
{
    qint64 probes_sent = 0;
    qint64 probe_replies = 0;
    double rtt_sum_ms = 0;
    double rtt_min_ms = -1;
    double rtt_max_ms = -1;
    quint64 rx_errors_delta = 0;
    quint64 tx_errors_delta = 0;
    quint64 rx_discards_delta = 0;
    quint64 tx_discards_delta = 0;
    qint64 route_changes = 0;
};

class Nax5NetworkDiagnostics
{
public:
    Nax5NetworkDiagnostics();
    ~Nax5NetworkDiagnostics();
    Nax5NetworkDiagnostics(const Nax5NetworkDiagnostics &) = delete;
    Nax5NetworkDiagnostics &operator=(const Nax5NetworkDiagnostics &) = delete;
    void start(const QString &session_id);
    void stop();
    Nax5NetworkDiagnosticsSummary summary() const;
private:
    class Impl;
    std::unique_ptr<Impl> impl;
};
