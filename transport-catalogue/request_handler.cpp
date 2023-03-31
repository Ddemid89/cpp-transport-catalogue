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
    if (render_settings_) {
        std::ostringstream out;
        map_renderer_.SetRenderSettings(render_settings_.value());

        std::vector<std::pair<std::string_view, geo::Coordinates>> used_stops = catalogue_.GetStopsUsed();
        std::set<domain::BusForRender> buses = catalogue_.GetBusesForRender();

        MapData data(used_stops, buses);
        map_renderer_.RenderMap(data, out);

        MapInfo map_info;
        map_info.id = request.id;
        map_info.map_str = out.str();

        printer_.Print(map_info);
    } else {
        throw std::runtime_error("No render setting!");
    }
}

RequestHandler::RequestHandler(TransportCatalogue& catalogue,
                               RequestReader& reader,
                               RequestPrinter& printer,
                               MapRenderer& map_renderer) : catalogue_(catalogue),
                                                            printer_(printer),
                                                            map_renderer_(map_renderer) {
    reader.Read();

    render_settings_ = reader.GetSettings();

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

} //namespace request_handler
