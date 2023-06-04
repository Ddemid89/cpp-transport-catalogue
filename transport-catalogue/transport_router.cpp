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

RouterItems TransportRouter::FindRouteById(VertexId from_id, VertexId to_id) {
    RouterItems result;

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

    RouterItem item;
    item.name  = bus_name;
    item.start = from;
    item.time  = weight;
    item.count = stops_count;

    edges_.push_back(item);
}


std::unordered_map<VertexId, StopRoutes> TransportRouter::MakeBaseData() {
    std::unordered_map<VertexId, StopRoutes> result;
    for (auto [from_name, from_id] : stop_vertexes_) {
        StopRoutes& routes = result[from_id];
        for (auto [to_name, to_id] : stop_vertexes_) {
            if (from_id != to_id) {
                auto route = router_.BuildRoute(from_id, to_id);
                if (route) {
                    routes[to_id].time = route->weight;
                    routes[to_id].edges_ids = std::move(route->edges);
                }
            }
        }
    }
    return result;
}

RouterItems TransportRouter::FindRoute(std::string_view from, std::string_view to) {
    RouterItems result;
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
    return FindRouteById(from_id, to_id);
}

LazyRouter::LazyRouter(LazyRouterData& data) : RouterBase(data.wait_time, data.bus_velocity) {
    stop_ids_.reserve(data.stops_ids.size());
    stop_by_id_.reserve(data.stops_ids.size());
    bus_ids_.reserve(data.buses_ids.size());
    bus_by_id_.reserve(data.buses_ids.size());

    for (auto [name, id] : data.stops_ids) {
        stop_ids_[name] = id;
        stop_by_id_[id] = name;
    }

    for (auto [name, id] : data.buses_ids) {
        bus_ids_[name] = id;
        bus_by_id_[id] = name;
    }

    for (auto& [id, item] : data.edges) {
        edges_.push_back(std::move(item));
        edge_ids_[id] = &edges_.back();
    }

    from_to_routes_ = std::move(data.routes);

    wait_time_ = data.wait_time;
    bus_velocity_ = data.bus_velocity;
}

RouterItems LazyRouter::FindRoute(std::string_view from, std::string_view to) {
    RouterItems result;

    if (from == to) {
        result.total_time = 0;
        return result;
    }

    auto it_end  = stop_ids_.end();
    auto it_from = stop_ids_.find(from);
    auto it_to   = stop_ids_.find(to);
    if (it_from == it_end || it_to == it_end) {
        return result;
    }

    size_t from_id = it_from->second;
    size_t to_id   = it_to->second;

    auto it_from_to = from_to_routes_.at(from_id).find(to_id);

    if (it_from_to == from_to_routes_.at(from_id).end()) {
        return {};
    }

    DeserializedRouterItems& route = it_from_to->second;

    result.total_time = route.total_time;

    result.items.reserve(route.items.size());

    for (size_t item : route.items) {
        result.items.push_back(ConvertRouterItem(item));
    }
    return result;
}

RouterItem LazyRouter::ConvertRouterItem(size_t item_id) {
    RouterItem result;
    DeserializedRouterItem& item = *edge_ids_[item_id];

    result.count = item.count;
    result.time  = item.time;
    result.start = stop_by_id_.at(item.start);
    result.name  = bus_by_id_.at(item.name);
    return result;
}

} //namespace transport_router
