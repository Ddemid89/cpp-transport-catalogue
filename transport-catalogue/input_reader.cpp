#include "input_reader.h"
#include <iostream>
#include "transport_catalogue.h"
#include <vector>
#include "domain.h"
#include "request_handler.h"

namespace stream_input {
request_handler::AddingBusRequest IStreamReader::GetBusReques(std::string_view request) {
    request_handler::AddingBusRequest result;

    std::string_view name = detail::RemoveFirstWord(detail::GetNextWord(request, ':'));

    result.name = GetStringRef(name);

    bool is_ring = request.find_first_of('>') != request.npos;
    char separator = is_ring ? '>' : '-';

    result.is_roundtrip = is_ring;

    while (!request.empty()) {
        name = detail::GetNextWord(request, separator);
        result.stops.push_back(GetStringRef(name));
    }

    return result;
}

request_handler::AddingStopRequest IStreamReader::GetStopReques(std::string_view request) {
    request_handler::AddingStopRequest result;

    std::string_view name = detail::GetNextWord(request, ':');
    name = detail::RemoveFirstWord(name);
    name = GetStringRef(name);
    std::string_view lat = detail::GetNextWord(request, ',');
    std::string_view lng = detail::GetNextWord(request, ',');

    result.name = name;
    result.latitude = std::stod(std::string(lat));
    result.longitude = std::stod(std::string(lng));

    while (!request.empty()) {
        std::string_view distance = detail::GetNextWord(request, 'm');
        std::string_view other_name = detail::GetNextWord(request, ',');
        other_name = detail::RemoveFirstWord(other_name);
        other_name = GetStringRef(other_name);
        result.neighbours[other_name] = std::stoi(std::string(distance));
    }

    return result;
}

request_handler::BusInfoRequest IStreamReader::GetBusStatRequest(std::string_view request) {
    request_handler::BusInfoRequest result;
    request = detail::RemoveFirstWord(request);
    result.name = GetStringRef(request);
    result.id = -1;
    return result;
}

request_handler::StopInfoRequest IStreamReader::GetStopStatRequest(std::string_view request) {
    request_handler::StopInfoRequest result;
    request = detail::RemoveFirstWord(request);
    result.name = GetStringRef(request);
    result.id = -1;
    return result;
}

request_handler::MapInfoRequest IStreamReader::GetMapStatRequest(std::string_view request) {
    request_handler::MapInfoRequest result;
    result.id = -1;
    return result;
}


void IStreamReader::GetBaseRequests() {
    if (in_.eof()) {
        return;
    }
    int n;
    in_ >> n;
    std::string escape;
    getline(in_, escape);
    for (int i = 0; i < n; ++i) {
        GetOneBaseRequest();
    }
}

void IStreamReader::GetStatRequests() {
    if (in_.eof()) {
        return;
    }
    int n;
    in_ >> n;
    std::string escape;
    getline(in_, escape);
    for (int i = 0; i < n; ++i) {
        GetOneStatRequest();
    }
}

void IStreamReader::GetOneBaseRequest() {
    std::string request;
    getline(in_, request);
    size_t pos = request.find_first_not_of(' ');

    if (request[pos] == 'S') {
        AddBaseRequest(GetStopReques(request));
    } else if (request[pos] == 'B') {
        AddBaseRequest(GetBusReques(request));
    } else {
        throw std::logic_error("json_reader::ProcessOneStat: only \"Bus\" and \"Stop\" requests supported!");
    }
}

void IStreamReader::GetOneStatRequest() {
    std::string request;
    getline(in_, request);
    size_t pos = request.find_first_not_of(' ');
    if (request[pos] == 'S') {
        AddStatRequest(GetStopStatRequest(request));
    } else if (request[pos] == 'B') {
        AddStatRequest(GetBusStatRequest(request));
    } else if (request[pos] == 'M') {
        AddStatRequest(GetMapStatRequest(request));
    } else {
        throw std::logic_error("json_reader::ProcessOneStat: only \"Bus\", \"Stop\" and, maybe \"Map\" requests supported!");
    }
}

void IStreamReader::GetRenderSettings() {

}

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
}//namespace detail
}//namespace stream_input
