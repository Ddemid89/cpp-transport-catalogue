#include "json_reader.h"
#include "transport_catalogue.h"
#include "json.h"
#include <string_view>
#include <algorithm>
#include "domain.h"
#include "map_renderer.h"
#include "request_handler.h"

namespace stream_input_json {
namespace detail {

request_handler::AddingBusRequest GetBusRequest(const json::Dict& bus_request) {
    request_handler::AddingBusRequest result;
    result.name = bus_request.at("name").AsString();
    result.is_roundtrip = bus_request.at("is_roundtrip").AsBool();

    const json::Array& stops = bus_request.at("stops").AsArray();
    for (const json::Node& stop : stops) {
        result.stops.push_back(stop.AsString());
    }
    return result;
}

request_handler::AddingStopRequest GetStopRequest(const json::Dict& stop_request) {
    request_handler::AddingStopRequest result;
    result.name = stop_request.at("name").AsString();
    result.latitude = stop_request.at("latitude").AsDouble();
    result.longitude = stop_request.at("longitude").AsDouble();

    const json::Dict& neighbours = stop_request.at("road_distances").AsMap();
    for (const std::pair<const std::string, json::Node>& neighbour : neighbours) {
        result.neighbours[neighbour.first] = neighbour.second.AsInt();
    }
    return result;
}

request_handler::BusInfoRequest GetBusStatRequest(const json::Dict& request) {
    request_handler::BusInfoRequest result;
    result.id = request.at("id").AsInt();
    result.name = request.at("name").AsString();
    return result;
}

request_handler::StopInfoRequest GetStopStatRequest(const json::Dict& request) {
    request_handler::StopInfoRequest result;
    result.id = request.at("id").AsInt();
    result.name = request.at("name").AsString();
    return result;
}

request_handler::MapInfoRequest GetMapStatRequest(const json::Dict& request) {
    request_handler::MapInfoRequest result;
    result.id = request.at("id").AsInt();
    return result;
}

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

domain::Point GetPoint(const json::Array& point) {
    double x = point[0].AsDouble();
    double y = point[1].AsDouble();
    return {x, y};
}
} // namespace detail


void JSONReader::GetOneBaseRequest(const json::Node& request) {
    const json::Dict& req_dict = request.AsMap();
    std::string_view type = req_dict.at("type").AsString();
    if (type == "Bus") {
        AddBaseRequest(detail::GetBusRequest(req_dict));
    } else if (type == "Stop") {
        AddBaseRequest(detail::GetStopRequest(req_dict));
    } else {
        throw std::logic_error("json_reader::ProcessOneStat: only \"Bus\" and \"Stop\" requests supported!");
    }
}

void JSONReader::GetBaseRequests(const json::Array& requests) {
    for (const json::Node& request : requests) {
        GetOneBaseRequest(request);
    }
}

request_handler::RenderSettings JSONReader::GetRenderSettings(const json::Dict& request) {
    request_handler::RenderSettings result;

    result.bus_label_font_size = request.at("bus_label_font_size").AsInt();
    result.stop_label_font_size = request.at("stop_label_font_size").AsInt();

    result.height = request.at("height").AsDouble();
    result.line_width = request.at("line_width").AsDouble();
    result.padding = request.at("padding").AsDouble();
    result.stop_radius = request.at("stop_radius").AsDouble();
    result.underlayer_width = request.at("underlayer_width").AsDouble();
    result.width = request.at("width").AsDouble();

    result.bus_label_offset = detail::GetPoint(request.at("bus_label_offset").AsArray());
    result.stop_label_offset = detail::GetPoint(request.at("stop_label_offset").AsArray());

    result.underlayer_color = detail::GetColor(request.at("underlayer_color"));

    const json::Array palette = request.at("color_palette").AsArray();
    for (const json::Node& color : palette) {
        result.color_palette.push_back(detail::GetColor(color));
    }

    return result;
}

void JSONReader::GetOneStatRequest(const json::Node& request) {
    const json::Dict& req_dict = request.AsMap();
    std::string_view type = req_dict.at("type").AsString();
    if (type == "Bus") {
        AddStatRequest(detail::GetBusStatRequest(req_dict));
    } else if (type == "Stop") {
        AddStatRequest(detail::GetStopStatRequest(req_dict));
    } else if (type == "Map") {
        AddStatRequest(detail::GetMapStatRequest(req_dict));
    } else {
        throw std::logic_error("json_reader::ProcessOneStat: only \"Map\", \"Bus\" and \"Stop\" requests supported!");
    }
}

void JSONReader::GetStatRequests(const json::Array& requests){
    for (const json::Node& request : requests) {
        GetOneStatRequest(request);
    }
}

json::Dict JSONPrinter::ProcessStopRequest (const request_handler::StopInfo& stop_info){
    json::Dict result;

    const domain::StopInfo& info = stop_info.info;

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

    result["request_id"] = stop_info.id;

    return result;
}

json::Dict JSONPrinter::ProcessBusRequest (const request_handler::BusInfo& bus_info){
    json::Dict result;

    const domain::BusInfo& info = bus_info.info;

    if (!info.stops.empty()) {
        result["curvature"] = info.length.curvature;
        result["route_length"] = info.length.real_length;
        result["stop_count"] = static_cast<int>(info.stops.size());
        result["unique_stop_count"] = info.unique_stops;
    } else {
        result["error_message"] = std::string("not found");
    }

    result["request_id"] = bus_info.id;

    return result;
}

} //namespace stream_input_json
