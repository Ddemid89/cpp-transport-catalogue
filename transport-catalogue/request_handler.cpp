#include "request_handler.h"
#include "domain.h"
#include <string_view>
#include <string>
#include <sstream>
#include <unordered_map>
#include <optional>
#include "map_renderer.h"
#include "transport_catalogue.h"

namespace request_handler {

using RH = RequstHandler;

RH::RequstHandler(TransportCatalogue& catalogue) : catalogue_(catalogue) {}

void RH::AddBus(const domain::BusRequest& request) {
    catalogue_.AddBus(request);
}

void RH::AddBus(std::string_view name, std::vector<std::string_view>& stops, bool is_roundtrip) {
    domain::BusRequest request{name, stops, is_roundtrip};
    catalogue_.AddBus(request);
}

void RH::AddStop (const domain::StopRequest& request) {
    catalogue_.AddStop(request);
}

void RH::AddStop(std::string_view name, double latitude, double longitude,
             std::unordered_map<std::string_view, int>& neighbours){
    domain::StopRequest request{name, {latitude, longitude}, neighbours};
    catalogue_.AddStop(request);
}

void RH::SetRenderSettings(map_renderer::RenderSettings settings) {
    render_settings_ = std::move(settings);
}

domain::BusInfo RH::GetBusInfo(std::string_view name) const {
    return catalogue_.GetBusInfo(name);
}

domain::StopInfo RH::GetStopInfo(std::string_view name) const {
    return catalogue_.GetStopInfo(name);
}

std::string RH::GetMap() const {
    if (!render_settings_) {
        throw std::runtime_error("No render settings!");
    } else {
        std::stringstream out;
        map_renderer::RenderMap(catalogue_.GetData(), render_settings_.value(), out);
        return out.str();
    }
}

const domain::TransportData& RH::GetData() const {
    return catalogue_.GetData();
}
} //namespace request_handler
