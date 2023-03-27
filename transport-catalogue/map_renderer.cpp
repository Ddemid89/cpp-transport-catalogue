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

SphereProjector GetProjector(const std::vector<domain::Stop*>& stops,
                             double width, double height, double padding) {
    std::vector<geo::Coordinates> coordinates(stops.size());
    std::transform(stops.begin(), stops.end(), coordinates.begin(),
               [](const domain::Stop* stop){
                    return stop->coordinates;
               });
    return SphereProjector(coordinates.begin(), coordinates.end(), width, height, padding);
}

std::map<std::string_view, svg::Point> GetStopPoints (const std::vector<domain::Stop*> stops,
                           double width, double height, double padding) {

    detail::SphereProjector projector = detail::GetProjector(stops, width,
                                                             height, padding);
    std::map<std::string_view, svg::Point> stops_points;

    std::for_each(stops.begin(), stops.end(),
                  [&projector, &stops_points](const domain::Stop* stop){
                        stops_points[stop->name] = projector(stop->coordinates);
                  });
    return stops_points;
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

std::vector<svg::Color> ConvertColors(const std::vector<domain::Color>& pallete) {
    std::vector<svg::Color> result;
    result.reserve(pallete.size());
    for (const domain::Color& color : pallete) {
        result.push_back(ConvertOneColor(color));
    }
    return result;
}

std::vector<domain::Stop*> GetUsedStops(const domain::TransportData& data) {
    const std::unordered_map<std::string_view, std::set<std::string_view>>& stops_to_buses
        = data.GetStopsToBuses();
    const std::unordered_map<std::string_view, domain::Stop*>& all_stops = data.GetStopsMap();

    std::vector<domain::Stop*> stops(stops_to_buses.size());

    std::transform(stops_to_buses.begin(), stops_to_buses.end(), stops.begin(),
        [&all_stops](const std::pair<const std::string_view, std::set<std::string_view>>& stop) {
            return all_stops.at(stop.first);
        });
    return stops;
}

void DrawRoute (svg::Document& doc, const domain::Bus& bus,
                const std::map<std::string_view, svg::Point>& stops_points,
                double line_width, svg::Color color,
                svg::StrokeLineCap line_cap = svg::StrokeLineCap::ROUND,
                svg::StrokeLineJoin line_join = svg::StrokeLineJoin::ROUND,
                svg::Color fill_color = svg::Color{}) {
    svg::Polyline line;
    line.SetStrokeWidth(line_width);
    line.SetStrokeColor(color);
    line.SetStrokeLineJoin(line_join);
    line.SetStrokeLineCap(line_cap);
    line.SetFillColor(fill_color);

    for (const domain::Stop* stop : bus.stops) {
        std::string_view name = stop->name;
        line.AddPoint(stops_points.at(name));
    }

    doc.Add(line);
}

void FillBusTextSettings(svg::Text& text, const RenderSettings& settings,
                         std::string_view name, svg::Point point) {
    svg::Point svg_point{point.x, point.y};
    svg::Point svg_offset{settings.bus_label_offset.x, settings.bus_label_offset.y};

    text.SetPosition(svg_point);
    text.SetOffset(svg_offset);
    text.SetFontSize(settings.bus_label_font_size);
    text.SetFontFamily("Verdana");
    text.SetFontWeight("bold");
    text.SetData(std::string(name));
}

void DrawOneRouteName (std::vector<std::unique_ptr<svg::Object>>& result, std::string_view name,
                       svg::Point stop_point, const RenderSettings& settings,
                       const svg::Color& color) {
    svg::Text underlayer;
    svg::Text bus_name;
    FillBusTextSettings(underlayer, settings, name, stop_point);
    FillBusTextSettings(bus_name, settings, name, stop_point);

    underlayer.SetFillColor(ConvertOneColor(settings.underlayer_color));
    underlayer.SetStrokeColor(ConvertOneColor(settings.underlayer_color));
    underlayer.SetStrokeWidth(settings.underlayer_width);
    underlayer.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    underlayer.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    bus_name.SetFillColor(color);

    result.push_back(std::move(std::make_unique<svg::Text>(std::move(underlayer))));
    result.push_back(std::move(std::make_unique<svg::Text>(std::move(bus_name))));
}

void DrawRouteNames (std::vector<std::unique_ptr<svg::Object>>& result, const domain::Bus& bus,
                    const std::map<std::string_view, svg::Point>& stops_points,
                    const svg::Color& color, const RenderSettings& settings) {

    svg::Point first_stop = stops_points.at(bus.stops.at(0)->name);

    DrawOneRouteName(result, bus.name, first_stop, settings, color);

    if (!bus.is_roundtrip) {
        size_t last_stop_index = (bus.stops.size() + 1) / 2 - 1;

        if (bus.stops.at(0) != bus.stops.at(last_stop_index)) {
            svg::Point last_stop = stops_points.at(bus.stops.at(last_stop_index)->name);
            DrawOneRouteName(result, bus.name, last_stop, settings, color);
        }
    }
}

void FillStopTextSettings (svg::Text& text, const RenderSettings& settings,
                           svg::Point point, std::string_view name) {
    svg::Point svg_offset(settings.stop_label_offset.x, settings.stop_label_offset.y);
    text.SetPosition(point);
    text.SetOffset(svg_offset);
    text.SetFontSize(settings.stop_label_font_size);
    text.SetFontFamily("Verdana");
    text.SetData(std::string(name));
}

void DrawStops (svg::Document& doc, std::vector<std::unique_ptr<svg::Object>>& result,
                std::string_view name, const svg::Point& stop_point, const RenderSettings& settings) {

    svg::Circle point;
    point.SetCenter(stop_point);
    point.SetRadius(settings.stop_radius);
    point.SetFillColor("white");
    doc.Add(point);

    svg::Text underlayer;

    FillStopTextSettings(underlayer, settings, stop_point, name);
    underlayer.SetFillColor(ConvertOneColor(settings.underlayer_color));
    underlayer.SetStrokeColor(ConvertOneColor(settings.underlayer_color));
    underlayer.SetStrokeWidth(settings.underlayer_width);
    underlayer.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    underlayer.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    svg::Text stop_name;
    FillStopTextSettings(stop_name, settings, stop_point, name);
    stop_name.SetFillColor("black");

    result.push_back(std::move(std::make_unique<svg::Text>(std::move(underlayer))));
    result.push_back(std::move(std::make_unique<svg::Text>(std::move(stop_name))));
}

} //namespace detail



