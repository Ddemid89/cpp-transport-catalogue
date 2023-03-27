#include "svg.h"

namespace svg {

using namespace std::literals;

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}


// -----------Polyline-------------------

Polyline& Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    bool is_first = true;
    for (const Point point : points_) {
        if (!is_first) {
            out << " "sv;
        }
        is_first = false;
        out << point.x << ","sv << point.y;
    }
    out << "\"";
    RenderAttrs(out);
    out << "/>"sv;
}

// ------------Text--------------------------

Text& Text::SetPosition(Point pos){
    pos_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset){
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size){
    font_size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family){
    font_family_ = std::move(font_family);
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight){
    font_weight_ = std::move(font_weight);
    return *this;
}

Text& Text::SetData(std::string data){
    data_ = std::move(data);
    return *this;
}

void Text::ReplaceLetter(std::string& text, const Change ch) const {
    auto npos = text.npos;
    auto pos = text.find_first_of(ch.c);
    while(pos != npos) {
        npos = text.npos;
        text = text.substr(0, pos) + ch.s + text.substr(pos + 1);
        pos = text.find_first_of(ch.c, pos + 1);
    }
}

std::string Text::NormText(std::string text) const {
    const Change changes[] = {{'&',"&amp;"},
                              {'"',"&quot;"},
                              {'\'',"&apos;"},
                              {'<',"&lt;"},
                              {'>',"&gt;"}};

    for (const Change& ch : changes) {
        ReplaceLetter(text, ch);
    }
    return text;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text x=\"" << pos_.x << "\" y=\"" << pos_.y << "\" ";
    out << "dx=\"" << offset_.x << "\" dy=\"" << offset_.y << "\" ";
    out << "font-size=\"" << font_size_ << "\"";
    if (!font_family_.empty()) {
        out << " font-family=\"" << font_family_ << "\"";
    }
    if (!font_weight_.empty()) {
        out << " font-weight=\"" << font_weight_ << "\"";
    }
    RenderAttrs(out);
    out << ">" << NormText(data_) << "</text>";
}

// -------------------Document-------------------------

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.emplace_back(std::move(obj));
}

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">";
    out << std::endl;
    RenderContext context(out, 0, 2);
    for (const std::unique_ptr<Object>& object : objects_) {
        object.get()->Render(context);
    }
    out << "</svg>";
}

std::ostream& operator<<(std::ostream& out, StrokeLineJoin l_join) {
    if (l_join == StrokeLineJoin::ARCS) {
        out << "arcs";
    } else if (l_join == StrokeLineJoin::BEVEL) {
        out << "bevel";
    } else if (l_join == StrokeLineJoin::MITER) {
        out << "miter";
    } else if (l_join == StrokeLineJoin::MITER_CLIP) {
        out << "miter-clip";
    } else if (l_join == StrokeLineJoin::ROUND) {
        out << "round";
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, StrokeLineCap l_cap) {
    if (l_cap == StrokeLineCap::BUTT) {
        out << "butt";
    } else if (l_cap == StrokeLineCap::ROUND) {
        out << "round";
    } else if (l_cap == StrokeLineCap::SQUARE) {
        out << "square";
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, Color color) {
    ColorPrinter printer(out);
    std::visit(printer, color);
    return out;
}
}  // namespace svg
