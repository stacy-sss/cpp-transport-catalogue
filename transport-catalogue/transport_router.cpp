#include "transport_router.h"
#include "geo.h"

namespace transport_catalogue {

    void TransportRouter::BuildGraph() {
        const auto& stops = catalog_.GetStops();
        const auto& buses = catalog_.GetBuses();

        // Назначаем номера вершинам
        for (size_t i = 0; i < stops.size(); ++i) {
            vertex_ids_[stops[i].name] = i;
            vertex_stops_.push_back(&stops[i]);
        }

        // Создаём граф заново с правильным размером
        graph_ = graph::DirectedWeightedGraph<double>(stops.size());

        // Добавляем рёбра
        for (const auto& bus : buses) {//проходимся по маршрутам автобусов
            for (size_t start = 0; start < bus.stops.size(); ++start) {//откуда едем
                double cumulative = 0.0;//общее время
                for (size_t end = start + 1; end < bus.stops.size(); ++end) {//куда едем
                    const auto* a = bus.stops[end - 1];
                    const auto* b = bus.stops[end];

                    double d = catalog_.GetDistance(a, b);
                    if (d == 0) d = catalog_.GetDistance(b, a);
                    if (d == 0) d = geo::ComputeDistance(a->coordinates, b->coordinates);

                    double travel_time = d / (settings_.bus_velocity * 1000.0 / 60.0);
                    cumulative += travel_time;

                    auto eid = graph_.AddEdge({
                        vertex_ids_[bus.stops[start]->name],//откуда
                        vertex_ids_[b->name],//куда
                        cumulative + settings_.bus_wait_time//время пути + ожидание
                        });

                    if (edge_meta_.size() <= eid) {
                        edge_meta_.resize(eid + 1);
                    }
                    edge_meta_[eid] = { bus.name, start, end };//добавляем данные ребра
                }
            }
        }

    }

    std::optional<TransportRouter::RouteResult> TransportRouter::FindRoute(
        std::string_view from, std::string_view to) const {

        auto from_it = vertex_ids_.find(std::string(from));
        auto to_it = vertex_ids_.find(std::string(to));

        if (from_it == vertex_ids_.end() || to_it == vertex_ids_.end()) {
            return std::nullopt;
        }

        auto result = router_->BuildRoute(from_it->second, to_it->second);

        if (!result) {
            return std::nullopt;
        }

        RouteResult route_result;
        route_result.total_time = result->weight;

        for (auto eid : result->edges) {
            const auto& meta = edge_meta_[eid];
            const auto& edge = graph_.GetEdge(eid);

            // Wait
            RouteItem wait_item;
            wait_item.type = RouteItem::Type::WAIT;
            wait_item.stop_name = vertex_stops_[edge.from]->name;
            wait_item.time = static_cast<double>(settings_.bus_wait_time);
            route_result.items.push_back(wait_item);

            // Bus
            RouteItem bus_item;
            bus_item.type = RouteItem::Type::BUS;
            bus_item.bus_name = meta.bus_name;
            bus_item.span_count = static_cast<int>(meta.end_idx - meta.start_idx);
            bus_item.time = edge.weight - settings_.bus_wait_time;
            route_result.items.push_back(bus_item);
        }

        return route_result;
    }

} // namespace transport_catalogue