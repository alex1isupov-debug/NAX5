#include "nax5/nax5apilane.h"

bool nax5ApiLaneAllowsConcurrent(Nax5ApiLane lane)
{
    return lane == Nax5ApiLaneTerminal;
}

bool nax5ApiShouldAbortExisting(Nax5ApiLane existing, Nax5ApiLane incoming)
{
    if (existing == Nax5ApiLaneTerminal || incoming == Nax5ApiLaneTerminal)
        return false;
    if (existing != incoming)
        return false;
    return !nax5ApiLaneAllowsConcurrent(incoming);
}
