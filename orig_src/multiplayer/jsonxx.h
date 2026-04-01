#ifndef MULTIPLAYER_JSONXX_H
#define MULTIPLAYER_JSONXX_H

#include <string>

namespace jsonxx
{
    typedef double Number;
    typedef bool Boolean;
    typedef std::string String;

    class Object;

    class Array
    {
    public:
        bool parse(const std::string& text) { (void)text; return true; }
        size_t size() const { return 0; }

        template <typename T>
        T get(size_t index) const
        {
            (void)index;
            return T();
        }

        Array& operator<<(const Object& value)
        {
            (void)value;
            return *this;
        }

        std::string json() const { return std::string("[]"); }
    };

    class Object
    {
    public:
        bool parse(const std::string& text) { (void)text; return true; }

        template <typename T>
        bool has(const std::string& key) const
        {
            (void)key;
            return false;
        }

        template <typename T>
        T get(const std::string& key) const
        {
            (void)key;
            return T();
        }

        Object& operator<<(const std::string& value)
        {
            (void)value;
            return *this;
        }

        Object& operator<<(const char* value)
        {
            (void)value;
            return *this;
        }

        Object& operator<<(Boolean value)
        {
            (void)value;
            return *this;
        }

        template <typename T>
        Object& operator<<(T value)
        {
            (void)value;
            return *this;
        }

        Object& operator<<(const Object& value)
        {
            (void)value;
            return *this;
        }

        Object& operator<<(const Array& value)
        {
            (void)value;
            return *this;
        }

        std::string json() const { return std::string("{}"); }
    };
}

#endif
