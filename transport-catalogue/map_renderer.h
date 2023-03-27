#pragma once

#include "svg.h"
#include <vector>
#include "domain.h"

namespace map_renderer {

struct RenderSettings {
    double width;
    double height;

    double padding;

    double line_width;
    double stop_radius;

    int bus_label_font_size;
    domain::Point bus_label_offset;

    int stop_label_font_size;
    domain::Point stop_label_offset;

    domain::Color underlayer_color;
    double underlayer_width;

    std::vector<domain::Color> color_palette;
};

void RenderMap(const domain::TransportData& data, const RenderSettings settings, std::ostream& out);

} //namespace map_renderer
