#include "json.h"

namespace json {

namespace {

//----------------------Load functions--------------------

Node LoadNode(std::istream& input);

Node LoadArray(std::istream& input) {
    using namespace std::literals;
    Array result;
    char c;

    for (; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }

    if (c != ']') {
        throw ParsingError("Expected ]"s);
    }

    return Node(std::move(result));
}

using Number = std::variant<int, double>;

Number LoadNumber(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    if (input.peek() == '0') {
        read_char();
    } else {
        read_digits();
    }

    bool is_int = true;
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            try {
                return std::stoi(parsed_num);
            } catch (...) {
            }
        }
        return std::stod(parsed_num);
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

std::string LoadString(std::istream& input) {
    using namespace std::literals;

    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            ++it;
            break;
        } else if (ch == '\\') {
            ++it;
            if (it == end) {
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            throw ParsingError("Unexpected end of line"s);
        } else {
            s.push_back(ch);
        }
        ++it;
    }

    return s;
}

Node LoadDict(std::istream& input) {
    using namespace std::literals;
    Dict result;

    char c;
    for (; input >> c && c != '}';) {
        if (c == ',') {
            input >> c;
        }

        std::string key = LoadString(input);
        input >> c;
        result.insert({std::move(key), LoadNode(input)});
    }

    if (c != '}') {
        throw json::ParsingError("Expected }"s);
    }

    return Node(std::move(result));
}

Node LoadNull(std::istream& input) {
    char c_u;
    char c_l1;
    char c_l2;
    input >> c_u >> c_l1 >> c_l2;
    if (c_u == 'u' && c_l1 == 'l' && c_l2 == 'l') {
        return nullptr;
    }
    throw json::ParsingError("Expected \"null\"");
}

Node LoadBool(std::istream& input) {
    char nxt;
    input >> nxt;
    if (nxt == 'r') {
        std::string res;
        input >> nxt;
        res += nxt;
        input >> nxt;
        res += nxt;
        if (res == "ue") {
            return true;
        }
    } else if (nxt == 'a') {
        std::string res;
        input >> nxt;
        res += nxt;
        input >> nxt;
        res += nxt;
        input >> nxt;
        res += nxt;
        if (res == "lse") {
            return false;
        }
    }
    throw json::ParsingError("Expected true or false");
}

Node LoadNode(std::istream& input) {
    char c;
    input >> c;

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == 'n') {
        return LoadNull(input);
    } else if (c == 't' || c == 'f') {
        return LoadBool(input);
    } else {
        input.putback(c);
        Number num = LoadNumber(input);
        if (std::holds_alternative<int>(num)) {
            return std::get<int>(num);
        } else {
            return std::get<double>(num);
        }
    }
}

}  // namespace

Document Load(std::istream& input) {
    return Document{LoadNode(input)};
}

//----------------------Load functions--------------------

//---------------------------Node-------------------------

Node::Node()
    : data_(nullptr) {
}

Node::Node(std::nullptr_t null)
    : data_(null) {
}

Node::Node(int value)
    : data_(value) {
}

Node::Node(double value)
    : data_(value) {
}

Node::Node(std::string value)
    : data_(std::move(value)) {
}

Node::Node(bool value)
    : data_(std::move(value)) {
}

Node::Node(Array array)
    : data_(std::move(array)) {
}

Node::Node(Dict map)
    : data_(std::move(map)) {
}

bool Node::IsNull() const {
    return std::holds_alternative<std::nullptr_t>(data_);
}

bool Node::IsInt() const {
    return std::holds_alternative<int>(data_);
}

bool Node::IsDouble() const {
    return std::holds_alternative<double>(data_) || IsInt();
}

bool Node::IsPureDouble() const {
    return std::holds_alternative<double>(data_);
}

bool Node::IsString() const {
    return std::holds_alternative<std::string>(data_);
}

bool Node::IsBool() const {
    return std::holds_alternative<bool>(data_);
}

bool Node::IsArray() const {
    return std::holds_alternative<Array>(data_);
}

