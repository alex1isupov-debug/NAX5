#pragma once

enum Nax5ApiLane
{
    Nax5ApiLaneAuth = 0,
    Nax5ApiLaneQuery,
    Nax5ApiLaneReserve,
    Nax5ApiLaneConnection,
    Nax5ApiLaneTerminal,
    Nax5ApiLaneOperator,
    Nax5ApiLaneReport,
    Nax5ApiLaneTelemetry
};

bool nax5ApiLaneAllowsConcurrent(Nax5ApiLane lane);
bool nax5ApiShouldAbortExisting(Nax5ApiLane existing, Nax5ApiLane incoming);
