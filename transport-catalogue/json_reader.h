#pragma once

#include "transport_catalogue.h"
#include "json.h"
#include <string_view>
#include <algorithm>
#include "domain.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "json_builder.h"

namespace stream_input_json {

class JSONReader : public request_handler::RequestReader {
public:
    JSONReader (std::istream& in = std::cin) : in_(in) {}

    void Read() override {
        using namespace json;

        doc_ = Load(in_);

        const Node& root = doc_.GetRoot();
        const Dict& requests = root.AsDict();

        auto settings_request_it = requests.find("render_settings");
        auto base_requests_it    = requests.find("base_requests");
        auto stat_requests_it    = requests.find("stat_requests");
        auto routing_settings_it = requests.find("routing_settings");

        auto end = requests.end();

        if (base_requests_it == end) {
            throw std::logic_error("No base requests!");
        }

        const Array& base_requests = base_requests_it->second.AsArray();
        GetBaseRequests(base_requests);

        if (settings_request_it != end) {
            const Dict& settings_request = settings_request_it->second.AsDict();
            settings_ = GetRenderSettings(settings_request);
        }

        if (routing_settings_it != end) {
            const Dict& routing_settings_request = routing_settings_it->second.AsDict();
            routing_settings_ = GetRoutingSettings(routing_settings_request);
        }

        if (stat_requests_it != end) {
            const Array& stat_requests = stat_requests_it->second.AsArray();
            GetStatRequests(stat_requests);
        }
    }

    void GetOneBaseRequest(const json::Node& request);
    void GetBaseRequests(const json::Array& requests);

    request_handler::RenderSettings GetRenderSettings(const json::Dict& request);
    request_handler::RoutingSettings GetRoutingSettings(const json::Dict& request);

    void GetOneStatRequest(const json::Node& request);
    void GetStatRequests(const json::Array& requests);

    ~JSONReader() override = default;
private:
    std::istream& in_;
    json::Document doc_ = json::Document{nullptr};
};

class JSONPrinter : public request_handler::RequestPrinter{
public:

    JSONPrinter(std::ostream& out) : out_(out) {}

    void Print(const request_handler::BusInfo& request) override {
        answers_.push_back(ProcessBusRequest(request));
    }

    void Print(const request_handler::StopInfo& request) override {
        answers_.push_back(ProcessStopRequest(request));
    }

    void Print(request_handler::MapInfo& request) override {
        answers_.push_back(json::Builder{}.StartDict()
                                              .Key("map").Value(request.map_str)
                                              .Key("request_id").Value(request.id)
                                          .EndDict().Build());
    }

    void Print(request_handler::RouteInfo& request) override {
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

    void RenderAll() override {
        json::Print(json::Document{answers_}, out_);
    }

    void Clear() override {
        answers_ = {nullptr};
    }

    ~JSONPrinter() override = default;
protected:
    json::Dict ProcessStopRequest (const request_handler::StopInfo& request);
    json::Dict ProcessBusRequest (const request_handler::BusInfo& request);

    std::ostream& out_;
    json::Array answers_;
};

} //namespace stream_input_json
