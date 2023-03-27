#include "stat_reader.h"
#include <iostream>
#include <unordered_set>
#include "transport_catalogue.h"
#include "input_reader.h"
#include "domain.h"
#include "request_handler.h"

namespace stream_stats {
namespace detail {

void PrintStopInfoToOStream(const domain::StopInfo& info, std::ostream& out) {
    out << "Stop " << info.name << ": ";
    if(info.was_found) {
        if(!info.buses.empty()) {
            out << "buses";
            for (std::string_view bus : info.buses) {
                out << " " << bus;
            }
        } else {
            out << "no buses";
        }
    } else {
        out << "not found";
    }
    out << std::endl;
}

void PrintBusInfoToOStream(const domain::BusInfo& info, std::ostream& out) {
    out << "Bus " << info.name << ": ";
    if (info.stops.empty()) {
        out << "not found" << std::endl;
        return;
    }

    out << info.stops.size() << " stops on route, ";

    out << info.unique_stops << " unique stops, " << info.length.real_length
    << " route length, " << info.length.curvature << " curvature" << std::endl;
}
} //namespace detail
void GetRequestsFromIStream(const request_handler::RequstHandler& catalogue,
                            std::istream& in,       // default
                            std::ostream& out) {    // values
    using namespace stream_input::detail;
    int req_count;
    in >> req_count;
    std::string request;
    getline(in, request);
    std::string_view name = request;

    for (int i = 0; i < req_count; ++i) {
        getline(in, request);
        auto pos = request.find_first_not_of(' ');
        if (request[pos] == 'B') {
            name = request;
            name = RemoveFirstWord(GetNextWord(name, ':'));
            domain::BusInfo bus = catalogue.GetBusInfo(name);
            detail::PrintBusInfoToOStream(bus, out);
        } else if (request[pos] == 'S') {
            name = request;
            name = RemoveFirstWord(GetNextWord(name, ':'));
            domain::StopInfo stop = catalogue.GetStopInfo(name);
            detail::PrintStopInfoToOStream(stop, out);
        } else {
            throw std::invalid_argument("Wrong request! Only \"Bus\" and \"Stop\" requests are supported now.");
        }
    }
}
}//namespace stream_stats
