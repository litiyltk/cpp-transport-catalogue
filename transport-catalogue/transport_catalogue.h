#pragma once

#include "domain.h"
#include "geo.h"

#include <algorithm>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>


/*
функции транспортного справочника: добавление и вывод данных об автобусных маршрутах и остановках
формат ввода маршрутов: A,B,C,A, true - круговой, A,B,C, false - прямой.
*/
namespace transport_catalogue {
  
// Класс хранения и обработки транспортного справочника
class TransportCatalogue {
public:
    explicit TransportCatalogue() = default;

    // Добавление остановки в базу данных
    void AddStop(const domain::Stop& stop);
    
    // Добавление маршрута в базу данных
    void AddBus(const domain::Bus& bus);
    
    // Остановка по названию остановки
    const domain::Stop* FindStop(const std::string_view name) const;
    
    // Автобус по названию автобуса
    const domain::Bus* FindBus(const std::string_view name) const;
    
    // Получение информации о маршруте
    std::optional<domain::BusInfo> GetBusInfo(const std::string_view name) const;

    // Возвращает маршруты (запрос Stop)
    const std::unordered_set<const domain::Bus*>* GetBusesByStop(const std::string_view name) const;

    // Получение информации об остановке
    std::optional<domain::StopInfo> GetStopInfo(const std::string_view name) const;

    // задание дистанции между остановками
    void SetDistance(const std::string_view start, const std::string_view finish, const int distance);
    
    // получение дистанции между остановками
    int GetDistance(const std::string_view start, const std::string_view finish) const;

    // Возвращает все маршруты автобусов в алфавитном порядке
    const std::deque<domain::Bus>& GetBuses();

    // Возвращает остановки (только на маршрутах) в алфавитном порядке 
    const std::deque<domain::Stop>& GetStops();

private:

    std::deque<domain::Stop> stops_; //все остановки
    std::deque<domain::Bus> buses_; //все автобусы

    std::unordered_map<std::string_view, const domain::Stop*> stopname_to_stop_; //по названию остановки - остановка
    std::unordered_map<std::string_view, const domain::Bus*> busname_to_bus_; //по названию автобуса - автобус

    std::unordered_map<const domain::Stop*, std::unordered_set<const domain::Bus*>, domain::Hasher> stopname_to_buses_; // название остановки - все автобусы

    std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, int, domain::Hasher> distances_; // расстояние между остановками
    
    // вычисляет количество уникальных остановок маршрута
    size_t ComputeCountUniqueStops(const std::vector<const domain::Stop*>& route) const;

    // вычисляет количество остановок маршрута
    size_t ComputeCountStops(const size_t stops_count, const bool is_roundtrip) const;
    
    // вычисляет длину всего маршрута по географическим координатам остановок
    double ComputeRouteLength(const std::vector<const domain::Stop*>& route, const bool is_roundtrip) const;

    // вычисляет фактическую длину всего маршрута по данным в distances_, полученным из входного запроса
    int ComputeRouteDistance(const std::vector<const domain::Stop*>& route, const bool is_roundtrip) const;

};

} // namespace transport_catalogue