#pragma once

#include "geo.h"
#include "domain.h"
#include "test_framework.h"
#include "transport_catalogue.h"
//#include "transport_router.h"
#include "map_renderer.h"

// тесты заполнения базы данных
namespace tests {
    using namespace transport_catalogue;
    using namespace transport_router;
    using namespace test_framework;
    using namespace domain;

// Проверка расчёта расстояния между точками по географическим координатам
void TestComputeDistance() {

    // координаты для примера
    const geo::Coordinates A = {55.837064, 37.628747};
    const geo::Coordinates B = {56.326569, 44.239952};
    const geo::Coordinates C = {54.942629, 43.306283};
    const geo::Coordinates E = {55.837064, 37.628747};

    // ожидаемые значения расстояний между точками https://geleot.ru/technology/navigation/coordinate_distance
    const double expected_distance_AB = 413641;
    const double expected_distance_AC = 371995;
    const double expected_distance_BC = 164664;
    const double expected_distance_EC = 371995;

    // проверяем значения с точностью до 1 км
    const double EPSILON = 1.0;
    ASSERT(std::abs(geo::ComputeDistance(A, B) - expected_distance_AB) < EPSILON);
    ASSERT(std::abs(geo::ComputeDistance(A, C) - expected_distance_AC) < EPSILON);
    ASSERT(std::abs(geo::ComputeDistance(B, C) - expected_distance_BC) < EPSILON);
    ASSERT(std::abs(geo::ComputeDistance(C, E) - expected_distance_EC) < EPSILON);
}

// проверка методов TransportCatalogue AddBus / FindBus
void TestFindBus() {
    TransportCatalogue catalogue;

    // Добавим остановки методом Add
    catalogue.AddStop({"Tolstopaltsevo", {55.611087, 37.20829}});
    catalogue.AddStop({"Marushkino", {55.595884, 37.209755}});

    // добавим автобус методом Add
    catalogue.AddBus({true, "256", {catalogue.FindStop("Tolstopaltsevo"),
                              catalogue.FindStop("Marushkino")}});

    // Проверим наличие автобуса                     
    const auto bus = catalogue.FindBus("256");
    ASSERT(bus != nullptr);
    ASSERT_EQUAL(bus->is_roundtrip, true);
    ASSERT_EQUAL(bus->name, "256");
    ASSERT_EQUAL(bus->route.size(), 2);
    ASSERT_EQUAL(bus->route.front()->name, "Tolstopaltsevo");
    ASSERT_EQUAL(bus->route.back()->name, "Marushkino");

    // добавим автобус с пустым маршрутом методом Add
    catalogue.AddBus({true, "222", {nullptr}});

    // Проверим наличие автобуса 
    ASSERT(catalogue.FindBus("222") != nullptr);

    // Проверим отсутствующий автобус
    ASSERT(catalogue.FindBus("XX") == nullptr);

}

// проверка методов TransportCatalogue AddStop / FindStop
void TestFindStop() {
    TransportCatalogue catalogue;

    // Добавим остановки методом Add
    catalogue.AddStop({"Tolstopaltsevo", {55.611087, 37.20829}});
    catalogue.AddStop({"Marushkino", {55.595884, 37.209755}});

    // Проверим наличие остановки                     
    ASSERT(catalogue.FindStop("Tolstopaltsevo") != nullptr);
    ASSERT(catalogue.FindStop("Marushkino") != nullptr);

    // Проверим отсутствующую остановку 
    ASSERT(catalogue.FindStop("XX") == nullptr);
}

// проверка методов TransportCatalogue SetDistance / GetDistance
void TestGetDistance() {
    TransportCatalogue catalogue;
    
    // Добавим остановки методом
    catalogue.AddStop({"Tolstopaltsevo", {55.611087, 37.20829}});
    catalogue.AddStop({"Marushkino", {55.595884, 37.209755}});
    catalogue.AddStop({"Rasskazovka", {55.632761, 37.333324}});

    // Установим дистанции между остановками
    catalogue.SetDistance("Marushkino"sv, "Marushkino"sv, 100); // петля A-A
    catalogue.SetDistance("Marushkino"sv, "Rasskazovka"sv, 9900); // разные пути A-B
    catalogue.SetDistance("Rasskazovka"sv, "Marushkino"sv, 9500); // разные пути B-A
    catalogue.SetDistance("Marushkino"sv, "Tolstopaltsevo"sv, 3900); // равные пути A-B
    catalogue.SetDistance("Tolstopaltsevo"sv, "Marushkino"sv, 3900); // равные пути B-A

    // Проверим дистанции между остановками
    ASSERT_EQUAL(catalogue.GetDistance("Marushkino"sv, "Marushkino"sv), 100); // петля A-A
    ASSERT_EQUAL(catalogue.GetDistance("Marushkino"sv, "Rasskazovka"sv), 9900); // разные пути A-B
    ASSERT_EQUAL(catalogue.GetDistance("Rasskazovka"sv, "Marushkino"sv), 9500); // разные пути B-A
    ASSERT_EQUAL(catalogue.GetDistance("Marushkino"sv, "Tolstopaltsevo"sv), 3900); // равные пути A-B
    ASSERT_EQUAL(catalogue.GetDistance("Tolstopaltsevo"sv, "Marushkino"sv), 3900); // равные пути B-A
}

// проверка методов TransportCatalogue GetBusInfo и методов private TransportCatalogue
void TestGetBusInfo() {
    TransportCatalogue catalogue;

    // Остановки для примера
    const Stop A = {"A", geo::Coordinates{55.837064, 37.628747}};
    const Stop B = {"B", geo::Coordinates{56.326569, 44.239952}};
    const Stop C = {"C", geo::Coordinates{54.942629, 43.306283}};
    const Stop E = {"E", geo::Coordinates{55.837064, 37.628747}};

    // Добавим все остановки
    for (const auto& stop : {A, B, C, E}) {
        catalogue.AddStop(stop);
    }

    // Создаем вектор указателей на остановки для прямого маршрута ABCE
    std::vector<const Stop*> route_ABCE; 
    for (const auto& stop : {A, B, C, E}) {
        route_ABCE.push_back(catalogue.FindStop(stop.name));
    }

    const Bus ABCE = {false, "ABCE", route_ABCE}; // Передаем указатели на остановки

    // Создаем вектор указателей на остановки для кольцевого маршрута ABCA
    std::vector<const Stop*> route_ABCA; 
    for (const auto& stop : {A, B, C, A}) {
        route_ABCA.push_back(catalogue.FindStop(stop.name));
    }

    const Bus ABCA = {true, "ABCA", route_ABCA}; // Передаем указатели на остановки
    
    // Добавим два маршрута
    for (const auto& bus : {ABCA, ABCE}) {
        catalogue.AddBus(bus);
    }

    // Добавим дистанции между остановками
    catalogue.SetDistance("A"sv, "B"sv, 100); 
    catalogue.SetDistance("A"sv, "C"sv, 300); 
    catalogue.SetDistance("A"sv, "E"sv, 500); 
    catalogue.SetDistance("B"sv, "A"sv, 110);
    catalogue.SetDistance("C"sv, "B"sv, 150);
    catalogue.SetDistance("C"sv, "E"sv, 80);
    catalogue.SetDistance("E"sv, "A"sv, 450);
    catalogue.SetDistance("E"sv, "B"sv, 250);
    catalogue.SetDistance("E"sv, "C"sv, 90);
    
    // Проверим информацию для маршрута ABCA
    const auto bus_info_ABCA = catalogue.GetBusInfo(ABCA.name);
    ASSERT(bus_info_ABCA.has_value());
    ASSERT_EQUAL(bus_info_ABCA->name, "ABCA");
    ASSERT_EQUAL(bus_info_ABCA->stops_on_route, 4);
    ASSERT_EQUAL(bus_info_ABCA->unique_stops, 3);
    ASSERT_EQUAL(bus_info_ABCA->route_distance, 550);

    // Проверяем значения с точностью до 1 км
    const double EPSILON = 1.0;
    ASSERT(std::abs(bus_info_ABCA->route_length - 950300) < EPSILON);

    // Проверим информацию для маршрута ABCE
    const auto bus_info_ABCE = catalogue.GetBusInfo(ABCE.name);
    ASSERT(bus_info_ABCE.has_value());
    ASSERT_EQUAL(bus_info_ABCE->name, "ABCE");
    ASSERT_EQUAL(bus_info_ABCE->stops_on_route, 7);
    ASSERT_EQUAL(bus_info_ABCE->unique_stops, 4);
    ASSERT_EQUAL(bus_info_ABCE->route_distance, 680);

    ASSERT(std::abs(bus_info_ABCE->route_length - 1900600) < EPSILON);
    
    // Проверим информацию для отсутствующего автобуса
    const auto bus_info_x = catalogue.GetBusInfo("XX");
    ASSERT(!bus_info_x.has_value());
}

// проверка методов TransportCatalogue GetStopInfo
void TestGetStopInfo() { 
    transport_catalogue::TransportCatalogue catalogue;

    // Добавим остановки
    catalogue.AddStop({"Tolstopaltsevo", {55.611087, 37.20829}});
    catalogue.AddStop({"Marushkino", {55.595884, 37.209755}});
    catalogue.AddStop({"Rasskazovka", {55.632761, 37.333324}});
    catalogue.AddStop({"X", {00.00, 00.00}}); // остановка без автобуса

    // Добавим маршруты
    catalogue.AddBus({true, "256", {catalogue.FindStop("Tolstopaltsevo"),
                              catalogue.FindStop("Marushkino"),
                              catalogue.FindStop("Rasskazovka")}});
    catalogue.AddBus({true, "750", {catalogue.FindStop("Tolstopaltsevo"),
                              catalogue.FindStop("Marushkino")}});

    // Проверим информацию для остановки Marushkino                         
    const auto stop_info_marushkino = catalogue.GetStopInfo("Marushkino");
    ASSERT_EQUAL(stop_info_marushkino.has_value(), true);
    ASSERT_EQUAL(stop_info_marushkino->name, "Marushkino");
    ASSERT_EQUAL(stop_info_marushkino->bus_names.size(), 2);
    ASSERT_EQUAL(stop_info_marushkino->bus_names[0], "256");
    ASSERT_EQUAL(stop_info_marushkino->bus_names[1], "750");

    // Проверим информацию для остановки Rasskazovka                         
    const auto stop_info_rasskazovka = catalogue.GetStopInfo("Rasskazovka");
    ASSERT(stop_info_rasskazovka.has_value());
    ASSERT_EQUAL(stop_info_rasskazovka->name, "Rasskazovka");
    ASSERT_EQUAL(stop_info_rasskazovka->bus_names.size(), 1);
    ASSERT_EQUAL(stop_info_rasskazovka->bus_names[0], "256");

    // Проверим информацию для остановки X без автобуса
    const auto stop_info_test_stop = catalogue.GetStopInfo("X");
    ASSERT(!stop_info_test_stop->bus_names.size()); 
    
    // Проверим информацию об отсутствующей остановке
    const auto stop_info_new_york_city = catalogue.GetStopInfo("New York City");
    ASSERT(!stop_info_new_york_city.has_value());
}

void TestSphereProjector() {

    // Задаём размер карты и отступ от краёв
    const double WIDTH = 600.0;
    const double HEIGHT = 400.0;
    const double PADDING = 50.0;
    
    // Точки, подлежащие проецированию
    const std::vector<geo::Coordinates> geo_coords = {
        {43.587795, 39.716901},
        {43.581969, 39.719848},
        {43.598701, 39.730623},
        {43.585586, 39.733879},
        {43.590317, 39.746833}
    };

    // Создаём проектор сферических координат на карту
    const sphere_projector::SphereProjector proj {
        geo_coords.begin(), geo_coords.end(), WIDTH, HEIGHT, PADDING
    };

    // Проецируем координаты на плоскость
    std::vector<svg::Point> points;
    for (const auto& geo_coord: geo_coords) {
        const svg::Point screen_coord = proj(geo_coord);
        points.push_back(screen_coord);
    }
    
    // Ожидаемые значения координат на плоскости
    const std::vector<svg::Point> expected_points = {
        {50, 232.18},
        {99.2283, 329.5},
        {279.22, 50},
        {333.61, 269.08},
        {550, 190.051}
    };

    // Проверяем значения с точностью до 0.001
    const double EPSILON = 0.001;
    for (size_t i = 0; i < points.size(); ++i) {
        ASSERT(std::abs(points[i].x_ - expected_points[i].x_) < EPSILON);
        ASSERT(std::abs(points[i].y_ - expected_points[i].y_) < EPSILON);
    }
}

/*
// проверка методов TransportCatalogue GetBusInfo и методов private TransportCatalogue
void GetOptimalRoute() {
    TransportCatalogue catalogue;

    // Остановки для примера
    const Stop A = {"Biryulyovo Zapadnoye", geo::Coordinates{}};
    const Stop B = {"Biryulyovo Tovarnaya", geo::Coordinates{}};
    const Stop C = {"Universam", geo::Coordinates{}};
    const Stop E = {"Prazhskaya", geo::Coordinates{}};

    // Добавим все остановки
    for (const auto& stop : {A, B, C, E}) {
        catalogue.AddStop(stop);
    }

    // Создаем вектор указателей на остановки для прямого маршрута ABCE
    std::vector<const Stop*> route_BCE; 
    for (const auto& stop : {B, C, E}) {
        route_BCE.push_back(catalogue.FindStop(stop.name));
    }

    const Bus BCE = {false, "635", route_BCE}; // Передаем указатели на остановки

    // Создаем вектор указателей на остановки для кольцевого маршрута ABCA
    std::vector<const Stop*> route_ABCA; 
    for (const auto& stop : {A, B, C, A}) {
        route_ABCA.push_back(catalogue.FindStop(stop.name));
    }

    const Bus ABCA = {true, "297", route_ABCA}; // Передаем указатели на остановки
    
    // Добавим два маршрута
    for (const auto& bus : {ABCA, BCE}) {
        catalogue.AddBus(bus);
    }

    // Добавим дистанции между остановками
    catalogue.SetDistance("A"sv, "B"sv, 2600); 
    catalogue.SetDistance("B"sv, "C"sv, 890);
    catalogue.SetDistance("C"sv, "A"sv, 2500);
    catalogue.SetDistance("C"sv, "B"sv, 1380);
    catalogue.SetDistance("C"sv, "E"sv, 4650);
    
    // Зададим параметры маршрутизации 
    TransportRouter router(catalogue);
    router.SetVertexCount(4);

    RouterSettings settings;
    settings.bus_velocity_ = 40;
    settings.bus_wait_time_ = 6;
    router.SetRouterSettings(settings);

    // Построим маршрутизатор
    router.BuildTransportRouter();

    // Проверим маршрут A->C
    // Проверяем значения с точностью до 0.001
    const double EPSILON = 0.001;
    const auto info_AC = router.GetOptimalRoute(A.name, C.name); 
    ASSERT(info_AC.has_value());
    ASSERT(std::abs(info_AC.value().time - 11.235) < EPSILON);
    const auto route_AC = info_AC.value().route_edges;
    ASSERT_EQUAL(route_AC.size(), 2);
    
}
*/

void RunTests() {
    //geo
    RUN_TEST(TestComputeDistance);

    // tc
    RUN_TEST(TestFindBus); 
    RUN_TEST(TestFindStop);
    RUN_TEST(TestGetDistance);
    RUN_TEST(TestGetBusInfo);
    RUN_TEST(TestGetStopInfo);

    // mr
    RUN_TEST(TestSphereProjector);

    // ro
    //RUN_TEST(GetOptimalRoute);

    std::cerr << std::endl << "All tests passed successfully!"s << std::endl << std::endl;
}

} //namespace transport_catalogue::tests