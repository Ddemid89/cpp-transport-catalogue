#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <unordered_map>
#include <set>
#include "geo.h"
#include <variant>

namespace domain {
struct DistanceInfo {
    double geo_length  = 0;
    double real_length = 0;
    double curvature   = 0;
};

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};

struct Bus {
    std::string name;
    std::vector<Stop*> stops;
    int unique_stops;
    bool is_roundtrip;
};

struct StopRequest {
    std::string_view name;
    geo::Coordinates coordinates;
    std::unordered_map<std::string_view, int> neighbours;
};

struct BusRequest {
   std::string_view name;
   std::vector<std::string_view> stops;
   bool is_roundtrip;
};

struct StopInfo {
    std::string_view name;
    const std::set<std::string_view>& buses;
    bool was_found;
};

struct BusInfo {
    std::string_view name;
    std::vector<std::string_view> stops;
    DistanceInfo length;
    int unique_stops;
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

class TransportData {
public:
    TransportData() = default;

    TransportData(const TransportData&) = delete;

    TransportData& operator=(const TransportData&) = delete;

    TransportData(TransportData&&) = default;

    void AddStop(const StopRequest& request);

    void AddBus(const BusRequest& request);

    const std::deque<Stop>& GetStopsArray() const;

    std::deque<Stop>& GetStopsArray();

    const std::deque<Bus>& GetBusesArray() const;

    std::deque<Bus>& GetBusesArray();

    const std::unordered_map<std::string_view, Bus*>& GetBusesMap() const;

    std::unordered_map<std::string_view, Bus*>& GetBusesMap();

    const std::unordered_map<std::string_view, Stop*>& GetStopsMap() const;

    std::unordered_map<std::string_view, Stop*>& GetStopsMap();

    const std::unordered_map<std::string_view, std::set<std::string_view>>&
    GetStopsToBuses () const;

    std::unordered_map<std::string_view, std::set<std::string_view>>&
    GetStopsToBuses ();

    Stop& GetStop(std::string_view name);

    Bus& GetBus(std::string_view name);

private:
    Stop& GetStopRef(std::string_view name);

    std::deque<Stop> stops_;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, Stop*> stops_refs_;
    std::unordered_map<std::string_view, Bus*> buses_refs_;
    std::unordered_map<std::string_view, std::set<std::string_view>> stops_to_buses_;
};

} //namespace domain
