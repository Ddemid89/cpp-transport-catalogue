#include "transport_catalogue.h"
#include "geo.h"
#include "domain.h"
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <string_view>
#include <cassert>
#include <algorithm>
#include <unordered_set>
#include <set>

TransportCatalogue::TransportCatalogue(TransportCatalogue&& other) {
    std::swap(stops_, other.stops_);
    std::swap(buses_, other.buses_);
    std::swap(stops_refs_, other.stops_refs_);
    std::swap(buses_refs_, other.buses_refs_);
    std::swap(stops_to_buses_, other.stops_to_buses_);
    std::swap(lengths_data_, other.lengths_data_);
    std::swap(neighbours_distance_, other.neighbours_distance_);
}

void TransportCatalogue::AddStop(const domain::StopRequest& request) {
    domain::Stop& stop = GetStopRef(request.name);
    stop.coordinates = request.coordinates;
    for (const auto& [name, distance] : request.neighbours) {
        domain::Stop& other_stop = GetStopRef(name);
        neighbours_distance_[{&stop, &other_stop}] = distance;
    }
}

void TransportCatalogue::AddBus(const domain::BusRequest& request) {
    domain::Bus& bus = buses_.emplace_back();
    bus.name = std::string(request.name);
    buses_refs_[bus.name] = &bus;
    bus.is_roundtrip = request.is_roundtrip;

    bus.stops.reserve(request.stops.size());

    std::unordered_set<std::string_view> unique_stops;

    for (const std::string_view stop : request.stops) {
        domain::Stop& stop_in_catalogue = GetStopRef(stop);
        unique_stops.insert(stop_in_catalogue.name);
        bus.stops.push_back(&stop_in_catalogue);
        stops_to_buses_[stop_in_catalogue.name].insert(bus.name);
    }

    if (!bus.is_roundtrip) {
        for (int i = bus.stops.size() - 2; i >= 0; --i) {
            bus.stops.push_back(bus.stops[i]);
        }
    }

    bus.unique_stops = unique_stops.size();
}

domain::StopInfo TransportCatalogue::GetStopInfo(const std::string_view name) const {
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

domain::BusInfo TransportCatalogue::GetBusInfo(const std::string_view name) const {
    domain::BusInfo result;
    result.name = name;

    auto it = buses_refs_.find(name);

    if (it == buses_refs_.end()) {
        return result;
    }

    domain::Bus& bus = *(*it).second;

    for (const domain::Stop* stop : bus.stops) {
        result.stops.push_back((*stop).name);
    }

    result.length = ComputeRouteLength(name);

    result.unique_stops = bus.unique_stops;

    return result;
}

using StopPtrPair = std::pair<const domain::Stop*,const domain::Stop*>;

domain::Stop& TransportCatalogue::GetStopRef(std::string_view name) {
    auto it = stops_refs_.find(name);
    if (it != stops_refs_.end()) {
        return *(*it).second;
    }
    domain::Stop& stop = stops_.emplace_back(domain::Stop{std::string(name), {}});
    stops_refs_[stop.name] = &stop;
    return stop;
}

int TransportCatalogue::GetRealDistance(const domain::Stop& a, const domain::Stop& b) const {
    auto it  = neighbours_distance_.find({&a, &b});
    auto end = neighbours_distance_.end();
    if (it != end) {
        return (*it).second;
    }
    it  = neighbours_distance_.find({&b, &a});
    if (it != end) {
        return (*it).second;
    }
    throw std::logic_error("GetRealDistance: NO DATA! (" + a.name + " -> " + b.name + ")\n");
    return -1;
}


int TransportCatalogue::GetDistance(std::string_view from, std::string_view to) const {
    auto from_it = stops_refs_.find(from);
    auto to_it   = stops_refs_.find(to);
    auto end_it = stops_refs_.end();

    if (from_it == end_it || to_it == end_it) {
        throw std::logic_error("GetDistance: NO STOPS!");
    }

    return GetRealDistance(*from_it->second, *to_it->second);
}

domain::DistanceInfo TransportCatalogue::ComputeRouteLength(std::string_view name) const {
    auto it = lengths_data_.find(name);
    if (it != lengths_data_.end()) {
        return (*it).second;
    }

    domain::Bus& bus = *buses_refs_.at(name);
    std::vector<domain::Stop*>& stops = bus.stops;
    domain::DistanceInfo result;

    for (size_t i = 1; i < stops.size(); ++i) {
        domain::Stop& cur_stop = *stops[i];
        domain::Stop& pre_stop = *stops[i - 1];
        result.geo_length  += ComputeDistance(pre_stop.coordinates, cur_stop.coordinates);
        result.real_length += GetRealDistance(pre_stop, cur_stop);
    }

    result.curvature = result.real_length / result.geo_length;

    lengths_data_[bus.name] = result;

    return lengths_data_.at(bus.name);
}

std::set<domain::BusForRender> TransportCatalogue::GetBusesForRender() const {
    std::set<domain::BusForRender> result;

    for (const domain::Bus& bus : buses_) {
        if (!bus.stops.empty()) {
            domain::BusForRender bus_for_render;
            bus_for_render.name = bus.name;
            bus_for_render.is_roundtrip = bus.is_roundtrip;

            for (const domain::Stop* stop : bus.stops) {
                bus_for_render.stops.push_back(stop->name);
            }

            result.insert(bus_for_render);
        }
    }
    return result;
}

std::vector<std::pair<std::string_view, geo::Coordinates>> TransportCatalogue::GetStopsUsed() const {
    std::vector<std::pair<std::string_view, geo::Coordinates>> result;
    using elem_type = std::pair<const std::string_view, std::set<std::string_view>>;

    for (const elem_type& stop : stops_to_buses_) {
        result.push_back({stop.first, stops_refs_.at(stop.first)->coordinates});
    }

    return result;
}

std::vector<const domain::Stop*> TransportCatalogue::GetAllStops() const {
    std::vector<const domain::Stop*> result(stops_.size());
    std::transform(stops_.begin(), stops_.end(), result.begin(),
                        [](const domain::Stop& stop){
                            return &stop;
                        });
    return result;
}

std::vector<const domain::Bus*> TransportCatalogue::GetAllBuses() const {
    std::vector<const domain::Bus*> result(buses_.size());
    std::transform(buses_.begin(), buses_.end(), result.begin(),
                        [](const domain::Bus& bus){
                            return &bus;
                        });
    return result;
}

std::vector<std::pair<std::pair<std::string_view, std::string_view>, int>>
TransportCatalogue::GetDistances() const {
    std::vector<std::pair<std::pair<std::string_view, std::string_view>, int>> result(neighbours_distance_.size());
        std::transform(neighbours_distance_.begin(), neighbours_distance_.end(), result.begin(),
                       [](auto& stops){
                            const domain::Stop* from = stops.first.first;
                            const domain::Stop* to = stops.first.second;
                            int distance = stops.second;
                            std::pair<std::string_view, std::string_view> names{from->name, to->name};
                            return std::make_pair(names, distance);
                       });
    return result;
}


void TransportCatalogue::AddDistance(std::string_view from, std::string_view to, int distance){
    domain::Stop* from_ptr = &GetStopRef(from);
    domain::Stop* to_ptr = &GetStopRef(to);
    neighbours_distance_[{from_ptr, to_ptr}] = distance;
}
