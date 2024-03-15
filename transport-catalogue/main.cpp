#include <iostream>

#include "json_reader.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
//#include "tests.h"


int main() {

    //tests::Tests();

    transport_catalogue::TransportCatalogue catalogue;
    map_renderer::MapRendererSVG renderer;

    request_handler::RequestHandler handler(catalogue, renderer);
    
    json_reader::JsonReader reader(handler);
    reader.LoadFromJson(std::cin);
    reader.PrintIntoJson(std::cout);
    
}