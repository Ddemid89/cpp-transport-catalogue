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
        auto base_requests_it = requests.find("base_requests");
        auto stat_requests_it = requests.find("stat_requests");

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

        if (stat_requests_it != end) {
            const Array& stat_requests = stat_requests_it->second.AsArray();
            GetStatRequests(stat_requests);
        }
    }

    void GetOneBaseRequest(const json::Node& request);
    void GetBaseRequests(const json::Array& requests);

    request_handler::RenderSettings GetRenderSettings(const json::Dict& request);

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
