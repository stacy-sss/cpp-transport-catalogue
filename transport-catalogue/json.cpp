#include "json.h"
#include <variant>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cctype>

using namespace std;

namespace json {

    namespace {

        void PrintNode(const Node& node, ostream& out);

        //функция для экранирования строк
        string EscapeString(const string& s) {
            ostringstream out;
            for (unsigned char c : s) {
                switch (c) {
                case '"':  out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '\b': out << "\\b"; break;
                case '\f': out << "\\f"; break;
                case '\n': out << "\\n"; break;
                case '\r': out << "\\r"; break;
                case '\t': out << "\\t"; break;
                default:
                    if (c < 0x20) {
                        out << "\\u" << hex << setw(4) << setfill('0')
                            << static_cast<int>(c);
                    }
                    else {
                        out << c;
                    }
                    break;
                }
            }
            return out.str();
        }

        // Перегрузки PrintValue для разных типов
        void PrintValue(std::nullptr_t, std::ostream& out) {
            out << "null"sv;
        }

        void PrintValue(bool value, std::ostream& out) {
            out << (value ? "true"sv : "false"sv);
        }

        void PrintValue(int value, std::ostream& out) {
            out << value;
        }

        void PrintValue(double value, std::ostream& out) {
            out << value;
        }

        void PrintValue(const std::string& value, std::ostream& out) {
            out << '"' << EscapeString(value) << '"';
        }

        // Рекурсивные вызовы для Array и Dict
        void PrintValue(const Array& array, std::ostream& out) {
            out << "["sv;
            for (size_t i = 0; i < array.size(); ++i) {
                if (i > 0) {
                    out << ", "sv;
                }
                PrintNode(array[i], out);
            }
            out << "]"sv;
        }

        void PrintValue(const Dict& dict, std::ostream& out) {
            out << "{"sv;
            bool first = true;
            for (const auto& [key, value] : dict) {
                if (!first) {
                    out << ", "sv;
                }
                first = false;
                out << '"' << EscapeString(key) << "\": "sv;
                PrintNode(value, out);
            }
            out << "}"sv;
        }

        void PrintNode(const Node& node, std::ostream& out) {
            std::visit(
                [&out](const auto& value) { PrintValue(value, out); },
                node.GetValue()
            );
        }

        Node LoadNode(istream& input);

        Node LoadArray(istream& input) {
            Array result;
            input >> ws;
            if (input.peek() == ']') {
                input.get();
                return Node(move(result));
            }

            while (true) {
                result.push_back(LoadNode(input));
                input >> ws;
                if (input.peek() == ']') {
                    input.get();
                    break;
                }
                if (input.peek() == ',') {
                    input.get();
                    input >> ws;
                }
                else {
                    throw ParsingError("Expected ',' or ']' in array");
                }
            }
            return Node(move(result));
        }

        Node LoadInt(istream& input) {
            string parsed_num;

            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream");
                }
                };

            auto read_digits = [&input, read_char] {
                if (!isdigit(input.peek())) {
                    throw ParsingError("A digit is expected");
                }
                while (isdigit(input.peek())) {
                    read_char();
                }
                };

            // Знак
            if (input.peek() == '-') {
                read_char();
            }

            // Целая часть
            if (input.peek() == '0') {
                read_char();
            }
            else {
                read_digits();
            }

            bool is_double = false;

