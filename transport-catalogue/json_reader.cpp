#include "json_reader.h"
#include "transport_catalogue.h"
#include "json.h"
#include <string_view>
#include <algorithm>
#include "domain.h"
#include "map_renderer.h"
#include "request_handler.h"

namespace stream_input_json{
namespace detail {

domain::Color GetColor(const json::Node& color) {
    domain::Color result;

    if (color.IsString()) {
        result = color.AsString();
    } else {
        assert(color.IsArray());
        const json::Array& array = color.AsArray();

        uint8_t red = static_cast<uint8_t>(array.at(0).AsInt());
        uint8_t green = static_cast<uint8_t>(array.at(1).AsInt());
        uint8_t blue = static_cast<uint8_t>(array.at(2).AsInt());

        if (array.size() == 3) {
            result = domain::Rgb{red, green, blue};
        } else {
            double opacity = array.at(3).AsDouble();
            result = domain::Rgba{red, green, blue, opacity};
        }
    }
    return result;
}

domain::Point GetPoint(const json::Array point) {
    double x = point[0].AsDouble();
    double y = point[1].AsDouble();
    return {x, y};
}

map_renderer::RenderSettings GetRenderSettings(const json::Dict& request) {
    map_renderer::RenderSettings result;

    result.bus_label_font_size = request.at("bus_label_font_size").AsInt();
    result.stop_label_font_size = request.at("stop_label_font_size").AsInt();

    result.height = request.at("height").AsDouble();
    result.line_width = request.at("line_width").AsDouble();
    result.padding = request.at("padding").AsDouble();
    result.stop_radius = request.at("stop_radius").AsDouble();
    result.underlayer_width = request.at("underlayer_width").AsDouble();
    result.width = request.at("width").AsDouble();

    result.bus_label_offset = GetPoint(request.at("bus_label_offset").AsArray());
    result.stop_label_offset = GetPoint(request.at("stop_label_offset").AsArray());

    result.underlayer_color = GetColor(request.at("underlayer_color"));

    const json::Array palette = request.at("color_palette").AsArray();
    for (const json::Node& color : palette) {
        result.color_palette.push_back(GetColor(color));
    }

    return result;
}

domain::BusRequest MakeBusRequest(const json::Dict& dict) {
    domain::BusRequest result;
    result.name = dict.at("name").AsString();

    const json::Array& stops = dict.at("stops").AsArray();
    result.stops.reserve(stops.size());
    for (const json::Node& stop : stops) {
        result.stops.push_back(stop.AsString());
    }
    result.is_roundtrip = true;
    if(!dict.at("is_roundtrip").AsBool()) {
        result.is_roundtrip = false;
        for (int i = result.stops.size() - 2; i >= 0; --i) {
            result.stops.push_back(result.stops[i]);
        }
    }
    return result;
}

domain::StopRequest MakeStopRequest(const json::Dict& dict) {
    domain::StopRequest result;
    result.name = dict.at("name").AsString();
    result.coordinates.lat = dict.at("latitude").AsDouble();
    result.coordinates.lng = dict.at("longitude").AsDouble();

    const json::Dict& neighbours = dict.at("road_distances").AsMap();

    for (const std::pair<const std::string, json::Node>& neighbour : neighbours) {
        result.neighbours[neighbour.first] = neighbour.second.AsInt();
    }
    return result;
}

void FillCatalogue(request_handler::RequstHandler& catalogue, const json::Array& requests) {
    for (const json::Node& node : requests) {
        const json::Dict& request = node.AsMap();
        const std::string type = request.at("type").AsString();
        if (type == "Stop") {
            catalogue.AddStop(MakeStopRequest(request));
        } else if (type == "Bus") {
            catalogue.AddBus(MakeBusRequest(request));
        }
    }
}

json::Dict ProcessMapStat(const request_handler::RequstHandler& catalogue) {
    json::Dict result;
    result["map"] = catalogue.GetMap();
    return result;
}

json::Dict ProcessBusStat(const request_handler::RequstHandler& catalogue, std::string_view name) {
    domain::BusInfo info = catalogue.GetBusInfo(name);
    json::Dict result;

    if (!info.stops.empty()) {
        result["curvature"] = info.length.curvature;
        result["route_length"] = static_cast<int>(info.length.real_length);
        result["stop_count"] = static_cast<int>(info.stops.size());
        result["unique_stop_count"] = info.unique_stops;
    } else {
        result["error_message"] = std::string("not found");
    }

    return result;
}

json::Dict ProcessStopStat(const request_handler::RequstHandler& catalogue, std::string_view name) {
    domain::StopInfo info = catalogue.GetStopInfo(name);
    json::Dict result;

    if (info.was_found) {
        json::Array buses(info.buses.size());
        std::transform(info.buses.begin(), info.buses.end(), buses.begin(),
                       [](std::string_view name){
                           return std::string(name);
                       });
        result["buses"] = std::move(buses);
    } else {
        result["error_message"] = std::string("not found");
    }

    return result;
}

json::Dict ProcessOneStat(const request_handler::RequstHandler& catalogue, const json::Dict& request) {
    int id = request.at("id").AsInt();
    std::string_view type = request.at("type").AsString();

    json::Dict result;
    if (type == "Bus") {
        std::string_view name = request.at("name").AsString();
        result = ProcessBusStat(catalogue, name);
    } else if (type == "Stop") {
        std::string_view name = request.at("name").AsString();
        result = ProcessStopStat(catalogue, name);
    } else if (type == "Map") {
        result = ProcessMapStat(catalogue);
    } else {
        throw std::logic_error("json_reader::ProcessOneStat: only \"Map\", \"Bus\" and \"Stop\" requests supported!");
    }

    //std::cout << std::endl << "-------------------------------" << std::endl;
    //json::Print(Document{result}, std::cout);
    //std::cout << std::endl << "-------------------------------" << std::endl;

    result["request_id"] = id;
    return result;
}

void ProcessStatRequests(const request_handler::RequstHandler& catalogue,
                         const json::Array& requests,
                         std::ostream& out) {
    if (requests.empty()) {
        return;
    }
    json::Array result;

    for (const json::Node& request : requests) {
        result.push_back(ProcessOneStat(catalogue, request.AsMap()));
    }

    json::Print(json::Document{result}, out);
}
} //namespace detail
void GetJSONDataFromIStream (request_handler::RequstHandler& catalogue,
                             std::istream& in,     //default std::cin
                             std::ostream& out) {  //default std::cout
    using namespace json;

    Document doc = Load(in);

    const Node& root = doc.GetRoot();
    const Dict& requests = root.AsMap();

    auto render_settings_request_it = requests.find("render_settings");
    if (render_settings_request_it != requests.end()) {
        const Dict& render_settings_request = render_settings_request_it->second.AsMap();
        catalogue.SetRenderSettings(detail::GetRenderSettings(render_settings_request));
    }

    auto base_requests_it = requests.find("base_requests");
    auto stat_requests_it = requests.find("stat_requests");
    assert(base_requests_it != requests.end());
    assert(stat_requests_it != requests.end());

    const Array& base_requests = base_requests_it->second.AsArray();
    const Array& stat_requests = stat_requests_it->second.AsArray();

    detail::FillCatalogue(catalogue, base_requests);

    detail::ProcessStatRequests(catalogue, stat_requests, out);

}

void GetJSONDataFromIStreamAndRenderMap(std::istream& in,    //default std::cin
                                        std::ostream& out) { //default std::cout
    using namespace json;

    TransportCatalogue data;
    request_handler::RequstHandler handler(data);
    Document doc = Load(in);

    const Node& root = doc.GetRoot();
    const Dict& requests = root.AsMap();

    auto base_requests_it = requests.find("base_requests");
    auto render_settings_request_it = requests.find("render_settings");
    assert(base_requests_it != requests.end());
    assert(render_settings_request_it != requests.end());

    const Array& base_requests = base_requests_it->second.AsArray();
    const Dict& render_settings_request = render_settings_request_it->second.AsMap();

    detail::FillCatalogue(handler, base_requests);
    map_renderer::RenderSettings render_settings = detail::GetRenderSettings(render_settings_request);
    map_renderer::RenderMap(handler.GetData(), std::move(render_settings), out);
}
} //namespace stream_input_json
