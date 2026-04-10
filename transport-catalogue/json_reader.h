#pragma once
#include "json.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include <vector>
#include <string>

namespace json_reader {
    class JsonReader {
    public:

        explicit JsonReader(transport_catalogue::TransportCatalogue& catalog);
        json::Document Process(const json::Node& input_info);
        const map_renderer::ColorSettings& GetRenderSettings() const;
    private:
        transport_catalogue::TransportCatalogue& catalog_;
        std::optional<map_renderer::ColorSettings> render_settings_;

        struct Distance {
            std::string from;
            std::string to;
            int dist = 0;
        };
        std::vector<Distance> info_distance_;
        void ParseBaseRequests(const json::Array& base_requests);
        void ParseStop(const json::Dict& stops);
        void ParseBus(const json::Dict& buses);
        void ApplyDistance();

        void RenderRequests(const json::Dict& settings);
        map_renderer::Color ParseColor(const json::Node& node);

        json::Array StatRequests(const json::Array& stat_requests);
        json::Dict StopRequests(const json::Dict& req);
        json::Dict BusRequests(const json::Dict& req);
        json::Dict MapRequest(const json::Dict& req);
    };
}//json_reader