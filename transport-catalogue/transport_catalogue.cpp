#include "transport_catalogue.h"
#include "geo.h"
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <string_view>
#include <cassert>
#include <algorithm>
#include <set>

using TC = TransportCatalogue;

void TC::AddStop(const StopRequest& request) {
    Stop& stop = GetStopRef(request.name);
    stop.coordinates = request.coordinates;
    for (const auto& [name, distance] : request.neighbours) {
        Stop& other_stop = GetStopRef(name);
        neighbours_distance_[{&stop, &other_stop}] = distance;
    }
}

void TC::AddBus(const BusRequest& request) {
    Bus& bus = buses_.emplace_back();
    bus.name = std::string(request.name);
    buses_refs_[bus.name] = &bus;

    bus.stops.reserve(request.stops.size());

    for (const std::string_view stop : request.stops) {
        Stop& stop_in_catalogue = GetStopRef(stop);
        bus.stops.push_back(&stop_in_catalogue);
        stops_to_buses_[stop_in_catalogue.name].insert(bus.name);
    }
}

TC::StopInfo TC::GetStopInfo(const std::string_view name) const {
    static const std::set<std::string_view> empty_;
    bool was_found = stops_refs_.count(name) != 0;
    bool have_buses = stops_to_buses_.count(name) != 0;
    if (was_found) {
        if (have_buses) {
            return {name, stops_to_buses_.at(name), true};
        } else {
            return {name, empty_, true};
        }
    } else {
        return {name, empty_, false};
    }
}

TC::BusInfo TC::GetBusInfo(const std::string_view name) const {
    BusInfo result;
    result.name = name;

    auto it = buses_refs_.find(name);

    if (it == buses_refs_.end()) {
        return result;
    }

    Bus& bus = *(*it).second;

    for (const Stop* stop : bus.stops) {
        result.stops.push_back((*stop).name);
    }

    result.length = ComputeRouteLength(name);

    return result;
}

using StopPtrPair = std::pair<const TC::Stop*,const TC::Stop*>;

TC::Stop& TC::GetStopRef(std::string_view name) {
    auto it = stops_refs_.find(name);
    if (it != stops_refs_.end()) {
        return *(*it).second;
    }
    Stop& stop = stops_.emplace_back(Stop{std::string(name), {}});
    stops_refs_[stop.name] = &stop;
    return stop;
}

int TC::GetRealDistance(const Stop& a, const Stop& b) const {
    auto it  = neighbours_distance_.find({&a, &b});
    auto end = neighbours_distance_.end();
    if (it != end) {
        return (*it).second;
    }
    it  = neighbours_distance_.find({&b, &a});
    if (it != end) {
        return (*it).second;
    }
    assert("GetRealDistance: NO DATA!!!" == a.name);
    return -1;
}

TC::DistanceInfo TC::ComputeRouteLength(std::string_view name) const {
    auto it = lengths_data_.find(name);
    if (it != lengths_data_.end()) {
        return (*it).second;
    }

    Bus& bus = *buses_refs_.at(name);
    std::vector<Stop*>& stops = bus.stops;
    DistanceInfo result;

    for (size_t i = 1; i < stops.size(); ++i) {
        Stop& cur_stop = *stops[i];
        Stop& pre_stop = *stops[i - 1];
        result.geo_length  += ComputeDistance(pre_stop.coordinates, cur_stop.coordinates);
        result.real_length += GetRealDistance(pre_stop, cur_stop);
    }

    result.curvature = result.real_length / result.geo_length;

    lengths_data_[bus.name] = result;

    return lengths_data_.at(bus.name);
}
