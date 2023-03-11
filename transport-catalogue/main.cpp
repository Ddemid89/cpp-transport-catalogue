#include <iostream>
#include "transport_catalogue.h"
#include "stat_reader.h"
#include "input_reader.h"

using namespace std;

int main() {
    TransportCatalogue catalogue;
    stream_input::GetDataFromIStream(catalogue);
    stream_stats::GetRequestsFromIStream(catalogue);
    return 0;
}
