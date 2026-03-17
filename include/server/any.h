// C++可以保存任意类型的容器
#pragma once
#include "./public.h"
class Any
{
private:
    class Base
    {
    public:
        virtual ~Base();
        virtual std::type_info const &GetType() const = 0;
        virtual Base *Clone() const = 0;
    };

    template <class T>
    class Placeholder : public Base
    {
    public:
        Placeholder(T const &value);
        virtual std::type_info const &GetType() const;
        virtual Base *Clone() const;
        T value;
    };
    Base *_content;

public:
    Any();
    template <class T>
    Any(T const &value);
    Any(Any const &other);
    ~Any();

    Any &Swap(Any &other);

    template <class T>
    T *Get();

    template <class T>
    Any &operator=(T const &value);

    Any &operator=(Any const &other);
};