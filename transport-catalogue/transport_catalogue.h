#pragma once

#include "geo.h"
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <string_view>
#include <cassert>
#include <set>

class TransportCatalogue {
public:
    struct DistanceInfo {
        double geo_length  = 0;
        double real_length = 0;
        double curvature   = 0;
    };

    struct Stop {
        std::string name;
        Coordinates coordinates;
    };

    struct Bus {
        std::string name;
        std::vector<Stop*> stops;
    };

    struct StopRequest {
        std::string_view name;
        Coordinates coordinates;
        std::unordered_map<std::string_view, int> neighbours;
    };

    struct BusRequest {
       std::string_view name;
       std::vector<std::string_view> stops;
    };

    struct StopInfo {
        std::string_view name;
        const std::set<std::string_view>& buses;
        bool was_found;
    };

    struct BusInfo {
        std::string_view name;
        std::vector<std::string_view> stops;
        DistanceInfo length;
    };

    void AddStop(const StopRequest& request);

    void AddBus(const BusRequest& request);

    StopInfo GetStopInfo(const std::string_view name) const;

    BusInfo GetBusInfo(const std::string_view name) const;
//Debug========================================================================
    size_t GetStopsSize() {
        assert(stops_.size() == stops_refs_.size());
        return stops_.size();
    }

    size_t GetBusesSize() {
        assert(buses_.size() == buses_refs_.size());
        return buses_.size();
    }
//Debug========================================================================
private:
    using StopPtrPair = std::pair<const Stop*,const Stop*>;

    Stop& GetStopRef(std::string_view name);

    int GetRealDistance(const Stop& a, const Stop& b) const;

    DistanceInfo ComputeRouteLength(std::string_view name) const;

    class StopsPairHasher {
    public:
        size_t operator()(const StopPtrPair& stop_pair) const {
            return hasher(stop_pair.first) + hasher(stop_pair.second) * 3571;
        }
    private:
        std::hash<const void*> hasher;
    };

    std::deque<Stop> stops_;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, Stop*> stops_refs_;
    std::unordered_map<std::string_view, Bus*> buses_refs_;
    std::unordered_map<std::string_view, std::set<std::string_view>> stops_to_buses_;
    mutable std::unordered_map<std::string_view, DistanceInfo> lengths_data_;
    std::unordered_map<StopPtrPair, int, StopsPairHasher> neighbours_distance_;
};
