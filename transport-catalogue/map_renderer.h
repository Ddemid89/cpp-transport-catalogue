#pragma once

#include "svg.h"
#include <vector>
#include "domain.h"
#include <optional>
#include "request_handler.h"
#include <map>

namespace map_renderer {

struct RenderSettings {
    double width;
    double height;

    double padding;

    double line_width;
    double stop_radius;

    int bus_label_font_size;
    svg::Point bus_label_offset;

    int stop_label_font_size;
    svg::Point stop_label_offset;

    svg::Color underlayer_color;
    double underlayer_width;

    std::vector<svg::Color> color_palette;
};

class MapRendererJSON : public request_handler::MapRenderer {
public:
    void SetRenderSettings(const request_handler::RenderSettings& settings) override {
        settings_ = ConvertRenderSettings(settings);
    }

    void SetStopPoints(std::map<std::string, domain::Point>& stops_points) override;

    std::map<std::string_view, domain::Point>
    GetStopPoints() const;

    void ComputeStopPoints(const std::vector<std::pair<std::string_view, geo::Coordinates>>& stops);

    void RenderMap(const request_handler::MapData& data, std::ostream& out) override;

    ~MapRendererJSON() override = default;
private:
    RenderSettings ConvertRenderSettings(const request_handler::RenderSettings& settings);

    void RenderStops();

    void SetBusLabelSettings(svg::Text& label, bool underlayer);
    void RenderBusLabel(const domain::BusInfo& bus, const svg::Color&);
    void SetBusLineSettings(svg::Polyline& line, const svg::Color& color);
    void RenderBuses(const std::set<domain::BusForRender>&);

    std::optional<RenderSettings> settings_;
    svg::Document doc_;
    std::map<std::string_view, svg::Point> stops_points_;
    bool have_stop_points_ = false;
};

} //namespace map_renderer
