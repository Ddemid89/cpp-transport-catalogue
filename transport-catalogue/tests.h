#pragma once

#include "transport_catalogue.h"
#include <cassert>
#include <iostream>
#include <random>
#include <vector>
#include <string>
#include "stat_reader.h"
#include "input_reader.h"
#include <sstream>
#include <algorithm>
#include "json_reader.h"
#include "domain.h"
#include "request_handler.h"

#define OK std::cout << "OK line " << __LINE__ << " - " << __FILE__ << std::endl;
#define STOP std::cout << "Stop! Enter number >";{int asdf; std::cin >> asdf;}

class VoidMapRenderer : public request_handler::MapRenderer {
public:
    void RenderMap(const request_handler::MapData& data, std::ostream& out) override {
        out << "This is a map";
    }
    void SetRenderSettings(const request_handler::RenderSettings& settings) override {}
    ~VoidMapRenderer() override = default;
};


using namespace domain;

class NothigDoingReader : public request_handler::RequestReader{
public:
    void Read() override {

    }
    ~NothigDoingReader() override = default;
};

namespace tests {

void testTC() {
    TransportCatalogue tc;
    assert(tc.GetStopsSize() == 0);
    assert(tc.GetBusesSize() == 0);

    std::string stop1_name = "stop1";
    domain::StopRequest req;
    req.name = stop1_name;
    req.coordinates = {1, 2};

    tc.AddStop(req);
    assert(tc.GetStopsSize() == 1);
    assert(tc.GetBusesSize() == 0);
    domain::StopInfo stop_info = tc.GetStopInfo(stop1_name);
    assert(stop_info.was_found == true);

    std::string stop2_name = "stop2";
    req.name = stop2_name;
    req.coordinates = {3, 4};
    req.neighbours = std::unordered_map<std::string_view, int>{{stop1_name, 4}};

    tc.AddStop(req);
    assert(tc.GetStopsSize() == 2);
    assert(tc.GetBusesSize() == 0);
    domain::StopInfo stop_info2 = tc.GetStopInfo(stop2_name);
    assert(stop_info2.was_found == true);

    std::string bus1_name = "bus1";
    std::string bus2_name = "bus2";

    domain::BusRequest req2;
    req2.name = bus1_name;
    req2.is_roundtrip = true;
    req2.stops = {stop1_name, stop2_name};

    tc.AddBus(req2);
    assert(tc.GetStopsSize() == 2);
    assert(tc.GetBusesSize() == 1);
    domain::BusInfo bus_info = tc.GetBusInfo(bus1_name);
    assert(bus_info.name == bus1_name);
    assert(bus_info.stops.size() == 2);

    req2.name = bus2_name;
    req2.is_roundtrip = false;
    req2.stops = {stop1_name, stop2_name};

    tc.AddBus(req2);
    assert(tc.GetStopsSize() == 2);
    assert(tc.GetBusesSize() == 2);
    domain::BusInfo bus2_info = tc.GetBusInfo(bus2_name);
    assert(bus2_info.name == bus2_name);
    assert(bus2_info.stops.size() == 3);

}

void newFormat() {
    std::string req = "{\"render_settings\": {\"width\": 89298.28369209476, \"height\": 58011.29248202205, \"padding\": 22325.567226490493, \"stop_radius\": 21462.68635533567, \"line_width\": 38727.57356370373, \"stop_label_font_size\": 86988, \"stop_label_offset\": [-23192.03299796056, 92100.21839665441], \"underlayer_color\": \"coral\", \"underlayer_width\": 34006.680510317055, \"color_palette\": [[195, 60, 81, 0.6244132141059138], [2, 81, 213], [81, 152, 19, 0.6834377639654173], [94, 70, 16, 0.7604371566734329], [191, 220, 104], \"brown\", [164, 135, 29], [224, 79, 33], [60, 83, 79], [180, 239, 251, 0.10891745780969919], \"thistle\", \"green\", [33, 11, 167], [109, 4, 131, 0.9177069334241762], [247, 229, 13, 0.3216590299988016], \"green\", [95, 179, 198], [91, 176, 239], \"peru\", \"chocolate\", [26, 29, 136], \"orange\", \"gray\", \"red\", \"khaki\", \"wheat\", [227, 109, 210, 0.48281657728221594], [241, 161, 45], \"sienna\", \"orange\", \"purple\", [72, 249, 41, 0.815095041128292], \"black\", [217, 166, 56, 0.5710638920827465], [41, 106, 150], \"teal\", [189, 73, 65], \"magenta\", [45, 221, 209], \"brown\", [32, 66, 192, 0.007171850904110877], \"maroon\", [109, 44, 208], [83, 94, 186], \"red\", \"tan\", [171, 253, 129], \"olive\", [222, 53, 126], [41, 124, 196], [67, 197, 163], \"navy\", \"gold\", [209, 141, 148, 0.13808356551454504], [207, 5, 255, 0.23583697757948507]], \"bus_label_font_size\": 78497, \"bus_label_offset\": [59718.916265509615, 15913.541281271406]}, \"base_requests\": [{\"type\": \"Bus\", \"name\": \"i\", \"stops\": [\"gtKSYiTpuO3KjmLenbqOj7iO\", \"vtkKOKMLWRQv\"], \"is_roundtrip\": false}, {\"type\": \"Bus\", \"name\": \"jUYheoA3Mcm2 kqTN\", \"stops\": [\"gtKSYiTpuO3KjmLenbqOj7iO\", \"vtkKOKMLWRQv\", \"gtKSYiTpuO3KjmLenbqOj7iO\"], \"is_roundtrip\": true}, {\"type\": \"Stop\", \"name\": \"gtKSYiTpuO3KjmLenbqOj7iO\", \"latitude\": 42.60758584742835, \"longitude\": 38.503607612387185, \"road_distances\": {\"vtkKOKMLWRQv\": 451128}}, {\"type\": \"Stop\", \"name\": \"vtkKOKMLWRQv\", \"latitude\": 39.663496626279645, \"longitude\": 39.296652056808824, \"road_distances\": {\"gtKSYiTpuO3KjmLenbqOj7iO\": 385612}}, {\"type\": \"Bus\", \"name\": \"RkYZl\", \"stops\": [\"vtkKOKMLWRQv\", \"gtKSYiTpuO3KjmLenbqOj7iO\"], \"is_roundtrip\": false}], \"stat_requests\": [{\"id\": 1658922989, \"type\": \"Bus\", \"name\": \"RkYZl\"}, {\"id\": 946024612, \"type\": \"Bus\", \"name\": \"jUYheoA3Mcm2 kqTN\"}, {\"id\": 1960236243, \"type\": \"Bus\", \"name\": \"i\"}, {\"id\": 946024612, \"type\": \"Bus\", \"name\": \"jUYheoA3Mcm2 kqTN\"}, {\"id\": 2067965337, \"type\": \"Stop\", \"name\": \"vtkKOKMLWRQv\"}, {\"id\": 1086415934, \"type\": \"Stop\", \"name\": \"XK5ujSfpmS7p9mSx03uY\"}, {\"id\": 967287382, \"type\": \"Bus\", \"name\": \"xfRx1Ldg7LpeL9CMIL13A5q\"}, {\"id\": 1425889316, \"type\": \"Stop\", \"name\": \"gtKSYiTpuO3KjmLenbqOj7iO\"}, {\"id\": 486321339, \"type\": \"Map\"}, {\"id\": 1658922989, \"type\": \"Bus\", \"name\": \"RkYZl\"}, {\"id\": 486321339, \"type\": \"Map\"}]}";
    std::stringstream in;
    in << req;
    stream_input_json::JSONReader reader(in);
    stream_input_json::JSONPrinter printer(std::cout);
    TransportCatalogue cat;
    map_renderer::MapRendererJSON map_rend;
    request_handler::RequestHandler handler(cat, reader, printer, map_rend);



}


void testAll() {
    testTC();
    //testStopAdding();
    //testBusAdding();
    //testStopRequestParse();
    //testBusRequestParse();
    //testInputAll();
    //testStopInfoRequest();
    //testNewStopFormat();
    //testJsonInput();
    //testLesson14();
    //testSVG();
    newFormat();
    std::cout << "All            OK" << std::endl;
}
}//namespace tests
