#pragma once
#include "svg.h"
#include "geo.h"
#include "domain.h"
#include "transport_catalogue.h"

#include <vector>
#include <string>
#include <variant>
#include <optional>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace map_renderer {
    struct Color {
        std::variant<std::string, std::vector<int>> value;
        std::string ToString() const;
    };

    struct ColorSettings {
        double width = 0.0;
        double height = 0.0;
        double padding = 0.0;
        double line_width = 0.0;
        double stop_radius = 0.0;
        int bus_label_font_size = 0;
        std::vector<double> bus_label_offset;
        int stop_label_font_size = 0;
        std::vector<double> stop_label_offset;
        double underlayer_width = 0.0;
        Color underlayer_color;
        std::vector<Color> color_palette;
    };
    //перевод градусов в пиксели
    class SphereProjector {
    public:

        template <typename PointInputIt>
        SphereProjector(PointInputIt points_begin, PointInputIt points_end,
            double max_width, double max_height, double padding);

        svg::Point operator()(geo::Coordinates coords) const;

    private:
        double padding_;
        double min_lon_ = 0;
        double max_lat_ = 0;
        double zoom_coeff_ = 0;
    };

    class MapRenderer {
    public:

        MapRenderer(const ColorSettings& settings, const transport_catalogue::TransportCatalogue& catalog);

        svg::Document Render() const;

    private:
        const ColorSettings& settings_;
        const transport_catalogue::TransportCatalogue& catalog_;

        std::vector<geo::Coordinates> GetAllCoordinates() const;//все координаты
        std::vector<const domain::Bus*> GetSortedBuses() const;//сортировка списка маршрутов
        Color GetBusColor(size_t index) const;//определить цвет маршрута
        std::vector<const domain::Stop*> GetSortedStops() const;
        void AddBusLabel(svg::Document& doc, svg::Point point,
            const std::string& name,
            const std::string& color_str,
            const std::string& underlayer_color_str) const;
        void RenderBusLines(svg::Document& doc, const SphereProjector& proj) const;//отрисовка линий маршрута
        void RenderBusLabels(svg::Document& doc, const SphereProjector& proj) const;//названия маршрутов на карте
        void RenderStopCircles(svg::Document& doc, const SphereProjector& proj) const;//нарисовать кружки остановок
        void RenderStopLabels(svg::Document& doc, const SphereProjector& proj) const;//названия остановок
    };
}