#pragma once

#include "domain.h"
#include <string_view>
#include <string>
#include <sstream>
#include <unordered_map>
#include <optional>
#include "transport_catalogue.h"
#include <memory>
#include "svg.h"


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

class RequestHandler;

struct BaseRequest {
    std::string_view name;

    virtual void AddMeTo(RequestHandler& handler) = 0;

    virtual ~BaseRequest() = default;
};

struct AddingStopRequest;
struct AddingBusRequest;

struct StatRequest {
    int id;

    virtual void ProcessMeBy(RequestHandler& handler) = 0;

    virtual ~StatRequest() = default;
};

struct StopInfoRequest;
struct BusInfoRequest;
struct MapInfoRequest;


class RequestReader{
public:
    virtual void Read() = 0;

    std::vector<std::unique_ptr<BaseRequest>>& GetBaseRequests() {
        return base_requests_;
    }

    std::vector<std::unique_ptr<StatRequest>>& GetStatRequests() {
        return stat_requests_;
    }

    RenderSettings& GetSettings() {
        return settings_;
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
};

class RequestPrinter{
public:
    virtual void Print(const BusInfo&) = 0;
    virtual void Print(const StopInfo&) = 0;
    virtual void Print(MapInfo&) = 0;
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


const std::vector<std::pair<std::string_view, geo::Coordinates>>& stops_used;
const std::set<domain::BusForRender>& buses;
};

class MapRenderer{
public:
    virtual void SetRenderSettings (const RenderSettings& settings) = 0;
    virtual void RenderMap (const MapData& data, std::ostream& out) = 0;
    virtual ~MapRenderer() = default;
};

class RequestHandler{
public:
    RequestHandler(TransportCatalogue& catalogue, RequestReader& reader,
                   RequestPrinter& printer, MapRenderer& map_renderer);

    void Add(AddingStopRequest&);

    void Add(AddingBusRequest&);

    void Process(StopInfoRequest&);

    void Process(BusInfoRequest&);

    void Process(MapInfoRequest&);

    void AddBus(const domain::BusRequest& request);

    void AddStop (const domain::StopRequest& request);

private:
    TransportCatalogue& catalogue_;
    RequestPrinter& printer_;
    MapRenderer& map_renderer_;
    std::optional<RenderSettings> render_settings_;
};

struct AddingStopRequest : BaseRequest {
    double latitude;
    double longitude;
    std::unordered_map<std::string_view, int> neighbours;

    void AddMeTo(RequestHandler& handler) override {
        handler.Add(*this);
    }

    ~AddingStopRequest() override = default;
};

struct AddingBusRequest : BaseRequest {
    std::vector<std::string_view> stops;
    bool is_roundtrip;

    void AddMeTo(RequestHandler& handler) override {
        handler.Add(*this);
    }

    ~AddingBusRequest() override = default;
};

struct StopInfoRequest : StatRequest{
    std::string_view name;

    void ProcessMeBy(RequestHandler& handler) override {
        handler.Process(*this);
    }

    ~StopInfoRequest() override = default;
};

struct BusInfoRequest : StatRequest{
    std::string_view name;

    void ProcessMeBy(RequestHandler& handler) override {
        handler.Process(*this);
    }

    ~BusInfoRequest() override = default;
};

struct MapInfoRequest : StatRequest{

    void ProcessMeBy(RequestHandler& handler) override {
        handler.Process(*this);
    }

    ~MapInfoRequest() override = default;
};

} //namespace request_handler
