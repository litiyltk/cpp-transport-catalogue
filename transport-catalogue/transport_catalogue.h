#pragma once

#include "geo.h"

#include <algorithm>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace transport_catalogue {

// остановка: название и координаты
struct Stop {
    std::string name; //название остановки или id
    geo::Coordinates coordinates;
};

// автобус: название и маршрут
struct Bus {
    std::string name; // название автобуса или id
    std::vector<const Stop*> route; //маршрут
};

/*  информация: количество всех остановок, количество остановки без повторений,
географическая длина маршрута, фактическая длина маршрута */
struct BusInfo {
    std::string name;
    size_t stops_on_route = 0;
    size_t unique_stops = 0;
    double route_length = 0.0;
    int route_distance = 0;
};

//  информация: название, упорядоченный вектор названий автобусов
struct StopInfo {
    std::string name;
    std::vector<std::string> bus_names;
};

// хешер для stopname_to_buses_
struct Hasher {
    const size_t simple_number = 67;

    size_t operator() (const Stop* pointer) const {
        return p_hasher_(pointer) * simple_number;
    }

    // для дистанций
    size_t operator()(const std::pair<const Stop*, const Stop*>& pointer) const {
        return p_hasher_(pointer.first) * simple_number + p_hasher_(pointer.second) * simple_number * simple_number;
    }

	private:
		std::hash<const void *> p_hasher_;
};

// Класс хранения и обработки транспортного справочника
class TransportCatalogue {
public:
    TransportCatalogue();

    // Добавление остановки в базу данных
    void AddStop(const Stop& stop);
    
    // Добавление маршрута в базу данных
    void AddBus(const Bus& bus);
    
    // Остановка по названию остановки
    const Stop* FindStop(const std::string_view name) const;
    
    // Автобус по названию автобуса
    const Bus* FindBus(const std::string_view name) const;
    
    // Получение информации о маршруте
    std::optional<BusInfo> GetBusInfo(const std::string_view name) const;

    // Получение информации об остановке
    std::optional<StopInfo> GetStopInfo(const std::string_view name) const;

    // задание дистанции между остановками
    void SetDistance(const std::string_view start, const std::string_view finish, const int distance);
    
    // получение дистанции между остановками
    int GetDistance(const std::string_view start, const std::string_view finish) const;

private:

    std::deque<Stop> stops_; //все остановки
    std::deque<Bus> buses_; //все автобусы

    std::unordered_map<std::string_view, const Stop*> stopname_to_stop_; //по названию остановки - остановка
    std::unordered_map<std::string_view, const Bus*> busname_to_bus_; //по названию автобуса - автобус

    std::unordered_map<const Stop*, std::unordered_set<const Bus*>, Hasher> stopname_to_buses_; // название остановки - все автобусы

    std::unordered_map<std::pair<const Stop*, const Stop*>, int, Hasher> distances_; // расстояние между остановками
    
    // вычисляет количество уникальных остановок маршрута
    size_t ComputeCountUniqueStops(const std::vector<const Stop*>& route) const;
    
    // вычисляет длину всего маршрута по географическим координатам остановок
    double ComputeRouteLength(const std::vector<const Stop*>& route) const;

    // вычисляет фактическую длину всего маршрута по данным в distances_, полученным из входного запроса
    int ComputeRouteDistance(const std::vector<const Stop*>& route) const;

};

} // namespace transport_catalogue