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

    void RenderMap(const request_handler::MapData& data, std::ostream& out) override {
        if(!settings_) {
            throw std::logic_error("MapRendererJSON: can`t render without render settings!");
        }

        stops_points_ = GetStopPoints(data.stops_used);
        RenderBuses(data.buses);
        RenderStops();

        doc_.Render(out);

        stops_points_.clear();
        doc_ = svg::Document();
    }

    ~MapRendererJSON() override = default;
private:
    RenderSettings ConvertRenderSettings(const request_handler::RenderSettings& settings);

    std::map<std::string_view, svg::Point>
    GetStopPoints(const std::vector<std::pair<std::string_view, geo::Coordinates>>& stops);

    void RenderStops();

    void SetBusLabelSettings(svg::Text& label, bool underlayer);
    void RenderBusLabel(const domain::BusInfo& bus, const svg::Color&);
    void SetBusLineSettings(svg::Polyline& line, const svg::Color& color);
    void RenderBuses(const std::set<domain::BusForRender>&);

    std::optional<RenderSettings> settings_;
    svg::Document doc_;
    std::map<std::string_view, svg::Point> stops_points_;
};


} //namespace map_renderer
