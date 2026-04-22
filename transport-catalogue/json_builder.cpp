#include "json_builder.h"

namespace json {

    Builder::Builder() : built_(false), has_root_(false) {}

    KeyContext Builder::Key(std::string key) {
        CheckNotBuilt();
        if (stack_.empty() || stack_.back().type != Context::DICT) {
            throw std::logic_error("Key() called outside a dict");
        }
        if (!stack_.back().expect_key) {
            throw std::logic_error("Key() called when value expected");
        }
        current_key_ = std::move(key);
        stack_.back().expect_key = false;
        return *this;
    }

    Builder& Builder::Value(Node value) {
        return ValueInternal(std::move(value));
    }

    Builder& Builder::ValueInternal(Node value) {
        CheckNotBuilt();
        Node node(std::move(value));

        if (stack_.empty() && !has_root_) {
            root_ = std::move(node);
            has_root_ = true;
        }
        else if (!stack_.empty() && stack_.back().type == Context::DICT && !stack_.back().expect_key) {
            auto& dict = const_cast<Dict&>(stack_.back().node->AsMap());
            dict.emplace(std::move(current_key_), std::move(node));
            stack_.back().expect_key = true;
        }
        else if (!stack_.empty() && stack_.back().type == Context::ARRAY) {
            auto& array = const_cast<Array&>(stack_.back().node->AsArray());
            array.push_back(std::move(node));
        }
        else {
            throw std::logic_error("Value() called in wrong context");
        }

        return *this;
    }

    DictContext Builder::StartDict() {
        CheckNotBuilt();

        Dict dict;
        Node node(std::move(dict));

        if (stack_.empty() && !has_root_) {
            root_ = std::move(node);
            has_root_ = true;
            stack_.push_back(Context{ Context::DICT, true, &root_ });
        }
        else if (!stack_.empty() && stack_.back().type == Context::DICT && !stack_.back().expect_key) {
            auto& dict_parent = const_cast<Dict&>(stack_.back().node->AsMap());
            auto [it, _] = dict_parent.emplace(std::move(current_key_), std::move(node));
            stack_.back().expect_key = true;
            stack_.push_back(Context{ Context::DICT, true, &it->second });
        }
        else if (!stack_.empty() && stack_.back().type == Context::ARRAY) {
            auto& array = const_cast<Array&>(stack_.back().node->AsArray());
            array.push_back(std::move(node));
            stack_.push_back(Context{ Context::DICT, true, &array.back() });
        }
        else {
            throw std::logic_error("StartDict() called in wrong context");
        }

        return *this;
    }

    ArrayContext Builder::StartArray() {
        CheckNotBuilt();

        Array arr;
        Node node(std::move(arr));

        if (stack_.empty() && !has_root_) {
            root_ = std::move(node);
            has_root_ = true;
            stack_.push_back(Context{ Context::ARRAY, false, &root_ });
        }
        else if (!stack_.empty() && stack_.back().type == Context::DICT && !stack_.back().expect_key) {
            auto& dict_parent = const_cast<Dict&>(stack_.back().node->AsMap());
            auto [it, _] = dict_parent.emplace(std::move(current_key_), std::move(node));
            stack_.back().expect_key = true;
            stack_.push_back(Context{ Context::ARRAY, false, &it->second });
        }
        else if (!stack_.empty() && stack_.back().type == Context::ARRAY) {
            auto& array = const_cast<Array&>(stack_.back().node->AsArray());
            array.push_back(std::move(node));
            stack_.push_back(Context{ Context::ARRAY, false, &array.back() });
        }
        else {
            throw std::logic_error("StartArray() called in wrong context");
        }

        return *this;
    }

    Builder& Builder::EndDict() {
        CheckNotBuilt();
        if (stack_.empty() || stack_.back().type != Context::DICT) {
            throw std::logic_error("EndDict() called outside a dict");
        }
        if (!stack_.back().expect_key) {
            throw std::logic_error("EndDict() called after Key without Value");
        }
        stack_.pop_back();
        return *this;
    }

    Builder& Builder::EndArray() {
        CheckNotBuilt();
        if (stack_.empty() || stack_.back().type != Context::ARRAY) {
            throw std::logic_error("EndArray() called outside an array");
        }
        stack_.pop_back();
        return *this;
    }

    Node Builder::Build() {
        CheckNotBuilt();
        if (!stack_.empty()) {
            throw std::logic_error("Build() called with unclosed containers");
        }
        if (!has_root_) {
            throw std::logic_error("Build() called on empty builder");
        }
        built_ = true;
        return root_;
    }

    void Builder::CheckNotBuilt() const {
        if (built_) {
            throw std::logic_error("Builder already finalized");
        }
    }

} // namespace json