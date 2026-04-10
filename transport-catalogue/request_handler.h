#pragma once
#include "transport_catalogue.h"
#include "domain.h"
#include "map_renderer.h"
#include <optional>
#include <string_view>

namespace request {
    class RequestHandler {
    public:
        // MapRenderer понадобится в следующей части итогового проекта
        explicit RequestHandler(const transport_catalogue::TransportCatalogue& db, const map_renderer::ColorSettings& renderer);

        // Возвращает информацию о маршруте (запрос Bus)
        std::optional<domain::BusInfo> GetBusStat(std::string_view bus_name) const;

        // Возвращает маршруты, проходящие через
        std::optional<domain::StopInfo> GetBusesByStop(std::string_view stop_name) const;


        svg::Document RenderMap() const;

    private:
        // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
        const transport_catalogue::TransportCatalogue& db_;
        const map_renderer::MapRenderer renderer_;
    };
}//request

