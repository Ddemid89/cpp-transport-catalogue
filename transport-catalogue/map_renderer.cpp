#include "map_renderer.h"
#include "domain.h"
#include "svg.h"
#include <string_view>
#include "geo.h"
#include <algorithm>
#include <optional>
#include <vector>
#include <deque>
#include <unordered_map>
#include <iostream>
#include <map>
#include <memory>

namespace map_renderer {
namespace detail {

inline const double EPSILON = 1e-6;
bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

class SphereProjector {
public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding)
        : padding_(padding) //
    {
        if (points_begin == points_end) {
            return;
        }

        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            zoom_coeff_ = *height_zoom;
        }
    }

    svg::Point operator()(geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

svg::Point ConvertPointToSvg(domain::Point point) {
    return {point.x, point.y};
}

svg::Color ConvertOneColor(const domain::Color& color) {
    svg::Color result;
    if (std::holds_alternative<std::string>(color)) {
        result = std::get<std::string>(color);
    } else if (std::holds_alternative<domain::Rgb>(color)) {
        domain::Rgb rgb = std::get<domain::Rgb>(color);
        result = svg::Rgb{rgb.red, rgb.green, rgb.blue};
    } else if (std::holds_alternative<domain::Rgba>(color)) {
        domain::Rgba rgb = std::get<domain::Rgba>(color);
        result = svg::Rgba{rgb.red, rgb.green, rgb.blue, rgb.opacity};
    }
    return result;
}

} //namespace detail


RenderSettings MapRendererJSON::ConvertRenderSettings(const request_handler::RenderSettings& settings) {
    RenderSettings result;
    result.bus_label_font_size = settings.bus_label_font_size;
    result.bus_label_offset    = detail::ConvertPointToSvg(settings.bus_label_offset);

    for (const domain::Color& color : settings.color_palette) {
        result.color_palette.push_back(detail::ConvertOneColor(color));
    }

    result.height     = settings.height;
    result.line_width = settings.line_width;
    result.padding    = settings.padding;

    result.stop_label_font_size = settings.stop_label_font_size;
    result.stop_label_offset    = detail::ConvertPointToSvg(settings.stop_label_offset);

    result.stop_radius      = settings.stop_radius;
    result.underlayer_color = detail::ConvertOneColor(settings.underlayer_color);
    result.underlayer_width = settings.underlayer_width;
    result.width            = settings.width;

    return result;
}

std::map<std::string_view, svg::Point>
MapRendererJSON::GetStopPoints
(const std::vector<std::pair<std::string_view, geo::Coordinates>>& stops){
    std::map<std::string_view, svg::Point> result;
    std::vector<geo::Coordinates> coordinates(stops.size());
    RenderSettings& settings = settings_.value();

    std::transform(stops.begin(), stops.end(), coordinates.begin(),
                   [](const std::pair<const std::string_view, geo::Coordinates>& stop){
                        return stop.second;
                   });
    detail::SphereProjector projector(coordinates.begin(), coordinates.end(),
                                      settings.width, settings.height, settings.padding);

    std::for_each(stops.begin(), stops.end(),
                   [&projector, &result](const std::pair<const std::string_view, geo::Coordinates>& stop){
                        result[stop.first] = projector(stop.second);
                   });

    return result;
}

void MapRendererJSON::RenderStops() {
    RenderSettings& settings = settings_.value();
    svg::Circle stop_point;
    stop_point.SetRadius(settings.stop_radius);
    stop_point.SetFillColor("white");

    std::vector<svg::Text> stop_labels;

    svg::Text underlayer;
    svg::Text label;

    underlayer.SetOffset(settings.stop_label_offset)
              .SetFontSize(settings.stop_label_font_size)
              .SetFontFamily("Verdana")

              .SetFillColor(settings.underlayer_color)
              .SetStrokeColor(settings.underlayer_color)
              .SetStrokeWidth(settings.underlayer_width)
              .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
              .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    label.SetOffset(settings.stop_label_offset)
         .SetFontSize(settings.stop_label_font_size)
         .SetFontFamily("Verdana")
         .SetFillColor("black");

    for (const std::pair<const std::string_view, svg::Point> stop : stops_points_) {
        stop_point.SetCenter(stop.second);
        doc_.Add(stop_point);

        underlayer.SetData(std::string(stop.first));
        label.SetData(std::string(stop.first));
        underlayer.SetPosition(stop.second);
        label.SetPosition(stop.second);
        stop_labels.push_back(underlayer);
        stop_labels.push_back(label);
    }

    for (svg::Text stop_label : stop_labels) {
        doc_.Add(stop_label);
    }
}

void MapRendererJSON::SetBusLabelSettings(svg::Text& label, bool underlayer){
    RenderSettings& settings = settings_.value();
    label.SetOffset(settings.bus_label_offset)
         .SetFontSize(settings.bus_label_font_size)
         .SetFontFamily("Verdana")
         .SetFontWeight("bold");
    if (underlayer) {
        label.SetFillColor(settings.underlayer_color)
             .SetStrokeColor(settings.underlayer_color)
             .SetStrokeWidth(settings.underlayer_width)
             .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
             .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    }
}

void MapRendererJSON::SetBusLineSettings(svg::Polyline& line, const svg::Color& color){
    const RenderSettings& settings = settings_.value();

    line.SetFillColor("none")
        .SetStrokeColor(color)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
        .SetStrokeWidth(settings.line_width);
}

void MapRendererJSON::RenderBuses(const std::set<domain::BusForRender>& buses) {
    int current_color_index = 0;
    const std::vector<svg::Color>& palette = settings_.value().color_palette;
    const int colors_count = palette.size();

    std::vector<svg::Text> bus_labels;
    svg::Text underlayer;
    svg::Text label;

    SetBusLabelSettings(underlayer, true);
    SetBusLabelSettings(label, false);

    for (const domain::BusForRender& bus : buses) {
        svg::Color current_color = palette[current_color_index % colors_count];
        current_color_index++;

        svg::Polyline line;
        SetBusLineSettings(line, current_color);

        for(std::string_view stop : bus.stops) {
            line.AddPoint(stops_points_.at(stop));
        }

        doc_.Add(line);

        svg::Point point = stops_points_.at(bus.stops[0]);

        underlayer.SetData(std::string(bus.name));
        label.SetData(std::string(bus.name));
        label.SetFillColor(current_color);
        underlayer.SetPosition(point);
        label.SetPosition(point);

        bus_labels.push_back(underlayer);
        bus_labels.push_back(label);

        if (!bus.is_roundtrip) {
            int last_stop_index = (bus.stops.size() - 1) / 2;
            if (bus.stops[0] != bus.stops[last_stop_index]) {
                point = stops_points_.at(bus.stops[last_stop_index]);
                underlayer.SetPosition(point);
                label.SetPosition(point);
                bus_labels.push_back(underlayer);
                bus_labels.push_back(label);
            }
        }
    }

    for (svg::Text bus_label : bus_labels) {
        doc_.Add(bus_label);
    }
}

} //namespace map_renderer
