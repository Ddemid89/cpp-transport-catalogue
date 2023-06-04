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

    void Read() override;

    void GetOneBaseRequest(const json::Node& request);
    void ParseBaseRequests(const json::Array& requests);

    request_handler::RenderSettings ParseRenderSettings(const json::Dict& request);
    request_handler::RoutingSettings ParseRoutingSettings(const json::Dict& request);
    request_handler::SerializationSettings ParseSerializationSettings(const json::Dict& request);

    void GetOneStatRequest(const json::Node& request);
    void ParseStatRequests(const json::Array& requests);

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

    void Print(request_handler::MapInfo& request) override;

    void Print(request_handler::RouteInfo& request) override;

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
