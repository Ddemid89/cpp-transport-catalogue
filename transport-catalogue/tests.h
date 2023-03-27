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

using namespace domain;

namespace tests {

geo::Coordinates random_coordinates(std::mt19937& generator) {
    geo::Coordinates result;
    double coors = std::uniform_int_distribution(490000,500000)(generator);
    result.lat = coors / 1000000;
    coors = std::uniform_int_distribution(490000,500000)(generator);
    result.lng = coors / 1000000;
    return result;
}

std::string getSpaces(int, int);

std::string randName(std::mt19937& gen, int count = 2) {
    std::string res = "";
    if (count < 1) {
        count = 1;
    }
    int n;
    bool is_first = true;

    for (int i = 0; i < count; ++i) {
        if (!is_first) {
            res += getSpaces(1, 2);
        }

        is_first = false;

        n = std::uniform_int_distribution(0, 1000)(gen);
        res += std::to_string(n);
    }
    return res;
}

std::vector<std::string> getRandomeNames(std::mt19937& generator, int count, int length) {
    std::vector<std::string> result;
    result.reserve(count);
    for (int i = 0; i < count; ++i) {
        result.push_back(randName(generator, length));
    }
    return result;
}

std::vector<domain::Stop> getRandomStops(std::mt19937& generator, int count) {
    std::vector<domain::Stop> result;
    result.reserve(count);

    for (int i = 0; i < count; ++i) {
        std::string name = randName(generator, 3);
        geo::Coordinates coors = random_coordinates(generator);
        result.push_back({std::move(name), coors});
    }

    return result;
}

domain::StopRequest getRandomStopRequest(std::mt19937& generator, std::string_view name) {
    domain::StopRequest result;
    result.name = name;
    result.coordinates = random_coordinates(generator);
    return result;
}

using Route = std::vector<int>;

Route getRoute(std::mt19937& generator, int max_length, int stops_count) {
    int length = std::uniform_int_distribution(5, max_length)(generator);

    Route result(length);

    for(int i = 0; i < length; ++i) {
        result[i] = std::uniform_int_distribution(0, stops_count)(generator);
    }

    return result;
}

std::vector<Route> getRandomRoutes(std::mt19937& generator, int max_len, int stops_count, int bus_count) {
    std::vector<Route> result;
    result.reserve(bus_count);

    for (int i = 0; i < bus_count; ++i) {
        result.push_back(getRoute(generator, max_len, stops_count - 1));
    }

    return result;
}

std::unordered_map<std::string_view, int> getNeighMap(const std::vector<domain::Stop>& stops) {
    std::unordered_map<std::string_view, int> result;

    for (const domain::Stop& stop : stops) {
        result[stop.name] = 1;
    }

    return result;
}

std::vector<domain::StopRequest> getRequestsForStops(const std::vector<domain::Stop>& stops) {
    auto neigh = getNeighMap(stops);
    std::vector<domain::StopRequest> result;
    result.reserve(stops.size());
    for (const domain::Stop& stop : stops) {
        result.push_back({stop.name, stop.coordinates, neigh});
    }
    return result;
}
#define OK std::cout << "OK line " << __LINE__ << " - " << __FILE__ << std::endl;

void testStopAdding() {
    TransportCatalogue tc;
    assert(tc.GetStopsSize() == 0);
    const int STOPS = 100000;
    std::mt19937 generator;

    std::vector<domain::Stop> stops;

    stops.reserve(STOPS);

    {
        std::string name = "Stop number 1";
        std::string_view name_v = name;
        geo::Coordinates coors = random_coordinates(generator);
        stops.push_back({name, coors});
        tc.AddStop({name_v, coors});
        assert(tc.GetStopsSize() == 1);

        coors = random_coordinates(generator);
        stops.push_back({"Stop number 2", coors});
        tc.AddStop({"Stop number 2", coors});
        assert(tc.GetStopsSize() == 2);

        coors = random_coordinates(generator);
        stops.push_back({"S", coors});
        tc.AddStop({"S", coors});
        assert(tc.GetStopsSize() == 3);
    }

    for(int i = 3; i < STOPS; ++i) {
        std::string name = std::to_string(i + 1);
        domain::StopRequest req = getRandomStopRequest(generator, name);
        domain::Stop stp{name, req.coordinates};
        stops.push_back(stp);
        tc.AddStop(req);
        assert(stops.size() == tc.GetStopsSize());
    }

    std::cout << "testStopAdding OK" << std::endl;
}

void testBusAdding() {
    const int STOPS_COUNT = 50;
    const int MAX_ROUTE_LEN = 10;
    const int BUSES_COUNT = 40;
    std::mt19937 generator;
    TransportCatalogue tc;

    assert(tc.GetBusesSize() == 0);

    std::vector<domain::Stop> stops = getRandomStops(generator, STOPS_COUNT);

    {
        std::vector<domain::StopRequest> reqs = getRequestsForStops(stops);
        for (int i = 0; i < STOPS_COUNT; ++i) {
            tc.AddStop(reqs[i]);
        }
    }

    assert(tc.GetStopsSize() == STOPS_COUNT);

    std::vector<Route> routs;
    routs.reserve(BUSES_COUNT);

    std::vector<std::string> names;
    names.reserve(BUSES_COUNT);

    routs.push_back({1, 3, 5, 7});
    routs.push_back({2, 2, 2, 2, 2, 2, 2});

    for (int i = routs.size(); i < BUSES_COUNT; ++i) {
        routs.push_back(getRoute(generator, MAX_ROUTE_LEN, STOPS_COUNT - 1));
    }

    assert(routs.size() == BUSES_COUNT);

    {
        std::string name = "BUS Number 1";
        domain::BusRequest req;
        req.name = name;
        names.push_back(name);
        for (size_t i = 0; i < routs[0].size(); ++i) {
            int stop_number = routs[0][i];
            std::string_view name = stops[stop_number].name;
            req.stops.push_back(name);
        }
        tc.AddBus(req);

        assert(tc.GetBusesSize() == 1);

        req.stops.clear();

        name = "BUS Number 2";
        req.name = name;
        names.push_back(name);
        for (size_t i = 0; i < routs[1].size(); ++i) {
            int stop_number = routs[1][i];
            std::string_view name = stops[stop_number].name;
            req.stops.push_back(name);
        }
        tc.AddBus(req);
        assert(tc.GetBusesSize() == 2);
    }

    for (size_t i = names.size(); i < BUSES_COUNT; ++i) {
        std::string name = std::to_string(i + 1);
        domain::BusRequest req;
        req.name = name;
        names.push_back(name);

        for (size_t j = 0; j < routs[i].size(); ++j) {
            int stop_number = routs[i][j];
            std::string_view name = stops[stop_number].name;
            req.stops.push_back(name);
        }
        tc.AddBus(req);
        assert(tc.GetBusesSize() == i + 1);
        req.stops.clear();
    }

    assert(tc.GetBusesSize() == BUSES_COUNT);

    assert(tc.GetBusInfo("aaa").stops.size() == 0);

    for (size_t i = 0; i < names.size(); ++i) {
        std::string_view name = names[i];
        domain::BusInfo bi = tc.GetBusInfo(name);
        assert(bi.name != "");
        assert(bi.stops.size() == routs[i].size());

        assert(bi.stops[0] == stops[routs[i][0]].name);

        double length = 0;
        int last_n = routs[i][0];

        for (size_t j = 1; j < routs[i].size(); ++j) {
            int n = routs[i][j];
            assert(stops[n].name == bi.stops[j]);
            length += ComputeDistance(stops[n].coordinates, stops[last_n].coordinates);
            last_n = n;
        }
        assert(length == bi.length.geo_length);
    }

    std::cout << "testBusAdding  OK" << std::endl;
}

void testStopRequestParse() {
    std::mt19937 generator;
    std::vector<std::string> names = {"Stop1",
                                      "Stop 2",
                                      "Stop   3",
                                      "Stop  4  ",
                                      "  Stop  5",
                                      "   Stop  6   ",
                                      };

    std::vector<std::string> norm_names = {"Stop1",
                                      "Stop 2",
                                      "Stop   3",
                                      "Stop  4",
                                      "Stop  5",
                                      "Stop  6",
                                      };

    std::vector<geo::Coordinates> coors;

    for (size_t i = 0; i < names.size(); ++i) {
        coors.push_back(random_coordinates(generator));
    }

    for (size_t i = 0; i < names.size(); ++i) {
        std::string text = "Stop " + names[i] + ": "
        + std::to_string(coors[i].lat) + ", "
        + std::to_string(coors[i].lng);
        domain::StopRequest req = stream_input::detail::ParseStopRequest(text);
        assert(req.name == norm_names[i]);
        assert(req.coordinates == coors[i]);
    }

    std::cout << "testInputFun1  OK" << std::endl;
}

void testBusRequestParse() {
    {
        std::string req = "    Bus     127  A   :    a1   >    a2   >    a3   >   a4    ";
        domain::BusRequest br1 = stream_input::detail::ParseBusRequest(req);
        assert(br1.name == "127  A");
        assert(br1.stops.size() == 4);
        assert(br1.stops[0] == "a1");
        assert(br1.stops[1] == "a2");
        assert(br1.stops[2] == "a3");
        assert(br1.stops[3] == "a4");

        req = "Bus 128   B   :   a1   -   a2   -   a3   -   a 4   ";
        domain::BusRequest br2 = stream_input::detail::ParseBusRequest(req);
        assert(br2.name == "128   B");
        assert(br2.stops.size() == 7);
        assert(br2.stops[0] == "a1");
        assert(br2.stops[1] == "a2");
        assert(br2.stops[2] == "a3");
        assert(br2.stops[3] == "a 4");
        assert(br2.stops[4] == "a3");
        assert(br2.stops[5] == "a2");
        assert(br2.stops[6] == "a1");
    }
    {
        std::string req = "Bus 127A: a1 > a2 > a3 > a4";
        domain::BusRequest br1 = stream_input::detail::ParseBusRequest(req);
        assert(br1.name == "127A");
        assert(br1.stops.size() == 4);
        assert(br1.stops[0] == "a1");
        assert(br1.stops[1] == "a2");
        assert(br1.stops[2] == "a3");
        assert(br1.stops[3] == "a4");

        req = "Bus 128B: a1 - a2 - a3 - a4";
        domain::BusRequest br2 =stream_input::detail::ParseBusRequest(req);
        assert(br2.name == "128B");
        assert(br2.stops.size() == 7);
        assert(br2.stops[0] == "a1");
        assert(br2.stops[1] == "a2");
        assert(br2.stops[2] == "a3");
        assert(br2.stops[3] == "a4");
        assert(br2.stops[4] == "a3");
        assert(br2.stops[5] == "a2");
        assert(br2.stops[6] == "a1");
    }
    std::cout << "testInputFun2  OK" << std::endl;
}

std::string getSpaces(int min = 0, int max = 5) {
    static std::mt19937 generator;
    int count = std::uniform_int_distribution(min, max)(generator);
    std::string result;
    for (int i = 0; i < count; ++i) {
        result += " ";
    }
    return result;
}

std::string getRandomStopReq(const domain::Stop& stop, const std::unordered_map<std::string_view, int>& neigh) {
    std::string result;
    result = getSpaces() + "Stop ";
    result += getSpaces() + stop.name + getSpaces() + ":";
    result += getSpaces() + std::to_string(stop.coordinates.lat) + getSpaces() + "," + getSpaces();
    result += std::to_string(stop.coordinates.lng) + getSpaces();
    for (auto& [name, dist] : neigh) {
        result += "," + getSpaces() + std::to_string(dist) + "m" + getSpaces() + " to " + getSpaces();
        result += std::string(name) + getSpaces();
    }
    return result;
}

std::string getRandomBusReq(const std::string& name,
                            const std::vector<domain::Stop>& stops,
                            const Route& route, bool isRing) {
    std::string result;
    result = getSpaces() + "Bus ";
    result += getSpaces() + name + getSpaces() + ":";

    bool is_first = true;

    char sep = isRing ? '>' : '-';

    for (int n : route) {
        result += getSpaces();
        if (!is_first) {
            result += sep + getSpaces();
        }
        is_first = false;
        result += stops[n].name + getSpaces();
    }


    return result;
}

double computeRouteLenght(const Route& route, const std::vector<domain::Stop>& stops, bool is_ring) {
    double res = 0;
    int prev, cur;

    for (size_t i = 1; i < route.size(); ++i) {
        prev = route[i - 1];
        cur  = route[i];
        const domain::Stop& prev_s = stops[prev];
        const domain::Stop& cur_s  = stops[cur];
        res += ComputeDistance(prev_s.coordinates, cur_s.coordinates);
    }

    if (!is_ring) {
        for (size_t i = route.size() - 1; i > 0; --i) {
            prev = route[i];
            cur  = route[i - 1];
            const domain::Stop& prev_s = stops[prev];
            const domain::Stop& cur_s  = stops[cur];
            res += ComputeDistance(prev_s.coordinates, cur_s.coordinates);
        }
    }

    return res;
}

std::string normDouble(double d) {
    std::stringstream str;
    std::string res;
    str << d;
    str >> res;
    return res;
}

std::string makeAnswer(const std::string bus_name,
                       const std::vector<domain::Stop>& stops,
                       const Route& route, bool is_ring) {

    std::string result;
    std::set<int> unique_stops(route.begin(), route.end());
    int route_size = route.size();
    route_size = is_ring ? route_size : route_size * 2 - 1;

    double geo_len = computeRouteLenght(route, stops, is_ring);
    int real_len = route.size() - 1;
    real_len = is_ring ? real_len : real_len * 2;

    result  = "Bus " + bus_name + ": " + std::to_string(route_size) + " stops on route, ";
    result += std::to_string(unique_stops.size()) + " unique stops, ";
    result += std::to_string(real_len) + " route length, ";
    result += normDouble(real_len / geo_len) + " curvature";

    return result;
}

void testInputAll(){
    std::stringstream buf;
    const int STOPS_COUNT = 300;
    const int MAX_LENGTH  = 30;
    const int BUS_COUNT = 100;
    std::mt19937 generator;

    std::vector<domain::Stop>  stops  = getRandomStops(generator, STOPS_COUNT);
    std::vector<Route> routes = getRandomRoutes(generator, MAX_LENGTH, STOPS_COUNT, BUS_COUNT);
    std::vector<std::string> bus_names = getRandomeNames(generator, BUS_COUNT, 2);
    std::vector<bool> is_ring;

    std::vector<std::string> requests;
    requests.reserve(stops.size() + routes.size());

    auto neigh = getNeighMap(stops);

    for (size_t i = 0; i < stops.size(); ++i) {
        requests.push_back(getRandomStopReq(stops[i], neigh));
        //std::cout << requests.back() << std::endl;
    }

    for (size_t i = 0; i < routes.size(); ++i) {
        is_ring.push_back(std::uniform_int_distribution(0, 1)(generator));
        requests.push_back(getRandomBusReq(bus_names[i], stops, routes[i], is_ring.back()));
        //std::cout << requests.back() << std::endl;
    }

    std::random_shuffle(requests.begin(), requests.end());

    buf << requests.size() << std::endl;

    for(auto& req : requests) {
        buf << req << std::endl;
    }

    TransportCatalogue tc;
    request_handler::RequstHandler h(tc);

    stream_input::GetDataFromIStream(h, buf);

    assert(tc.GetStopsSize() == STOPS_COUNT);
    assert(tc.GetBusesSize() == BUS_COUNT);

    for (size_t i = 0; i < bus_names.size(); ++i) {
        std::string ans = makeAnswer(bus_names[i], stops, routes[i], is_ring[i]);
        domain::BusInfo info = tc.GetBusInfo(bus_names[i]);
        buf.clear();
        stream_stats::detail::PrintBusInfoToOStream(info, buf);
        std::string ans2;
        getline(buf, ans2);
        assert(ans == ans2);
    }

    std::stringstream in, out;

    std::vector<int> indexes(BUS_COUNT);
    std::iota(indexes.begin(), indexes.end(), 0);
    std::random_shuffle(indexes.begin(), indexes.end());

    std::vector<std::string> anss(BUS_COUNT);

    in << BUS_COUNT << std::endl;

    for (int i = 0; i < BUS_COUNT; ++i) {
        int j = indexes[i];
        in << "Bus " << bus_names[j] << std::endl;
        anss[i] = makeAnswer(bus_names[j], stops, routes[j], is_ring[j]);
    }

    stream_stats::GetRequestsFromIStream(tc, in, out);

    for (int i = 0; i < BUS_COUNT; ++i) {
        std::string tmp;
        getline(out, tmp);
        assert(tmp == anss[i]);
    }

    std::cout << "testInputAll   OK" << std::endl;
}

void testStopInfoRequest() {
    std::vector<std::string> stop_requests;
    stop_requests.push_back("Stop 0: 0, 0"); //0   2
    stop_requests.push_back("Stop 1: 1, 1"); //  1   3     5
    stop_requests.push_back("Stop 2: 2, 2"); //0        4  5
    stop_requests.push_back("Stop 3: 3, 3"); //  1 2       5
    stop_requests.push_back("Stop 4: 4, 4"); //
    stop_requests.push_back("Stop 5: 5, 5"); //  1      4  5
    stop_requests.push_back("Stop 6: 6, 6"); //0   2       5
    stop_requests.push_back("Stop 7: 7, 7"); //  1   3     5
    stop_requests.push_back("Stop 8: 8, 8"); //0

    std::vector<std::string> bus_requests;
    bus_requests.push_back("Bus 0: 0 > 2 > 6 > 8");
    bus_requests.push_back("Bus 1: 1 - 3 - 5 - 7");
    bus_requests.push_back("Bus 2: 0 > 3 > 6");
    bus_requests.push_back("Bus 3: 1 - 7");
    bus_requests.push_back("Bus 4: 2 > 5");
    bus_requests.push_back("Bus 5: 1 - 2 - 3 - 5 - 6 - 7");

    TransportCatalogue tc;
    for (const std::string& req : stop_requests) {
        tc.AddStop(stream_input::detail::ParseStopRequest(req));
    }

    for (const std::string& req : bus_requests) {
        tc.AddBus(stream_input::detail::ParseBusRequest(req));
    }

    std::stringstream in;
    std::stringstream out;

    in << stop_requests.size() + 1 << std::endl;

    for (size_t i = 0; i < stop_requests.size() + 1; ++i) {
        in << "Stop " << std::to_string(i) << std::endl;
    }

    stream_stats::GetRequestsFromIStream(tc, in, out);

    std::vector<std::string> except_ans;
    except_ans.push_back("Stop 0: buses 0 2");
    except_ans.push_back("Stop 1: buses 1 3 5");
    except_ans.push_back("Stop 2: buses 0 4 5");
    except_ans.push_back("Stop 3: buses 1 2 5");
    except_ans.push_back("Stop 4: no buses");
    except_ans.push_back("Stop 5: buses 1 4 5");
    except_ans.push_back("Stop 6: buses 0 2 5");
    except_ans.push_back("Stop 7: buses 1 3 5");
    except_ans.push_back("Stop 8: buses 0");
    except_ans.push_back("Stop 9: not found");

    for (size_t i = 0; i < stop_requests.size() + 1; ++i) {
        std::string res;
        getline(out, res);
        assert(res == except_ans[i]);
    }

    std::cout << "testStopInfo   OK" << std::endl;
}

void assertsNewStop(const TransportCatalogue& tc) {
        assert(tc.GetBusInfo("0").length.real_length == 35);
        assert(tc.GetBusInfo("1").length.real_length == 40);
        assert(tc.GetBusInfo("2").length.real_length == 45);
        assert(tc.GetBusInfo("3").length.real_length == 11);

        assert(tc.GetBusInfo("4").length.real_length == 80);
        assert(tc.GetBusInfo("5").length.real_length == 80);
        assert(tc.GetBusInfo("6").length.real_length == 90);
        assert(tc.GetBusInfo("7").length.real_length == 22);
}

void testNewStopFormat() {
    std::vector<std::string> stop_reqs;
    stop_reqs.push_back("Stop 0: 0, 0, 10m to 1, 5m to 3");           //0->1=10  0->2=30  0->3=5
    stop_reqs.push_back("Stop 1: 1, 1, 20m to 0");                    //1->0=20  1->2=25  1->3=10
    stop_reqs.push_back("Stop 2: 2, 2, 30m to 0, 25m to 1, 15m to 3");//2->0=30  2->1=25  2->3=15
    stop_reqs.push_back("Stop 3: 3, 3, 10m to 1, 1m to 3");           //3->0=5   3->1=10  3->2=15  3->3=1
    std::vector<std::string> bus_reqs;
    bus_reqs.push_back("Bus 0: 0 > 1 > 2"); // 0 -(10)> 1 -(25)> 2 = 35
    bus_reqs.push_back("Bus 1: 1 > 2 > 3"); // 1 -(25)> 2 -(15)> 3 = 40
    bus_reqs.push_back("Bus 2: 0 > 2 > 3"); // 0 -(30)> 2 -(15)> 3 = 45
    bus_reqs.push_back("Bus 3: 1 > 3 > 3"); // 1 -(10)> 3 -(1 )> 3 = 11

    bus_reqs.push_back("Bus 4: 0 - 1 - 2"); // 0 -(10)> 1 -(25)> 2 -(25)> 1 -(20)> 0 = 80
    bus_reqs.push_back("Bus 5: 1 - 2 - 3"); // 1 -(25)> 2 -(15)> 3 -(15)> 2 -(25)> 1 = 80
    bus_reqs.push_back("Bus 6: 0 - 2 - 3"); // 0 -(30)> 2 -(15)> 3 -(15)> 2 -(30)> 0 = 90
    bus_reqs.push_back("Bus 7: 1 - 3 - 3"); // 1 -(10)> 3 -(1 )> 3 -(1 )> 3 -(10)> 1 = 22

    {
        TransportCatalogue tc;
        for (const std::string& req : stop_reqs) {
            tc.AddStop(stream_input::detail::ParseStopRequest(req));
        }
        for (const std::string& req : bus_reqs) {
            tc.AddBus(stream_input::detail::ParseBusRequest(req));
        }

        assertsNewStop(tc);
    }

    {
        TransportCatalogue tc;
        for (const std::string& req : bus_reqs) {
            tc.AddBus(stream_input::detail::ParseBusRequest(req));
        }
        for (const std::string& req : stop_reqs) {
            tc.AddStop(stream_input::detail::ParseStopRequest(req));
        }
        assertsNewStop(tc);
    }
    std::cout << "testINewStopF  OK" << std::endl;
}

std::string getJsonStopRequest(const domain::Stop& stop,
                               const std::unordered_map<std::string_view,
                               int>& neigh) {
    std::string result;
    result = "\t\t\{\n\t\t\t\"type\": \"Stop\",\n\t\t\t\"name\": \"" + stop.name + "\",\n\t\t\t";
    result += "\"latitude\": " + std::to_string(stop.coordinates.lat) + ",\n\t\t\t";
    result += "\"longitude\": " + std::to_string(stop.coordinates.lng);

    if (!neigh.empty()){
        result += ",\n\t\t\t\"road_distances\": {\n";
        bool is_first = true;
        for (auto& [name, dist] : neigh) {
            if (!is_first) {
                result += ",\n";
            }
            is_first = false;
            result += "\t\t\t\t\"" + std::string(name) + "\": ";
            result +=  std::to_string(dist);
        }
        result += "\n\t\t\t}";
    }

    result += "\n\t\t}";

    return result;
}

std::string getJsonBusRequest(const std::string& name,
                              const std::vector<domain::Stop>& stops,
                              const Route& route, bool isRing) {
    std::string result;
    result = "\t\t{\n\t\t\t\"type\": \"Bus\",\n\t\t\t\"name\": \"" + name + "\",\n\t\t\t";
    result += "\"stops\": [\n\t\t\t\t\"";

    bool is_first = true;

    for (int n : route) {
        if (!is_first) {
            result += ",\n\t\t\t\t\"";
        }
        is_first = false;
        result += stops[n].name + "\"";
    }

    result += "\n\t\t\t],\n\t\t\t\"is_roundtrip\": ";
    result += isRing ? "true" : "false";
    result += "\n\t\t}";

    return result;
}

void testLesson14() {
std::stringstream buf;
        std::stringstream jout;

        std::string bus_request = "{\n";
        bus_request += "\t\"base_requests\" : [\n";
        bus_request += "\t\t{\n";
        bus_request += "\t\t\t\"type\": \"Bus\",\n";
        bus_request += "\t\t\t\"name\": \"bus1\",\n";
        bus_request += "\t\t\t\"is_roundtrip\": false,\n";
        bus_request += "\t\t\t\"stops\": [\n";
        bus_request += "\t\t\t\t\"stop_A\",\n";
        bus_request += "\t\t\t\t\"stop_B\"\n";
        bus_request += "\t\t\t]\n";
        bus_request += "\t\t},\n";

        bus_request += "\t\t{\n";
        bus_request += "\t\t\t\"type\": \"Stop\",\n";
        bus_request += "\t\t\t\"name\": \"stop_A\",\n";
        bus_request += "\t\t\t\"longitude\": 1,\n";
        bus_request += "\t\t\t\"latitude\": 1,\n";
        bus_request += "\t\t\t\"road_distances\": {\n";
        bus_request += "\t\t\t\t\"stop_B\": 5\n";
        bus_request += "\t\t\t}\n";
        bus_request += "\t\t},\n";

        bus_request += "\t\t{\n";
        bus_request += "\t\t\t\"type\": \"Stop\",\n";
        bus_request += "\t\t\t\"name\": \"stop_B\",\n";
        bus_request += "\t\t\t\"longitude\": 1,\n";
        bus_request += "\t\t\t\"latitude\": 1,\n";
        bus_request += "\t\t\t\"road_distances\": {\n";
        bus_request += "\t\t\t\t\"stop_A\": 10\n";
        bus_request += "\t\t\t}\n";
        bus_request += "\t\t}\n";

        bus_request += "\t],\n";
        bus_request += "\t\"stat_requests\" : [\n";

        bus_request += "\t\t\{\n";
        bus_request += "\t\t\t\"id\": 1,\n";
        bus_request += "\t\t\t\"type\": \"Stop\",\n";
        bus_request += "\t\t\t\"name\": \"stop_A\"\n";
        bus_request += "\t\t},\n";

        bus_request += "\t\t\{\n";
        bus_request += "\t\t\t\"id\": 2,\n";
        bus_request += "\t\t\t\"type\": \"Stop\",\n";
        bus_request += "\t\t\t\"name\": \"stop_C\"\n";
        bus_request += "\t\t},\n";

        bus_request += "\t\t\{\n";
        bus_request += "\t\t\t\"id\": 3,\n";
        bus_request += "\t\t\t\"type\": \"Bus\",\n";
        bus_request += "\t\t\t\"name\": \"bus1\"\n";
        bus_request += "\t\t},\n";

        bus_request += "\t\t\{\n";
        bus_request += "\t\t\t\"id\": 4,\n";
        bus_request += "\t\t\t\"type\": \"Bus\",\n";
        bus_request += "\t\t\t\"name\": \"bus2\"\n";
        bus_request += "\t\t},\n";

        bus_request += "\t\t\{\n";
        bus_request += "\t\t\t\"id\": 5,\n";
        bus_request += "\t\t\t\"type\": \"Bus\",\n";
        bus_request += "\t\t\t\"name\": \"stop_C\"\n";
        bus_request += "\t\t}\n";

        bus_request += "\t]\n";
        bus_request += "}\n";

        TransportCatalogue t;
        request_handler::RequstHandler h(t);
        buf << bus_request;
        stream_input_json::GetJSONDataFromIStream(h, buf, jout);
        assert(t.GetStopsSize() == 2);
        assert(t.GetBusesSize() == 1);
        buf.clear();
}

void testJsonInput() {
    std::stringstream buf;
    std::stringstream j_out;
    const int STOPS_COUNT = 300;
    const int MAX_LENGTH  = 10;
    const int BUS_COUNT = 100;
    std::mt19937 generator;

    std::vector<domain::Stop>  stops  = getRandomStops(generator, STOPS_COUNT);
    std::vector<Route> routes = getRandomRoutes(generator, MAX_LENGTH, STOPS_COUNT, BUS_COUNT);
    std::vector<std::string> bus_names = getRandomeNames(generator, BUS_COUNT, 2);
    std::vector<bool> is_ring;

    domain::Stop s;

    {
        std::stringstream buf;
        std::stringstream jout;

        std::string bus_request = "{\n";
        bus_request += "\t\"base_requests\" : [\n";
        bus_request += "\t\t{\n";
        bus_request += "\t\t\t\"type\": \"Bus\",\n";
        bus_request += "\t\t\t\"name\": \"bus1\",\n";
        bus_request += "\t\t\t\"is_roundtrip\": false,\n";
        bus_request += "\t\t\t\"stops\": [\n";
        bus_request += "\t\t\t\t\"stop_A\",\n";
        bus_request += "\t\t\t\t\"stop_B\"\n";
        bus_request += "\t\t\t]\n";
        bus_request += "\t\t},\n";

        bus_request += "\t\t{\n";
        bus_request += "\t\t\t\"type\": \"Bus\",\n";
        bus_request += "\t\t\t\"name\": \"bus2\",\n";
        bus_request += "\t\t\t\"is_roundtrip\": true,\n";
        bus_request += "\t\t\t\"stops\": [\n";
        bus_request += "\t\t\t\t\"stop_A\",\n";
        bus_request += "\t\t\t\t\"stop_B\"\n";
        bus_request += "\t\t\t]\n";
        bus_request += "\t\t},\n";

        bus_request += "\t\t{\n";
        bus_request += "\t\t\t\"type\": \"Stop\",\n";
        bus_request += "\t\t\t\"name\": \"stop_A\",\n";
        bus_request += "\t\t\t\"longitude\": 1,\n";
        bus_request += "\t\t\t\"latitude\": 1,\n";
        bus_request += "\t\t\t\"road_distances\": {\n";
        bus_request += "\t\t\t\t\"stop_B\": 5\n";
        bus_request += "\t\t\t}\n";
        bus_request += "\t\t}\n";

        bus_request += "\t],\n";
        bus_request += "\t\"stat_requests\" : [\n";

        bus_request += "\t\t\{\n";
        bus_request += "\t\t\t\"id\": 1,\n";
        bus_request += "\t\t\t\"type\": \"Stop\",\n";
        bus_request += "\t\t\t\"name\": \"stop_A\"\n";
        bus_request += "\t\t},\n";

        bus_request += "\t\t\{\n";
        bus_request += "\t\t\t\"id\": 2,\n";
        bus_request += "\t\t\t\"type\": \"Stop\",\n";
        bus_request += "\t\t\t\"name\": \"stop_C\"\n";
        bus_request += "\t\t},\n";

        bus_request += "\t\t\{\n";
        bus_request += "\t\t\t\"id\": 3,\n";
        bus_request += "\t\t\t\"type\": \"Bus\",\n";
        bus_request += "\t\t\t\"name\": \"bus1\"\n";
        bus_request += "\t\t},\n";

        bus_request += "\t\t\{\n";
        bus_request += "\t\t\t\"id\": 4,\n";
        bus_request += "\t\t\t\"type\": \"Bus\",\n";
        bus_request += "\t\t\t\"name\": \"bus2\"\n";
        bus_request += "\t\t},\n";

        bus_request += "\t\t\{\n";
        bus_request += "\t\t\t\"id\": 5,\n";
        bus_request += "\t\t\t\"type\": \"Bus\",\n";
        bus_request += "\t\t\t\"name\": \"stop_C\"\n";
        bus_request += "\t\t}\n";

        bus_request += "\t]\n";
        bus_request += "}\n";

        TransportCatalogue t;
        request_handler::RequstHandler h(t);
        buf << bus_request;
        stream_input_json::GetJSONDataFromIStream(h, buf, jout);
        assert(t.GetStopsSize() == 2);
        assert(t.GetBusesSize() == 2);
        buf.clear();
    }

    {
        std::stringstream buf;
        std::string stop_request = "{\n";
        stop_request += "\t\"base_requests\" : [\n";
        stop_request += "\t\t{\n";
        stop_request += "\t\t\t\"type\": \"Stop\",\n";
        stop_request += "\t\t\t\"name\": \"stop1\",\n";
        stop_request += "\t\t\t\"longitude\": 1,\n";
        stop_request += "\t\t\t\"latitude\": 1,\n";
        stop_request += "\t\t\t\"road_distances\": {\n";
        stop_request += "\t\t\t\t\"stop_A\": 3,\n";
        stop_request += "\t\t\t\t\"stop_B\": 5\n";
        stop_request += "\t\t\t}\n";
        stop_request += "\t\t}\n";
        stop_request += "\t],\n";
        stop_request += "\t\"stat_requests\" : []\n";
        stop_request += "}\n";

        TransportCatalogue t;
        request_handler::RequstHandler h(t);
        buf << stop_request;
        stream_input_json::GetJSONDataFromIStream(h, buf);
        assert(t.GetStopsSize() == 3);
        assert(t.GetBusesSize() == 0);
        buf.clear();
    }

    std::vector<std::string> requests;
    requests.reserve(stops.size() + routes.size());

    auto neigh = getNeighMap(stops);

    for (size_t i = 0; i < stops.size(); ++i) {
        requests.push_back(getJsonStopRequest(stops[i], neigh));
    }

    for (size_t i = 0; i < routes.size(); ++i) {
        is_ring.push_back(std::uniform_int_distribution(0, 1)(generator));
        requests.push_back(getJsonBusRequest(bus_names[i], stops, routes[i], is_ring.back()));
        //std::cout << requests.back() << std::endl;
    }

    std::random_shuffle(requests.begin(), requests.end());

    buf << "{\n\t\"base_requests\": [\n";

    bool is_first = true;

    for(auto& req : requests) {
        if(!is_first) {
            buf << ",\n";
        }
        is_first = false;
        buf << req;
    }

    buf << "\n\t],\n\t\"stat_requests\": []\n}";

    TransportCatalogue tc;
    request_handler::RequstHandler h(tc);

    stream_input_json::GetJSONDataFromIStream(h, buf, j_out);

    assert(tc.GetStopsSize() == STOPS_COUNT);
    assert(tc.GetBusesSize() == BUS_COUNT);

    for (size_t i = 0; i < bus_names.size(); ++i) {
        std::string ans = makeAnswer(bus_names[i], stops, routes[i], is_ring[i]);
        domain::BusInfo info = tc.GetBusInfo(bus_names[i]);
        buf.clear();
        stream_stats::detail::PrintBusInfoToOStream(info, buf);
        std::string ans2;
        getline(buf, ans2);
        assert(ans == ans2);
    }

    std::stringstream in, out;

    std::vector<int> indexes(BUS_COUNT);
    std::iota(indexes.begin(), indexes.end(), 0);
    std::random_shuffle(indexes.begin(), indexes.end());

    std::vector<std::string> anss(BUS_COUNT);

    in << BUS_COUNT << std::endl;

    for (int i = 0; i < BUS_COUNT; ++i) {
        int j = indexes[i];
        in << "Bus " << bus_names[j] << std::endl;
        anss[i] = makeAnswer(bus_names[j], stops, routes[j], is_ring[j]);
    }

    stream_stats::GetRequestsFromIStream(tc, in, out);

    for (int i = 0; i < BUS_COUNT; ++i) {
        std::string tmp;
        getline(out, tmp);
        assert(tmp == anss[i]);
    }
    std::cout << "testJsonInput  OK" << std::endl;
}

void testSVG() {
    std::string req;
    req += "{\n";
    req += "\t\"base_requests\": [\n";
    req += "\t\t{\n";
    req += "\t\t\t\"type\": \"Bus\",\n";
    req += "\t\t\t\"name\": \"14\",\n";
    req += "\t\t\t\"stops\": [\n";
    req += "\t\t\t\t\"Ulitsa Lizy Chaikinoi\",\n";
    req += "\t\t\t\t\"Elektroseti\",\n";
    req += "\t\t\t\t\"Ulitsa Dokuchaeva\",\n";
    req += "\t\t\t\t\"Ulitsa Lizy Chaikinoi\"\n";
    req += "\t\t\t],\n";
    req += "\t\t\t\"is_roundtrip\": true\n";
    req += "\t\t},\n";
    req += "\t\t{\n";
    req += "\t\t\t\"type\": \"Bus\",\n";
    req += "\t\t\t\"name\": \"114\",\n";
    req += "\t\t\t\"stops\": [\n";
    req += "\t\t\t\t\"Morskoy vokzal\",\n";
    req += "\t\t\t\t\"Rivierskiy most\"\n";
    req += "\t\t\t],\n";
    req += "\t\t\t\"is_roundtrip\": false\n";
    req += "\t\t},\n";
    req += "\t\t{\n";
    req += "\t\t\t\"type\": \"Stop\",\n";
    req += "\t\t\t\"name\": \"Rivierskiy most\",\n";
    req += "\t\t\t\"latitude\": 43.587795,\n";
    req += "\t\t\t\"longitude\": 39.716901,\n";
    req += "\t\t\t\"road_distances\": {\n";
    req += "\t\t\t\t\"Morskoy vokzal\": 850\n";
    req += "\t\t\t}\n";
    req += "\t\t},\n";
    req += "\t\t{\n";
    req += "\t\t\t\"type\": \"Stop\",\n";
    req += "\t\t\t\"name\": \"Morskoy vokzal\",\n";
    req += "\t\t\t\"latitude\": 43.581969,\n";
    req += "\t\t\t\"longitude\": 39.719848,\n";
    req += "\t\t\t\"road_distances\": {\n";
    req += "\t\t\t\t\"Rivierskiy most\": 850\n";
    req += "\t\t\t}\n";
    req += "\t\t},\n";
    req += "\t\t{\n";
    req += "\t\t\t\"type\": \"Stop\",\n";
    req += "\t\t\t\"name\": \"Elektroseti\",\n";
    req += "\t\t\t\"latitude\": 43.598701,\n";
    req += "\t\t\t\"longitude\": 39.730623,\n";
    req += "\t\t\t\"road_distances\": {\n";
    req += "\t\t\t\t\"Ulitsa Dokuchaeva\": 3000,\n";
    req += "\t\t\t\t\"Ulitsa Lizy Chaikinoi\": 4300\n";
    req += "\t\t\t}\n";
    req += "\t\t},\n";
    req += "\t\t{\n";
    req += "\t\t\t\"type\": \"Stop\",\n";
    req += "\t\t\t\"name\": \"Ulitsa Dokuchaeva\",\n";
    req += "\t\t\t\"latitude\": 43.585586,\n";
    req += "\t\t\t\"longitude\": 39.733879,\n";
    req += "\t\t\t\"road_distances\": {\n";
    req += "\t\t\t\t\"Ulitsa Lizy Chaikinoi\": 2000,\n";
    req += "\t\t\t\t\"Elektroseti\": 3000\n";
    req += "\t\t\t}\n";
    req += "\t\t},\n";
    req += "\t\t{\n";
    req += "\t\t\t\"type\": \"Stop\",\n";
    req += "\t\t\t\"name\": \"Ulitsa Lizy Chaikinoi\",\n";
    req += "\t\t\t\"latitude\": 43.590317,\n";
    req += "\t\t\t\"longitude\": 39.746833,\n";
    req += "\t\t\t\"road_distances\": {\n";
    req += "\t\t\t\t\"Elektroseti\": 4300,\n";
    req += "\t\t\t\t\"Ulitsa Dokuchaeva\": 2000\n";
    req += "\t\t\t}\n";
    req += "\t\t}\n";
    req += "\t],\n";
    req += "\t\"render_settings\": {\n";
    req += "\t\t\"width\": 600,\n";
    req += "\t\t\"height\": 400,\n";
    req += "\t\t\"padding\": 50,\n";
    req += "\t\t\"stop_radius\": 5,\n";
    req += "\t\t\"line_width\": 14,\n";
    req += "\t\t\"bus_label_font_size\": 20,\n";
    req += "\t\t\"bus_label_offset\": [\n";
    req += "\t\t\t7,\n";
    req += "\t\t\t15\n";
    req += "\t\t],\n";
    req += "\t\t\"stop_label_font_size\": 20,\n";
    req += "\t\t\"stop_label_offset\": [\n";
    req += "\t\t\t7,\n";
    req += "\t\t\t-3\n";
    req += "\t\t],\n";
    req += "\t\t\"underlayer_color\": [\n";
    req += "\t\t\t255,\n";
    req += "\t\t\t255,\n";
    req += "\t\t\t255,\n";
    req += "\t\t\t0.85\n";
    req += "\t\t],\n";
    req += "\t\t\"underlayer_width\": 3,\n";
    req += "\t\t\"color_palette\": [\n";
    req += "\t\t\t\"green\",\n";
    req += "\t\t\t[255, 160, 0],\n";
    req += "\t\t\t\"red\"\n";
    req += "\t\t]\n";
    req += "\t},\n";
    req += "\t\"stat_requests\": [\n";
    req += "\t]\n";
    req += "}\n";

    std::stringstream in;
    in << req;
    stream_input_json::GetJSONDataFromIStreamAndRenderMap(in);
}

void testAll() {
    testStopAdding();
    testBusAdding();
    testStopRequestParse();
    testBusRequestParse();
    testInputAll();
    testStopInfoRequest();
    testNewStopFormat();
    testJsonInput();
    testLesson14();
    testSVG();
    std::cout << "All            OK" << std::endl;
}
}//namespace tests
