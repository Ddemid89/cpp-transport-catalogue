#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace svg {

struct Rgb {
    Rgb() = default;
    Rgb(uint8_t r, uint8_t g, uint8_t b)  : red(r),
                                            green(g),
                                            blue(b) {}
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};

struct Rgba {
    Rgba() = default;
    Rgba(uint8_t r, uint8_t g, uint8_t b, double a)  :  red(r),
                                                        green(g),
                                                        blue(b),
                                                        opacity(a) {}
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    double opacity = 1.0;
};

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;
inline const Color NoneColor{"none"};

class ColorPrinter {
public:
    ColorPrinter(std::ostream& out) : out_(out) {}

    void operator()(std::monostate) const {
        out_ << "none";
    }

    void operator()(std::string str) const {
        out_ << str;
    }

    void operator()(Rgb rgb) const {
        out_ << "rgb("
        << static_cast<int>(rgb.red) << ","
        << static_cast<int>(rgb.green) << ","
        << static_cast<int>(rgb.blue) << ")";
    }

    void operator()(Rgba rgba) const {
        out_ << "rgba(" << static_cast<int>(rgba.red) << ","
                        << static_cast<int>(rgba.green) << ","
                        << static_cast<int>(rgba.blue) << ","
                        << rgba.opacity << ")";
    }
private:
    std::ostream& out_;
};

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

std::ostream& operator<<(std::ostream& out, StrokeLineJoin l_join);
std::ostream& operator<<(std::ostream& out, StrokeLineCap l_cap);
std::ostream& operator<<(std::ostream& out, Color color);

template <class Owner>
class PathProps{
public:
    Owner& SetFillColor(Color color) {
        fill_color_ = color;
        return AsOwner();
    }

    Owner& SetStrokeColor(Color color) {
        stroke_color_ = color;
        return AsOwner();
    }

    Owner& SetStrokeWidth(double width) {
        stroke_width_ = width;
        return AsOwner();
    }

    Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
        line_cap_ = line_cap;
        return AsOwner();
    }

    Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
        line_join_ = line_join;
        return AsOwner();
    }

protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream& out) const {
        using namespace std::literals;

        PrintIf(fill_color_,   " fill=\"",            out);
        PrintIf(stroke_color_, " stroke=\"",          out);
        PrintIf(stroke_width_, " stroke-width=\"",    out);
        PrintIf(line_cap_,     " stroke-linecap=\"",  out);
        PrintIf(line_join_,    " stroke-linejoin=\"", out);
    }

private:
    Owner& AsOwner() {
        return static_cast<Owner&>(*this);
    }

    template <class T>
    void PrintIf(std::optional<T> opt, const std::string& pref, std::ostream& out) const {
        using namespace std::literals;
        if (opt) {
            out << pref << *opt << "\""sv;
        }
    }

    std::optional<Color>          fill_color_;
    std::optional<Color>          stroke_color_;
    std::optional<double>         stroke_width_;
    std::optional<StrokeLineCap>  line_cap_;
    std::optional<StrokeLineJoin> line_join_;
};

struct Point {
    Point() = default;
    Point(double x, double y)
        : x(x)
        , y(y) {
    }
    double x = 0;
    double y = 0;
};

struct RenderContext {
    RenderContext(std::ostream& out)
        : out(out) {
    }

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out)
        , indent_step(indent_step)
        , indent(indent) {
    }

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

class Object {
public:
    void Render(const RenderContext& context) const;

    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

class Circle final : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);
private:
    void RenderObject(const RenderContext& context) const override;

    Point center_;
    double radius_ = 1.0;
};

class Polyline : public Object, public PathProps<Polyline> {
public:
    Polyline& AddPoint(Point point);

private:
    void RenderObject(const RenderContext& context) const override;

    std::vector<Point> points_;
};

class Text : public Object, public PathProps<Text> {
public:
    Text& SetPosition(Point pos);

    Text& SetOffset(Point offset);

    Text& SetFontSize(uint32_t size);

    Text& SetFontFamily(std::string font_family);

    Text& SetFontWeight(std::string font_weight);

    Text& SetData(std::string data);
private:
    struct Change {
        char c;
        std::string s;
    };

    void RenderObject(const RenderContext& context) const override;

    void ReplaceLetter(std::string& text, const Change ch) const;

    std::string NormText(std::string text) const;

    Point pos_;
    Point offset_;
    uint32_t font_size_ = 1;
    std::string font_family_;
    std::string font_weight_;
    std::string data_;
};

// ------------Interfaces-----------------

class ObjectContainer {
public:
    template <class SomeObject>
    void Add(SomeObject obj) {
        AddPtr(std::move(std::make_unique<SomeObject>(std::move(obj))));
    }

    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
protected:
    ~ObjectContainer() {};
};

class Drawable {
public:
    virtual void Draw(svg::ObjectContainer& container) const = 0;
    virtual ~Drawable() = default;
};

class Document : public ObjectContainer {
public:
    void AddPtr(std::unique_ptr<Object>&& obj) override;

    void Render(std::ostream& out) const;
private:
    std::vector<std::unique_ptr<Object>> objects_;
};

}  // namespace svg
