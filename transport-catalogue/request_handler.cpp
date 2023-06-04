#include "transport_router.h"
#include "request_handler.h"
#include "domain.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "svg.h"
#include "serialization.h"

#include <string_view>
#include <string>
#include <sstream>
#include <unordered_map>
#include <optional>
#include <algorithm>


namespace request_handler {

void BaseRequestHandler::Add(AddingStopRequest& request) {
    domain::StopRequest result{request.name, {request.latitude, request.longitude}, std::move(request.neighbours)};
    catalogue_.AddStop(result);
}

void BaseRequestHandler::Add(AddingBusRequest& request) {
    domain::BusRequest result{request.name, std::move(request.stops), request.is_roundtrip};
    catalogue_.AddBus(result);
}

void BaseRequestHandler::ProcessBaseRequests(std::vector<std::unique_ptr<BaseRequest>>& base_requests) {
    for (std::unique_ptr<BaseRequest>& request : base_requests) {
        request.get()->AddMeTo(*this);
    }
}

void BaseRequestHandler::ProcessBaseRequests(RequestReader& filled_reader) {
    for (std::unique_ptr<BaseRequest>& request : filled_reader.GetBaseRequests()) {
        request.get()->AddMeTo(*this);
    }
}

void StatRequestHandler::Process(StopInfoRequest& request) {
    StopInfo info{catalogue_.GetStopInfo(request.name), request.id};
    printer_.Print(info);
}

void StatRequestHandler::Process(BusInfoRequest& request) {
    BusInfo info{catalogue_.GetBusInfo(request.name), request.id};
    printer_.Print(info);
}

void StatRequestHandler::Process(MapInfoRequest& request) {
        std::ostringstream out;

        map_renderer_.RenderMap(GetMapData(), out);

        MapInfo map_info;
        map_info.id = request.id;
        map_info.map_str = out.str();
        printer_.Print(map_info);
}



void StatRequestHandler::Process(RoutingInfoRequest& request) {
    if (!router_) {
        router_ = std::make_unique<transport_router::TransportRouter>(routing_settings_,
                                                                      DistanceComputer(catalogue_),
                                                                      GetMapData());
    }

    transport_router::RouterItems route_items = router_->FindRoute(request.stop_from, request.stop_to);

    RouteInfo route_info;
    route_info.id = request.id;

    route_info.total_time = route_items.total_time;
    int wait_time = router_->GetWaitTime();

    for (size_t i = 0; i < route_items.items.size(); ++i) {
        transport_router::RouterItem& item = route_items.items[i];

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

const MapData& StatRequestHandler::GetMapData() const {
    static const MapData data = MapData(catalogue_.GetStopsUsed(), catalogue_.GetBusesForRender());
    return data;
}

void StatRequestHandler::ProcessRequests(RequestReader& filled_reader) {
    ProcessRequests(filled_reader.GetStatRequests());
}

void StatRequestHandler::ProcessRequests(std::vector<std::unique_ptr<StatRequest>>& requests) {
    for (std::unique_ptr<StatRequest>& request : requests) {
        request.get()->ProcessMeBy(*this);
    }
    printer_.RenderAll();
    printer_.Clear();
}

void CatalogueSerializationHandler::Serialize(RequestReader& reader, MapRenderer& renderer) {
    TransportCatalogue catalogue;
    request_handler::BaseRequestHandler br_handler(catalogue);
    br_handler.ProcessBaseRequests(reader);

    renderer.SetRenderSettings(reader.GetSettings());
    renderer.ComputeStopPoints(catalogue.GetStopsUsed());

    DistanceComputer computer(catalogue);

    MapData map_data = {catalogue.GetStopsUsed(), catalogue.GetBusesForRender()};

    transport_router::TransportRouter router(reader.GetRoutingSettings(), computer, map_data);

    serialization::CatalogueSerializator serializator(file_);

    auto buses           = catalogue.GetAllBuses();
    auto distances       = catalogue.GetDistances();
    auto render_settings = reader.GetSettings();
    auto stops           = catalogue.GetAllStops();
    auto stop_points     = renderer.GetStopPoints();
    auto router_data     = router.GetSerializationData();

    serialization::SerializationData data {stops, buses,
                                           distances,
                                           stop_points,
                                           render_settings,
                                           router_data};

    serializator.Serialize(data);
}

CatalogueDeserializationHandler::CatalogueDeserializationHandler(const SerializationSettings& settings)
        : file_(settings.file) {}

void CatalogueDeserializationHandler::Deserialize(RequestPrinter& printer,
                                                  MapRenderer& renderer,
                                                  RequestReader& reader) {

    serialization::CatalogueDeserializator deserializator(file_);
    serialization::DeserializationData catalogue_data = deserializator.Deserialize();

    TransportCatalogue catalogue;

    for (domain::Stop& stop : catalogue_data.stops) {
        catalogue.AddStop(domain::StopRequest{stop.name, stop.coordinates, {}});
    }

    for (domain::Bus& bus : catalogue_data.buses) {
        domain::BusRequest bus_request;
        bus_request.name = bus.name;
        bus_request.is_roundtrip = bus.is_roundtrip;
        bus_request.stops.resize(bus.stops.size());

        std::transform(bus.stops.begin(), bus.stops.end(), bus_request.stops.begin(),
                            [](domain::Stop* stop){
                                return std::string_view(stop->name);
                            });

        catalogue.AddBus(bus_request);
    }

    for (serialization::distance_t& element : catalogue_data.distances) {
        serialization::sv_names_t& names = element.first;
        catalogue.AddDistance(names.first, names.second, element.second);
    }

    renderer.SetRenderSettings(catalogue_data.render_settings);
    renderer.SetStopPoints(catalogue_data.stop_points);

    StatRequestHandler sr_handler(catalogue, printer, renderer);

    std::unique_ptr<transport_router::LazyRouter> router
    = std::make_unique<transport_router::LazyRouter>(catalogue_data.router_data);


    sr_handler.SetCustomRouter(std::move(router));

    sr_handler.ProcessRequests(reader);
}

} //namespace request_handler
