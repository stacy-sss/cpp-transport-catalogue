#include <iostream>
#include "transport_catalogue.h"
#include "json_reader.h"
#include "json.h"

int main() {
    transport_catalogue::TransportCatalogue catalog;
    json::Document input_doc = json::Load(std::cin);
    json_reader::JsonReader reader(catalog);
    json::Document output_doc = reader.Process(input_doc.GetRoot());
    json::Print(output_doc, std::cout);
    return 0;
}