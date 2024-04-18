#include "json_reader.h"


namespace json_reader {
    using namespace std::literals;
    using namespace json;
    using namespace domain;
    using namespace map_renderer;
    using namespace transport_catalogue;

BusBaseRequest JsonReader::ParseBus(const Dict& dict) {
    BusBaseRequest request;
    request.name = dict.at("name"s).AsString();
    for (const auto &node:dict.at("stops"s).AsArray()) {
        request.stops.push_back(node.AsString());
    }
    request.is_roundtrip = dict.at("is_roundtrip"s).AsBool();   
    return request;
}

StopBaseRequest JsonReader::ParseStop(const Dict& dict) {
    StopBaseRequest request;
    request.name = dict.at("name"s).AsString();
    request.lat = dict.at("latitude"s).AsDouble();
    request.lng = dict.at("longitude"s).AsDouble();
    for (const auto &node : dict.at("road_distances"s).AsDict()) {
        request.road_distances.emplace_back(node.first, node.second.AsInt());
    }
    return request;
}

StatRequest JsonReader::ParseStat(const Dict& dict) {
    StatRequest request;
    request.id = dict.at("id"s).AsInt();
    request.type = dict.at("type"s).AsString();
    request.name = (dict.count("name"s) ? dict.at("name"s).AsString() : "without name for 'map'-request"s);
    request.to = (dict.count("to"s) ? dict.at("to"s).AsString() : "without 'to'"s);  
    request.from = (dict.count("from"s) ? dict.at("from"s).AsString() : "without 'from'"s); 
    return request;
}

svg::Color JsonReader::ParseColor(const json::Node& node) {
	svg::Color color;
    if (node.IsArray()) {
		if (node.AsArray().size() == 3) {
			svg::Rgb rgb;
			rgb.red = static_cast<uint8_t>(node.AsArray()[0].AsInt());
			rgb.green = static_cast<uint8_t>(node.AsArray()[1].AsInt());
			rgb.blue = static_cast<uint8_t>(node.AsArray()[2].AsInt());
			color = rgb;
		} else if (node.AsArray().size() == 4) {
			svg::Rgba rgba;
			rgba.red = static_cast<uint8_t>(node.AsArray()[0].AsInt());
			rgba.green = static_cast<uint8_t>(node.AsArray()[1].AsInt());
			rgba.blue = static_cast<uint8_t>(node.AsArray()[2].AsInt());
			rgba.opacity = node.AsArray()[3].AsDouble();
			color = rgba;
		} // только валидный случай: rgb или rgba
	}
	if (node.IsString()) {
		color = node.AsString();
	}
	return color;
}

RenderSettings JsonReader::ParseRenderSettings(const Dict& dict) {
    RenderSettings settings;
	settings.width = dict.at("width"s).AsDouble();
	settings.height = dict.at("height"s).AsDouble();
	settings.padding = dict.at("padding"s).AsDouble();
	settings.line_width = dict.at("line_width"s).AsDouble();
	settings.stop_radius = dict.at("stop_radius"s).AsDouble();
	settings.bus_label_font_size = dict.at("bus_label_font_size"s).AsInt();
	settings.bus_label_offset.x_ = dict.at("bus_label_offset"s).AsArray().front().AsDouble();
	settings.bus_label_offset.y_ = dict.at("bus_label_offset"s).AsArray().back().AsDouble();
	settings.stop_label_font_size = dict.at("stop_label_font_size"s).AsInt();
	settings.stop_label_offset.x_ = dict.at("stop_label_offset"s).AsArray().front().AsDouble();
	settings.stop_label_offset.y_ = dict.at("stop_label_offset"s).AsArray().back().AsDouble();
	settings.underlayer_width = dict.at("underlayer_width"s).AsDouble();
	settings.underlayer_color = ParseColor(dict.at("underlayer_color"s));
	for (const auto& color : dict.at("color_palette"s).AsArray()) {
		settings.color_palette.push_back(ParseColor(color));
	}
    return settings;
}

RouterSettings JsonReader::ParseRouterSettings(const Dict& dict) {
    RouterSettings settings;
    settings.bus_wait_time_ = static_cast<size_t>(dict.at("bus_wait_time"s).AsInt());
    settings.bus_velocity_ = dict.at("bus_velocity"s).AsDouble();
    return settings;
}

void JsonReader::LoadFromJson(std::istream& input) {
    auto load_from_json = Load(input).GetRoot().AsDict();
    
    // base_requests - добавляем в handler все запросы на добавление данных в справочник
    for (const auto &item : load_from_json.at("base_requests"s).AsArray()) {
        auto request = item.AsDict();
        if (request.at("type"s).AsString() == "Stop"s) {
            rh_.AddStopBaseRequest(ParseStop(std::move(request)));
        } else if (request.at("type"s).AsString() == "Bus"s) {
            rh_.AddBusBaseRequest(ParseBus(std::move(request)));
        } /*else { //валидный входной json: только Stop и Bus
            std::cerr << "not Stop, not Bus" << std::endl;
        }*/
    }

    // выполняем все запросы на добавление остановок и автобусов в справочник
    rh_.ApplyAllRequests();

    // добавляем render_settings в renderer
    rh_.AddRenderSettings(ParseRenderSettings(std::move(load_from_json.at("render_settings"s).AsDict())));

    // добавляем не пустые маршруты (с остановками) и не пустые остановки (с автобусами через них) в handler
    rh_.AddAllBuses();
    rh_.AddAllStops();

    // добавляем routing_settings в tr.router
    rh_.AddRouterSettings(ParseRouterSettings(std::move(load_from_json.at("routing_settings"s).AsDict())));
    
    // добавляем количество вершин в tr.router и строим маршрутизатор 
    rh_.SetTransportRouter(); 

    // добавляем все запросы на вывод информации из справочника
    for (const auto &item : std::move(load_from_json.at("stat_requests"s).AsArray())) {
        auto request = item.AsDict();
        auto type = request.at("type"s).AsString();
        rh_.AddStatResult(ParseStat(std::move(request)));
    }

}

void JsonReader::PrintIntoJson(std::ostream& output) {    
    Array json_output;
    for (const auto& stat_res : rh_.GetStatResults()) { 
        Dict request_dict;
        // выводим информацию по запросу маршрута
        if (std::holds_alternative<StatResultBus>(stat_res)) { 
            const auto& id = std::get<StatResultBus>(stat_res).first;
            if (std::get<StatResultBus>(stat_res).second != std::nullopt) {
                const auto& info = std::get<StatResultBus>(stat_res).second.value();
                request_dict = AddBusStatIntoDict(id, info);
            } else {
                request_dict = AddErrorInfoIntoDict(id);
            } 

        // выводим информацию по запросу остановки
        } else if (std::holds_alternative<StatResultStop>(stat_res)) { 
            const auto& id = std::get<StatResultStop>(stat_res).first;
            if (std::get<StatResultStop>(stat_res).second != std::nullopt) {
                const auto& info = std::get<StatResultStop>(stat_res).second.value();
                request_dict = AddStopStatIntoDict(id, info);
            } else {
                request_dict = AddErrorInfoIntoDict(id);
            }
        
        // выводим информацию по запросу карты
        } else if (std::holds_alternative<StatResultMap>(stat_res)) { 
            const auto& id = std::get<StatResultMap>(stat_res).first;
            const auto& svg_map = std::get<StatResultMap>(stat_res).second;
            request_dict = AddSVGIntoDict(id, svg_map);

        // выводим информацию по запросу оптимального маршрута
        } else if (std::holds_alternative<StatResultRoute>(stat_res)) {
            const auto& id = std::get<StatResultRoute>(stat_res).first;
            const auto& route_info = std::get<StatResultRoute>(stat_res).second;
            if (route_info != std::nullopt) {
                request_dict = AddRouteInfoIntoDict(id, route_info.value());
            } else {
                request_dict = AddErrorInfoIntoDict(id);
            }
        }
        
        json_output.emplace_back(std::move(request_dict)); 
    }
    json::Document doc(std::move(json_output)); 
    Print(std::move(doc), output);
}

Dict JsonReader::AddBusStatIntoDict(const int id, const BusInfo& info) {
    return Node{
        Builder{}
        .StartDict()
            .Key("curvature"s).Value(info.route_distance / info.route_length)
            .Key("request_id"s).Value(id)
            .Key("route_length"s).Value(info.route_distance)
            .Key("stop_count"s).Value(static_cast<int>(info.stops_on_route))
            .Key("unique_stop_count"s).Value(static_cast<int>(info.unique_stops))
        .EndDict()
        .Build()
    }.AsDict();
}

Dict JsonReader::AddStopStatIntoDict(const int id, const StopInfo& info) {
    Array buses;
    for (const auto& bus_name : info.bus_names) {
        buses.push_back(bus_name);
    }

    return Node{
        Builder{}
        .StartDict()
            .Key("buses"s).Value(std::move(buses))
            .Key("request_id"s).Value(id)
        .EndDict()
        .Build()
    }.AsDict();
}

Dict JsonReader::AddErrorInfoIntoDict(const int id) {
    return Node{
        Builder{}
        .StartDict()
            .Key("request_id"s).Value(id)
            .Key("error_message"s).Value("not found"s)
        .EndDict()
        .Build()
    }.AsDict();
}

Dict JsonReader::AddSVGIntoDict(const int id, const std::string& svg_map) {
    return Node{
        Builder{}
        .StartDict()
            .Key("request_id"s).Value(id)
            .Key("map"s).Value(svg_map)
        .EndDict()
        .Build()
    }.AsDict();
}

Dict JsonReader::AddRouteInfoIntoDict(const int id, const RouteInfo& route_info) {
    Array spans;
    for (const auto& route_edge : route_info.route_edges) {

        // ребро ожидания
        if (std::holds_alternative<WaitEdgeInfo>(route_edge)) { 
            const auto& wait_edge_info = std::get<WaitEdgeInfo>(route_edge);
            Node dict = Builder{}
                .StartDict()
                    .Key("type").Value("Wait")
                    .Key("stop_name").Value(wait_edge_info.name)
                    .Key("time").Value(wait_edge_info.time)
                .EndDict()
                .Build();
            spans.push_back(dict);

        // ребро движения
        } else if (std::holds_alternative<BusEdgeInfo>(route_edge)) { 
            const auto& bus_edge_info = std::get<BusEdgeInfo>(route_edge);
            Node dict = Builder{}
                .StartDict()
                    .Key("type").Value("Bus")
                    .Key("bus").Value(bus_edge_info.name)
                    .Key("span_count").Value(static_cast<int>(bus_edge_info.span_count))
                    .Key("time").Value(bus_edge_info.time)
                    .EndDict()
                    .Build();
            spans.push_back(dict);
        }
    }

    return Node{
        Builder{}
        .StartDict()
            .Key("request_id").Value(id)
            .Key("total_time").Value(route_info.time)
            .Key("items").Value(spans)
        .EndDict()
        .Build()
    }.AsDict();
}

} // namespace json_reader