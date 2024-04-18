#pragma once

#include "transport_catalogue.h"
#include "domain.h"
#include "router.h"
#include "graph.h"

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>


/*
функции маршрутизации транспортного справочника: построение графа
формат ввода маршрутов: A,B,C,A, true - круговой, A,B,C, false - прямой.
вершины графа соответствуют двум состояниям: ожидание автобуса и движение на автобусе
*/
namespace transport_router {

constexpr static double METERS_PER_KM = 1000.0;
constexpr static double MIN_PER_HOUR = 60.0;

// ребро маршрута
using Edge = graph::Edge<double>;

// тип графа
using Graph = graph::DirectedWeightedGraph<double>;

//тип маршрутизатора
using Router = graph::Router<double>;


class TransportRouter {
public:        
    explicit TransportRouter(transport_catalogue::TransportCatalogue& db)
        : db_(db) {}

    // задаёт настройки маршрутизации: скорость автобуса и время ожидания на остановке
    void SetRouterSettings(const domain::RouterSettings& settings);

    // задаёт количество узлов на графе для задания графа
    void SetVertexCount(const size_t count);

    // задаёт все рёбра графа по парам вершин и весу ребра из EdgeInfo
    void BuildTransportRouter();

    // возвращает вектор рёбер для оптимального маршрута
    std::optional<domain::RouteInfo> GetOptimalRoute(const std::string& from_stop, const std::string& to_stop) const;

private:
    const transport_catalogue::TransportCatalogue& db_; 
    domain::RouterSettings settings_;
    size_t vertex_count_;
    Graph graph_; 
    std::unique_ptr<Router> router_;
    std::vector<domain::EdgeInfo> id_to_edge_infos_; // id ребра - информация о ребре
    std::unordered_map<const domain::Stop*, std::pair<size_t, size_t>> stop_to_id_vertices_; // словарь остановка - пара id их вершин (с первой уезжаем, на вторую приезжаем)

    // строит граф
    void BuildGraph();
        
    // строит маршрутизатор
    void BuildRouter();

    // Возвращает пару вершин для остановки from, to
    const std::pair<size_t, size_t> GetStopPairID(const std::string& stop_name) const;

    // создаёт ребро маршрута
    void AddRouteToTransportRouter(const size_t from, const size_t to, const double time);

    // вычисляет время на ребре маршрута
    double ComputeRouteTime(const std::string& from_stop, const std::string& to_stop) const;

    // добавляет описание для ребра ожидания (пересадка)
    void AddWaitEdgeInfo(const std::string& stop_name, const double bus_wait_time);

    // добавляет описание для ребра движения
    void AddBusEdgeInfo(const std::string& bus_name, const double time, const size_t span_count);

    // добавляет описание для всех рёбер ожидания (пересадка)
    void AddAllWaitEdgeInfos();

    // добавляет описание для всех рёбер движения
    void AddAllBusEdgeInfos(const std::string& bus_name, const std::vector<const domain::Stop*>& route);

};

} // namespace transport_router