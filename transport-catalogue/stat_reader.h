#pragma once

#include <iostream>
#include "transport_catalogue.h"
#include "domain.h"
#include "request_handler.h"

namespace stream_stats {
namespace detail {

void PrintStopInfoToOStream(const domain::StopInfo& info, std::ostream& out);

void PrintBusInfoToOStream(const domain::BusInfo& info, std::ostream& out);

} //namespace detail
void GetRequestsFromIStream(const request_handler::RequstHandler& catalogue, std::istream& in = std::cin,
                                                                 std::ostream& out = std::cout);
}//namespace stream_stats
