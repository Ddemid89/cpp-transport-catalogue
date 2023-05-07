#include "transport_router.h"

#include "request_handler.h"
#include "graph.h"
#include "router.h"
#include <string_view>
#include "domain.h"
#include <set>
#include <vector>
#include <unordered_map>
#include <algorithm>

namespace transport_router {

RouteItems TransportRouter::FindRoute(std::string_view from, std::string_view to) {
    RouteItems result;
    if (from == to) {
        result.total_time = 0;
        return result;
    }

    VertexId from_id, to_id;

    try {
        from_id = GetVertexId(from);
    } catch (std::invalid_argument& e) {
        return result;
    }

    try {
        to_id   = GetVertexId(to);
    } catch (std::invalid_argument& e) {
        return result;
    }

    auto opt_info = router_.BuildRoute(from_id, to_id);

    if (!opt_info) {
        return result;
    }
    graph::Router<double>::RouteInfo info = opt_info.value();

    result.items.resize(info.edges.size());

    std::transform(info.edges.begin(), info.edges.end(), result.items.begin(),
                   [this](size_t edge_id){
                        return edges_[edge_id];
                   });

    result.total_time = info.weight;

    return result;
}

graph::Router<double> TransportRouter::MakeRouter(const request_handler::MapData& data) {
    const std::vector<std::pair<std::string_view, geo::Coordinates>>& stops_used = data.stops_used;

    for (size_t i = 0; i < stops_used.size(); ++i) {
        stop_vertexes_[stops_used[i].first] = i;
    }

    for (const domain::BusForRender& bus : data.buses) {
        assert(!bus.stops.empty());
        AddBus(bus);
    }

    return graph::Router<double>(graph_);
}

std::vector<double> TransportRouter::GetIntervalsTime(const std::vector<std::string_view>& stops) const {
    std::vector<double> result;
    const double MINS_IN_HOUR = 60;
    const double METERS_IN_KM = 1000;

    for (int i = 0; i < static_cast<int>(stops.size()) - 1; ++i) {
        double distance_km = distance_computer_.ComputeDistance(stops[i], stops[i + 1]) / METERS_IN_KM;
        double time = (distance_km / bus_velocity_) * MINS_IN_HOUR;
        result.push_back(time);
    }

    return result;
}

double TransportRouter::ComputeTimeSum(const std::vector<double>& times, size_t from, size_t to) const {
    double res = 0;

    for (; from < to; ++from) {
        res += times[from];
    }

    return res;
}

void TransportRouter::AddBus(const domain::BusForRender& bus) {
    const std::vector<std::string_view>& stop_names = bus.stops;

    std::vector<double> intervals_time = GetIntervalsTime(stop_names);

    int last_index = static_cast<int>(stop_names.size()) - 1;

    for (int i = 1; i < last_index; ++i) {
        AddEdge(stop_names[0], stop_names[i], bus.name, i,
                ComputeTimeSum(intervals_time, 0, i) + wait_time_);
    }

    for (int i = 1; i < last_index; ++i) {
        std::string_view from_name = stop_names[i];

        for (int j = i + 1; j <= last_index; ++j) {
            AddEdge(from_name, stop_names[j], bus.name, j - i,
                    ComputeTimeSum(intervals_time, i, j) + wait_time_);
        }
    }
}

VertexId TransportRouter::GetVertexId(std::string_view vertex_name) const {
    auto it = stop_vertexes_.find(vertex_name);
    if (it == stop_vertexes_.end()) {
        throw std::invalid_argument("Stop \"" + std::string(vertex_name) + "\" is unavailable!\n");
    }
    return it->second;
}

void TransportRouter::AddEdge(std::string_view from, std::string_view to,
             std::string_view bus_name, int stops_count, double weight) {
    VertexId from_id = GetVertexId(from);
    VertexId to_id   = GetVertexId(to);

    graph::Edge<double> edge{from_id, to_id, weight};
    edge.from = from_id;
    edge.to = to_id;
    graph_.AddEdge(edge);

    RouteItem item;
    item.name  = bus_name;
    item.start = from;
    item.time  = weight;
    item.count = stops_count;

    edges_.push_back(item);
}

} //namespace transport_router
