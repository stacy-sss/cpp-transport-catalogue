#include "json_reader.h"

namespace json_reader {

    JsonReader::JsonReader(transport_catalogue::TransportCatalogue& catalog) : catalog_(catalog) {}

    json::Document JsonReader::Process(const json::Node& input_info) {
        const auto& map = input_info.AsMap();
        if (map.count("base_requests")) {
            ParseBaseRequests(map.at("base_requests").AsArray());
        }
        if (map.count("render_settings")) {
            RenderRequests(map.at("render_settings").AsMap());
        }
        if (map.count("routing_settings")) {
            ParseRoutingSettings(map.at("routing_settings").AsMap());
            BuildRouteGraph();
        }
        json::Array ar;
        if (map.count("stat_requests")) {
            ar = StatRequests(map.at("stat_requests").AsArray());
        }
        return json::Document(json::Node(std::move(ar)));
    }

    const map_renderer::ColorSettings& JsonReader::GetRenderSettings() const {
        return *render_settings_;
    }

    void JsonReader::ParseBaseRequests(const json::Array& base_requests) {
        for (const auto& req : base_requests) {
            const auto& req_map = req.AsMap();
            const std::string& type = req_map.at("type").AsString();
            if (type == "Stop") {
                ParseStop(req_map);
            }
        }
        ApplyDistance();
        for (const auto& req : base_requests) {
            const auto& req_map = req.AsMap();
            const std::string& type = req_map.at("type").AsString();
            if (type == "Bus") {
                ParseBus(req_map);
            }
        }
    }

    void JsonReader::RenderRequests(const json::Dict& settings) {
        map_renderer::ColorSettings rs;
        rs.width = settings.at("width").AsDouble();
        rs.height = settings.at("height").AsDouble();
        rs.padding = settings.at("padding").AsDouble();
        rs.line_width = settings.at("line_width").AsDouble();
        rs.stop_radius = settings.at("stop_radius").AsDouble();
        rs.bus_label_font_size = settings.at("bus_label_font_size").AsInt();

        const auto& bus_offset = settings.at("bus_label_offset").AsArray();
        rs.bus_label_offset = { bus_offset[0].AsDouble(), bus_offset[1].AsDouble() };

        rs.stop_label_font_size = settings.at("stop_label_font_size").AsInt();

        const auto& stop_offset = settings.at("stop_label_offset").AsArray();
        rs.stop_label_offset = { stop_offset[0].AsDouble(), stop_offset[1].AsDouble() };

        rs.underlayer_color = ParseColor(settings.at("underlayer_color"));
        rs.underlayer_width = settings.at("underlayer_width").AsDouble();

        const auto& palette = settings.at("color_palette").AsArray();
        for (const auto& color_node : palette) {
            rs.color_palette.push_back(ParseColor(color_node));
        }

        render_settings_ = std::move(rs);
    }

    map_renderer::Color JsonReader::ParseColor(const json::Node& node) {
        if (node.IsString()) {
            return map_renderer::Color{ node.AsString() };
        }

        const auto& arr = node.AsArray();

        if (arr.size() == 4) {
            std::ostringstream oss;
            oss << arr[3].AsDouble();
            std::string rgba_str = "rgba(" +
                std::to_string(arr[0].AsInt()) + "," +
                std::to_string(arr[1].AsInt()) + "," +
                std::to_string(arr[2].AsInt()) + "," +
                oss.str() + ")";
            return map_renderer::Color{ rgba_str };
        }

        std::vector<int> rgb;
        for (size_t i = 0; i < arr.size(); ++i) {
            rgb.push_back(arr[i].AsInt());
        }
        return map_renderer::Color{ rgb };
    }

    void JsonReader::ParseStop(const json::Dict& stops) {
        std::string name = stops.at("name").AsString();
        double latitude = stops.at("latitude").AsDouble();
        double longitude = stops.at("longitude").AsDouble();

        catalog_.AddStop(name, { latitude, longitude });
        if (stops.count("road_distances")) {
            const auto& dist = stops.at("road_distances").AsMap();
            for (const auto& [to_name, distance] : dist) {
                info_distance_.push_back({ name, to_name, distance.AsInt() });
            }
        }
    }

