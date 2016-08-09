# PEG-Combinator
Parsing Expression Grammar in c++14 based on parser combinators

## Example Usage

### 1. Define Rules

```c++
using Input = std::string::const_iterator;
    
Rule<Input, string> NUMBER  = R"([0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?)"_r,
                    ADD     = R"(\+)"_r,   SUB     = R"(\-)"_r,
                    MUL     = R"(\*)"_r,   DIV     = R"(\/)"_r,
                    L_PAREN = R"(\()"_r,   R_PAREN = R"(\))"_r;
Rule<Input, optional<string>> SPACE_OPT = opt(R"([ \r\t\n]+)"_r);
    
Rule<Input, double> Number, Primary, Unary, Term, Expr;

void init_parser() {
    Number = NUMBER >> [](auto x) {
        return stod(x);
    };
        
    Primary = Number
            | (L_PAREN, SPACE_OPT, Expr, SPACE_OPT, R_PAREN) >> [](auto x) {
                return get<2>(x);
            }
            ;
        
    Unary = (ADD | SUB, SPACE_OPT, Unary) >> [](auto t) {
                return get<0>(t) == "+" ? get<2>(t) : - get<2>(t);
          }
          | Primary
          ;

    Term = (Unary, SPACE_OPT, MUL | DIV, SPACE_OPT, Term) >> [](auto t) {
             return get<2>(t) == "*" ?
                get<0>(t) * get<4>(t) : get<0>(t) / get<4>(t);
         }
         | Unary
         ;
        
    Expr = (Term, SPACE_OPT, ADD | SUB, SPACE_OPT, Expr) >> [](auto t) {
             return get<2>(t) == "+" ?
                get<0>(t) + get<4>(t) : get<0>(t) - get<4>(t);
         }
         | Term
         ;
}
```

### 2. Initialize parser and parse

```c++
int main () {
    // initialize parser
    init_parser();
    
    // expression
    string expr = "12 + (-5 - - - (5 + 12 / 3) / 3)";
    
    // parse
    auto parse_result = Expr(expr.cbegin(), expr.cend()); // decltype(parse_result) = optional<pair<double, Input>>
    double ans = parse_result ? parse_result->first : NAN;

    // print result
    cout << ans << endl;
    
    return 0;
}
```

### See full code [here](https://github.com/wenyuzhao/PEG-Combinator/blob/master/PEG/test.cpp)