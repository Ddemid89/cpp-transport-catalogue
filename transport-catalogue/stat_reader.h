#pragma once

#include <iostream>
#include "transport_catalogue.h"
#include "domain.h"
#include "request_handler.h"

namespace stream_stats {
class IStreamPrinter : public request_handler::RequestPrinter {
public:
    IStreamPrinter(std::ostream& out = std::cout) : out_(out) {}

    void Print(const request_handler::BusInfo& bus_info) override {
        const domain::BusInfo& info = bus_info.info;
        result_ << "Bus " << std::string(bus_info.info.name) << ": ";
        if (!info.stops.empty()) {
            result_ << info.stops.size() << " stops on route, ";
            result_ << info.unique_stops << " unique stops, ";
            result_ << info.length.real_length << " route length, ";
            result_ << info.length.curvature << " curvature";
        } else {
            result_ << "not found";
        }
        result_ << "\n";
    }

    void Print(const request_handler::StopInfo& stop_info) override {
        result_ << "Stop " << std::string(stop_info.info.name) << ": ";

        if (stop_info.info.was_found) {
            if (!stop_info.info.buses.empty()) {
                result_ << "buses";
                for (std::string_view bus : stop_info.info.buses) {
                    result_ << " " << std::string(bus);
                }
            } else {
                result_ << "no buses";
            }
        } else {
            result_ << "not found";
        }
        result_ << "\n";
    }

    void Print(request_handler::MapInfo& request) override {

    }

    void RenderAll() override {
        out_ << result_.str();
    }

    void Clear() override {
        result_.clear();
    }

    ~IStreamPrinter() override = default;
private:
    std::ostream& out_;
    std::stringstream result_;
};
}//namespace stream_stats