    void JsonReader::ParseBus(const json::Dict& buses) {
        std::string name = buses.at("name").AsString();
        bool is_roundtrip = buses.at("is_roundtrip").AsBool();

        std::vector<std::string> stops;
        const auto& stops_array = buses.at("stops").AsArray();
        for (const auto& stop_node : stops_array) {
            stops.push_back(stop_node.AsString());
        }

        std::vector<std::string> full_route = stops;
        if (!is_roundtrip) {
            for (int i = static_cast<int>(stops.size()) - 2; i >= 0; --i) {
                full_route.push_back(stops[i]);
            }
        }

        catalog_.AddBus(name, full_route, is_roundtrip);
    }

    void JsonReader::ApplyDistance() {
        for (const auto& info : info_distance_) {
            const domain::Stop* from = catalog_.FindStop(info.from);
            const domain::Stop* to = catalog_.FindStop(info.to);

            if (from && to) {
                catalog_.AddDistance(from, to, info.dist);
            }
        }
        info_distance_.clear();
    }

    json::Array JsonReader::StatRequests(const json::Array& stat_requests) {
        json::Array responses;

        for (const auto& req : stat_requests) {
            const auto& req_map = req.AsMap();
            const std::string& type = req_map.at("type").AsString();

            if (type == "Bus") {
                responses.push_back(json::Node(BusRequests(req_map)));
            }
            else if (type == "Stop") {
                responses.push_back(json::Node(StopRequests(req_map)));
            }
            else if (type == "Map") {
                responses.push_back(json::Node(MapRequest(req_map)));
            }
            else if (type == "Route") {
                responses.push_back(json::Node(RouteRequest(req_map)));
            }
        }

        return responses;
    }

    json::Dict JsonReader::MapRequest(const json::Dict& req) {
        int request_id = req.at("id").AsInt();

        if (!render_settings_) {
            json::Builder builder;
            builder.StartDict()
                .Key("request_id").Value(request_id)
                .Key("error_message").Value(json::Node(std::string("render settings not found")))
                .EndDict();
            return builder.Build().AsMap();
        }

        request::RequestHandler handler(catalog_, *render_settings_);
        svg::Document svg_doc = handler.RenderMap();

        std::ostringstream svg_stream;
        svg_doc.Render(svg_stream);

        json::Builder builder;
        builder.StartDict()
            .Key("map").Value(json::Node(std::string(svg_stream.str())))
            .Key("request_id").Value(request_id)
            .EndDict();
        return builder.Build().AsMap();
    }

    json::Dict JsonReader::BusRequests(const json::Dict& req) {
        int request_id = req.at("id").AsInt();
        std::string bus_name = req.at("name").AsString();

        request::RequestHandler handler(catalog_, *render_settings_);
        auto bus_info = handler.GetBusStat(bus_name);

        json::Builder builder;

        if (!bus_info) {
            builder.StartDict()
                .Key("request_id").Value(request_id)
                .Key("error_message").Value(json::Node(std::string("not found")))
                .EndDict();
        }
        else {
            builder.StartDict()
                .Key("curvature").Value(bus_info->curvature)
                .Key("request_id").Value(request_id)
                .Key("route_length").Value(static_cast<double>(bus_info->length))
                .Key("stop_count").Value(bus_info->count_stops)
                .Key("unique_stop_count").Value(bus_info->unique_stops)
                .EndDict();
        }

        return builder.Build().AsMap();
    }

    json::Dict JsonReader::StopRequests(const json::Dict& req) {
        int request_id = req.at("id").AsInt();
        std::string name = req.at("name").AsString();

        auto stop_info_opt = catalog_.GetStopInfo(name);

        if (!stop_info_opt) {
            json::Builder builder;
            builder.StartDict()
                .Key("request_id").Value(request_id)
                .Key("error_message").Value(json::Node(std::string("not found")))
                .EndDict();
            return builder.Build().AsMap();
        }

        json::Array buses_array;
        auto buses_ptr = *stop_info_opt;
        if (buses_ptr != nullptr) {
            for (const auto& bus : *buses_ptr) {
                buses_array.push_back(json::Node(std::string(bus)));
            }
        }

        json::Builder builder;
        builder.StartDict()
            .Key("request_id").Value(request_id)
            .Key("buses").Value(buses_array)
            .EndDict();
        return builder.Build().AsMap();
    }

