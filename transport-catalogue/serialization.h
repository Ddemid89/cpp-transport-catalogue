#pragma once

#include "domain.h"
#include "request_handler.h"

#include <string_view>
#include <vector>
#include <unordered_map>
#include <fstream>

#include <transport_catalogue.pb.h>

namespace serialization {

using sv_names_t = std::pair<std::string_view, std::string_view>;
using distance_t = std::pair<sv_names_t, int>;

struct SerializationData {
    std::vector<const domain::Stop*> stops;
    std::vector<const domain::Bus*> buses;
    std::vector<distance_t> distances;
    std::map<std::string_view, domain::Point> stop_points;
    request_handler::RenderSettings render_settings;
    transport_router::RouterSerializationData router_data;
};

struct DeserializationData {
    std::vector<domain::Stop> stops;
    std::vector<domain::Bus> buses;
    std::vector<distance_t> distances;
    std::map<std::string, domain::Point> stop_points;
    request_handler::RenderSettings render_settings;
    transport_router::LazyRouterData router_data;
};

class CatalogueSerializator {
public:
    CatalogueSerializator(std::string file) : file_(std::move(file)) {}
    void Serialize(const SerializationData& data);
private:
    void FillStops(const std::vector<const domain::Stop*>& stops);
    void FillBuses(const std::vector<const domain::Bus*>& buses);
    void FillDistances(const std::vector<distance_t>& distances);
    void FillRenderSettings(const request_handler::RenderSettings& render_settings);
    void FillStopPoints(const std::map<std::string_view, domain::Point>& stop_points);
    void FillRouterVertexIds(const std::unordered_map<std::string_view, size_t>& stop_vertexes);
    void FillRouterEdges(const std::vector<transport_router::RouterItem>& edges);
    void FillRoutes(const std::unordered_map<size_t, transport_router::StopRoutes>& data);
    void FillRoutingSettings(int wait_time, double bus_velocity);

    static transport_catalogue_serialize::StopRoutes
    ConvertStopsRoutes(const std::unordered_map<size_t, transport_router::WholeRoute>& routes,
                       size_t from_vertex_id);

    static transport_catalogue_serialize::Route
    ConvertRoute(const transport_router::WholeRoute& route, size_t to_vertex_id);

    static transport_catalogue_serialize::Point ConvertPoint(domain::Point point);
    static transport_catalogue_serialize::Color ConvertColor(domain::Color color);

    uint32_t GetStopId(std::string_view name);
    uint32_t GetBusId(std::string_view name);

    std::unordered_map<std::string_view, uint32_t> stops_ids_;
    std::unordered_map<std::string_view, uint32_t> stops_router_ids_;
    std::unordered_map<std::string_view, uint32_t> buses_ids_;

    std::string file_;
    transport_catalogue_serialize::TransportCatalogue pb_catalogue_;
};

//==================================================================================================

class CatalogueDeserializator {
public:
    CatalogueDeserializator(std::string file) : file_(std::move(file)) {}
    DeserializationData Deserialize();
private:

    void ParseStops();
    void ParseBuses();
    void ParseDistances();
    void ParseRenderSettings();
    void ParseStopPoints();
    void ParseRouterStops();
    void ParseRouterBuses();
    void ParseRouterEdges();
    void ParseRouterRoutes();
    void ParseRouterSettings();

    static transport_router::DeserializedRouterItems
    ConvertItems(const transport_catalogue_serialize::Route& route);

    static domain::Point ConvertPoint(const transport_catalogue_serialize::Point& point);
    static domain::Color ConvertColor(const transport_catalogue_serialize::Color& color);

    DeserializationData result_;

    std::string file_;
    transport_catalogue_serialize::TransportCatalogue pb_catalogue_;
    std::unordered_map<uint32_t, std::string_view> stops_;
    std::unordered_map<uint32_t, std::string_view> buses_;

    std::unordered_map<uint32_t, domain::Stop*> stops_ptrs_;
};

} //namespace serialization
