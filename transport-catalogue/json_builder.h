#pragma once
#include "json.h"
#include <string>
#include <vector>

namespace json {

    class Builder;
    class KeyContext;
    class DictContext;
    class ArrayContext;

    class BaseContext {
    public:
        BaseContext(Builder& builder) : builder_(builder) {}
    protected:
        Builder& builder_;
    };

    class KeyContext : public BaseContext {
    public:
        KeyContext(Builder& builder) : BaseContext(builder) {}
        DictContext Value(Node value);
        DictContext StartDict();
        ArrayContext StartArray();
    };

    class DictContext : public BaseContext {
    public:
        DictContext(Builder& builder) : BaseContext(builder) {}
        KeyContext Key(std::string key);
        Builder& EndDict();
    };

    class ArrayContext : public BaseContext {
    public:
        ArrayContext(Builder& builder) : BaseContext(builder) {}
        ArrayContext Value(Node value);
        DictContext StartDict();
        ArrayContext StartArray();
        Builder& EndArray();
    };

    class Builder {
    public:
        Builder();
        DictContext StartDict();
        ArrayContext StartArray();
        Builder& Value(Node value);
        KeyContext Key(std::string key);
        Builder& EndDict();
        Builder& EndArray();
        Node Build();

    private:
        struct Context {
            enum Type { ARRAY, DICT };
            Type type;
            bool expect_key;
            Node* node;
            Context(Type t, bool ek, Node* n) : type(t), expect_key(ek), node(n) {}
        };

        std::vector<Context> stack_;
        Node root_;
        bool built_ = false;
        bool has_root_ = false;
        std::string current_key_;

        void CheckNotBuilt() const;
        Builder& ValueInternal(Node value);
        friend class KeyContext;
        friend class DictContext;
        friend class ArrayContext;
    };

    // Inline implementations
    inline KeyContext DictContext::Key(std::string key) {
        return builder_.Key(std::move(key));
    }

    inline Builder& DictContext::EndDict() {
        return builder_.EndDict();
    }

    inline DictContext KeyContext::Value(Node value) {
        builder_.ValueInternal(std::move(value));
        return DictContext(builder_);
    }

    inline DictContext KeyContext::StartDict() {
        return builder_.StartDict();
    }

    inline ArrayContext KeyContext::StartArray() {
        return builder_.StartArray();
    }

    inline ArrayContext ArrayContext::Value(Node value) {
        builder_.ValueInternal(std::move(value));
        return ArrayContext(builder_);
    }

    inline DictContext ArrayContext::StartDict() {
        return builder_.StartDict();
    }

    inline ArrayContext ArrayContext::StartArray() {
        return builder_.StartArray();
    }

    inline Builder& ArrayContext::EndArray() {
        return builder_.EndArray();
    }

} // namespace json