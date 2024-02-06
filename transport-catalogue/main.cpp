#include <iostream>
#include <string>

#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"

using namespace std;


int main() {
    
    transport_catalogue::TransportCatalogue catalogue;
    transport_catalogue::input_reader::InputReader reader;

    reader.InputRequests(cin, catalogue);

    transport_catalogue::stat_reader::OutputRequests(catalogue, cin, cout);

}