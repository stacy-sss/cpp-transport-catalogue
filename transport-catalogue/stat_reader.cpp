#include "stat_reader.h"

namespace stat {
    void ParseAndPrintStat(const TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output) {
        if (request.substr(0, 3) == "Bus") {
            std::string bus_name(request.substr(4));
            transport_catalogue.GetBusInfo(bus_name, output);
        }
        if (request.substr(0, 4) == "Stop") {
            std::string stop_name(request.substr(5));
            transport_catalogue.GetStopInfo(stop_name, output);
        }
    }
}