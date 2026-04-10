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
#include <optional>
#include "geo.h"
#include "domain.h"

namespace transport_catalogue {
	struct Hasher {
		size_t operator()(const std::pair<const domain::Stop*, const domain::Stop*>& p) const {
			auto h1 = std::hash<const domain::Stop*>()(p.first);
			auto h2 = std::hash<const domain::Stop*>()(p.second);
			return h1 ^ h2;
		}
	};
	class TransportCatalogue {
	public:

		void AddDistance(const domain::Stop* ptr1, const domain::Stop* ptr2, double dist);
		int GetDistance(const domain::Stop* ptr1, const domain::Stop* ptr2) const;

		void AddStop(const std::string& name, geo::Coordinates cord);
		void AddBus(const std::string& name, const std::vector<std::string>& stops, bool is_roundtrip);

		const domain::Stop* FindStop(std::string_view name) const;
		const domain::Bus* FindBus(std::string_view name) const;

		std::optional<domain::BusInfo> GetBusInfo(std::string_view bus_name) const;
		std::optional<const std::set<std::string>*> GetStopInfo(std::string_view stop_name) const;

		const std::deque<domain::Bus>& GetBuses() const;
		const std::deque<domain::Stop>& GetStops() const;

	private:
		std::deque<domain::Stop> stops_;//имя,широта,долгота
		std::deque<domain::Bus> buses_;//имя,ссылка на остановки
		std::unordered_map<std::string, domain::Stop*> info_stop;//имя остановки и ссылка на нее
		std::unordered_map<std::string, domain::Bus*> info_bus;//имя автобуса, ссылка на маршрут

		std::unordered_map<std::string, std::set<std::string>> stop_to_buses;//список автобусов для остановки
		std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, int, Hasher> distance_;
	};
}