#include "stat_reader.h"

namespace stat {
    void ParseAndPrintStat(const TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output) {
        if (request.substr(0, 3) == "Bus") {
            std::string_view bus_name(request.substr(4));
            auto info = transport_catalogue.GetBusInfo(bus_name);
            if (!info) {
                output << "Bus " << bus_name << ": not found" << std::endl;
            }
            else {
                output << "Bus " << bus_name << ": "
                    << info->count_stops << " stops on route, "
                    << info->unique_stops << " unique stops, "
                    << info->length << " route length, "
                    << info->curvature << " curvature" << std::endl;
            }
        }
        if (request.substr(0, 4) == "Stop") {
            std::string_view stop_name(request.substr(5));
            auto info = transport_catalogue.GetStopInfo(stop_name);
            if (!info.has_value()) {
                output << "Stop " << stop_name << ": not found" << std::endl;
                return;
            }
            const std::set<std::string>* buses_ptr = info.value();
            if (!buses_ptr) {
                output << "Stop " << stop_name << ": no buses" << std::endl;
            }
            else {
                output << "Stop " << stop_name << ": buses";
                for (const auto& bus : *buses_ptr) {
                    output << " " << bus;
                }
                output << std::endl;
            }
        }
    }
    void StatRequest(const TransportCatalogue& transport_catalogue, std::istream& input, std::ostream& output) {
        int stat_request_count;
        input >> stat_request_count >> std::ws;
        for (int i = 0; i < stat_request_count; ++i) {
            std::string line;
            std::getline(input, line);
            ParseAndPrintStat(transport_catalogue, line, output);
        }
    }
}