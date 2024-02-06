#pragma once

#include "geo.h"

#include <algorithm>
#include <deque>
#include <iostream>
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

//  информация: количество всех остановок, количество остановки без повторений, длина маршрута
struct BusInfo {
    std::string name;
    size_t stops_on_route = 0;
    size_t unique_stops = 0;
    double route_length = 0.0;
};

//  информация: название, упорядоченный вектор названий автобусов
struct StopInfo {
    std::string name;
    std::vector<std::string> bus_names;
};

// хешер для stopname_to_buses_
struct Hasher {
    size_t operator() (const Stop* pointer) const {
        return p_hasher_(pointer) * 67;
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
    //BusInfo GetBusInfo(const std::string_view name) const;

    // Получение информации об остановке
    std::optional<StopInfo> GetStopInfo(const std::string_view name) const;

private:

    std::deque<Stop> stops_; //все остановки
    std::deque<Bus> buses_; //все автобусы

    std::unordered_map<std::string_view, const Stop*> stopname_to_stop_; //по названию остановки - остановка
    std::unordered_map<std::string_view, const Bus*> busname_to_bus_; //по названию автобуса - автобус

    std::unordered_map<const Stop*, std::unordered_set<const Bus*>, Hasher> stopname_to_buses_; // название остановки - все автобусы

    // вычисляет длину всего маршрута
    double ComputeRouteLength(std::vector<const Stop*> route) const;

    // вычисляет количество уникальных остановок маршрута
    size_t ComputeCountUniqueStops(std::vector<const Stop*> route) const;

};

} // namespace transport_catalogue