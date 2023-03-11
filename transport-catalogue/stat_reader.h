#pragma once

#include <iostream>
#include "transport_catalogue.h"


namespace stream_stats {
namespace detail {

void PrintStopInfoToOStream(const TransportCatalogue::StopInfo& info, std::ostream& out);

void PrintBusInfoToOStream(const TransportCatalogue::BusInfo& info, std::ostream& out);

} //namespace detail
void GetRequestsFromIStream(const TransportCatalogue& catalogue, std::istream& in = std::cin,
                                                                 std::ostream& out = std::cout);
}//namespace stream_stats
