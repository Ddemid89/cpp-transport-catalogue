#include "domain.h"
#include <string>
#include <string_view>
#include <deque>
#include <unordered_map>
#include <set>

namespace domain {

using TD = TransportData;

void TD::AddStop(const StopRequest& request) {
    Stop& stop = GetStopRef(request.name);
    stop.coordinates = request.coordinates;
}

void TD::AddBus(const BusRequest& request) {
    Bus& bus = buses_.emplace_back();
    bus.name = std::string(request.name);
    buses_refs_[bus.name] = &bus;
    bus.is_roundtrip = request.is_roundtrip;

    bus.stops.reserve(request.stops.size());

    for (const std::string_view stop : request.stops) {
        Stop& stop_in_catalogue = GetStopRef(stop);
        bus.stops.push_back(&stop_in_catalogue);
        stops_to_buses_[stop_in_catalogue.name].insert(bus.name);
    }
}

const std::deque<Stop>& TD::GetStopsArray() const {
    return stops_;
}

std::deque<Stop>& TD::GetStopsArray() {
    return stops_;
}

const std::deque<Bus>& TD::GetBusesArray() const {
    return buses_;
}

std::deque<Bus>& TD::GetBusesArray() {
    return buses_;
}

const std::unordered_map<std::string_view, Bus*>& TD::GetBusesMap() const {
    return buses_refs_;
}

std::unordered_map<std::string_view, Bus*>& TD::GetBusesMap() {
    return buses_refs_;
}

const std::unordered_map<std::string_view, Stop*>& TD::GetStopsMap() const {
    return stops_refs_;
}

std::unordered_map<std::string_view, Stop*>& TD::GetStopsMap() {
    return stops_refs_;
}

const std::unordered_map<std::string_view, std::set<std::string_view>>&
TD::GetStopsToBuses () const {
    return stops_to_buses_;
}

std::unordered_map<std::string_view, std::set<std::string_view>>&
TD::GetStopsToBuses () {
    return stops_to_buses_;
}

Stop& TD::GetStop(std::string_view name) {
    return GetStopRef(name);
}

Bus& TD::GetBus(std::string_view name) {
    auto it = buses_refs_.find(name);
    if (it != buses_refs_.end()) {
        return *(*it).second;
    }
    throw std::invalid_argument("No stop \"" + std::string(name) + "\"");
}

Stop& TD::GetStopRef(std::string_view name) {
    auto it = stops_refs_.find(name);
    if (it != stops_refs_.end()) {
        return *(*it).second;
    }
    Stop& stop = stops_.emplace_back(Stop{std::string(name), {}});
    stops_refs_[stop.name] = &stop;
    return stop;
}
} //namespace domain
