#include "nax5/nax5networkhint.h"
#include "nax5/nax5processlog.h"

#include <QByteArray>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QThread>
#include <QWaitCondition>
#include <QtGlobal>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <wlanapi.h>
#endif

namespace {

QString boolText(bool value)
{
    return value ? QStringLiteral("true") : QStringLiteral("false");
}

QString adapterTypeName(unsigned long if_type)
{
#ifdef Q_OS_WIN
    if (if_type == IF_TYPE_IEEE80211)
        return QStringLiteral("wifi");
    if (if_type == IF_TYPE_ETHERNET_CSMACD)
        return QStringLiteral("ethernet");
#else
    Q_UNUSED(if_type);
#endif
    return QStringLiteral("unknown");
}

#ifdef Q_OS_WIN
ULONG defaultRouteInterfaceIndex()
{
    PMIB_IPFORWARD_TABLE2 table = nullptr;
    if (GetIpForwardTable2(AF_INET, &table) != NO_ERROR || !table)
        return 0;
    ULONG index = 0;
    quint64 best_metric = ~quint64(0);
    for (ULONG i = 0; i < table->NumEntries; ++i)
    {
        const MIB_IPFORWARD_ROW2 &row = table->Table[i];
        if (row.DestinationPrefix.PrefixLength != 0)
            continue;
        MIB_IPINTERFACE_ROW iface;
        InitializeIpInterfaceEntry(&iface);
        iface.Family = AF_INET;
        iface.InterfaceIndex = row.InterfaceIndex;
        if (GetIpInterfaceEntry(&iface) != NO_ERROR || !iface.Connected) continue;
        const quint64 metric = quint64(row.Metric) + iface.Metric;
        if (metric < best_metric) { best_metric = metric; index = row.InterfaceIndex; }
    }
    FreeMibTable(table);
    return index;
}
#endif

} // namespace

Nax5NetworkHint nax5QueryNetworkHint()
{
    Nax5NetworkHint hint;
#ifdef Q_OS_WIN
    const ULONG flags = GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG size = 16 * 1024;
    QByteArray buffer(static_cast<int>(size), 0);
    auto *addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
    ULONG status = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, addresses, &size);
    if (status == ERROR_BUFFER_OVERFLOW)
    {
        buffer.resize(static_cast<int>(size));
        addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
        status = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, addresses, &size);
    }
    if (status != NO_ERROR)
        return hint;

    const ULONG preferred_index = defaultRouteInterfaceIndex();
    PIP_ADAPTER_ADDRESSES chosen = nullptr;
    for (PIP_ADAPTER_ADDRESSES adapter = addresses; adapter; adapter = adapter->Next)
    {
        if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
            continue;
        if (adapter->OperStatus != IfOperStatusUp)
            continue;
        if (preferred_index != 0 && adapter->IfIndex == preferred_index)
        {
            chosen = adapter;
            break;
        }
        if (chosen)
            continue;
        if (!adapter->FirstGatewayAddress)
            continue;
        if (adapter->IfType != IF_TYPE_IEEE80211 && adapter->IfType != IF_TYPE_ETHERNET_CSMACD)
            continue;
        chosen = adapter;
    }
    if (!chosen)
        return hint;

    hint.active = true;
    hint.adapter_type = adapterTypeName(chosen->IfType);
    if (chosen->FriendlyName)
        hint.adapter_name = QString::fromWCharArray(chosen->FriendlyName);
    if (chosen->TransmitLinkSpeed > 0)
        hint.link_speed_mbps = static_cast<qint64>(chosen->TransmitLinkSpeed / 1000000ull);
#endif
    return hint;
}

QString nax5NetworkHintText(const Nax5NetworkHint &hint)
{
    QString text;
    text += QStringLiteral("network_hint_scope=lowest_metric_ipv4_default_route_not_ps5_path\n");
    text += QStringLiteral("network_adapter_type=%1\n").arg(hint.adapter_type.isEmpty() ? QStringLiteral("unknown") : hint.adapter_type);
    text += QStringLiteral("network_adapter_name=%1\n").arg(hint.adapter_name);
    if (hint.link_speed_mbps >= 0)
        text += QStringLiteral("network_link_speed_mbps=%1\n").arg(hint.link_speed_mbps);
    else
        text += QStringLiteral("network_link_speed_mbps=\n");
    text += QStringLiteral("network_active=%1\n").arg(boolText(hint.active));
    return text;
}

