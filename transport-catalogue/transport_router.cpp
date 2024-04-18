#include "transport_router.h"


namespace transport_router {
    using namespace domain;
    
    void TransportRouter::SetRouterSettings(const RouterSettings& settings) {
        settings_ = settings;
    }

    void TransportRouter::SetVertexCount(const size_t count) {
        vertex_count_ = 2 * count; //для каждой остановки есть две вершины: wait, bus
    }

    void TransportRouter::BuildTransportRouter() {
        // создаём граф
        BuildGraph();

        // добавляем рёбра ожиданий (пересадка на остановке)
        AddAllWaitEdgeInfos();
        //std::cerr << "Wait-edges count: " << id_to_edge_infos_.size() << std::endl;

        // добавляем рёбра маршрута
        for (const auto& [ is_roundtrip, bus_name, route ] : db_.GetBuses()) {
            AddAllBusEdgeInfos(bus_name, route);
            if (!is_roundtrip) { // // для прямого маршрута A,B,C,B,A путь туда-обратно A,B,C + C,B,A
                std::vector<Stop const*> reversed_route(route.rbegin(), route.rend());
                AddAllBusEdgeInfos(bus_name, std::move(reversed_route));
            }
        }
        //std::cerr << "Total edges count: " << id_to_edge_infos_.size() << std::endl;

        // создаём маршрутизатор
        BuildRouter();
    }

    std::optional<RouteInfo> TransportRouter::GetOptimalRoute(const std::string& from_stop, const std::string& to_stop) const {    
        std::vector<EdgeInfo> optimal_route;
        double total_time = 0.0;

        // нет указанных остановок
        if (stop_to_id_vertices_.count(db_.FindStop(from_stop)) == 0 || stop_to_id_vertices_.count(db_.FindStop(to_stop)) == 0) {
            return std::nullopt;
        }

        auto from = GetStopPairID(from_stop).first; //уезжаем с ожиданием
        auto to = GetStopPairID(to_stop).first; // приезжаем без ожидания
        const auto route = router_.get()->BuildRoute(from, to);

        // не найден маршрут
        if (!route.has_value()) {
            return std::nullopt;
        }

        // добавляем все рёбра маршрута
        for (const auto& id : route.value().edges) {
            optimal_route.push_back(id_to_edge_infos_[id]);
            if (std::holds_alternative<WaitEdgeInfo>(id_to_edge_infos_[id])) { 
                const auto& wait_edge_info = std::get<WaitEdgeInfo>(id_to_edge_infos_[id]);
                total_time += wait_edge_info.time;
            } else if (std::holds_alternative<BusEdgeInfo>(id_to_edge_infos_[id])) { 
                const auto& bus_edge_info = std::get<BusEdgeInfo>(id_to_edge_infos_[id]);
                total_time += bus_edge_info.time;
            }
        }
        return RouteInfo{total_time, optimal_route};
    }

    void TransportRouter::BuildGraph() {
        graph_ = Graph(vertex_count_);
    }

    void TransportRouter::BuildRouter() {
        router_ = std::make_unique<Router>(graph_);
    }

    const std::pair<size_t, size_t> TransportRouter::GetStopPairID(const std::string& stop_name) const {
        return stop_to_id_vertices_.at(db_.FindStop(stop_name));
    }

    void TransportRouter::AddRouteToTransportRouter(const size_t from, const size_t to, const double time) {
        graph_.AddEdge(Edge{ from, to, time });
    }

    double TransportRouter::ComputeRouteTime(const std::string& from_stop, const std::string& to_stop) const {
        return db_.GetDistance(from_stop, to_stop) / METERS_PER_KM / settings_.bus_velocity_ * MIN_PER_HOUR;
    }

    void TransportRouter::AddWaitEdgeInfo(const std::string& stop_name, const double bus_wait_time) {
        id_to_edge_infos_.emplace_back(WaitEdgeInfo{ stop_name, bus_wait_time }); //вынести в приват-метод
    }

    void TransportRouter::AddBusEdgeInfo(const std::string& bus_name, const double time, const size_t span_count) {
        id_to_edge_infos_.emplace_back(BusEdgeInfo{ bus_name, time, span_count });
    }

    void TransportRouter::AddAllWaitEdgeInfos() {
        size_t i = 0;
        for (const auto& [ name, _ ] : db_.GetStops()) {
            stop_to_id_vertices_[db_.FindStop(name)] = { i, i + 1 };
            const double time = settings_.bus_wait_time_;
            AddRouteToTransportRouter(i, i + 1, time); 
            AddWaitEdgeInfo(name, time);
            i += 2;
        }
    }

    void TransportRouter::AddAllBusEdgeInfos(const std::string& bus_name, const std::vector<const domain::Stop*>& route) {    
        for (size_t i = 0; i < route.size() - 1; i++) { // остановка from
            double time = 0;
            size_t span_count = 0;
            size_t prev = i;
            const size_t from = stop_to_id_vertices_[route[i]].second;
            for (size_t j = i + 1; j < route.size(); j++) { // остановка to
                //std::cerr << "bus edge: " << i << " -> " << j << std::endl;
                //std::cerr << bus_name << ": " << route[i]->name << " -> " << route[j]->name << std::endl;
                const size_t to = stop_to_id_vertices_[route[j]].first;
                span_count += 1;
                time += ComputeRouteTime(route[prev]->name, route[j]->name);
                prev = j;       
                AddRouteToTransportRouter(from, to, time);
                AddBusEdgeInfo(bus_name, time, span_count);
            }
        }
    }

}