#include "nax5/nax5apilane.h"

bool nax5ApiLaneAllowsConcurrent(Nax5ApiLane lane)
{
    return lane == Nax5ApiLaneTerminal || lane == Nax5ApiLaneReport || lane == Nax5ApiLaneTelemetry;
}

bool nax5ApiShouldAbortExisting(Nax5ApiLane existing, Nax5ApiLane incoming)
{
    if (existing == Nax5ApiLaneTerminal || incoming == Nax5ApiLaneTerminal)
        return false;
    if (existing == Nax5ApiLaneReport || incoming == Nax5ApiLaneReport)
        return false;
    if (existing == Nax5ApiLaneTelemetry || incoming == Nax5ApiLaneTelemetry)
        return false;
    if (existing != incoming)
        return false;
    return !nax5ApiLaneAllowsConcurrent(incoming);
}
