#include "transport_catalogue.h"
#include "geo.h"

void TransportCatalogue::AddDistance(const Stop* ptr1, const Stop* ptr2, double dist) {
    if (ptr1 == nullptr || ptr2 == nullptr) {
        return;
    }
    if (FindStop(ptr1->name) == ptr1 && FindStop(ptr2->name) == ptr2) {
        distance_[{ptr1, ptr2}] = dist;
    }
}
int TransportCatalogue::GetDistance(const Stop* ptr1, const Stop* ptr2) const {
    auto it1 = distance_.find({ ptr1, ptr2 });
    if (it1 != distance_.end()) {
        return it1->second;
    }
    auto it2 = distance_.find({ ptr2, ptr1 });
    if (it2 != distance_.end()) {
        return it2->second;
    }
    return 0;
}

void TransportCatalogue::AddStop(const std::string& name, geo::Coordinates cord) {
    Stop stop;
    stop.name = name;
    stop.coordinates = cord;
    stops_.push_back(stop);
    auto it = &stops_.back();
    info_stop[it->name] = it;

}

const Stop* TransportCatalogue::FindStop(std::string_view name) const {
    std::string key(name);
    auto it = info_stop.find(key);
    if (it != info_stop.end()) {
        return it->second;
    }
    return nullptr;
}

void TransportCatalogue::AddBus(const std::string& name, const std::vector<std::string>& stops) {
    Bus bus;
    bus.name = name;

    for (const auto& stop : stops) {
        Stop* stop1 = const_cast<Stop*>(FindStop(stop));
        if (stop1) {
            bus.stops.push_back(stop1);
            stop_to_buses[stop].insert(bus.name);
        }
    }
    buses_.push_back(bus);
    info_bus[buses_.back().name] = &buses_.back();
}

const Bus* TransportCatalogue::FindBus(std::string_view name) const {
    std::string key(name);
    auto it = info_bus.find(key);
    if (it != info_bus.end()) {
        return it->second;
    }
    return nullptr;
}

std::optional<BusInfo> TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
    BusInfo info;
    const Bus* it_bus = FindBus(bus_name);
    //если не найдено
    if (!it_bus) {
        return std::nullopt;
    }

    info.count_stops = it_bus->stops.size();
    std::unordered_set<std::string> unique;
    double geo_lngth = 0.0;
    int l_lngth = 0;

    for (size_t i = 0; i < it_bus->stops.size(); i++) {
        unique.insert(it_bus->stops[i]->name);
        if (i > 0) {
            int road_lngth = GetDistance(it_bus->stops[i - 1], it_bus->stops[i]);
            if (road_lngth == 0) {
                road_lngth = GetDistance(it_bus->stops[i], it_bus->stops[i - 1]);
            }
            geo::Coordinates from = it_bus->stops[i - 1]->coordinates;
            geo::Coordinates to = it_bus->stops[i]->coordinates;
            geo_lngth += geo::ComputeDistance(from, to);

            if (road_lngth > 0) {
                l_lngth += road_lngth;
            }
            else {
                l_lngth += static_cast<int>(geo::ComputeDistance(from, to));
            }
        }
    }

    info.unique_stops = unique.size();
    info.length = l_lngth;

    if (geo_lngth > 0) {
        info.curvature = l_lngth / geo_lngth;
    }
    else {
        info.curvature = 1.0;
    }
    return info;
}
std::optional<const std::set<std::string>*> TransportCatalogue::GetStopInfo(std::string_view stop_name) const {

    std::string key(stop_name);
    auto it_stop = info_stop.find(key);//существует ли остановка
    if (it_stop == info_stop.end()) {
        return std::nullopt;
    }
    auto it_buses = stop_to_buses.find(key);
    if (it_buses != stop_to_buses.end()) {
        return &it_buses->second;
    }
    return nullptr;
}