#include "input_reader.h"
#include <iostream>
#include "transport_catalogue.h"
#include <vector>
#include "domain.h"
#include "request_handler.h"

namespace stream_input {
namespace detail {

std::string_view RemoveFirstWord(std::string_view text) {
    auto pos = text.find_first_of(' ');
    pos = text.find_first_not_of(' ', pos);
    text.remove_prefix(pos);
    return text;
}

std::string_view GetNextWord(std::string_view& text, char separator) {
    std::string_view result;
    auto pos = text.find_first_not_of(' ');
    if (pos == text.npos) {
        return result;
    }

    text.remove_prefix(pos);

    pos = text.find_first_of(separator);

    if (pos == text.npos) {
        result = text;
        text.remove_prefix(text.size());
    } else {
        result = text.substr(0, pos);
        text.remove_prefix(pos + 1);
    }

    pos = result.find_last_not_of(' ');
    result = result.substr(0, pos + 1);
    return result;
}

domain::StopRequest ParseStopRequest(std::string_view request) {
    assert(!request.empty());
    domain::StopRequest result;

    std::string_view name = GetNextWord(request, ':');
    name = RemoveFirstWord(name);
    std::string_view lat = GetNextWord(request, ',');
    std::string_view lng = GetNextWord(request, ',');

    result.name = name;
    result.coordinates.lat = std::stod(std::string(lat));
    result.coordinates.lng = std::stod(std::string(lng));

    while (!request.empty()) {
        std::string_view distance = GetNextWord(request, 'm');
        std::string_view other_name = GetNextWord(request, ',');
        other_name = RemoveFirstWord(other_name);
        result.neighbours[other_name] = std::stoi(std::string(distance));
    }

    return result;
}

domain::BusRequest ParseBusRequest(std::string_view request) {
    domain::BusRequest result;

    result.name = RemoveFirstWord(GetNextWord(request, ':'));

    bool is_ring = request.find_first_of('>') != request.npos;
    char separator = is_ring ? '>' : '-';

    while (!request.empty()) {
        result.stops.push_back(GetNextWord(request, separator));
    }

    if (!is_ring) {
        for (int i = result.stops.size() - 2; i >= 0; --i) {
            result.stops.push_back(result.stops[i]);
        }
    }

    return result;
}
}//namespace detail

void GetDataFromIStream(request_handler::RequstHandler& catalogue, std::istream& in) { //default in = std::cin (in .h file)
    int request_count;
    in >> request_count;
    std::string request;
    getline(in, request);

    for (int i = 0; i < request_count; ++i) {
        getline(in, request);
        int first_letter = request.find_first_not_of(' ');
        if (request[first_letter] == 'S') {
            catalogue.AddStop(detail::ParseStopRequest(request));
        } else if (request[first_letter] == 'B'){
            catalogue.AddBus(detail::ParseBusRequest(request));
        } else {
            throw std::invalid_argument("Wrong request! Only \"Bus\" and \"Stop\" requests are supported now.");
        }
    }
}
}//namespace stream_input
