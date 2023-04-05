#include "json_builder.h"

namespace json {

Builder::Builder() {
    nodes_stack_.push_back(&root_);
}

Builder& Builder::EndArray() {
    if (nodes_stack_.empty()) {
        throw std::logic_error("EndArray: Stack is empty!");
    } else {
        Node& node = *nodes_stack_.back();
        if (node.IsArray()) {
            nodes_stack_.pop_back();
        } else {
            throw std::logic_error("EndArray: Can`t close Array. On the top of Stack not Array!");
        }
    }
    return *this;
}

Builder& Builder::EndDict() {
    if (nodes_stack_.empty()) {
        throw std::logic_error("EndDict: Stack is empty!");
    } else {
        Node& node = *nodes_stack_.back();
        if (node.IsDict()) {
            nodes_stack_.pop_back();
        } else {
            throw std::logic_error("EndDict: Can`t close Dict. On the top of Stack not Dict!");
        }
    }
    return *this;
}

Node Builder::Build() {
    if (!nodes_stack_.empty()) {
        throw std::logic_error("Stack should be empty for Build!");
    }
    return root_;
}

ArrayContext Builder::StartArray() {
    if (nodes_stack_.empty()) {
        throw std::logic_error("StartArray: Stack is empty!");
    } else {
        Node& node = *nodes_stack_.back();
        if (node.IsArray()) {
            Array& arr = const_cast<Array&>(node.AsArray());
            arr.push_back(Array{});
            nodes_stack_.push_back(&arr.back());
        } else if (node.IsDict()) {
            throw std::logic_error("StartArray: Can`t add Array to dict. Need Key!");
        } else {
            nodes_stack_.pop_back();
            node = Array{};
            nodes_stack_.push_back(&node);
        }
    }
    return ArrayContext(*this);
}

DictKeyContext::DictKeyContext(Builder& builder) : builder_(builder) {}

Builder& DictKeyContext::EndDict() {
    builder_.EndDict();
    return builder_;
}

DictKeyContext Builder::StartDict() {
    if (nodes_stack_.empty()) {
        throw std::logic_error("StartDict: Stack is empty!");
    } else {
        Node& node = *nodes_stack_.back();
        if (node.IsArray()) {
            Array& arr = const_cast<Array&>(node.AsArray());
            arr.push_back(Dict{});
            nodes_stack_.push_back(&arr.back());
        } else if (node.IsDict()) {
            throw std::logic_error("StartDict: Can`t add Dict to dict. Need Key!");
        } else {
            nodes_stack_.pop_back();
            node = Dict{};
            nodes_stack_.push_back(&node);
        }
    }
    return DictKeyContext(*this);
}

DictValueContext::DictValueContext(Builder& builder) : builder_(builder) {}

DictKeyContext DictValueContext::StartDict() {
    builder_.StartDict();
    return DictKeyContext(builder_);
}

DictValueContext Builder::Key(std::string key) {
    if (nodes_stack_.empty()) {
        throw std::logic_error("Key: Stack is empty!");
    } else {
        Node& node = *nodes_stack_.back();
        if (node.IsDict()) {
            Dict& dict = const_cast<Dict&>(node.AsDict());
            nodes_stack_.push_back(&dict[key]);
        } else {
            throw std::logic_error("Key: Can`t add a Key not to the Dict!");
        }
    }
    return DictValueContext(*this);
}

DictValueContext DictKeyContext::Key(std::string key) {
    builder_.Key(key);
    return DictValueContext(builder_);
}

ArrayContext::ArrayContext(Builder& builder) : builder_(builder) {}

ArrayContext ArrayContext::StartArray() {
    builder_.StartArray();
    return *this;
}

DictKeyContext ArrayContext::StartDict() {
    builder_.StartDict();
    return DictKeyContext(builder_);
}

Builder& ArrayContext::EndArray() {
    builder_.EndArray();
    return builder_;
}

ArrayContext DictValueContext::StartArray () {
    builder_.StartArray();
    return ArrayContext(builder_);
}

} //namespace json
