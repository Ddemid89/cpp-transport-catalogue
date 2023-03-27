#pragma once

#include "transport_catalogue.h"
#include "json.h"
#include <string_view>
#include <algorithm>
#include "domain.h"
#include "map_renderer.h"
#include "request_handler.h"

namespace stream_input_json{
    void GetJSONDataFromIStream (request_handler::RequstHandler& catalogue,
                                 std::istream& in = std::cin,
                                 std::ostream& out = std::cout);

    void GetJSONDataFromIStreamAndRenderMap(std::istream& in = std::cin,
                                            std::ostream& out = std::cout);

} //namespace stream_input_json
