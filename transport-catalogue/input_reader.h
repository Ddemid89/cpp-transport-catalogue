#pragma once

#include <iostream>
#include <unordered_set>
#include "transport_catalogue.h"
#include "domain.h"
#include "request_handler.h"

namespace stream_input {
namespace detail {

std::string_view RemoveFirstWord(std::string_view text);

std::string_view GetNextWord(std::string_view& text, char separator);

domain::StopRequest ParseStopRequest(std::string_view request);

domain::BusRequest ParseBusRequest(std::string_view request);

}//namespace detail

class IStreamReader : public request_handler::RequestReader {
public:
    IStreamReader(std::istream& in = std::cin) : in_(in) {}

    void Read() override {
        GetBaseRequests();
        GetStatRequests();
        GetRenderSettings();
    }

    ~IStreamReader() override = default;
private:
    std::string_view GetStringRef(std::string_view str) {
        auto it = names_refs_.find(str);
        if (it != names_refs_.end()) {
            return *it;
        } else {
            names_.push_back(std::string(str));
            names_refs_.insert(names_.back());
            return names_.back();
        }
    }

    void GetOneBaseRequest();
    void GetBaseRequests();
    void GetOneStatRequest();
    void GetStatRequests();
    void GetRenderSettings();

    request_handler::AddingBusRequest GetBusReques(std::string_view request);
    request_handler::AddingStopRequest GetStopReques(std::string_view request);
    request_handler::BusInfoRequest GetBusStatRequest(std::string_view request);
    request_handler::StopInfoRequest GetStopStatRequest(std::string_view request);
    request_handler::MapInfoRequest GetMapStatRequest(std::string_view request);

    std::istream& in_;
    std::unordered_set<std::string_view> names_refs_;
    std::deque<std::string> names_;
};

void GetDataFromIStream(request_handler::RequestHandler& catalogue, std::istream& in = std::cin);
}//namespace stream_input
