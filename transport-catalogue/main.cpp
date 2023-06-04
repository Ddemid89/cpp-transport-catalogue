#include "request_handler.h"
#include "transport_catalogue.h"
#include "json_reader.h"
#include <fstream>
#include <string_view>
#include <iostream>

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    stream_input_json::JSONReader reader(std::cin);
    reader.Read();

    if (mode == "make_base"sv) {
        map_renderer::MapRendererJSON renderer;
        request_handler::CatalogueSerializationHandler serializator(reader.GetSerializationSettings());
        serializator.Serialize(reader, renderer);
    } else if (mode == "process_requests"sv) {
        stream_input_json::JSONPrinter printer(std::cout);
        map_renderer::MapRendererJSON renderer;
        request_handler::CatalogueDeserializationHandler deserializator(reader.GetSerializationSettings());
        deserializator.Deserialize(printer, renderer, reader);
    } else {
        PrintUsage();
        return 1;
    }

    return 0;
}
