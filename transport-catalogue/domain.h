#pragma once

#include "geo.h"

#include <optional>
#include <variant>
#include <vector>
#include <string>


// классы основных сущностей, описывают автобусы и остановки
namespace domain {

// остановка: название и координаты
struct Stop {
    std::string name; 
    geo::Coordinates coordinates;
};

/*
автобус: тип маршрута, название, маршрут.
Валидные маршруты: ABCA - прямой, ABC - кольцевой.
*/
struct Bus {
    bool is_roundtrip; 
    std::string name; 
    std::vector<const Stop*> route; // остановки на маршруте автобуса
};

/*
информация о маршруте: количество всех остановок,
количество остановки без повторений,
географическая длина маршрута (расчёт geo::ComputeDistance),
фактическая длина маршрута (transport_catalogue::SetDistance)
*/
struct BusInfo {
    std::string name;
    size_t stops_on_route = 0;
    size_t unique_stops = 0;
    double route_length = 0.0;
    int route_distance = 0;
};

//  информация об остановке: название, вектор названий автобусов
struct StopInfo {
    std::string name;
    std::vector<std::string> bus_names;
};

struct Hasher {
    const size_t simple_number = 67;

    // хешер для  stopname_to_buses_ (transport_catalogue)
    size_t operator() (const Stop* pointer) const;

    // хешер для distances_ (transport_catalogue)
    size_t operator()(const std::pair<const Stop*, const Stop*>& pointer) const;

	private:
		std::hash<const void *> p_hasher_;
};

// структура запроса на вывод данных об остановке 
struct BusBaseRequest {
    std::string name;
    std::vector<std::string> stops; 
    bool is_roundtrip;
};

/*
структура запроса на ввод данных об остановке:
название, координаты, пары расстояний в формате название-остановка
*/
struct StopBaseRequest {
    std::string name;
    double lat;
    double lng;
    std::vector<std::pair<std::string, int>> road_distances; 
};

/*
структура запроса на вывод информации из справочника:
ID запроса, тип запроса, название для Bus/Stop
*/
struct StatRequest {
    int id;
    std::string type;
    std::string name;
};

/*
Для вектора результатов запросов на вывод:
пара id запроса - BusInfo/StopInfo/std::string,
для запросов Bus, Stop, Map соответственно
*/
using StatResultBus = std::pair<int, std::optional<BusInfo>>;
using StatResultStop = std::pair<int, std::optional<StopInfo>>;
using StatResultMap = std::pair<int, std::string>;

using StatResult = std::variant<std::nullptr_t,
                                StatResultBus,
                                StatResultStop,
                                StatResultMap>;

} //namespace domain