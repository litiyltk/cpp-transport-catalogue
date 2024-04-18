#include <iostream>

#include "json_reader.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "tests.h"


int main() {

    tests::RunTests();

    transport_catalogue::TransportCatalogue catalogue;
    map_renderer::MapRendererSVG renderer;
    transport_router::TransportRouter router(catalogue);

    request_handler::RequestHandler handler(catalogue, renderer, router);

    json_reader::JsonReader reader(handler);

    reader.LoadFromJson(std::cin); 
    reader.PrintIntoJson(std::cout);
    
}