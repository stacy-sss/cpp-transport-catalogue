#pragma once
#include "json.h"
#include "json_builder.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "graph.h"
#include "router.h"
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
        void ParseRoutingSettings(const json::Dict& settings);
        void ApplyDistance();

        void RenderRequests(const json::Dict& settings);
        map_renderer::Color ParseColor(const json::Node& node);

        json::Array StatRequests(const json::Array& stat_requests);
        json::Dict StopRequests(const json::Dict& req);
        json::Dict BusRequests(const json::Dict& req);
        json::Dict MapRequest(const json::Dict& req);
        json::Dict RouteRequest(const json::Dict& req);

        struct RoutingSettings {
            int bus_wait_time = 0;//время ожидания автобуса
            double bus_velocity = 0.0;//скорость автобуса
        };
        std::optional<RoutingSettings> routing_settings_;


        struct EdgeMeta {
            std::string bus_name;
            size_t start_idx;   // индекс начальной остановки в маршруте автобуса
            size_t end_idx;     // индекс конечной остановки в маршруте автобуса
        };
        std::vector<EdgeMeta> edge_meta_;//маршрут автобуса
        std::optional<graph::DirectedWeightedGraph<double>> route_graph_;//граф, содержит все остановки (вершины) и возможные маршруты (ребра) с временем
        std::optional<graph::Router<double>> route_router_;//кратчайший путь в графе
        std::unordered_map<std::string, graph::VertexId> vertex_ids_;//связывает название остановки с номером вершины
        std::vector<const domain::Stop*> vertex_stops_;//по номеру вершины получить указатель на остановку

        void BuildRouteGraph();//строим граф после парсинга данных
    };
}//json_reader