#pragma once

#include <string>
#include <memory>
#include <experimental/optional>
#include <utility>
#include <type_traits>
#include <regex>
#include <iostream>
#include <set>
#include <functional>


namespace std {
    namespace experimental {};
    using namespace std::experimental;
}

namespace peg {
    
    using namespace std;

template <class StringT>
struct Input {
    using Iter = typename StringT::const_iterator;
    using CharT = typename StringT::value_type;
    static Iter it() {
        return Iter();
    }
    Iter iter;
    std::size_t row, col;
    Input(Iter iter, std::size_t row = 1, std::size_t col = 1): iter(iter), row(row), col(col) {}
    Input advance(std::size_t n) const {
        if (*iter == CharT('\n')) {
            return Input(iter + n, row + 1, 0);
        } else {
            return Input(iter + n, row, col + 1);
        }
    }
};
    
template <class I, class O>
using Result = std::tuple<std::optional<O>, I, std::set<std::string>>; // (result?, input, expected tokens)

template <class I, class O>
using Combinator = std::function<Result<I, O>(I, I)>;

// operator >> to map parser results

#define MAP(REF, CAPTURE) \
template <class I, class O, class Mapper> \
inline auto operator >> (Combinator<I, O> REF p, Mapper f) -> Combinator<I, decltype(f(*get<0>(p(I(I::it()), I(I::it())))))> { \
    return [CAPTURE p, f] (I input, I end) { \
        auto&& x = p(input, end); \
        return make_tuple(get<0>(x) ? make_optional(f(*get<0>(x))) : nullopt, std::get<1>(x), std::get<2>(x)); \
    }; \
}

MAP(&, &)
MAP(&&, )
    
#undef MAP

// choice parser combinator

#define CHOICE(REF1, CAPTURE1, REF2, CAPTURE2) \
template <class I, class O> \
inline Combinator<I, O> operator | (Combinator<I, O> REF1 a, Combinator<I, O> REF2 b) { \
    return [CAPTURE1 a, CAPTURE2 b] (I input, I end) { \
        auto&& result_a = a(input, end); \
        if (get<0>(result_a)) { \
            return result_a; \
        } else { \
            auto&& result_b = b(input, end); \
            get<2>(result_b).insert(get<2>(result_a).begin(), get<2>(result_a).end()); \
            return result_b; \
        } \
    }; \
}

CHOICE( &, &,  &, &)
CHOICE( &, &, &&,  )
CHOICE(&&,  ,  &, &)
CHOICE(&&,  , &&,  )

#undef CHOICE
    
    
#define OPT(REF, CAPTURE) \
template <class I, class O> \
inline Combinator<I, std::optional<O>> opt(Combinator<I, O> REF p) { \
    return [CAPTURE p](I input, I end) { \
        auto&& x = p(input, end); \
        get<2>(x).clear(); \
        return make_tuple(make_optional(get<0>(x)), get<1>(x), get<2>(x)); \
    }; \
};

OPT(&, &)
OPT(&&, )
    
#undef OPT

// sequence parser combinator

#define SEQUENCE(REF1, CAPTURE1, REF2, CAPTURE2) \
template <class I, class O1, class O2> \
inline Combinator<I, std::tuple<O1, O2>> operator , (Combinator<I, O1> REF1 a, Combinator<I, O2> REF2 b) { \
    return [CAPTURE1 a, CAPTURE2 b] (I input, I end) { \
        auto&& x = a(input, end); \
        if (get<0>(x)) { \
            auto&& y = b(get<1>(x), end); \
            return make_tuple( \
                get<0>(y) ? make_optional(make_tuple(*get<0>(x), *get<0>(y))): nullopt, \
                get<1>(y), get<2>(y) \
            ); \
        } else { \
            return make_tuple(std::optional<std::tuple<O1, O2>>(), get<1>(x), get<2>(x)); \
        } \
    }; \
}
    
SEQUENCE( &, &,  &, &)
SEQUENCE( &, &, &&,  )
SEQUENCE(&&,  ,  &, &)
SEQUENCE(&&,  , &&,  )

#undef SEQUENCE
    
#define SEQUENCE(REF1, CAPTURE1, REF2, CAPTURE2) \
template <class I, class O2, class... O1> \
inline Combinator<I, std::tuple<O1..., O2>> operator , (Combinator<I, std::tuple<O1...>> REF1 a, Combinator<I, O2> REF2 b) { \
    return [CAPTURE1 a, CAPTURE2 b] (I input, I end) { \
        auto&& x = a(input, end); \
        if (get<0>(x)) { \
            auto&& y = b(get<1>(x), end); \
            return make_tuple( \
                get<0>(y) ? make_optional(std::tuple_cat(*get<0>(x), std::make_tuple(*get<0>(y)))): nullopt, \
                get<1>(y), get<2>(y) \
            ); \
        } else { \
            return make_tuple(std::optional<std::tuple<O1..., O2>>(), get<1>(x), get<2>(x)); \
        } \
    }; \
}
    
SEQUENCE( &, &,  &, &)
SEQUENCE( &, &, &&,  )
SEQUENCE(&&,  ,  &, &)
SEQUENCE(&&,  , &&,  )
    
#undef SEQUENCE

// repeat parser combinator


#define REPEAT(REF, CAPTURE) \
template <class I, class O> \
inline Combinator<I, std::vector<O>> repeat(Combinator<I, O> REF p, std::size_t n = 0) { \
    return [CAPTURE p, n](I _input, I end) { \
        std::vector<O> ts; \
        auto input = _input; \
        std::set<std::string> s; \
        for (std::size_t i = 0; i < n; i++) { \
            auto&& x = p(input, end); \
            auto $ = p; \
            if (get<0>(x)) { \
                ts.push_back(*get<0>(x)); \
                input = get<1>(x); \
                s.swap(get<2>(x)); \
            } else { \
                return make_tuple(std::optional<std::vector<O>>(), _input, std::move(s)); \
            } \
        } \
        while (true) { \
            auto&& x = p(input, end); \
            if (get<0>(x)) { \
                ts.push_back(*get<0>(x)); \
                input = get<1>(x); \
                s.swap(get<2>(x)); \
            } else { \
                break; \
            } \
        } \
        return make_tuple(make_optional(ts), input, std::move(s)); \
    }; \
}

    
REPEAT(&, &)
REPEAT(&&, )
    
#undef REPEAT

    

template <class I, class O>
inline Combinator<I, std::vector<O>> operator * (Combinator<I, O>& p) {
    return repeat(p, 0);
}
template <class I, class O>
inline Combinator<I, std::vector<O>> operator + (Combinator<I, O>& p) {
    return repeat(p, 1);
}

template <class I, class O>
inline Combinator<I, std::vector<O>> operator * (Combinator<I, O>&& p) {
    return repeat(std::move(p), 0);
}
template <class I, class O>
inline Combinator<I, std::vector<O>> operator + (Combinator<I, O>&& p) {
    return repeat(std::move(p), 1);
}
    



    

template <class StringT>
Combinator<Input<StringT>, StringT> make_regex_parser(const StringT& str, string name) {
    using CharT = typename StringT::value_type;
    basic_regex<CharT> re(str[0] != CharT('^') ? CharT('^') + str : str);
    return [re, name](Input<StringT> input, Input<StringT> end) {
        regex_iterator<typename StringT::const_iterator> re_it(input.iter, end.iter, re), re_end;
        if (re_it != re_end) {
            auto str = re_it->str();
            return make_tuple(make_optional(str), input.advance(str.size()), set<string>());
        } else {
            return make_tuple(std::optional<StringT>(), input, set<string>({ name }));
        }
    };
}
    
    
template<class StringT, class Result>
class Grammar {
private:
    using Input = Input<StringT>;
    using CharT = typename StringT::value_type;
public:
    template<class O>
    using Rule = Combinator<Input, O>;
    
    
    using Token = Rule<StringT>;
    using OptToken = Rule<optional<StringT>>;
    
    
    Rule<StringT> token(string name, const CharT* str) const {
        return make_regex_parser(std::basic_string<CharT>(str), name);
    }
    Rule<optional<StringT>> token_opt(string name, const CharT* str) const {
        return opt(make_regex_parser(std::basic_string<CharT>(str), name));
    }
    
    
    
    void setStartRule(Rule<Result>& start) {
        start_rule = &start;
    }
    virtual std::optional<Result> parse(const StringT& input) const {
        auto&& result = (*start_rule)(Input(input.cbegin()), Input(input.cend()));
        if (!get<0>(result) || get<1>(result).iter != input.cend()) {
            cerr << "expected one of";
            for (string s : get<2>(result))
                cerr << " " << s;
            cerr << ".\n";
        }
        return get<0>(result);
    }

private:
    Rule<Result>* start_rule;
};
    
    
    
    
}
