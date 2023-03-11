#pragma once

#include <iostream>
#include "transport_catalogue.h"

namespace stream_input {
namespace detail {

std::string_view RemoveFirstWord(std::string_view text);

std::string_view GetNextWord(std::string_view& text, char separator);

TransportCatalogue::StopRequest ParseStopRequest(std::string_view request);

TransportCatalogue::BusRequest ParseBusRequest(std::string_view request);

}//namespace detail

void GetDataFromIStream(TransportCatalogue& catalogue, std::istream& in = std::cin);
}//namespace stream_input
