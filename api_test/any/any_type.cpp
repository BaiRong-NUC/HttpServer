// C++可以保存任意类型的容器
#include <algorithm>
#include <iostream>
#include <string>
#include <assert.h>
#include <unistd.h>
class Any
{
private:
    class Base
    {
    public:
        virtual ~Base() {}
        virtual std::type_info const &GetType() const = 0;
        virtual Base *Clone() const = 0;
    };

    template <class T>
    class Placeholder : public Base
    {
    public:
        Placeholder(T const &value) : value(value) {}
        virtual std::type_info const &GetType() const { return typeid(T); }
        virtual Base *Clone() const { return new Placeholder(value); }
        T value;
    };
    Base *_content;

public:
    Any() : _content(nullptr) {}
    template <class T>
    Any(T const &value) : _content(new Placeholder<T>(value)) {}
    Any(Any const &other) : _content(other._content ? other._content->Clone() : nullptr) {}
    ~Any() { delete _content; }

    Any &Swap(Any &other)
    {
        std::swap(_content, other._content);
        return *this;
    }; // 交换函数

    template <class T>
    T *Get()
    {
        if (_content && _content->GetType() == typeid(T))
            return &static_cast<Placeholder<T> *>(_content)->value;
        return nullptr;
    }; // 获取保存的值

    template <class T>
    Any &operator=(T const &value)
    {
        Any(value).Swap(*this);
        return *this;
    }; // 赋值操作符

    Any &operator=(Any const &other)
    {
        Any(other).Swap(*this);
        return *this;
    }; // 复制赋值操作符
};

class Test
{
public:
    Test()
    {
        std::cout << "Test Constructor" << std::endl;
    }
    Test(const Test &other)
    {
        std::cout << "Test Copy Constructor" << std::endl;
    }
    ~Test()
    {
        std::cout << "Test Destructor" << std::endl;
    }
};

int main(int argc, char const *argv[])
{
    Any a = 1;
    std::cout << *a.Get<int>() << std::endl; // 输出1
    Any b = std::string("Hello World");
    std::cout << *b.Get<std::string>() << std::endl;

    // 数组类型不支持
    // Any c = "Hello World";
    // std::cout << *c.Get<const char *>() << std::endl;

    // 验证内存泄漏
    // {
    //     Any d = Test();
    // }

    Any e = nullptr;
    {
        Test d;
        e = d;
    }
    while (true)
        sleep(1);
    return 0;
}
