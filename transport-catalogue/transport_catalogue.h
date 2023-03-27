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
#include "domain.h"

class TransportCatalogue {
public:
    TransportCatalogue();

    TransportCatalogue(domain::TransportData&& data);

    TransportCatalogue(const TransportCatalogue&) = delete;

    TransportCatalogue& operator=(const TransportCatalogue&) = delete;

    void AddStop(const domain::StopRequest& request);

    void AddBus(const domain::BusRequest& request);

    domain::StopInfo GetStopInfo(const std::string_view name) const;

    domain::BusInfo GetBusInfo(const std::string_view name) const;

    const domain::TransportData& GetData() const {
        return data_;
    }

//Debug========================================================================
    size_t GetStopsSize() const {
        assert(stops_.size() == stops_refs_.size());
        return stops_.size();
    }

    size_t GetBusesSize() const {
        assert(buses_.size() == buses_refs_.size());
        return buses_.size();
    }
//Debug========================================================================
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

    domain::TransportData data_;
    std::deque<domain::Stop>& stops_;
    std::deque<domain::Bus>& buses_;
    std::unordered_map<std::string_view, domain::Stop*>& stops_refs_;
    std::unordered_map<std::string_view, domain::Bus*>& buses_refs_;
    std::unordered_map<std::string_view, std::set<std::string_view>>& stops_to_buses_;

    mutable std::unordered_map<std::string_view, domain::DistanceInfo> lengths_data_;
    std::unordered_map<StopPtrPair, int, StopsPairHasher> neighbours_distance_;
};