    json::Dict JsonReader::RouteRequest(const json::Dict& req) {
        int request_id = req.at("id").AsInt();
        std::string from = req.at("from").AsString();
        std::string to = req.at("to").AsString();
        //проверяем есть ли остановки from и to
        if (!catalog_.FindStop(from) || !catalog_.FindStop(to)) {
            json::Builder builder;
            builder.StartDict()
                .Key("request_id").Value(request_id)
                .Key("error_message").Value(json::Node(std::string("not found")))
                .EndDict();
            return builder.Build().AsMap();
        }
        if (!route_router_) {
            json::Builder builder;
            builder.StartDict()
                .Key("request_id").Value(request_id)
                .Key("error_message").Value(json::Node(std::string("routing settings not found")))
                .EndDict();
            return builder.Build().AsMap();
        }
        auto result = route_router_->BuildRoute(vertex_ids_[from], vertex_ids_[to]);//ищем путь
        if (!result) {
            json::Builder builder;
            builder.StartDict()
                .Key("request_id").Value(request_id)
                .Key("error_message").Value(json::Node(std::string("not found")))
                .EndDict();
            return builder.Build().AsMap();
        }
        //вычисление времени
        double total_time = result->weight;

        json::Builder builder;
        auto dict = builder.StartDict()
            .Key("request_id").Value(request_id)
            .Key("total_time").Value(total_time)
            .Key("items").StartArray();
        for (auto eid : result->edges) {
            const auto& meta = edge_meta_[eid];//данные (номер автобуса, начало и конец маршрута)
            const auto& edge = route_graph_->GetEdge(eid);//откуда, куда, время

            // Wait
            dict.StartDict()
                .Key("type").Value(json::Node(std::string("Wait")))
                .Key("stop_name").Value(json::Node(std::string(vertex_stops_[edge.from]->name)))
                .Key("time").Value(routing_settings_->bus_wait_time)
                .EndDict();

            //Bus
            int span_count = static_cast<int>(meta.end_idx - meta.start_idx);
            double bus_time = edge.weight - routing_settings_->bus_wait_time;

            dict.StartDict()
                .Key("type").Value(json::Node(std::string("Bus")))
                .Key("bus").Value(json::Node(std::string(meta.bus_name)))
                .Key("span_count").Value(span_count)
                .Key("time").Value(bus_time)
                .EndDict();
        }
        return dict.EndArray().EndDict().Build().AsMap();

    }

    void JsonReader::ParseRoutingSettings(const json::Dict& settings) {
        RoutingSettings rs;
        rs.bus_wait_time = settings.at("bus_wait_time").AsInt();
        rs.bus_velocity = settings.at("bus_velocity").AsDouble();
        routing_settings_ = std::move(rs);
    }

    void JsonReader::BuildRouteGraph() {
        const auto& stops = catalog_.GetStops();
        const auto& buses = catalog_.GetBuses();

        for (size_t i = 0; i < stops.size(); i++) {
            vertex_ids_[stops[i].name] = i;
            vertex_stops_.push_back(&stops[i]);
        }

        route_graph_.emplace(stops.size());

        for (const auto& bus : buses) {//проходимся по маршрутам автобусов
            for (size_t start = 0; start < bus.stops.size(); ++start) {//откуда едем
                double cumulative = 0.0;//общее время
                for (size_t end = start + 1; end < bus.stops.size(); ++end) {//куда едем
                    const auto* a = bus.stops[end - 1];
                    const auto* b = bus.stops[end];

                    double d = catalog_.GetDistance(a, b);
                    if (d == 0) {
                        d = catalog_.GetDistance(b, a);
                    }
                    if (d == 0) {
                        d = geo::ComputeDistance(a->coordinates, b->coordinates);
                    }
                    double travel_time = d / (routing_settings_->bus_velocity * 1000.0 / 60.0);
                    cumulative += travel_time;

                    auto eid = route_graph_->AddEdge({
                        vertex_ids_[bus.stops[start]->name],//откуда
                        vertex_ids_[b->name],//куда
                        cumulative + routing_settings_->bus_wait_time//время пути + ожидание
                    });
                    if (edge_meta_.size() <= eid) {
                        edge_meta_.resize(eid + 1);
                    }
                    edge_meta_[eid] = { bus.name, start, end };//добавляем данные ребра

                }
            }
        }
        route_router_.emplace(*route_graph_);
    }

} // namespace json_reader