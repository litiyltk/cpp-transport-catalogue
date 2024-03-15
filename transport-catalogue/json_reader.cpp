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
    for (const auto &node : dict.at("road_distances"s).AsMap()) {
        request.road_distances.emplace_back(node.first, node.second.AsInt());
    }
    return request;
}

StatRequest JsonReader::ParseStat(const Dict& dict) {
    StatRequest request;
    request.id = dict.at("id"s).AsInt();
    request.type = dict.at("type"s).AsString();
    request.name = (dict.count("name"s) ? dict.at("name"s).AsString() : "without name for 'map'-request"s); 
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

void JsonReader::LoadFromJson(std::istream& input) {
    auto load_from_json = Load(input).GetRoot().AsMap();
    // добавляем все запросы на добавление данных в справочник
    for (const auto &item : load_from_json.at("base_requests"s).AsArray()) {
        auto request = item.AsMap();
        if (request.at("type"s).AsString() == "Stop"s) {
            rh_.AddStopBaseRequest(ParseStop(std::move(request)));
        } else if (request.at("type"s).AsString() == "Bus"s) {
            rh_.AddBusBaseRequest(ParseBus(std::move(request)));
        } /*else { //валидный json: только Stop и Bus
            std::cerr << "not Stop, not Bus" << std::endl;
        }*/
    }

    // выполняем все запросы на добавление остановок и автобусов в справочник
    rh_.ApplyAllRequests();

    // добавляем render_settings в renderer
    rh_.AddRenderSettings(ParseRenderSettings(std::move(load_from_json.at("render_settings"s).AsMap())));
    // добавляем не пустые маршруты (с остановками) и не пустые остановки (с автобусами через них) в handler
    rh_.AddAllBuses();
    rh_.AddAllStops();

    // добавляем все запросы на вывод информации из справочника
    for (const auto &item : std::move(load_from_json.at("stat_requests"s).AsArray())) {
        auto request = item.AsMap();
        auto type = request.at("type"s).AsString();
        //rh_.AddStatRequest(ParseStat(std::move(request))); //валидный json: только Stop, Bus, Map
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
                AddBusStatIntoDict(id, info, request_dict);
            } else {
                AddErrorInfoIntoDict(id, request_dict);
            } 

        // выводим информацию по запросу остановки
        } else if (std::holds_alternative<StatResultStop>(stat_res)) { 
            const auto& id = std::get<StatResultStop>(stat_res).first;
            if (std::get<StatResultStop>(stat_res).second != std::nullopt) {
                const auto& info = std::get<StatResultStop>(stat_res).second.value();
                AddStopStatIntoDict(id, info, request_dict);
            } else {
                AddErrorInfoIntoDict(id, request_dict);
            }
        
        // выводим информацию по запросу карты
        } else if  (std::holds_alternative<StatResultMap>(stat_res)) { 
            const auto& id = std::get<StatResultMap>(stat_res).first;
            const auto& svg_map = std::get<StatResultMap>(stat_res).second;
            AddSVGIntoDict(id, svg_map, request_dict);
        }
        
        json_output.emplace_back(std::move(request_dict)); 
    }
    json::Document doc(std::move(json_output)); 
    Print(std::move(doc), output);
}

/*
// изначально вызывал сохранённый в request_handler вектор запросов на вывод

void JsonReader::PrintIntoJson(std::ostream& output) {    
    Array json_output;
    for (const auto& request : rh_.GetStatRequests()) { 
        Dict request_dict;
        // выводим информацию по запросу маршрута
        if (request.type == "Bus") {
            //auto bus_info = catalogue.GetBusInfo(request.name);
            auto bus_info = rh_.GetBusStat(request.name);
            if (bus_info != std::nullopt) {
                const auto& info = bus_info.value();
                AddBusStatIntoDict(request.id, info, request_dict);
            } else {
                AddErrorInfoIntoDict(request.id, request_dict);
            }
        // выводим информацию по запросу остановки
        } else if (request.type == "Stop") {
            //auto stop_info = catalogue.GetStopInfo(request.name);
            auto stop_info = rh_.GetStopStat(request.name);
            if (stop_info != std::nullopt) {
                const auto& info = stop_info.value();
                AddStopStatIntoDict(request.id, info, request_dict);
            } else {
                AddErrorInfoIntoDict(request.id, request_dict);
            }
        // выводим информацию по запросу карты
        } else if (request.type == "Map") {
            AddSVGIntoDict(request.id, rh_.GetMapSVG(), request_dict);
        }
        json_output.emplace_back(std::move(request_dict)); 
    }
    json::Document doc(std::move(json_output)); 
    Print(std::move(doc), output);
}
*/

void JsonReader::AddBusStatIntoDict(const int id, const BusInfo& info, Dict& request_dict) {
    request_dict["curvature"s] = info.route_distance / info.route_length;
    request_dict["request_id"s] = id;
    request_dict["route_length"s] = info.route_distance;
    request_dict["stop_count"s] = static_cast<int>(info.stops_on_route);
    request_dict["unique_stop_count"s] = static_cast<int>(info.unique_stops);
}

void JsonReader::AddStopStatIntoDict(const int id, const StopInfo& info, Dict& request_dict) {
    Array buses;
    for (const auto& bus_name : info.bus_names) {
        buses.push_back(bus_name);
    }
    request_dict["buses"s] = std::move(buses); 
    request_dict["request_id"s] = id;
}

void JsonReader::AddErrorInfoIntoDict(const int id, Dict& request_dict) {
    request_dict["request_id"s] = id;
    request_dict["error_message"s] = "not found"s;
}

void JsonReader::AddSVGIntoDict(const int id, const std::string& svg_map, Dict& request_dict) {
    request_dict["request_id"s] = id;
    request_dict["map"s] = svg_map;
}

} // namespace json_reader