void RenderMap(const domain::TransportData& data,
               const RenderSettings settings, std::ostream& out){

    const std::vector<domain::Stop*> used_stops = detail::GetUsedStops(data);

    const std::map<std::string_view, svg::Point> stops_points
        = detail::GetStopPoints(used_stops, settings.width, settings.height, settings.padding);

    const std::unordered_map<std::string_view, domain::Bus*>& buses
        = data.GetBusesMap();

    std::map<std::string_view, domain::Bus*> sorted_buses(buses.begin(), buses.end());

    int current_color_index = 0;
    const int colors_count = settings.color_palette.size();

    std::vector<svg::Color> palette = detail::ConvertColors(settings.color_palette);

    svg::Document document;

    std::vector<std::unique_ptr<svg::Object>> bus_labels;

    for (const std::pair<const std::string_view, domain::Bus*>& bus : sorted_buses) {
        if (!bus.second->stops.empty()) {
            svg::Color current_color = palette[current_color_index % colors_count];
            current_color_index++;
            detail::DrawRoute(document, *bus.second, stops_points,
                              settings.line_width, current_color);
            detail::DrawRouteNames(bus_labels, *bus.second, stops_points,
                                   current_color, settings);
        }
    }

    for (std::unique_ptr<svg::Object>& obj : bus_labels) {
        document.AddPtr(std::move(obj));
    }

    bus_labels.clear();

    std::vector<std::unique_ptr<svg::Object>> stop_labels;

    for (const std::pair<const std::string_view, svg::Point>& point : stops_points) {
        detail::DrawStops(document, stop_labels, point.first, point.second, settings);
    }

    for (std::unique_ptr<svg::Object>& obj : stop_labels) {
        document.AddPtr(std::move(obj));
    }

    document.Render(out);
}

} //namespace map_renderer
