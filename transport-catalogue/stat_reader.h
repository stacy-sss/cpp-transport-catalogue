#pragma once

#include <iosfwd>
#include <string_view>

#include "transport_catalogue.h"

namespace stat {
	void ParseAndPrintStat(const TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output);
}