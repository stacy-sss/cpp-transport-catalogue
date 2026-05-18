#pragma once
#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"

#include <optional>
#include <unordered_map>
#include <vector>
#include <string>

namespace transport_catalogue {

    class TransportRouter {
    public:
        struct RoutingSettings {
            int bus_wait_time = 0;//врем€ ожидани€ автобуса
            double bus_velocity = 0.0;//скорость автобуса
        };

        // Ёлемент маршрута
        struct RouteItem {
            enum class Type { WAIT, BUS };
            Type type;
            std::string stop_name;
            std::string bus_name;
            int span_count = 0;
            double time = 0.0;
        };

        // –езультат поиска пути
        struct RouteResult {
            double total_time;
            std::vector<RouteItem> items;
        };

        TransportRouter(const TransportCatalogue& catalog, const RoutingSettings& settings)
            : catalog_(catalog)
            , settings_(settings)
            , graph_(0) {
            BuildGraph();
            router_.emplace(graph_);
        }

        std::optional<RouteResult> FindRoute(std::string_view from, std::string_view to) const;

    private:
        const TransportCatalogue& catalog_;
        RoutingSettings settings_;



        struct EdgeMeta {
            std::string bus_name;
            size_t start_idx;   // индекс начальной остановки в маршруте автобуса
            size_t end_idx;     // индекс конечной остановки в маршруте автобуса
        };
        std::vector<EdgeMeta> edge_meta_;//маршрут автобуса
        graph::DirectedWeightedGraph<double> graph_;//граф, содержит все остановки (вершины) и возможные маршруты (ребра) с временем
        std::optional<graph::Router<double>> router_;//кратчайший путь в графе
        std::unordered_map<std::string, graph::VertexId> vertex_ids_;//св€зывает название остановки с номером вершины
        std::vector<const domain::Stop*> vertex_stops_;//по номеру вершины получить указатель на остановку

        void BuildGraph();//строим граф после парсинга данных
    };

} // namespace transport_catalogue