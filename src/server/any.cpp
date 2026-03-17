#include "../include/any.h"

Any::Base::~Base() {}

template <class T>
Any::Placeholder<T>::Placeholder(T const &value) : value(value) {}

template <class T>
std::type_info const &Any::Placeholder<T>::GetType() const { return typeid(T); }

template <class T>
Any::Base *Any::Placeholder<T>::Clone() const { return new Placeholder(value); }

Any::Any() : _content(nullptr) {}

template <class T>
Any::Any(T const &value) : _content(new Placeholder<T>(value)) {}

Any::Any(Any const &other) : _content(other._content ? other._content->Clone() : nullptr) {}

Any::~Any() { delete _content; }

Any &Any::Swap(Any &other)
{
    std::swap(_content, other._content);
    return *this;
}

template <class T>
T *Any::Get()
{
    if (_content && _content->GetType() == typeid(T))
        return &static_cast<Placeholder<T> *>(_content)->value;
    return nullptr;
}

template <class T>
Any &Any::operator=(T const &value)
{
    Any(value).Swap(*this);
    return *this;
}

Any &Any::operator=(Any const &other)
{
    Any(other).Swap(*this);
    return *this;
}
