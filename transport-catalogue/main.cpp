#include <iostream>
#include "transport_catalogue.h"
#include "tests.h"
#include "stat_reader.h"
#include "input_reader.h"
#include "json_reader.h"
#include "request_handler.h"


using namespace std;

int main() {
    //tests::testAll();
    TransportCatalogue tc;
    request_handler::RequstHandler handler(tc);

    stream_input_json::GetJSONDataFromIStream(handler);

    //stream_input_json::GetJSONDataFromIStreamAndRenderMap();
    //TransportCatalogue catalogue;
    //stream_input_json::GetJSONDataFromIStream(catalogue);
    //stream_input::GetDataFromIStream(catalogue);
    //stream_stats::GetRequestsFromIStream(catalogue);
    return 0;
}
