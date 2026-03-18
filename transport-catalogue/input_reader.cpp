#include "input_reader.h"
#include <algorithm>
#include <cassert>
#include <iterator>

namespace reader {
    /**
     * Парсит строку вида "10.123,  -30.1837" и возвращает пару координат (широта, долгота)
     */
    geo::Coordinates ParseCoordinates(std::string_view str) {
        static const double nan = std::nan("");
        auto space_pos = str.find(' ');
        if (space_pos == str.npos) {
            return { nan, nan };
        }

        double lat = std::stod(std::string(str.substr(0, space_pos)));

        auto start_lng = str.find_first_not_of(' ', space_pos);

        //конец второго числа
        auto end_lng = str.find_first_of(" ,", start_lng);
        if (end_lng == str.npos) {
            end_lng = str.length();
        }

        double lng = std::stod(std::string(str.substr(start_lng, end_lng - start_lng)));

        return { lat, lng };
    }

    /**
     * Удаляет пробелы в начале и конце строки
     */
    std::string_view Trim(std::string_view string) {
        const auto start = string.find_first_not_of(' ');
        if (start == string.npos) {
            return {};
        }
        return string.substr(start, string.find_last_not_of(' ') + 1 - start);
    }

    /**
     * Разбивает строку string на n строк, с помощью указанного символа-разделителя delim
     */
    std::vector<std::string_view> Split(std::string_view string, char delim) {
        std::vector<std::string_view> result;

        size_t pos = 0;
        while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
            auto delim_pos = string.find(delim, pos);
            if (delim_pos == string.npos) {
                delim_pos = string.size();
            }
            if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
                result.push_back(substr);
            }
            pos = delim_pos + 1;
        }

        return result;
    }

    /**
     * Парсит маршрут.
     * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок [A,B,C,A]
     * Для некольцевого маршрута (A-B-C-D) возвращает массив названий остановок [A,B,C,D,C,B,A]
     */
    std::vector<std::string_view> ParseRoute(std::string_view route) {
        if (route.find('>') != route.npos) {
            return Split(route, '>');
        }

        auto stops = Split(route, '-');
        std::vector<std::string_view> results(stops.begin(), stops.end());
        results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

        return results;
    }

    CommandDescription ParseCommandDescription(std::string_view line) {
        auto colon_pos = line.find(':');
        if (colon_pos == line.npos) {
            return {};
        }

        auto space_pos = line.find(' ');
        if (space_pos >= colon_pos) {
            return {};
        }

        auto not_space = line.find_first_not_of(' ', space_pos);
        if (not_space >= colon_pos) {
            return {};
        }
        //имя команды
        std::string command = std::string(line.substr(0, space_pos));
        //название остановки или автобуса
        std::string id = std::string(line.substr(not_space, colon_pos - not_space));
        //после двоеточия
        std::string_view cont = Trim(line.substr(colon_pos + 1));
        auto first_comma = cont.find(',');
        if (first_comma == cont.npos) {
            return CommandDescription{ command, id, std::string(cont) };
        }
        std::string_view cords = Trim(cont.substr(0, first_comma));
        std::string_view dist = Trim(cont.substr(first_comma + 1));
        std::string description = std::string(cords);
        if (!dist.empty()) {
            description += " " + std::string(dist);
        }
        return CommandDescription{ command, id, description };
    }

    void InputReader::ParseLine(std::string_view line) {
        auto command_description = ParseCommandDescription(line);
        if (command_description) {
            commands_.push_back(std::move(command_description));
        }
    }

    void Read(TransportCatalogue& catalogue, std::istream& input) {
        int base_request_count;
        input >> base_request_count >> std::ws;
        InputReader reader;
        for (int i = 0; i < base_request_count; ++i) {
            std::string line;
            std::getline(input, line);
            reader.ParseLine(line);
        }
        reader.ApplyCommands(catalogue);
    }

    void InputReader::ApplyCommands([[maybe_unused]] TransportCatalogue& catalogue) const {
        for (const auto& com : commands_) {
            if (com.command == "Stop") {
                geo::Coordinates cord = ParseCoordinates(com.description);
                catalogue.AddStop(com.id, cord);
            }
        }
        for (const auto& com : commands_) {
            if (com.command == "Stop") {
                std::string_view desc = com.description;

                auto cord_end = desc.find(',');
                std::string_view distan = Trim(desc.substr(cord_end + 1));
                std::vector<std::string_view> distances = Split(distan, ',');

                for (const auto& dist_str : distances) {
                    auto dist_trimmed = Trim(dist_str);
                    auto m_pos = dist_trimmed.find('m');

                    // Расстояние
                    std::string dist_value = std::string(Trim(dist_trimmed.substr(0, m_pos)));

                    int distance = std::stoi(dist_value);
                    auto to_pos = dist_trimmed.find("to", m_pos);

                    std::string to_stop = std::string(Trim(dist_trimmed.substr(to_pos + 2)));

                    const Stop* from_stop = catalogue.FindStop(com.id);
                    const Stop* to_stop_ptr = catalogue.FindStop(to_stop);

                    if (from_stop && to_stop_ptr) {
                        catalogue.AddDistance(from_stop, to_stop_ptr, distance);
                    }
                }
            }
        }
        for (const auto& com : commands_) {
            if (com.command == "Bus") {
                auto route_stops = ParseRoute(com.description);

                std::vector<std::string> stop_names;
                for (auto stops : route_stops) {
                    stop_names.push_back(std::string(stops));
                }

                catalogue.AddBus(com.id, stop_names);
            }
        }
    }

}//reader