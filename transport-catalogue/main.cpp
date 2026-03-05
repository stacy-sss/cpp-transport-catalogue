#include <iostream>
#include <string>

#include "input_reader.h"
#include "stat_reader.h"

using namespace std;

int main() {
    TransportCatalogue transport_catalogue;

    reader::Read(transport_catalogue, cin);

    stat::StatRequest(transport_catalogue, cin, cout);
}