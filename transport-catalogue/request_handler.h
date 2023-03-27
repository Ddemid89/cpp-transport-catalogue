#pragma once

#include "domain.h"
#include <string_view>
#include <string>
#include <sstream>
#include <unordered_map>
#include <optional>
#include "map_renderer.h"
#include "transport_catalogue.h"

namespace request_handler {
class RequstHandler{
public:
    RequstHandler(TransportCatalogue& catalogue);

    void AddBus(const domain::BusRequest& request);

    void AddBus(std::string_view name, std::vector<std::string_view>& stops, bool is_roundtrip);

    void AddStop (const domain::StopRequest& request);

    void AddStop(std::string_view name, double latitude, double longitude,
                 std::unordered_map<std::string_view, int>& neighbours);

    void SetRenderSettings(map_renderer::RenderSettings settings);

    domain::BusInfo GetBusInfo(std::string_view name) const;

    domain::StopInfo GetStopInfo(std::string_view name) const;

    std::string GetMap() const;

    const domain::TransportData& GetData() const;

private:
    TransportCatalogue& catalogue_;
    std::optional<map_renderer::RenderSettings> render_settings_;
};
} //namespace request_handler
