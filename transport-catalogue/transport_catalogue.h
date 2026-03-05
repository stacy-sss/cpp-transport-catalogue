#pragma once
#include <iostream>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>
#include <unordered_set> 
#include <set>
#include <algorithm>
#include <string_view>
#include "geo.h"
struct Stop {
	std::string name;
	geo::Coordinates coordinates;
};
struct Bus {
	std::string name;
	std::vector<Stop*> stops;
};
struct BusInfo {
	int count_stops = 0;
	int unique_stops = 0;
	double length = 0.0;
	bool found = false;
};
struct StopInfo {
	bool found = false;
	std::set<std::string> buses;
};
class TransportCatalogue {
public:

	void AddStop(const std::string& name, geo::Coordinates cord);
	void AddBus(const std::string& name, const std::vector<std::string>& stops);

	const Stop* FindStop(std::string_view name) const;
	const Bus* FindBus(std::string_view name) const;

	BusInfo GetBusInfo(std::string_view bus_name) const;
	StopInfo GetStopInfo(std::string_view stop_name) const;

private:
	std::deque<Stop> stops_;//имя,широта,долгота
	std::deque<Bus> buses_;//имя,ссылка на остановки
	std::unordered_map<std::string, Stop*> info_stop;//имя остановки и ссылка на нее
	std::unordered_map<std::string, Bus*> info_bus;//имя автобуса, ссылка на маршрут

	std::unordered_map<std::string, std::set<std::string>> stop_to_buses;//список автобусов для остановки
};