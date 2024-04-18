#pragma once

#include "domain.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <variant> 
#include <vector>


/*
функции, выполняющие взаимодействие с транспортным справочником:
от json_reader приходят запросы (добавление и вывод),
из transport_catalogue извлекается нужная информация для запросов
для map_renderer
для transport_router
*/
namespace request_handler {

class RequestHandler {
public:

    explicit RequestHandler(transport_catalogue::TransportCatalogue& db,
                            map_renderer::MapRendererSVG& mr,
                            transport_router::TransportRouter& ro)
                                : db_(db), mr_(mr), ro_(ro) {}

    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<domain::BusInfo> GetBusStat(const std::string_view& bus_name) const;

    // Возвращает информацию о маршруте (запрос Stop)
    std::optional<domain::StopInfo> GetStopStat(const std::string_view& stop_name) const;

    // Возвращает маршруты (запрос Stop)
    const std::unordered_set<const domain::Bus*>* GetBusesByStop(const std::string_view& stop_name) const;

    // Добавление запроса на добавление автобуса
    void AddBusBaseRequest(const domain::BusBaseRequest& request);

    // Добавление запроса на добавление остановки
    void AddStopBaseRequest(const domain::StopBaseRequest& request);

    // Добавление запроса на вывод c получением результата BusInfo/StopInfo
    void AddStatResult(const domain::StatRequest& request);

    // Выполнение запросов на добавление информации в транспортный справочник
    void ApplyAllRequests() const;

    // Добавление настроек визуализации в рендерер
    void AddRenderSettings(const map_renderer::RenderSettings& settings);

    // Добавление настроек графа
    void AddRouterSettings(const domain::RouterSettings& settings);

    // Добавление количества вершин графа и построение маршрутизатора
    void SetTransportRouter();

    // Получение результатов по запросам на вывод информации из транспортного справочника
    const std::vector<domain::StatResult>& GetStatResults() const;

    // Получает данные о маршрутах у транспортного справочника, исключает маршруты без остановок и сортирует по алфавиту
    void AddAllBuses();

    // Получает данные об остановках у транспортного справочника, исключает остановки без маршрутов и сортирует по алфавиту
    void AddAllStops();

    // Возвращает все маршруты в алфавитном порядке
    const std::vector<domain::Bus>& GetAllBuses() const;

    // Возвращает все остановки на маршрутах в алфавитном порядке
    const std::vector<domain::Stop>& GetAllStops() const;

    // Преобразует SVG-объект из потока в строку
    const std::string GetStringSVG() const;

private:
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
    transport_catalogue::TransportCatalogue& db_;
    map_renderer::MapRendererSVG& mr_;
    transport_router::TransportRouter& ro_; //и "Расчёт маршрута"
    
    std::vector<domain::BusBaseRequest> bus_base_requests_; // запросы на ввод автобусных маршрутов
    std::vector<domain::StopBaseRequest> stop_base_requests_; // запросы на ввод остановок

    std::vector<domain::Bus> buses_; // упорядоченные по алфавиту маршруты, проходящие через остановки
    std::vector<domain::Stop> stops_; // упорядоченные по алфавиту остановки, через которые проходят маршруты

    std::vector<domain::StatResult> stat_results_; // результаты по запросам на вывод инфомации

};

} //namespace request_handler