#pragma once
#include <iostream>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>
#include <unordered_set> 
#include <set>
#include <algorithm>

struct Stop {
	std::string name;
	double latitude;//широта
	double longitude;//долгота
};
struct Bus {
	std::string name;
	std::vector<Stop*> stops;
};
class TransportCatalogue {
public:

	void AddStop(std::string name, double latitude, double longitude);
	void AddBus(std::string name, std::vector<std::string>& stops);

	const Stop* FindStop(const std::string& name) const;
	const Bus* FindBus(const std::string& name) const;

	void GetBusInfo(const std::string& bus_name, std::ostream& out) const;
	void GetStopInfo(const std::string& stop_name, std::ostream& out) const;

private:
	std::deque<Stop> stops_;//имя,широта,долгота
	std::deque<Bus> buses_;//имя,ссылка на остановки
	std::unordered_map<std::string, Stop*> info_stop;//имя остановки и ссылка на нее
	std::unordered_map<std::string, Bus*> info_bus;//имя автобуса, ссылка на маршрут

	std::unordered_map<std::string, std::set<std::string>> stop_to_buses;//список автобусов для остановки
};