            // Дробная часть
            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_double = true;
            }

            // Экспонента
            if (int ch = input.peek(); ch == 'e' || ch == 'E') {
                read_char();
                if (ch = input.peek(); ch == '+' || ch == '-') {
                    read_char();
                }
                read_digits();
                is_double = true;
            }

            if (is_double) {
                return Node(stod(parsed_num));
            }
            else {
                return Node(stoi(parsed_num));
            }
        }

        string LoadString(istream& input) {
            string result;
            char c;

            while (input.get(c)) {
                if (c == '"') {
                    return result;
                }
                if (c == '\\') {
                    if (!input.get(c)) {
                        throw ParsingError("Unexpected end of string after escape");
                    }
                    switch (c) {
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    default: result += c; break;
                    }
                }
                else {
                    result += c;
                }
            }

            throw ParsingError("Unterminated string");
        }

        Node LoadDict(istream& input) {
            Dict result;
            input >> ws;
            if (input.peek() == '}') {
                input.get();
                return Node(move(result));
            }

            while (true) {
                if (input.peek() != '"') {
                    throw ParsingError("Expected string key in dict");
                }
                input.get();
                string key = LoadString(input);
                input >> ws;
                if (input.peek() != ':') {
                    throw ParsingError("Expected ':' after key");
                }
                input.get();
                input >> ws;
                result.insert({ move(key), LoadNode(input) });
                input >> ws;
                if (input.peek() == '}') {
                    input.get();
                    break;
                }
                if (input.peek() == ',') {
                    input.get();
                    input >> ws;
                }
                else {
                    throw ParsingError("Expected ',' or '}' in dict");
                }
            }
            return Node(move(result));
        }

        Node LoadNode(istream& input) {
            input >> ws;
            if (!input) {
                throw ParsingError("Unexpected end of input");
            }

            char c = input.peek();

            if (c == '[') {
                input.get();
                return LoadArray(input);
            }
            else if (c == '{') {
                input.get();
                return LoadDict(input);
            }
            else if (c == '"') {
                input.get();
                string str = LoadString(input);
                return Node(str);
            }
            else if (c == 't') {
                string word;
                for (int i = 0; i < 4; ++i) {
                    if (!input.get(c)) {
                        throw ParsingError("Unexpected end of input while parsing true");
                    }
                    word += c;
                }
                if (word != "true") {
                    throw ParsingError("Expected true");
                }

                if (isalnum(input.peek()) || input.peek() == '_') {
                    throw ParsingError("Invalid token after true");
                }
                return Node(true);
            }
            else if (c == 'f') {

                string word;
                for (int i = 0; i < 5; ++i) {
                    if (!input.get(c)) {
                        throw ParsingError("Unexpected end of input while parsing false");
                    }
                    word += c;
                }
                if (word != "false") {
                    throw ParsingError("Expected false");
                }

                if (isalnum(input.peek()) || input.peek() == '_') {
                    throw ParsingError("Invalid token after false");
                }
                return Node(false);
            }
            else if (c == 'n') {

                string word;
                for (int i = 0; i < 4; ++i) {
                    if (!input.get(c)) {
                        throw ParsingError("Unexpected end of input while parsing null");
                    }
                    word += c;
                }
                if (word != "null") {
                    throw ParsingError("Expected null");
                }

                if (isalnum(input.peek()) || input.peek() == '_') {
                    throw ParsingError("Invalid token after null");
                }
                return Node(nullptr);
            }
            else if (isdigit(c) || c == '-') {
                return LoadInt(input);
            }

            throw ParsingError("Unexpected character in JSON");
        }
    }  // namespace

    // Конструкторы Node
    Node::Node(Array array) : data_(move(array)) {}
    Node::Node(Dict map) : data_(move(map)) {}
    Node::Node(int value) : data_(value) {}
    Node::Node(double value) : data_(value) {}
    Node::Node(bool value) : data_(value) {}
    Node::Node(string value) : data_(move(value)) {}
    Node::Node(std::nullptr_t) : data_(nullptr) {}


    const Node::Variant& Node::GetValue() const {
        return data_;
    }

    // Проверки типа
    bool Node::IsInt() const {
        return holds_alternative<int>(data_);
    }

    bool Node::IsDouble() const {
        return holds_alternative<double>(data_) || holds_alternative<int>(data_);
    }

    bool Node::IsPureDouble() const {
        return holds_alternative<double>(data_);
    }

    bool Node::IsBool() const {
        return holds_alternative<bool>(data_);
    }

    bool Node::IsString() const {
        return holds_alternative<string>(data_);
    }

    bool Node::IsNull() const {
        return holds_alternative<std::nullptr_t>(data_);
    }

    bool Node::IsArray() const {
        return holds_alternative<Array>(data_);
    }

    bool Node::IsMap() const {
        return holds_alternative<Dict>(data_);
    }


    const Array& Node::AsArray() const {
        if (!IsArray()) {
            throw logic_error("Not an array");
        }
        return get<Array>(data_);
    }

    const Dict& Node::AsMap() const {
        if (!IsMap()) {
            throw logic_error("Not a map");
        }
        return get<Dict>(data_);
    }

    int Node::AsInt() const {
        if (!IsInt()) {
            throw logic_error("Not an int");
        }
        return get<int>(data_);
    }

    bool Node::AsBool() const {
        if (!IsBool()) {
            throw logic_error("Not a bool");
        }
        return get<bool>(data_);
    }

    double Node::AsDouble() const {
        if (IsInt()) {
            return static_cast<double>(get<int>(data_));
        }
        if (IsPureDouble()) {
            return get<double>(data_);
        }
        throw logic_error("Not a double");
    }

    const string& Node::AsString() const {
        if (!IsString()) {
            throw logic_error("Not a string");
        }
        return get<string>(data_);
    }

    // Document
    Document::Document(Node root) : root_(move(root)) {}

    const Node& Document::GetRoot() const {
        return root_;
    }
    bool operator==(const Node& lhs, const Node& rhs) {
        return lhs.GetValue() == rhs.GetValue();
    }

    bool operator!=(const Node& lhs, const Node& rhs) {
        return !(lhs == rhs);
    }

    Document Load(istream& input) {
        return Document{ LoadNode(input) };
    }

    bool operator==(const Document& lhs, const Document& rhs) {
        return lhs.GetRoot() == rhs.GetRoot();
    }

    bool operator!=(const Document& lhs, const Document& rhs) {
        return !(lhs == rhs);
    }
    void Print(const Document& doc, std::ostream& output) {
        PrintNode(doc.GetRoot(), output);
    }

}  // namespace json