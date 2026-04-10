#include "request_handler.h"

namespace request {
    RequestHandler::RequestHandler(const transport_catalogue::TransportCatalogue& db, const map_renderer::ColorSettings& renderer) : db_(db), renderer_(renderer, db) {}

    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<domain::BusInfo> RequestHandler::GetBusStat(std::string_view bus_name) const {
        return db_.GetBusInfo(bus_name);
    }

    // Возвращает маршруты, проходящие через
    std::optional<domain::StopInfo> RequestHandler::GetBusesByStop(std::string_view stop_name) const {
        auto result = db_.GetStopInfo(stop_name);
        if (!result) {
            return std::nullopt;
        }
        domain::StopInfo info;
        info.buses = *result;
        return info;
    }
    svg::Document RequestHandler::RenderMap() const {
        return renderer_.Render();
    }
}