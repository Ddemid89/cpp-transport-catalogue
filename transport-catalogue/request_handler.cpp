#include "transport_router.h"
#include "request_handler.h"
#include "domain.h"
#include <string_view>
#include <string>
#include <sstream>
#include <unordered_map>
#include <optional>
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "svg.h"
#include <algorithm>


namespace request_handler {

void RequestHandler::Add(AddingStopRequest& request) {
    domain::StopRequest result{request.name, {request.latitude, request.longitude}, std::move(request.neighbours)};
    catalogue_.AddStop(result);
}

void RequestHandler::Add(AddingBusRequest& request) {
    domain::BusRequest result{request.name, std::move(request.stops), request.is_roundtrip};
    catalogue_.AddBus(result);
}

void RequestHandler::Process(StopInfoRequest& request) {
    StopInfo info{catalogue_.GetStopInfo(request.name), request.id};
    printer_.Print(info);
}

void RequestHandler::Process(BusInfoRequest& request) {
    BusInfo info{catalogue_.GetBusInfo(request.name), request.id};
    printer_.Print(info);
}

void RequestHandler::Process(MapInfoRequest& request) {
        std::ostringstream out;
        map_renderer_.SetRenderSettings(render_settings_);

        //std::vector<std::pair<std::string_view, geo::Coordinates>> used_stops = catalogue_.GetStopsUsed();
        //std::set<domain::BusForRender> buses = catalogue_.GetBusesForRender();

        //MapData data(used_stops, buses);
        //map_renderer_.RenderMap(data, out);
        map_renderer_.RenderMap(GetMapData(), out);

        MapInfo map_info;
        map_info.id = request.id;
        map_info.map_str = out.str();

        printer_.Print(map_info);
}

void RequestHandler::Process(RoutingInfoRequest& request) {
    static transport_router::TransportRouter router (routing_settings_,
                                                     DistanceComputer(catalogue_),
                                                     GetMapData());

    transport_router::RouteItems route_items = router.FindRoute(request.stop_from, request.stop_to);

    RouteInfo route_info;
    route_info.id = request.id;

    route_info.total_time = route_items.total_time;
    int wait_time = routing_settings_.wait_time;

    for (size_t i = 0; i < route_items.items.size(); ++i) {
        transport_router::RouteItem& item = route_items.items[i];

        RouteItem wait_item;

        wait_item.type = ItemType::Wait;
        wait_item.name = item.start;
        wait_item.time = wait_time;
        wait_item.count = 1;

        RouteItem bus_item;

        bus_item.type = ItemType::Bus;
        bus_item.name = item.name;
        bus_item.time = item.time - wait_time;
        bus_item.count = item.count;

        route_info.items.push_back(wait_item);
        route_info.items.push_back(bus_item);
    }

    printer_.Print(route_info);
}

RequestHandler::RequestHandler(TransportCatalogue& catalogue,
                               RequestReader& reader,
                               RequestPrinter& printer,
                               MapRenderer& map_renderer) : catalogue_(catalogue),
                                                            printer_(printer),
                                                            map_renderer_(map_renderer) {
    reader.Read();

    render_settings_  = reader.GetSettings();
    routing_settings_ = reader.GetRoutingSettings();

    std::vector<std::unique_ptr<BaseRequest>>& requests = reader.GetBaseRequests();
    for (std::unique_ptr<BaseRequest>& request : requests) {
        request.get()->AddMeTo(*this);
    }

    std::vector<std::unique_ptr<StatRequest>>& stat_requests = reader.GetStatRequests();
    for (std::unique_ptr<StatRequest>& request : stat_requests) {
        request.get()->ProcessMeBy(*this);
    }

    printer_.RenderAll();
    printer_.Clear();
}

void RequestHandler::AddBus(const domain::BusRequest& request) {
    catalogue_.AddBus(request);
}

void RequestHandler::AddStop (const domain::StopRequest& request) {
    catalogue_.AddStop(request);
}

const MapData& RequestHandler::GetMapData() const {
    static const MapData data = MapData(catalogue_.GetStopsUsed(), catalogue_.GetBusesForRender());
    return data;
}

} //namespace request_handler
