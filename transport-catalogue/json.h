#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>
#include <cassert>

namespace json {

class Node;
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;
using Data = std::variant<std::nullptr_t, int, double, std::string, bool, Array, Dict>;

class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node {
public:
    Node();
    Node(std::nullptr_t);
    Node(int value);
    Node(double value);
    Node(std::string value);
    Node(bool value);
    Node(Array array);
    Node(Dict map);

    int AsInt() const;
    double AsDouble() const;
    const std::string& AsString() const;
    bool AsBool() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    bool IsNull() const;
    bool IsInt() const;
    bool IsDouble() const;
    bool IsPureDouble() const;
    bool IsString() const;
    bool IsBool() const;
    bool IsArray() const;
    bool IsMap() const;

    bool operator==(const Node& other) const;
    bool operator!=(const Node& other) const;

    const Data& GetData() const;

private:
    template <class T>
    bool Compare(const Node& other) const {
        if (std::holds_alternative<T>(data_)
            && std::holds_alternative<T>(other.data_))
        {
            return std::get<T>(data_) == std::get<T>(other.data_);
        } else {
            return false;
        }
    }

    Data data_;
};

class Document {
public:
    explicit Document(Node root);
    const Node& GetRoot() const;
    bool operator==(const Document& other) const;
private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

struct PrintContext {
    std::ostream& out;
    int indent_step = 4;
    int indent = 0;

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    PrintContext Indented() const {
        return {out, indent_step, indent_step + indent};
    }
};

class NodePrinter{
public:
    NodePrinter(PrintContext context);
    void operator()(std::nullptr_t) const;
    void operator()(int value) const;
    void operator()(double value) const;
    void operator()(const std::string& value) const;
    void operator()(bool value) const;
    void operator()(const Array& array) const;
    void operator()(const Dict& map) const;
private:
    std::string NormText(const std::string& text) const;
    PrintContext context_;
};

}  // namespace json
