#pragma once

#include "domain.h"
#include <string_view>
#include <string>
#include <sstream>
#include <unordered_map>
#include <optional>
#include "transport_catalogue.h"
#include <memory>
#include <map>
#include "svg.h"

namespace transport_router {

using VertexId = size_t;
using EdgeId = size_t;

struct WholeRoute {
    double time;
    std::vector<VertexId> edges_ids;
};

using StopRoutes = std::unordered_map<VertexId, WholeRoute>;

struct DeserializedRouterItem {
    size_t name;
    size_t start;
    double time;
    int count;
};

struct DeserializedRouterItems {
    double total_time = -1;
    std::vector<size_t> items;
};

struct RouterItem {
    std::string_view name;
    std::string_view start;
    double time;
    int count;
};

struct RouterItems {
    double total_time = -1;
    std::vector<RouterItem> items;
};

struct RouterSerializationData {
    const std::unordered_map<std::string_view, VertexId>& stop_vertexes;
    const std::vector<RouterItem>& edges;
    std::unordered_map<VertexId, StopRoutes> data;
    int wait_time;
    double bus_velocity;
};

struct LazyRouterData {
    std::vector<std::pair<std::string_view, size_t>> stops_ids;
    std::vector<std::pair<std::string_view, size_t>> buses_ids;
    std::vector<std::pair<size_t, DeserializedRouterItem>> edges;
    std::unordered_map<size_t, std::unordered_map<size_t, DeserializedRouterItems>> routes;
    int wait_time;
    double bus_velocity;
};

class RouterBase {
public:
    RouterBase(int wait_time, double bus_velocity) : wait_time_(wait_time), bus_velocity_(bus_velocity) {}
    virtual RouterItems FindRoute(std::string_view from, std::string_view to) = 0;
    virtual RouterSerializationData GetSerializationData() = 0;
    int GetWaitTime () {return wait_time_;}
    double GetBusVelocity () {return bus_velocity_;}
protected:
    int wait_time_;
    double bus_velocity_;
};
} //namespace transport_router
namespace request_handler {

struct StopInfo {
    domain::StopInfo info;
    int id;
};

struct BusInfo {
    domain::BusInfo info;
    int id;
};

struct MapInfo {
    std::string map_str;
    int id;
};

enum class ItemType {
    Bus,
    Wait
};

struct RouteItem {
    ItemType type;
    std::string_view name;
    double time;
    int count;
};

struct RouteInfo {
    double total_time;
    std::vector<RouteItem> items;

    int id;
};

struct RenderSettings {
    double width;
    double height;

    double padding;

    double line_width;
    double stop_radius;

    int bus_label_font_size;
    domain::Point bus_label_offset;

    int stop_label_font_size;
    domain::Point stop_label_offset;

    domain::Color underlayer_color;
    double underlayer_width;

    std::vector<domain::Color> color_palette;
};

struct RoutingSettings {
    double bus_velocity;
    int    wait_time;
};

struct SerializationSettings {
    std::string file;
};

class RequestHandler;
class BaseRequestHandler;
class StatRequestHandler;

struct BaseRequest {
    std::string_view name;

    virtual void AddMeTo(BaseRequestHandler& handler) = 0;

    virtual ~BaseRequest() = default;
};

struct AddingStopRequest;
struct AddingBusRequest;

struct StatRequest {
    int id;

    virtual void ProcessMeBy(StatRequestHandler& handler) = 0;

    virtual ~StatRequest() = default;
};

struct StopInfoRequest;
struct BusInfoRequest;
struct MapInfoRequest;
struct RoutingInfoRequest;

class RequestReader{
public:
    virtual void Read() = 0;

    std::vector<std::unique_ptr<BaseRequest>>& GetBaseRequests() {
        return base_requests_;
    }

    std::vector<std::unique_ptr<StatRequest>>& GetStatRequests() {
        return stat_requests_;
    }

    const RenderSettings& GetSettings() const {
        return settings_;
    }

    const RoutingSettings& GetRoutingSettings() const {
        return routing_settings_;
    }

    const SerializationSettings& GetSerializationSettings() const {
        return serialization_settings_;
    }

protected:
    template <class RequestType>
    void AddStatRequest(RequestType&& request) {
        stat_requests_.push_back(std::make_unique<RequestType>(request));
    }

    template <class RequestType>
    void AddBaseRequest(RequestType&& request) {
        base_requests_.push_back(std::make_unique<RequestType>(request));
    }

