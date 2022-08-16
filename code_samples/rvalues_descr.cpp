// #define SEND_VAL_RECV_VAL
// #define SEND_RVAL_RECV_VAL
// #define SEND_RVAL_RECV_RVAL
#define SEND_RVAL_RECV_RVAL_UB

#include <iostream>
#include <string>
#include <vector>

// only for thread delay in copy ctor
#include <chrono>
#include <thread>

class ClassName
{
public:
    ClassName()
    {
        std::cout << "Default ctor\n";
    }

    ClassName(int const max_letter_number)
    : v(max_letter_number, "aaa"), s("Vector of triplets")
    {
        std::cout << "User defined ctor\n";
    }

    ClassName(ClassName const&other)
    : v(other.v), s(other.s)
    {
        std::cout <<"Copy ctor\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout <<"Copy ctor done\n";
    }

    ClassName& operator=(ClassName const&other)
    {
        std::cout << "Copy assignment\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        v = other.v;
        s = other.s;
        std::cout << "Copy assignment done\n";
        return *this;
    }

    ClassName(ClassName &&other)
    : v(std::move(other.v)), s(std::move(other.s))
    {
        std::cout <<"Move ctor\n";
    }

    ClassName& operator=(ClassName &&other)
    {
        std::cout << "Move assignment\n";
        v = std::move(other.v);
        s = std::move(other.s);
        return *this;
    }

    std::size_t get_size() const
    {
        return v.size();
    }

    void add_two()
    {
        v.push_back("ddd");
        v.push_back("fff");
        return;
    }

private:
    std::vector<std::string> v;
    std::string s;
};

#if defined SEND_VAL_RECV_VAL || defined SEND_RVAL_RECV_VAL

void foo(ClassName arg)
{
    std::cout << "foo\n";
    arg.add_two();
    std::cout << arg.get_size() << "\n";
    return;
}

#elif defined SEND_RVAL_RECV_RVAL

void foo(ClassName &&arg)
{
    std::cout << "foo\n";
    arg.add_two();
    std::cout << arg.get_size() << "\n";
    return;
}

#elif defined SEND_RVAL_RECV_RVAL_UB

void foo(ClassName &&arg)
{
    std::cout << "foo\n";
    auto local_arg = std::move(arg); // воруем
    std::cout << local_arg.get_size() << "\n";
    return;
}

#endif

int main()
{
    std::cout << "Start\n";

    ClassName instance1(3);

#if defined SEND_VAL_RECV_VAL

    foo(instance1);
    std::cout << instance1.get_size() << "\n";

#elif defined SEND_RVAL_RECV_VAL

    foo(std::move(instance1));
    std::cout << instance1.get_size() << "\n"; // так делать нельзя, это UB

#elif defined SEND_RVAL_RECV_RVAL || defined SEND_RVAL_RECV_RVAL_UB

    foo(std::move(instance1));
    std::cout << instance1.get_size() << "\n"; // UB в случае SEND_RVAL_RECV_RVAL_UB

#endif

    std::cout << "Finish\n";
    return 0;
}
