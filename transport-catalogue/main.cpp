#include <iostream>
#include "request_handler.h"
#include "transport_catalogue.h"
#include "tests.h"
#include "stat_reader.h"
#include "input_reader.h"
#include "json_reader.h"



using namespace std;

int main() {
    //tests::testAll();
    TransportCatalogue tc;
    stream_input_json::JSONReader reader(std::cin);
    stream_input_json::JSONPrinter stat_printer(std::cout);
    map_renderer::MapRendererJSON map_renderer;

    request_handler::RequestHandler handler(tc, reader, stat_printer, map_renderer);

    return 0;
}
