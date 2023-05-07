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

struct RouteItem {
    std::string_view name;
    std::string_view start;
    double time;
    int count;
};

struct RouteItems {
    double total_time = -1;
    std::vector<RouteItem> items;
};

class TransportRouter {
public:
    TransportRouter(request_handler::RoutingSettings settings,
                    const request_handler::DistanceComputer& distance_computer,
                    const request_handler::MapData& data)
                                : wait_time_(settings.wait_time),
                                  bus_velocity_(settings.bus_velocity),
                                  distance_computer_(distance_computer),
                                  graph_ (data.stops_used.size()),
                                  router_(MakeRouter(data))
                                            {}

    RouteItems FindRoute(std::string_view from, std::string_view to);
private:
    graph::Router<double> MakeRouter(const request_handler::MapData& data);

    std::vector<double> GetIntervalsTime(const std::vector<std::string_view>& stops) const;

    double ComputeTimeSum(const std::vector<double>& times, size_t from, size_t to) const;

    void AddBus(const domain::BusForRender& bus);

    VertexId GetVertexId(std::string_view vertex_name) const;

    void AddEdge(std::string_view from, std::string_view to,
                 std::string_view bus_name, int stops_count, double weight);

    std::unordered_map<std::string_view, VertexId> stop_vertexes_;
    std::vector<RouteItem> edges_;

    int wait_time_;
    double bus_velocity_;
    const request_handler::DistanceComputer& distance_computer_;
    graph::DirectedWeightedGraph<double> graph_;
    graph::Router<double> router_;
};

} //namespace transport_router
