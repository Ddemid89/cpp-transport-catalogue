#pragma once

#include "geo.h"
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <string_view>
#include <cassert>
#include <set>
#include <variant>
#include <algorithm>
#include "domain.h"

class TransportCatalogue {
public:
    TransportCatalogue() = default;

    TransportCatalogue(const TransportCatalogue&) = delete;

    TransportCatalogue& operator=(const TransportCatalogue&) = delete;

    TransportCatalogue(TransportCatalogue&& other);

    void AddStop(const domain::StopRequest& request);

    void AddBus(const domain::BusRequest& request);

    domain::StopInfo GetStopInfo(const std::string_view name) const;

    domain::BusInfo GetBusInfo(const std::string_view name) const;

    std::set<domain::BusForRender> GetBusesForRender() const;

    std::vector<std::pair<std::string_view, geo::Coordinates>> GetStopsUsed() const;

    int GetDistance(std::string_view from, std::string_view to) const;

    std::vector<const domain::Stop*> GetAllStops() const;

    std::vector<const domain::Bus*> GetAllBuses() const;

    std::vector<std::pair<std::pair<std::string_view, std::string_view>, int>> GetDistances() const;

    void AddDistance(std::string_view from, std::string_view to, int distance);

private:
    using StopPtrPair = std::pair<const domain::Stop*,const domain::Stop*>;

    domain::Stop& GetStopRef(std::string_view name);

    int GetRealDistance(const domain::Stop& a, const domain::Stop& b) const;

    domain::DistanceInfo ComputeRouteLength(std::string_view name) const;

    class StopsPairHasher {
    public:
        size_t operator()(const StopPtrPair& stop_pair) const {
            return hasher(stop_pair.first) + hasher(stop_pair.second) * 3571;
        }
    private:
        std::hash<const void*> hasher;
    };

    std::deque<domain::Stop> stops_;
    std::deque<domain::Bus> buses_;
    std::unordered_map<std::string_view, domain::Stop*> stops_refs_;
    std::unordered_map<std::string_view, domain::Bus*> buses_refs_;
    std::unordered_map<std::string_view, std::set<std::string_view>> stops_to_buses_;
    mutable std::unordered_map<std::string_view, domain::DistanceInfo> lengths_data_;
    std::unordered_map<StopPtrPair, int, StopsPairHasher> neighbours_distance_;
};
