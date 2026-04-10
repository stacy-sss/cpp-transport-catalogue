#include "map_renderer.h"

namespace map_renderer {

    inline const double EPSILON = 1e-6;
    bool IsZero(double value) {
        return std::abs(value) < EPSILON;
    }

    template <typename PointInputIt>
    SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end,
        double max_width, double max_height, double padding)
        : padding_(padding) 
    {
     
        if (points_begin == points_end) {
            return;
        }

        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        
        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        }
        else if (width_zoom) {
           
            zoom_coeff_ = *width_zoom;
        }
        else if (height_zoom) {
            
            zoom_coeff_ = *height_zoom;
        }
    }

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }


    std::string Color::ToString() const {
        if (std::holds_alternative<std::string>(value)) {
            return std::get<std::string>(value);
        }

        const auto& rgb = std::get<std::vector<int>>(value);
        if (rgb.size() == 3) {
            return "rgb(" + std::to_string(rgb[0]) + "," +
                std::to_string(rgb[1]) + "," +
                std::to_string(rgb[2]) + ")";
        }
        return "black";
    }



    MapRenderer::MapRenderer(const ColorSettings& settings, const transport_catalogue::TransportCatalogue& catalog) : settings_(settings),
        catalog_(catalog) {}

    svg::Document MapRenderer::Render() const {
        svg::Document doc;

        auto coords = GetAllCoordinates();
        if (coords.empty()) {
            return doc;
        }
        SphereProjector proj(coords.begin(), coords.end(), settings_.width,
            settings_.height, settings_.padding);

        RenderBusLines(doc, proj);
        RenderBusLabels(doc, proj);
        RenderStopCircles(doc, proj);
        RenderStopLabels(doc, proj);

        return doc;
    }

    std::vector<geo::Coordinates> MapRenderer::GetAllCoordinates() const {
        std::vector<geo::Coordinates> coords;

        for (const auto& bus : catalog_.GetBuses()) {
            for (const auto* stop : bus.stops) {
                coords.push_back(stop->coordinates);
            }
        }

        return coords;
    }
    std::vector<const domain::Bus*> MapRenderer::GetSortedBuses() const {
        std::vector<const domain::Bus*> buses;

        for (const auto& bus : catalog_.GetBuses()) {
            if (!bus.stops.empty()) {
                buses.push_back(&bus);
            }
        }

        std::sort(buses.begin(), buses.end(),
            [](const domain::Bus* lhs, const domain::Bus* rhs) {
                return lhs->name < rhs->name;
            });

        return buses;
    }

    Color MapRenderer::GetBusColor(size_t index) const {
        if (settings_.color_palette.empty()) {
            return Color{ "black" };
        }
        return settings_.color_palette[index % settings_.color_palette.size()];
    }

    void MapRenderer::RenderBusLines(svg::Document& doc, const SphereProjector& proj) const {
        auto buses = GetSortedBuses();
        for (size_t i = 0; i < buses.size(); i++) {
            const auto* bus = buses[i];
            svg::Polyline polyline;
            for (const auto* stop : bus->stops) {
                polyline.AddPoint(proj(stop->coordinates));
            }
            Color color = GetBusColor(i);
            polyline.SetFillColor("none")
                .SetStrokeColor(color.ToString())
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
                .SetStrokeWidth(settings_.line_width);
            doc.Add(polyline);
        }
    }
    void MapRenderer::RenderBusLabels(svg::Document& doc, const SphereProjector& proj) const {
        auto buses = GetSortedBuses();
        std::string underlayer_color_str = settings_.underlayer_color.ToString();

        for (size_t i = 0; i < buses.size(); ++i) {
            const auto* bus = buses[i];

            if (bus->stops.empty()) {
                continue;
            }

            Color color = GetBusColor(i);
            std::string color_str = color.ToString();

            const auto* first_stop = bus->stops.front();
            svg::Point first_point = proj(first_stop->coordinates);

            AddBusLabel(doc, first_point, bus->name, color_str, underlayer_color_str);

            if (!bus->is_roundtrip) {
                size_t original_size = (bus->stops.size() + 1) / 2;
                const auto* last_stop = bus->stops[original_size - 1];

                if (last_stop != first_stop) {
                    svg::Point last_point = proj(last_stop->coordinates);
                    AddBusLabel(doc, last_point, bus->name, color_str, underlayer_color_str);
                }
            }
        }
    }
    void MapRenderer::RenderStopCircles(svg::Document& doc, const SphereProjector& proj) const {
        auto stops = GetSortedStops();

        for (const auto* stop : stops) {
            doc.Add(svg::Circle()
                .SetCenter(proj(stop->coordinates))
                .SetRadius(settings_.stop_radius)
                .SetFillColor("white"));
        }
    }
    void MapRenderer::RenderStopLabels(svg::Document& doc, const SphereProjector& proj) const {
        auto stops = GetSortedStops();
        std::string underlayer_color_str = settings_.underlayer_color.ToString();

        for (const auto* stop : stops) {
            svg::Point point = proj(stop->coordinates);

            // Подложка
            doc.Add(svg::Text()
                .SetPosition(point)
                .SetOffset({ settings_.stop_label_offset[0], settings_.stop_label_offset[1] })
                .SetFontSize(settings_.stop_label_font_size)
                .SetFontFamily("Verdana")
                .SetData(stop->name)
                .SetFillColor(underlayer_color_str)
                .SetStrokeColor(underlayer_color_str)
                .SetStrokeWidth(settings_.underlayer_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND));

            // Текст (без font-weight!)
            doc.Add(svg::Text()
                .SetPosition(point)
                .SetOffset({ settings_.stop_label_offset[0], settings_.stop_label_offset[1] })
                .SetFontSize(settings_.stop_label_font_size)
                .SetFontFamily("Verdana")
                .SetData(stop->name)
                .SetFillColor("black"));
        }
    }
    std::vector<const domain::Stop*> MapRenderer::GetSortedStops() const {
        std::vector<const domain::Stop*> stops;
        std::unordered_set<std::string> seen;

        for (const auto& bus : catalog_.GetBuses()) {
            for (const auto* stop : bus.stops) {
                if (seen.insert(stop->name).second) {
                    stops.push_back(stop);
                }
            }
        }

        std::sort(stops.begin(), stops.end(),
            [](const domain::Stop* lhs, const domain::Stop* rhs) {
                return lhs->name < rhs->name;
            });

        return stops;
    }
    void MapRenderer::AddBusLabel(svg::Document& doc, svg::Point point,
        const std::string& name,
        const std::string& color_str,
        const std::string& underlayer_color_str) const {
        // Подложка
        doc.Add(svg::Text()
            .SetPosition(point)
            .SetOffset({ settings_.bus_label_offset[0], settings_.bus_label_offset[1] })
            .SetFontSize(settings_.bus_label_font_size)
            .SetFontFamily("Verdana")
            .SetFontWeight("bold")
            .SetData(name)
            .SetFillColor(underlayer_color_str)
            .SetStrokeColor(underlayer_color_str)
            .SetStrokeWidth(settings_.underlayer_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND));

        // Текст
        doc.Add(svg::Text()
            .SetPosition(point)
            .SetOffset({ settings_.bus_label_offset[0], settings_.bus_label_offset[1] })
            .SetFontSize(settings_.bus_label_font_size)
            .SetFontFamily("Verdana")
            .SetFontWeight("bold")
            .SetData(name)
            .SetFillColor(color_str));
    }
}//namespace