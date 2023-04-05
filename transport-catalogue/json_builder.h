#pragma once

#include "json.h"
#include <vector>

namespace json {

class ArrayContext;
class DictKeyContext;
class DictValueContext;


class Builder {
public:
    Builder();

    template <class T>
    Builder& Value(T val) {
        if (nodes_stack_.empty()) {
            throw std::logic_error("Value: Stack is empty!");
        } else {
            Node& node = *nodes_stack_.back();
            if (node.IsArray()) {
                Array& arr = const_cast<Array&>(node.AsArray());
                arr.push_back(val);
            } else if (node.IsDict()) {
                throw std::logic_error("Can`t add Value to dict. Need Key!");
            } else {
                node = val;
                nodes_stack_.pop_back();
            }
        }
        return *this;
    }

    ArrayContext StartArray();

    Builder& EndArray();

    DictKeyContext StartDict();

    Builder& EndDict();

    DictValueContext Key(std::string key);

    Node Build();
protected:
    std::vector<Node*> nodes_stack_;
    Node root_;

};

class DictKeyContext {
public:
    DictKeyContext(Builder& builder);

    DictValueContext Key(std::string key);

    Builder& EndDict();

private:
    Builder& builder_;
};

class DictValueContext {
public:
    DictValueContext(Builder& builder);

    template <class T>
    DictKeyContext Value(T val) {
        builder_.Value(val);
        return DictKeyContext(builder_);
    }

    DictKeyContext StartDict();

    ArrayContext StartArray();

private:
    Builder& builder_;
};

class ArrayContext {
public:
    ArrayContext(Builder& builder);

    template<class T>
    ArrayContext Value(T val) {
        builder_.Value(val);
        return *this;
    }

    ArrayContext StartArray();

    DictKeyContext StartDict();

    Builder& EndArray();

private:
    Builder& builder_;
};
} //namespace json
