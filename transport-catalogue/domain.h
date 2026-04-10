#pragma once
#include <string>
#include <vector>
#include <set>
#include <optional>
#include "geo.h"

namespace domain {
	struct Stop {
		std::string name;
		geo::Coordinates coordinates;
	};
	struct Bus {
		std::string name;
		std::vector<Stop*> stops;
		bool is_roundtrip = false;
	};
	struct BusInfo {
		int count_stops = 0;
		int unique_stops = 0;
		double length = 0.0;
		double curvature = 0.0;
	};
	struct StopInfo {
		std::optional<const std::set<std::string>*> buses = nullptr;
	};
}//domain