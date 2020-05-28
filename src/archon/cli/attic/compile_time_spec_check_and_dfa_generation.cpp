// Requires C++20

#include <cstddef>
#include <algorithm>
#include <array>
#include <tuple>


namespace cli {

template<class T> struct Assign {};

template<class T> Assign<T> assign(T& var)
{
    static_cast<void>(var);
    return {};
}

template<class C, std::size_t N> struct StrLit {
    constexpr StrLit(const C(&str)[N])
    {
        std::copy_n(str, N, value.data());
    }

    std::array<C, N> value;
};

template<StrLit S> struct Str {};
template<StrLit S> inline constexpr Str<S> s = {};

template<class S, class F> class Pattern {};
template<class S, class A> class Option {};

template<class S, class F> Pattern<S, F> pat(S spec, const char* desc, F func)
{
    static_cast<void>(spec);
    static_cast<void>(desc);
    static_cast<void>(func);
    return {};
}

template<class S, class A> Option<S, A> opt(S spec, const char* desc, A action)
{
    static_cast<void>(spec);
    static_cast<void>(desc);
    static_cast<void>(action);
    return {};
}

template<class... T> int process(int argc, char* argv[], const std::tuple<T...>& spec)
{
    static_cast<void>(argc);
    static_cast<void>(argv);
    static_cast<void>(spec);
    return 0;
}

} // namespace cli


int main(int argc, char* argv[])
{
    float x, y;

    using cli::s;
    std::tuple spec = {
//        opt(cli::help),
        pat(s<"foo <val>">, "Lorem ipsum.", [](int) {
            // ...
        }),
        opt(s<"-x">, "Lorem ipsum.", cli::assign(x)),
        pat(s<"bar <val>">, "Lorem ipsum.", [](int) {
            // ...
        }),
        opt(s<"--y">, "Lorem ipsum.", cli::assign(y)),
    };

    return cli::process(argc, argv, spec);
}
