#pragma once

#include <iostream>
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

void GetDataFromIStream(request_handler::RequstHandler& catalogue, std::istream& in = std::cin);
}//namespace stream_input
