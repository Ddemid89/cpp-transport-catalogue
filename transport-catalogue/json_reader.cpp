#include "json_reader.h"
#include "transport_catalogue.h"
#include "json.h"
#include <string_view>
#include <algorithm>
#include "domain.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "json_builder.h"

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

    const json::Dict& neighbours = stop_request.at("road_distances").AsDict();
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

request_handler::RoutingInfoRequest GetRouteStatRequest(const json::Dict& request) {
    request_handler::RoutingInfoRequest result;
    result.id = request.at("id").AsInt();
    result.stop_from = request.at("from").AsString();
    result.stop_to = request.at("to").AsString();
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

//======================JSONReader==================================================

void JSONReader::GetOneBaseRequest(const json::Node& request) {
    const json::Dict& req_dict = request.AsDict();
    std::string_view type = req_dict.at("type").AsString();
    if (type == "Bus") {
        AddBaseRequest(detail::GetBusRequest(req_dict));
    } else if (type == "Stop") {
        AddBaseRequest(detail::GetStopRequest(req_dict));
    } else {
        throw std::logic_error("json_reader::ProcessOneStat: only \"Bus\" and \"Stop\" requests supported!");
    }
}

void JSONReader::Read() {
    using namespace json;

    doc_ = Load(in_);

    const Node& root = doc_.GetRoot();
    const Dict& requests = root.AsDict();

    auto settings_request_it = requests.find("render_settings");
    auto base_requests_it    = requests.find("base_requests");
    auto stat_requests_it    = requests.find("stat_requests");
    auto routing_settings_it = requests.find("routing_settings");
    auto serializ_settings_it  = requests.find("serialization_settings");

    auto end = requests.end();

    if (base_requests_it != end) {
        const Array& base_requests = base_requests_it->second.AsArray();
        ParseBaseRequests(base_requests);
    }

    if (settings_request_it != end) {
        const Dict& settings_request = settings_request_it->second.AsDict();
        settings_ = ParseRenderSettings(settings_request);
    }

    if (routing_settings_it != end) {
        const Dict& routing_settings_request = routing_settings_it->second.AsDict();
        routing_settings_ = ParseRoutingSettings(routing_settings_request);
    }

    if (stat_requests_it != end) {
        const Array& stat_requests = stat_requests_it->second.AsArray();
        ParseStatRequests(stat_requests);
    }

    if (serializ_settings_it != end) {
        const Dict& serialization_settings = serializ_settings_it->second.AsDict();
        serialization_settings_ = ParseSerializationSettings(serialization_settings);
    }
}

void JSONReader::ParseBaseRequests(const json::Array& requests) {
    for (const json::Node& request : requests) {
        GetOneBaseRequest(request);
    }
}

request_handler::RenderSettings JSONReader::ParseRenderSettings(const json::Dict& request) {
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

request_handler::RoutingSettings JSONReader::ParseRoutingSettings(const json::Dict& request) {
    request_handler::RoutingSettings result;

    result.bus_velocity = request.at("bus_velocity").AsDouble();
    result.wait_time    = request.at("bus_wait_time").AsInt();

    return result;
}

void JSONReader::GetOneStatRequest(const json::Node& request) {
    const json::Dict& req_dict = request.AsDict();
    std::string_view type = req_dict.at("type").AsString();
    if (type == "Bus") {
        AddStatRequest(detail::GetBusStatRequest(req_dict));
    } else if (type == "Stop") {
        AddStatRequest(detail::GetStopStatRequest(req_dict));
    } else if (type == "Map") {
        AddStatRequest(detail::GetMapStatRequest(req_dict));
    } else if (type == "Route"){
        AddStatRequest(detail::GetRouteStatRequest(req_dict));
    } else {
        throw std::logic_error("json_reader::ProcessOneStat: unsupported request \"" + std::string(type) + "\"\n");
    }
}

void JSONReader::ParseStatRequests(const json::Array& requests){
    for (const json::Node& request : requests) {
        GetOneStatRequest(request);
    }
}

request_handler::SerializationSettings JSONReader::ParseSerializationSettings(const json::Dict& request) {
    request_handler::SerializationSettings result;

    result.file = request.at("file").AsString();

    return result;
}

//=====================JSONPrinter===================================================

json::Dict JSONPrinter::ProcessStopRequest (const request_handler::StopInfo& stop_info){
    using namespace std::literals;

    const domain::StopInfo& info = stop_info.info;

    json::Builder builder{};

    builder.StartDict();

    if (info.was_found) {
        json::Array buses(info.buses.size());
        std::transform(info.buses.begin(), info.buses.end(), buses.begin(),
                       [](std::string_view name){
                           return std::string(name);
                       });
        builder.Key("buses").Value(std::move(buses));
    } else {
        builder.Key("error_message").Value("not found"s);
    }

    return builder.Key("request_id").Value(stop_info.id)
                  .EndDict().Build().AsDict();
}

json::Dict JSONPrinter::ProcessBusRequest (const request_handler::BusInfo& bus_info){
    using namespace std::literals;

    const domain::BusInfo& info = bus_info.info;

    json::Builder builder{};

    builder.StartDict();

    if (!info.stops.empty()) {
        builder.Key("curvature").Value(info.length.curvature)
               .Key("route_length").Value(info.length.real_length)
               .Key("stop_count").Value(static_cast<int>(info.stops.size()))
               .Key("unique_stop_count").Value(info.unique_stops);
    } else {
        builder.Key("error_message").Value("not found"s);
    }

    return builder.Key("request_id").Value(bus_info.id)
                  .EndDict().Build().AsDict();
}

void JSONPrinter::Print(request_handler::RouteInfo& request) {
    using namespace std::literals;
    json::Builder builder{};

    if (request.total_time == -1) {
        builder.StartDict().Key("request_id").Value(request.id)
                       .Key("error_message").Value("not found"s)
                       .EndDict();
        answers_.push_back(builder.Build());
        return;
    }

    builder.StartDict().Key("request_id").Value(request.id)
                       .Key("total_time").Value(request.total_time)
                       .Key("items").StartArray();

    for (request_handler::RouteItem& item : request.items) {
        builder.StartDict().Key("time").Value(item.time);

        if (item.type == request_handler::ItemType::Bus) {
            builder.Key("type").Value("Bus"s)
                   .Key("bus").Value(std::string(item.name))
                   .Key("span_count").Value(item.count);
        } else if (item.type == request_handler::ItemType::Wait) {
            builder.Key("type").Value("Wait"s)
                   .Key("stop_name").Value(std::string(item.name));
        }

        builder.EndDict();
    }


    answers_.push_back(builder.EndArray().EndDict().Build());
}

void JSONPrinter::Print(request_handler::MapInfo& request) {
    answers_.push_back(json::Builder{}.StartDict()
                                          .Key("map").Value(request.map_str)
                                          .Key("request_id").Value(request.id)
                                      .EndDict().Build());
}

} //namespace stream_input_json