bool Node::IsDict() const {
    return std::holds_alternative<Dict>(data_);
}

    //--------------------As-----------------------

int Node::AsInt() const {
    if (!IsInt()) {
        throw std::logic_error("AsInt: it`s not int");
    }
    return std::get<int>(data_);
}

double Node::AsDouble() const {
    if (IsPureDouble()) {
        return std::get<double>(data_);
    } else if (IsInt()) {
        return std::get<int>(data_);
    }
    throw std::logic_error("AsDouble: not double or int!");
}

const std::string& Node::AsString() const {
    if (!IsString()) {
        throw std::logic_error("AsString: it`s not std::string");
    }
    return std::get<std::string>(data_);
}

bool Node::AsBool() const {
    if (!IsBool()) {
        throw std::logic_error("AsBool: it`s not bool");
    }
    return std::get<bool>(data_);
}

const Array& Node::AsArray() const {
    if (!IsArray()) {
        throw std::logic_error("AsArray: it`s not Array");
    }
    return std::get<Array>(data_);
}

const Dict& Node::AsDict() const {
    if (!IsDict()) {
        throw std::logic_error("AsMap: it`s not Dict");
    }
    return std::get<Dict>(data_);
}

const Data& Node::GetData() const {
    return data_;
}

bool Node::operator==(const Node& other) const {
    return *this == other;
}

bool Node::operator!=(const Node& other) const {
    return !(*this == other);
}

//---------------------------Node-------------------------

//-------------------------Document-----------------------

Document::Document(Node root)
    : root_(std::move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

bool Document::operator==(const Document& other) const {
    return root_ == other.root_;
}


void Print(const Document& doc, std::ostream& output) {
    PrintContext context{output};
    std::visit(NodePrinter{context}, doc.GetRoot().GetData());
}

//-------------------------Document-----------------------

//------------------------NodePrinter---------------------

NodePrinter::NodePrinter(PrintContext context) : context_(context) {}

void NodePrinter::operator()(std::nullptr_t) const {
    context_.out << "null";
}

void NodePrinter::operator()(int value) const {
    context_.out << value;
}

void NodePrinter::operator()(double value) const {
    context_.out << value;
}

void NodePrinter::operator()(const std::string& value) const {
    context_.out << "\"" << NormText(value) << "\"";
}

void NodePrinter::operator()(bool value) const {
    context_.out << (value ? "true" : "false");
}

void NodePrinter::operator()(const Array& array) const {
    PrintContext new_context = context_.Indented();
    NodePrinter printer(new_context);
    context_.out << "[" << std::endl;
    bool is_first = true;
    for(const Node& node : array) {
        if (!is_first) {
            new_context.out << "," << std::endl;
        }
        is_first = false;
        new_context.PrintIndent();
        std::visit(printer, node.GetData());
    }
    context_.out << std::endl;
    context_.PrintIndent();
    context_.out << "]";
}

void NodePrinter::operator()(const Dict& map) const {
    PrintContext new_context = context_.Indented();
    NodePrinter printer(new_context);
    context_.out << "{" << std::endl;
    bool is_first = true;
    for(const std::pair<const std::string, Node>& value : map) {
        if (!is_first) {
            new_context.out << "," << std::endl;
        }
        is_first = false;
        new_context.PrintIndent();
        new_context.out << "\"" << value.first << "\": ";
        std::visit(printer, value.second.GetData());
    }
    context_.out << std::endl;
    context_.PrintIndent();
    context_.out << "}";
}



std::string NodePrinter::NormText(const std::string& text) const {
    std::string res;
    using namespace std::literals;
    res.reserve(text.size());

    for (char c : text) {
        if (c == '\n') {
            res += "\\n"s;
        } else if (c == '\r') {
            res += "\\r"s;
        } else if (c == '\"') {
            res += "\\\""s;
        } else if (c == '\\') {
            res += "\\\\"s;
        } else {
            res += c;
        }
    }
    return res;
}

//------------------------NodePrinter---------------------
}  // namespace json
