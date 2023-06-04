#pragma once

#include "request_handler.h"
#include "graph.h"
#include "router.h"
#include <string_view>
#include "domain.h"
#include <set>
#include <vector>
#include <unordered_map>

namespace transport_router {

using VertexId = size_t;
using EdgeId = size_t;

class TransportRouter : public RouterBase {
public:
    TransportRouter(request_handler::RoutingSettings settings,
                    const request_handler::DistanceComputer& distance_computer,
                    const request_handler::MapData& data)
                                : RouterBase(settings.wait_time, settings.bus_velocity),
                                  distance_computer_(distance_computer),
                                  graph_ (data.stops_used.size()),
                                  router_(MakeRouter(data)) {}

    RouterSerializationData GetSerializationData() override {
        return {stop_vertexes_,
                edges_,
                MakeBaseData(),
                wait_time_,
                bus_velocity_};
    }

    RouterItems FindRoute(std::string_view from, std::string_view to) override;

private:
    RouterItems FindRouteById(VertexId from_id, VertexId to_id);

    graph::Router<double> MakeRouter(const request_handler::MapData& data);

    std::vector<double> GetIntervalsTime(const std::vector<std::string_view>& stops) const;

    double ComputeTimeSum(const std::vector<double>& times, size_t from, size_t to) const;

    void AddBus(const domain::BusForRender& bus);

    VertexId GetVertexId(std::string_view vertex_name) const;

    void AddEdge(std::string_view from, std::string_view to,
                 std::string_view bus_name, int stops_count, double weight);

    std::unordered_map<VertexId, StopRoutes> MakeBaseData();



    std::unordered_map<std::string_view, VertexId> stop_vertexes_;
    std::vector<RouterItem> edges_;

    const request_handler::DistanceComputer& distance_computer_;
    graph::DirectedWeightedGraph<double> graph_;
    graph::Router<double> router_;
};

class LazyRouter : public RouterBase {
public:
    LazyRouter(LazyRouterData& data);

    RouterItems FindRoute(std::string_view from, std::string_view to) override;

    RouterSerializationData GetSerializationData() override {
        throw std::runtime_error("GetSerializationData() not available now for LazyRouter\n");
    }

private:
    RouterItem ConvertRouterItem(size_t item_id);
    std::deque<DeserializedRouterItem> edges_;

    std::unordered_map<std::string_view, size_t> stop_ids_;
    std::unordered_map<std::string_view, size_t> bus_ids_;

    std::unordered_map<size_t, std::string_view> stop_by_id_;
    std::unordered_map<size_t, std::string_view> bus_by_id_;

    std::unordered_map<size_t, DeserializedRouterItem*> edge_ids_;
    std::unordered_map<size_t, std::unordered_map<size_t, DeserializedRouterItems>> from_to_routes_;
};


} //namespace transport_router
