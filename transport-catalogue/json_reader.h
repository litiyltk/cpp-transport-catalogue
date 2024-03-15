#pragma once

#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "json.h" 
#include "domain.h"

#include <iostream>
#include <optional> 
#include <string>
#include <string_view>
#include <variant> 
#include <vector>





/*
код наполнения транспортного справочника данными из JSON,
а также код обработки запросов к базе и формирование массива ответов в формате JSON
*/
namespace json_reader {

class JsonReader {
public:
    explicit JsonReader(request_handler::RequestHandler& rh)
        : rh_(rh) {
    }

    // прочитать JSON из входного потока и сохранить вектор запросов 
    void LoadFromJson(std::istream& input);

    // вывести JSON в соответствии с вектором запросов 
    void PrintIntoJson(std::ostream& output);

    // возвращает структуру с полями запроса на добавление автобуса
    domain::BusBaseRequest ParseBus(const json::Dict& dict); 

    // возвращает структуру с полями запроса на добавление автобуса
    domain::StopBaseRequest ParseStop(const json::Dict& dict);

    // возвращает структуру с полями запроса на вывод информации из транспортного справочника
    domain::StatRequest ParseStat(const json::Dict& dict);

    // возвращает структуру цвета
    svg::Color ParseColor(const json::Node& node);
    
    // возвращает структуру с параметрами для визуализации схемы
    map_renderer::RenderSettings ParseRenderSettings(const json::Dict& dict);

    // заполняют request_dict по ссылке результатами по запросу на вывод информации
    void AddBusStatIntoDict(const int id, const domain::BusInfo& info, json::Dict& request_dict);
    void AddStopStatIntoDict(const int id, const domain::StopInfo& info, json::Dict& request_dict);
    void AddErrorInfoIntoDict(const int id, json::Dict& request_dict);
    void AddSVGIntoDict(const int id, const std::string& svg_map, json::Dict& request_dict);

private:
    request_handler::RequestHandler& rh_; //методы для обработки запросов

};

} // namespace json_reader 