#include <iostream>

#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"


int main() {

    transport_catalogue::TransportCatalogue catalogue;
    transport_catalogue::input_reader::InputReader reader;

    reader.InputRequests(std::cin, catalogue);

    transport_catalogue::stat_reader::OutputRequests(catalogue, std::cin, std::cout);
    
}