    RequestReader() = default;
    virtual ~RequestReader() = default;
    std::vector<std::unique_ptr<BaseRequest>> base_requests_;
    std::vector<std::unique_ptr<StatRequest>> stat_requests_;
    RenderSettings settings_;
    RoutingSettings routing_settings_;
    SerializationSettings serialization_settings_;
};

class RequestPrinter{
public:
    virtual void Print(const BusInfo&) = 0;
    virtual void Print(const StopInfo&) = 0;
    virtual void Print(MapInfo&) = 0;
    virtual void Print(RouteInfo&) = 0;
    virtual void RenderAll() = 0;
    virtual void Clear() = 0;
protected:
    RequestPrinter() = default;
    virtual ~RequestPrinter() = default;
};

struct MapData{
MapData(const std::vector<std::pair<std::string_view, geo::Coordinates>>& stops_used_in,
        const std::set<domain::BusForRender>& buses_in) : stops_used(stops_used_in),
                                                          buses(buses_in) {}


const std::vector<std::pair<std::string_view, geo::Coordinates>> stops_used;
const std::set<domain::BusForRender> buses;
};

class MapRenderer{
public:
    virtual void SetRenderSettings (const RenderSettings& settings) = 0;
    virtual void RenderMap (const MapData& data, std::ostream& out) = 0;
    virtual void SetStopPoints(std::map<std::string, domain::Point>& stops_points) = 0;
    virtual void ComputeStopPoints(const std::vector<std::pair<std::string_view, geo::Coordinates>>& stops) = 0;
    virtual std::map<std::string_view, domain::Point>
    GetStopPoints() const  = 0;

    virtual ~MapRenderer() = default;
};

class RequestHandler {
public:
    RequestHandler (TransportCatalogue& catalogue) : catalogue_(catalogue) {}
protected:
    TransportCatalogue& catalogue_;
};

class BaseRequestHandler : public RequestHandler {
public:
    BaseRequestHandler(TransportCatalogue& catalogue) : RequestHandler(catalogue) {}

    void Add(AddingStopRequest&);
    void Add(AddingBusRequest&);

    void ProcessBaseRequests(std::vector<std::unique_ptr<BaseRequest>>& base_requests);
    void ProcessBaseRequests(RequestReader& filled_reader);
};

class StatRequestHandler : public RequestHandler {
public:
    StatRequestHandler(TransportCatalogue& catalogue,
                       RequestPrinter& printer,
                       MapRenderer& renderer) : RequestHandler(catalogue),
                                                printer_(printer),
                                                map_renderer_(renderer) {}

    void Process(StopInfoRequest&);
    void Process(BusInfoRequest&);
    void Process(MapInfoRequest&);
    void Process(RoutingInfoRequest&);

    void ProcessRequests(RequestReader& reader);
    void ProcessRequests(std::vector<std::unique_ptr<StatRequest>>& requests);

    void SetCustomRouter(std::unique_ptr<transport_router::RouterBase>&& router) {
        router_ = std::move(router);
    }

private:
    const MapData& GetMapData() const;

    RequestPrinter& printer_;
    MapRenderer& map_renderer_;
    RoutingSettings routing_settings_;
    std::unique_ptr<transport_router::RouterBase> router_;
};

struct AddingStopRequest : BaseRequest {
    double latitude;
    double longitude;
    std::unordered_map<std::string_view, int> neighbours;

    virtual void AddMeTo(BaseRequestHandler& handler) override {
        handler.Add(*this);
    }

    ~AddingStopRequest() override = default;
};

struct AddingBusRequest : BaseRequest {
    std::vector<std::string_view> stops;
    bool is_roundtrip;

    virtual void AddMeTo(BaseRequestHandler& handler) override {
        handler.Add(*this);
    }

    ~AddingBusRequest() override = default;
};

struct StopInfoRequest : StatRequest{
    std::string_view name;

    void ProcessMeBy(StatRequestHandler& handler) override {
        handler.Process(*this);
    }

    ~StopInfoRequest() override = default;
};

struct BusInfoRequest : StatRequest{
    std::string_view name;

    void ProcessMeBy(StatRequestHandler& handler) override {
        handler.Process(*this);
    }

    ~BusInfoRequest() override = default;
};

struct MapInfoRequest : StatRequest{

    void ProcessMeBy(StatRequestHandler& handler) override {
        handler.Process(*this);
    }

    ~MapInfoRequest() override = default;
};

struct RoutingInfoRequest : StatRequest {
    std::string_view stop_from;
    std::string_view stop_to;

    void ProcessMeBy(StatRequestHandler& handler) override {
        handler.Process(*this);
    }

    ~RoutingInfoRequest() override = default;
};

class DistanceComputer {
public:
    DistanceComputer(const TransportCatalogue& catalogue) : catalogue_(catalogue) {}

    int ComputeDistance(std::string_view from, std::string_view to) const {
        return catalogue_.GetDistance(from, to);
    }
private:
    const TransportCatalogue& catalogue_;
};

class CatalogueSerializationHandler {
public:
    CatalogueSerializationHandler(const SerializationSettings& settings) : file_(settings.file) {}

    void Serialize(RequestReader& reader, MapRenderer& renderer);
private:
    std::string file_;
};

class CatalogueDeserializationHandler {
public:
    CatalogueDeserializationHandler(const SerializationSettings& settings);
    void Deserialize(RequestPrinter& printer, MapRenderer& renderer, RequestReader& reader);
private:
    std::string file_;
};

} //namespace request_handler

