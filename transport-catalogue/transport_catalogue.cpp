#include "transport_catalogue.h"
#include "geo.h"

void TransportCatalogue::AddStop(std::string name, double latitude, double longitude) {
    stops_.push_back({ std::move(name), latitude,longitude });
    auto it = &stops_.back();
    info_stop[it->name] = it;

}

const Stop* TransportCatalogue::FindStop(const std::string& name) const {
    auto it = info_stop.find(name);
    if (it != info_stop.end()) {
        return it->second;
    }
    return nullptr;
}

void TransportCatalogue::AddBus(std::string name, std::vector<std::string>& stops) {
    Bus bus;
    bus.name = name;

    for (const auto& stop : stops) {
        Stop* stop1 = const_cast<Stop*>(FindStop(stop));
        if (stop1) {
            bus.stops.push_back(stop1);
            stop_to_buses[stop].insert(bus.name);
        }
    }
    buses_.push_back({ std::move(bus) });
    info_bus[buses_.back().name] = &buses_.back();
}

const Bus* TransportCatalogue::FindBus(const std::string& name) const {
    auto it = info_bus.find(name);
    if (it != info_bus.end()) {
        return it->second;
    }
    return nullptr;
}

void TransportCatalogue::GetBusInfo(const std::string& bus_name, std::ostream& out) const {
    const Bus* it_bus = FindBus(bus_name);
    //если не найдено
    if (!it_bus) {
        out << "Bus " << bus_name << ": not found" << std::endl;
        return;
    }

    size_t count_stops = it_bus->stops.size();
    std::unordered_set<std::string> unique_stops;
    double length = 0.0;

    for (size_t i = 0; i < it_bus->stops.size(); i++) {
        unique_stops.insert(it_bus->stops[i]->name);
        if (i > 0) {
            geo::Coordinates from = { it_bus->stops[i - 1]->latitude,it_bus->stops[i - 1]->longitude };
            geo::Coordinates to = { it_bus->stops[i]->latitude,it_bus->stops[i]->longitude };
            length += geo::ComputeDistance(from, to);
        }
    }

    out << "Bus " << bus_name << ": " << count_stops << " stops on route, " <<
        unique_stops.size() << " unique stops, " << length << " route length" << std::endl;
}
void TransportCatalogue::GetStopInfo(const std::string& stop_name, std::ostream& out) const {
    auto it_stop = info_stop.find(stop_name);//существует ли остановка

    if (it_stop == info_stop.end()) {
        out << "Stop " << stop_name << ": not found" << std::endl;
        return;
    }

    auto it_buses = stop_to_buses.find(stop_name);
    if (it_buses == stop_to_buses.end() || it_buses->second.empty()) {
        out << "Stop " << stop_name << ": no buses" << std::endl;
    }
    else {
        out << "Stop " << stop_name << ": buses";
        for (const auto& bus : it_buses->second) {
            out << " " << bus;
        }
        out << std::endl;
    }
}