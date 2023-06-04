#include "serialization.h"

#include "domain.h"
#include "request_handler.h"

#include <string_view>
#include <vector>
#include <unordered_map>
#include <fstream>

#include <transport_catalogue.pb.h>

namespace serialization {

//==========================Serializator==========================================

void CatalogueSerializator::Serialize(const SerializationData& data) {
    FillStops(data.stops);
    FillDistances(data.distances);
    FillBuses(data.buses);
    FillRenderSettings(data.render_settings);
    FillStopPoints(data.stop_points);
    FillRouterVertexIds(data.router_data.stop_vertexes);
    FillRouterEdges(data.router_data.edges);
    FillRoutes(data.router_data.data);
    FillRoutingSettings(data.router_data.wait_time, data.router_data.bus_velocity);

    std::ofstream out(file_, std::ios::binary);
    pb_catalogue_.SerializeToOstream(&out);
}

void CatalogueSerializator::FillStops(const std::vector<const domain::Stop*>& stops) {
    using namespace transport_catalogue_serialize;
    for (const domain::Stop* stop : stops) {
        const std::string& name = stop->name;
        uint32_t id = GetStopId(name);
        double latitude  = stop->coordinates.lat;
        double longitude = stop->coordinates.lng;

        Stop& pb_stop = *pb_catalogue_.add_stops();
        pb_stop.set_name(name);
        pb_stop.set_id(id);
        pb_stop.set_latitude(latitude);
        pb_stop.set_longitude(longitude);
    }

}

void CatalogueSerializator::FillBuses(const std::vector<const domain::Bus*>& buses) {
    using namespace transport_catalogue_serialize;

    for (const domain::Bus* bus : buses) {
        const std::string& name = bus->name;
        bool is_roundtrip = bus->is_roundtrip;
        Bus& pb_bus = *pb_catalogue_.add_buses();
        pb_bus.set_name(name);
        pb_bus.set_is_roundtrip(is_roundtrip);
        pb_bus.set_id(GetBusId(name));

        int stop_number = is_roundtrip ? bus->stops.size() : (bus->stops.size() / 2 + 1);

        for (int i = 0; i < stop_number; ++i) {
            const domain::Stop* stop = bus->stops[i];
            pb_bus.add_stops(stops_ids_.at(stop->name));
        }

    }
}

void CatalogueSerializator::FillDistances(const std::vector<distance_t>& distances) {
    using namespace transport_catalogue_serialize;

    for (const distance_t& element : distances) {
        const sv_names_t& names = element.first;
        std::string_view from = names.first;
        std::string_view to   = names.second;
        int distance = element.second;

        Distance& pb_distance = *pb_catalogue_.add_distances();

        pb_distance.set_from_id(stops_ids_.at(from));
        pb_distance.set_to_id(stops_ids_.at(to));
        pb_distance.set_distance(distance);
    }
}

void CatalogueSerializator::FillRenderSettings(const request_handler::RenderSettings& render_settings) {
    auto& pb_settings = *pb_catalogue_.mutable_map_renderer()->mutable_settings();
    pb_settings.set_stop_label_font_size(render_settings.stop_label_font_size);
    pb_settings.set_bus_label_font_size(render_settings.bus_label_font_size);

    pb_settings.set_width(render_settings.width);
    pb_settings.set_height(render_settings.height);
    pb_settings.set_padding(render_settings.padding);
    pb_settings.set_line_width(render_settings.line_width);
    pb_settings.set_stop_radius(render_settings.stop_radius);
    pb_settings.set_underlayer_width(render_settings.underlayer_width);

    *pb_settings.mutable_bus_label_offset() = ConvertPoint(render_settings.bus_label_offset);
    *pb_settings.mutable_stop_label_offset() = ConvertPoint(render_settings.stop_label_offset);

    *pb_settings.mutable_underlayer_color() = ConvertColor(render_settings.underlayer_color);

    for (domain::Color color : render_settings.color_palette) {
        *pb_settings.add_color_palette() = ConvertColor(color);
    }
}

void CatalogueSerializator::FillStopPoints(const std::map<std::string_view, domain::Point>& stop_points) {
    auto& pb_map_renderer = *pb_catalogue_.mutable_map_renderer();

    for (const auto& [name, point] : stop_points) {
        auto& pb_stop_point = *pb_map_renderer.add_stop_point();
        pb_stop_point.set_name(stops_ids_.at(name));
        *pb_stop_point.mutable_coordinates() = ConvertPoint(point);
    }
}

void CatalogueSerializator::FillRouterVertexIds(const std::unordered_map<std::string_view, size_t>& stop_vertexes) {
    using namespace transport_catalogue_serialize;
    RouterData& pb_data = *pb_catalogue_.mutable_router_data();

    for (auto [name, id] : stop_vertexes) {
        CatalogueIdToRouterId& pb_vertex_id = *pb_data.add_stop_ids();
        stops_router_ids_[name] = id;
        pb_vertex_id.set_router_id(id);
        pb_vertex_id.set_catalogue_id(stops_ids_.at(name));
    }
}

void CatalogueSerializator::FillRouterEdges(const std::vector<transport_router::RouterItem>& edges) {
    using namespace transport_catalogue_serialize;
    RouterData& pb_data = *pb_catalogue_.mutable_router_data();

    int index = 0;

    for (const transport_router::RouterItem& item : edges) {
        Edge& pb_edge = *pb_data.add_edges();
        pb_edge.set_edge_id(index++);
        pb_edge.set_stop_id(stops_router_ids_.at(item.start));
        pb_edge.set_bus_id(buses_ids_.at(item.name));
        pb_edge.set_time(item.time);
        pb_edge.set_count(item.count);
    }
}

void CatalogueSerializator::FillRoutes(const std::unordered_map<size_t, transport_router::StopRoutes>& data) {
    using namespace transport_catalogue_serialize;
    RouterData& pb_data = *pb_catalogue_.mutable_router_data();

    for (auto& [from_id, routes] : data) {
        *pb_data.add_stop_route() = ConvertStopsRoutes(routes, from_id);
    }
}


void CatalogueSerializator::FillRoutingSettings(int wait_time, double bus_velocity) {
    using namespace transport_catalogue_serialize;
    RouterData& pb_data = *pb_catalogue_.mutable_router_data();
    pb_data.set_bus_velocity(bus_velocity);
    pb_data.set_wait_time(wait_time);
}

uint32_t CatalogueSerializator::GetStopId(std::string_view name) {
    static uint32_t counter = 0;
    auto it = stops_ids_.find(name);
    if (it == stops_ids_.end()) {
        stops_ids_[name] = counter;
        return counter++;
    } else {
        return it->second;
    }
}

uint32_t CatalogueSerializator::GetBusId(std::string_view name) {
    static uint32_t counter = 0;
    auto it = buses_ids_.find(name);
    if (it == buses_ids_.end()) {
        buses_ids_[name] = counter;
        return counter++;
    } else {
        return it->second;
    }
}

transport_catalogue_serialize::Route
CatalogueSerializator::ConvertRoute(const transport_router::WholeRoute& route,
                                    size_t to_vertex_id) {
    transport_catalogue_serialize::Route result;
    result.set_to_id(to_vertex_id);
    result.set_weight(route.time);
    for (size_t edge_id : route.edges_ids) {
        result.add_edge_id(edge_id);
    }
    return result;
}

transport_catalogue_serialize::StopRoutes
CatalogueSerializator::ConvertStopsRoutes
            (const std::unordered_map<size_t, transport_router::WholeRoute>& routes,
             size_t from_vertex_id)
{
    transport_catalogue_serialize::StopRoutes result;
    result.set_from_id(from_vertex_id);
    for (auto& [to_id, route] : routes) {
        *result.add_route() = ConvertRoute(route, to_id);
    }
    return result;
}

transport_catalogue_serialize::Point CatalogueSerializator::ConvertPoint(domain::Point point) {
    transport_catalogue_serialize::Point result;
    result.set_x(point.x);
    result.set_y(point.y);
    return result;
}

transport_catalogue_serialize::Color CatalogueSerializator::ConvertColor(domain::Color color) {
    transport_catalogue_serialize::Color result;
    if (std::holds_alternative<domain::Rgb>(color)) {
        auto& pb_rgb = *result.mutable_rgb();
        domain::Rgb rgb = std::get<domain::Rgb>(color);

        pb_rgb.set_r(rgb.red);
        pb_rgb.set_g(rgb.green);
        pb_rgb.set_b(rgb.blue);
    } else if (std::holds_alternative<domain::Rgba>(color)) {
        auto& pb_rgba = *result.mutable_rgba();
        domain::Rgba rgba = std::get<domain::Rgba>(color);

        pb_rgba.set_r(rgba.red);
        pb_rgba.set_g(rgba.green);
        pb_rgba.set_b(rgba.blue);
        pb_rgba.set_a(rgba.opacity);
    } else if (std::holds_alternative<std::string>(color)) {
        result.set_str_color(std::get<std::string>(color));
    }

    return result;
}

//==========================Deserializator========================================

DeserializationData CatalogueDeserializator::Deserialize() {
    using namespace transport_catalogue_serialize;
    std::ifstream in(file_, std::ios::binary);
    if (!pb_catalogue_.ParseFromIstream(&in)) {
        return result_;
    }

    ParseStops();
    ParseBuses();
    ParseDistances();
    ParseRenderSettings();
    ParseStopPoints();

    ParseRouterStops();
    ParseRouterBuses();
    ParseRouterEdges();
    ParseRouterRoutes();
    ParseRouterSettings();

    return result_;
}


void CatalogueDeserializator::ParseStops() {
    using namespace transport_catalogue_serialize;
    int stops_number = pb_catalogue_.stops_size();
    result_.stops.resize(stops_number);

    for (int i = 0; i < stops_number; ++i) {
        const Stop& stop = pb_catalogue_.stops(i);
        stops_[stop.id()] = stop.name();

        domain::Stop& stop_ref = result_.stops[i];
        stop_ref.name = stop.name();
        stop_ref.coordinates.lat = stop.latitude();
        stop_ref.coordinates.lng = stop.longitude();
        stops_ptrs_[stop.id()] = &stop_ref;
    }
}

void CatalogueDeserializator::ParseBuses() {
    using namespace transport_catalogue_serialize;
    int buses_number = pb_catalogue_.buses_size();
    result_.buses.resize(buses_number);

    for (int i = 0; i < buses_number; ++i) {
        const Bus& bus = pb_catalogue_.buses(i);
        domain::Bus& bus_ref = result_.buses[i];
        bus_ref.name = bus.name();
        bus_ref.is_roundtrip = bus.is_roundtrip();

        int stops_number = bus.stops_size();
        bus_ref.stops.resize(stops_number);

        size_t id = bus.id();

        buses_[id] = bus_ref.name;

        for (int j = 0; j < stops_number; ++j) {
            bus_ref.stops[j] = stops_ptrs_.at(bus.stops(j));
        }
    }
}

void CatalogueDeserializator::ParseDistances() {
    using namespace transport_catalogue_serialize;
    int dist_number = pb_catalogue_.distances_size();
    result_.distances.resize(dist_number);

    for (int i = 0; i < dist_number; ++i) {
        const Distance& dist = pb_catalogue_.distances(i);
        distance_t& dist_ref = result_.distances[i];
        std::string_view from = stops_ptrs_[dist.from_id()]->name;
        std::string_view to = stops_ptrs_[dist.to_id()]->name;
        sv_names_t names{from, to};
        dist_ref = {names, dist.distance()};
    }
}

void CatalogueDeserializator::ParseRenderSettings() {
    using namespace transport_catalogue_serialize;
    const RenderSettings& pb_settings = pb_catalogue_.map_renderer().settings();
    request_handler::RenderSettings& settings = result_.render_settings;

    settings.bus_label_font_size  = pb_settings.bus_label_font_size();
    settings.stop_label_font_size = pb_settings.stop_label_font_size();

    settings.height           = pb_settings.height();
    settings.line_width       = pb_settings.line_width();
    settings.padding          = pb_settings.padding();
    settings.stop_radius      = pb_settings.stop_radius();
    settings.underlayer_width = pb_settings.underlayer_width();
    settings.width            = pb_settings.width();

    settings.bus_label_offset  = ConvertPoint(pb_settings.bus_label_offset());
    settings.stop_label_offset = ConvertPoint(pb_settings.stop_label_offset());

    settings.underlayer_color = ConvertColor(pb_settings.underlayer_color());

    int palette_size = pb_settings.color_palette_size();
    settings.color_palette.resize(palette_size);

    for (int i = 0; i < palette_size; i++) {
        settings.color_palette[i] = ConvertColor(pb_settings.color_palette(i));
    }
}

void CatalogueDeserializator::ParseStopPoints() {
    using namespace transport_catalogue_serialize;
    const MapRenderer& pb_map_renderer = pb_catalogue_.map_renderer();
    std::map<std::string, domain::Point>& stop_points = result_.stop_points;

    int point_number = pb_map_renderer.stop_point_size();

    for (int i = 0; i < point_number; ++i) {
        const StopPoint& pb_point = pb_map_renderer.stop_point(i);
        domain::Point tmp_point;
        std::string name = stops_ptrs_.at(pb_point.name())->name;
        tmp_point.x = pb_point.coordinates().x();
        tmp_point.y = pb_point.coordinates().y();
        stop_points[name] = tmp_point;
    }
}

void CatalogueDeserializator::ParseRouterStops() {
    using namespace transport_catalogue_serialize;
    const RouterData& pb_data = pb_catalogue_.router_data();
    std::vector<std::pair<std::string_view, size_t>>& res_stops
    = result_.router_data.stops_ids;


    int stops_number = pb_data.stop_ids_size();
    res_stops.reserve(stops_number);

    for (int i = 0; i < stops_number; ++i) {
        size_t cat_id  = pb_data.stop_ids(i).catalogue_id();
        size_t rout_id = pb_data.stop_ids(i).router_id();
        std::string_view name = stops_.at(cat_id);
        res_stops.push_back({name, rout_id});
    }
}

void CatalogueDeserializator::ParseRouterBuses() {
    std::vector<std::pair<std::string_view, size_t>>& res_buses
    = result_.router_data.buses_ids;

    int buses_number = buses_.size();
    res_buses.reserve(buses_number);

    for (auto [id, name] : buses_) {
        res_buses.push_back({name, id});
    }
}

void CatalogueDeserializator::ParseRouterEdges() {
    using namespace transport_catalogue_serialize;
    const RouterData& pb_data = pb_catalogue_.router_data();
    std::vector<std::pair<size_t, transport_router::DeserializedRouterItem>>& res_edges
    = result_.router_data.edges;

    int edges_number = pb_data.edges_size();
    res_edges.reserve(edges_number);

    for (int i = 0; i < edges_number; ++i) {
        const Edge& edge = pb_data.edges(i);
        transport_router::DeserializedRouterItem temp;
        temp.start = edge.stop_id();
        temp.name  = edge.bus_id();
        temp.time  = edge.time();
        temp.count = edge.count();
        res_edges.push_back({edge.edge_id(), temp});
    }
}

void CatalogueDeserializator::ParseRouterRoutes() {
    using namespace transport_catalogue_serialize;
    const RouterData& pb_data = pb_catalogue_.router_data();
    std::unordered_map<size_t, std::unordered_map<size_t, transport_router::DeserializedRouterItems>>& res_map
    = result_.router_data.routes;

    int from_stop_number = pb_data.stop_route_size();

    for (int i = 0; i < from_stop_number; ++i) {
        const StopRoutes& pb_routes = pb_data.stop_route(i);
        std::unordered_map<size_t, transport_router::DeserializedRouterItems>& res_route
        = res_map[pb_routes.from_id()];

        int to_stop_number = pb_routes.route_size();

        for(int j = 0; j < to_stop_number; ++j) {
            const Route& pb_route = pb_routes.route(j);
            res_route[pb_route.to_id()] = ConvertItems(pb_route);
        }
    }
}

void CatalogueDeserializator::ParseRouterSettings() {
    result_.router_data.bus_velocity = pb_catalogue_.router_data().bus_velocity();
    result_.router_data.wait_time = pb_catalogue_.router_data().wait_time();
}

transport_router::DeserializedRouterItems
CatalogueDeserializator::ConvertItems(const transport_catalogue_serialize::Route& route) {
    transport_router::DeserializedRouterItems result;
    result.total_time = route.weight();

    int edges_number = route.edge_id_size();

    for (int i = 0; i < edges_number; ++i) {
        size_t edge_id = route.edge_id(i);
        result.items.push_back(route.edge_id(i));
    }
    return result;
}

domain::Point CatalogueDeserializator::ConvertPoint(const transport_catalogue_serialize::Point& point) {
    domain::Point res;
    res.x = point.x();
    res.y = point.y();
    return res;
}

domain::Color CatalogueDeserializator::ConvertColor(const transport_catalogue_serialize::Color& color) {
    using namespace transport_catalogue_serialize;
    if (color.has_rgb()) {
        const RGB& pb_rgb = color.rgb();
        domain::Rgb rgb;
        rgb.red   = pb_rgb.r();
        rgb.green = pb_rgb.g();
        rgb.blue  = pb_rgb.b();
        return rgb;
    } else if (color.has_rgba()) {
        const RGBA& pb_rgba = color.rgba();
        domain::Rgba rgba;
        rgba.red      = pb_rgba.r();
        rgba.green    = pb_rgba.g();
        rgba.blue     = pb_rgba.b();
        rgba.opacity  = pb_rgba.a();
        return rgba;
    } else {
        if (color.str_color() != "") {
            return color.str_color();
        } else {
            return {};
        }
    }
}

} //namespace serialization
