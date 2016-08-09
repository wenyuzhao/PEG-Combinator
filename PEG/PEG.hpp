#pragma once

#include <string>
#include <memory>
#include <experimental/optional>
#include <utility>
#include <type_traits>



namespace std {
    namespace experimental {};
    using namespace std::experimental;
}

namespace peg {

    
    
    
    
// helper functions

template <class T, class Input>
inline auto resolve(const T& t, Input input) {
    return std::make_optional(std::make_pair(t, input));
}

template <class T, class Input>
inline auto resolve(const std::optional<std::pair<T, Input>>& t) {
    return t;
}

template <class T, class Input>
inline auto resolve(std::optional<std::pair<T, Input>>&& t) {
    return t;
}

template <class T, class Input>
inline auto resolve(T&& t, Input input) {
    return std::make_optional(std::make_pair(t, input));
}

template <class T = std::nullopt_t>
inline T reject() {
    return std::nullopt;
}

// operator >> to map parser results

#define MAP(T, CAPTURE)                                              \
template <class Parser, class F>                                     \
inline auto operator >> (Parser T p, F f) {                          \
    return [CAPTURE p, f] (auto input, auto end) {                   \
        auto x = p(input, end);                                      \
        return x ? resolve(f(x->first), x->second) : std::nullopt;   \
    };                                                               \
}

MAP(&&, )
MAP(&, &)

#undef MAP

// choice parser combinator

#define CHOICE(T1, CAPTURE1, T2, CAPTURE2)                      \
template <class ParserA1, class ParserA2>                       \
inline auto operator | (ParserA1 T1 a, ParserA2 T2 b) {         \
    return [CAPTURE1 a, CAPTURE2 b] (auto input, auto end) {    \
        static_assert(std::is_same<decltype(a(input, end)),     \
            decltype(b(input, end))>::value,                    \
            "Parser combinators at both sides of operator | "   \
            "should have same parse result types.");            \
        auto result_a = a(input, end);                          \
        if (result_a) return result_a;                          \
        auto result_b = b(input, end);                          \
        return result_b ? resolve(result_b) : reject();         \
    };                                                          \
}

CHOICE(&&, , &&, )
CHOICE(&&, , &, &)
CHOICE(&, &, &&, )
CHOICE(&, &, &, &)

#undef CHOICE

// optional parser combinator

#define OPT(REF, CAPTURE)                                                     \
template <class Parser>                                                       \
inline auto opt(Parser REF p) {                                               \
    return [CAPTURE p](auto input, auto end) {                                \
        auto x = p(input, end);                                               \
        return x ? resolve(std::make_optional(x->first), x->second)           \
                 : resolve(std::optional<decltype(x->first)>(std::nullopt),   \
                        input);                                               \
    };                                                                        \
};

OPT(&&, )
OPT(&, &)

#undef OPT

// helper functions

template <class T>
inline auto as_tuple(T&& t) {
    return std::make_tuple(t);
}

template <class T>
inline auto as_tuple(const T& t) {
    return std::make_tuple(t);
}

template <class... Ts>
inline auto as_tuple(std::tuple<Ts...> t) {
    return t;
}

// sequence parser combinator

#define SEQUENCE(T1, CAPTURE1, T2, CAPTURE2)                                   \
template <class ParserA, class ParserB>                                        \
inline auto operator , (ParserA T1 a, ParserB T2 b) {                          \
    return [CAPTURE1 a, CAPTURE2 b] (auto input, auto end) -> std::optional<   \
            decltype(std::make_pair(std::tuple_cat(                            \
                as_tuple(a(input, end)->first),                                \
                as_tuple(b(input, end)->first)), input))> {                    \
        if (auto x = a(input, end)) {                                          \
            auto y = b(x->second, end);                                        \
            return y ? std::make_optional(std::make_pair(std::tuple_cat(       \
                        as_tuple(x->first), as_tuple(y->first)), y->second))   \
                     : reject();                                               \
        } else {                                                               \
            return reject();                                                   \
        }                                                                      \
    };                                                                         \
}

SEQUENCE(&&, , &&, )
SEQUENCE(&&, , &, &)
SEQUENCE(&, &, &&, )
SEQUENCE(&, &, &, &)

#undef SEQUENCE

// repeat parser combinator

#define REPEAT(T, CAPTURE)                                                \
template <class Parser>                                                   \
inline auto repeat(Parser T p, std::size_t n = 0) {                       \
    return [CAPTURE p, n](auto i, auto end) -> std::optional<std::pair<   \
            std::vector<decltype(p(end, end)->first)>, decltype(i)>> {    \
        std::vector<decltype(p(end, end)->first)> ts;                     \
        auto input = i;                                                   \
        for (std::size_t i = 0; i < n; i++) {                             \
            if (auto x = p(input, end)) {                                 \
                ts.push_back(x->first);                                   \
                input = x->second;                                        \
            } else {                                                      \
                return reject();                                          \
            }                                                             \
        }                                                                 \
        while (true) {                                                    \
            if (auto x = p(input, end)) {                                 \
                ts.push_back(x->first);                                   \
                input = x->second;                                        \
            } else {                                                      \
                break;                                                    \
            }                                                             \
        }                                                                 \
        return resolve(ts, input);                                        \
    };                                                                    \
}

REPEAT(&&, )
REPEAT(&, &)

#undef REPEAT

template <class Parser>
inline auto repeat0(Parser&& p) {
    return repeat(std::move(p), 0);
}
template <class Parser>
inline auto repeat0(Parser& p) {
    return repeat(p, 0);
}
template <class Parser>
inline auto repeat1(Parser&& p) {
    return repeat(std::move(p), 1);
}
template <class Parser>
inline auto repeat1(Parser& p) {
    return repeat(p, 1);
}




template <class Input, class ResultT> using Rule =
    std::function<std::optional<std::pair<ResultT, Input>>(Input, Input)>;

// error recovering utilities

class Panic {
    struct X {
        template <class F> auto operator >> (F f) { return f; }
    };
public:
    auto operator ! () { return X {}; }
} panic;



    
    
}