namespace {
struct NetworkSample
{
    bool available = false;
    bool route_changed = false;
    QString adapter_type = QStringLiteral("unknown");
    quint64 rx_bytes_delta = 0, tx_bytes_delta = 0;
    quint64 rx_errors_delta = 0, tx_errors_delta = 0;
    quint64 rx_discards_delta = 0, tx_discards_delta = 0;
    qint64 rx_link_mbps = -1, tx_link_mbps = -1;
    qint64 wifi_signal_percent = -1, wifi_rx_rate_kbps = -1, wifi_tx_rate_kbps = -1;
    bool gateway_probe_attempted = false, gateway_probe_ok = false;
    double gateway_rtt_ms = -1;
};

#ifdef Q_OS_WIN
struct InterfaceCounters
{
    bool valid = false;
    ULONG index = 0;
    quint64 in_octets = 0, out_octets = 0;
    quint64 in_errors = 0, out_errors = 0;
    quint64 in_discards = 0, out_discards = 0;
};
quint64 counterDelta(quint64 current, quint64 previous) { return current >= previous ? current - previous : 0; }

bool defaultRoute(ULONG *interface_index, SOCKADDR_INET *gateway)
{
    PMIB_IPFORWARD_TABLE2 table = nullptr;
    if (GetIpForwardTable2(AF_INET, &table) != NO_ERROR || !table) return false;
    bool found = false;
    quint64 best_metric = ~quint64(0);
    for (ULONG i = 0; i < table->NumEntries; ++i)
    {
        const MIB_IPFORWARD_ROW2 &row = table->Table[i];
        if (row.DestinationPrefix.PrefixLength != 0 || row.NextHop.si_family != AF_INET) continue;
        MIB_IPINTERFACE_ROW iface;
        InitializeIpInterfaceEntry(&iface);
        iface.Family = AF_INET;
        iface.InterfaceIndex = row.InterfaceIndex;
        if (GetIpInterfaceEntry(&iface) != NO_ERROR || !iface.Connected) continue;
        const quint64 metric = quint64(row.Metric) + iface.Metric;
        if (metric >= best_metric) continue;
        best_metric = metric;
        *interface_index = row.InterfaceIndex;
        *gateway = row.NextHop;
        found = true;
    }
    FreeMibTable(table);
    return found;
}

void queryWifi(ULONG interface_index, NetworkSample *sample)
{
    NET_LUID luid{};
    GUID guid{};
    if (ConvertInterfaceIndexToLuid(interface_index, &luid) != NO_ERROR
        || ConvertInterfaceLuidToGuid(&luid, &guid) != NO_ERROR) return;
    DWORD negotiated = 0;
    HANDLE client = nullptr;
    if (WlanOpenHandle(2, nullptr, &negotiated, &client) != ERROR_SUCCESS) return;
    DWORD size = 0;
    WLAN_OPCODE_VALUE_TYPE opcode{};
    WLAN_CONNECTION_ATTRIBUTES *attributes = nullptr;
    const DWORD status = WlanQueryInterface(client, &guid, wlan_intf_opcode_current_connection,
        nullptr, &size, reinterpret_cast<PVOID *>(&attributes), &opcode);
    if (status == ERROR_SUCCESS && attributes)
    {
        sample->wifi_signal_percent = attributes->wlanAssociationAttributes.wlanSignalQuality;
        sample->wifi_rx_rate_kbps = attributes->wlanAssociationAttributes.ulRxRate;
        sample->wifi_tx_rate_kbps = attributes->wlanAssociationAttributes.ulTxRate;
    }
    if (attributes) WlanFreeMemory(attributes);
    WlanCloseHandle(client, nullptr);
}

NetworkSample collectNetworkSample(InterfaceCounters *previous)
{
    NetworkSample sample;
    ULONG interface_index = 0;
    SOCKADDR_INET gateway{};
    if (!defaultRoute(&interface_index, &gateway)) return sample;
    MIB_IF_ROW2 row{};
    row.InterfaceIndex = interface_index;
    if (GetIfEntry2(&row) != NO_ERROR) return sample;
    sample.available = true;
    sample.adapter_type = adapterTypeName(row.Type);
    sample.route_changed = previous->valid && previous->index != interface_index;
    sample.rx_link_mbps = static_cast<qint64>(row.ReceiveLinkSpeed / 1000000ull);
    sample.tx_link_mbps = static_cast<qint64>(row.TransmitLinkSpeed / 1000000ull);
    if (previous->valid && previous->index == interface_index)
    {
        sample.rx_bytes_delta = counterDelta(row.InOctets, previous->in_octets);
        sample.tx_bytes_delta = counterDelta(row.OutOctets, previous->out_octets);
        sample.rx_errors_delta = counterDelta(row.InErrors, previous->in_errors);
        sample.tx_errors_delta = counterDelta(row.OutErrors, previous->out_errors);
        sample.rx_discards_delta = counterDelta(row.InDiscards, previous->in_discards);
        sample.tx_discards_delta = counterDelta(row.OutDiscards, previous->out_discards);
    }
    previous->valid = true;
    previous->index = interface_index;
    previous->in_octets = row.InOctets; previous->out_octets = row.OutOctets;
    previous->in_errors = row.InErrors; previous->out_errors = row.OutErrors;
    previous->in_discards = row.InDiscards; previous->out_discards = row.OutDiscards;
    if (row.Type == IF_TYPE_IEEE80211) queryWifi(interface_index, &sample);
    if (gateway.Ipv4.sin_addr.S_un.S_addr != 0)
    {
        sample.gateway_probe_attempted = true;
        HANDLE icmp = IcmpCreateFile();
        if (icmp != INVALID_HANDLE_VALUE)
        {
            const char payload[] = "nax5-gateway-probe";
            QByteArray reply(static_cast<int>(sizeof(ICMP_ECHO_REPLY) + sizeof(payload) + 8), 0);
            const DWORD count = IcmpSendEcho(icmp, gateway.Ipv4.sin_addr.S_un.S_addr,
                const_cast<char *>(payload), static_cast<WORD>(sizeof(payload)), nullptr,
                reply.data(), static_cast<DWORD>(reply.size()), 180);
            if (count > 0)
            {
                const auto *echo = reinterpret_cast<const ICMP_ECHO_REPLY *>(reply.constData());
                sample.gateway_probe_ok = echo->Status == IP_SUCCESS;
                if (sample.gateway_probe_ok) sample.gateway_rtt_ms = echo->RoundTripTime;
            }
            IcmpCloseHandle(icmp);
        }
    }
    return sample;
}
#else
struct InterfaceCounters {};
NetworkSample collectNetworkSample(InterfaceCounters *) { return NetworkSample(); }
#endif

QJsonObject networkSampleJson(const QString &session_id, const NetworkSample &sample,
    const Nax5NetworkDiagnosticsSummary &summary)
{
    QJsonObject json{{"schema", 3}, {"event", "network_sample"}, {"session_id", session_id},
        {"utc", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {"available", sample.available}, {"adapter_type", sample.adapter_type},
        {"route_changed", sample.route_changed}, {"rx_bytes_delta", double(sample.rx_bytes_delta)},
        {"tx_bytes_delta", double(sample.tx_bytes_delta)}, {"rx_errors_delta", double(sample.rx_errors_delta)},
        {"tx_errors_delta", double(sample.tx_errors_delta)}, {"rx_discards_delta", double(sample.rx_discards_delta)},
        {"tx_discards_delta", double(sample.tx_discards_delta)}, {"rx_link_mbps", double(sample.rx_link_mbps)},
        {"tx_link_mbps", double(sample.tx_link_mbps)}, {"gateway_probe_attempted", sample.gateway_probe_attempted},
        {"gateway_probe_ok", sample.gateway_probe_ok}, {"gateway_probes_sent_total", double(summary.probes_sent)},
        {"gateway_probe_replies_total", double(summary.probe_replies)}};
    if (sample.gateway_rtt_ms >= 0) json.insert("gateway_rtt_ms", sample.gateway_rtt_ms);
    if (sample.wifi_signal_percent >= 0) json.insert("wifi_signal_percent", double(sample.wifi_signal_percent));
    if (sample.wifi_rx_rate_kbps >= 0) json.insert("wifi_rx_rate_kbps", double(sample.wifi_rx_rate_kbps));
    if (sample.wifi_tx_rate_kbps >= 0) json.insert("wifi_tx_rate_kbps", double(sample.wifi_tx_rate_kbps));
    return json;
}
}

class Nax5NetworkDiagnostics::Impl
{
public:
    class Worker final : public QThread
    {
    public:
        Worker(Impl *owner, const QString &session_id) : owner(owner), session_id(session_id) {}
        void wake() { QMutexLocker lock(&wait_mutex); wait_condition.wakeAll(); }
    protected:
        void run() override
        {
            InterfaceCounters previous;
            while (!isInterruptionRequested())
            {
                const NetworkSample sample = collectNetworkSample(&previous);
                Nax5NetworkDiagnosticsSummary current;
                {
                    QMutexLocker lock(&owner->mutex);
                    if (sample.route_changed) owner->value.route_changes++;
                    owner->value.rx_errors_delta += sample.rx_errors_delta;
                    owner->value.tx_errors_delta += sample.tx_errors_delta;
                    owner->value.rx_discards_delta += sample.rx_discards_delta;
                    owner->value.tx_discards_delta += sample.tx_discards_delta;
                    if (sample.gateway_probe_attempted) owner->value.probes_sent++;
                    if (sample.gateway_probe_ok)
                    {
                        owner->value.probe_replies++;
                        owner->value.rtt_sum_ms += sample.gateway_rtt_ms;
                        if (owner->value.rtt_min_ms < 0 || sample.gateway_rtt_ms < owner->value.rtt_min_ms) owner->value.rtt_min_ms = sample.gateway_rtt_ms;
                        if (owner->value.rtt_max_ms < 0 || sample.gateway_rtt_ms > owner->value.rtt_max_ms) owner->value.rtt_max_ms = sample.gateway_rtt_ms;
                    }
                    current = owner->value;
                }
                nax5ProcessLogWrite("nax5.network", QString::fromUtf8(
                    QJsonDocument(networkSampleJson(session_id, sample, current)).toJson(QJsonDocument::Compact)));
                QMutexLocker lock(&wait_mutex);
                if (!isInterruptionRequested()) wait_condition.wait(&wait_mutex, 2000);
            }
        }
    private:
        Impl *owner;
        QString session_id;
        QMutex wait_mutex;
        QWaitCondition wait_condition;
    };
    mutable QMutex mutex;
    Nax5NetworkDiagnosticsSummary value;
    std::unique_ptr<Worker> worker;
};

Nax5NetworkDiagnostics::Nax5NetworkDiagnostics() : impl(new Impl) {}
Nax5NetworkDiagnostics::~Nax5NetworkDiagnostics() { stop(); }
void Nax5NetworkDiagnostics::start(const QString &session_id)
{
    stop();
    { QMutexLocker lock(&impl->mutex); impl->value = Nax5NetworkDiagnosticsSummary(); }
    impl->worker.reset(new Impl::Worker(impl.get(), session_id));
    impl->worker->start(QThread::LowPriority);
}
void Nax5NetworkDiagnostics::stop()
{
    if (!impl->worker) return;
    impl->worker->requestInterruption();
    impl->worker->wake();
    impl->worker->wait();
    impl->worker.reset();
}
Nax5NetworkDiagnosticsSummary Nax5NetworkDiagnostics::summary() const
{
    QMutexLocker lock(&impl->mutex);
    return impl->value;
}
