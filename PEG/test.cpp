#include <iostream>     // std::cin, std::cout
#include <sstream>
#include <tuple>
#include <string>
#include <regex>
#include <cmath>

#include "PEG.hpp"



using namespace std;
using namespace peg;

#define LOG >> [](auto x) { cout << x; return x; }



struct ExprGrammar: Grammar<string, double> {
    
    Token NUMBER  = token("number", R"([0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?)"),
          ADD     = token("'+'", R"(\+)"),
          SUB     = token("'-'", R"(\-)"),
          MUL     = token("'*'", R"(\*)"),
          DIV     = token("'/'", R"(\/)"),
          L_PAREN = token("'('", R"(\()"),
          R_PAREN = token("')'", R"(\))");

    OptToken _ = token_opt("space", R"([ \r\t\n]+)");
    
    Rule<double> Number, Primary, Unary, Term, Expr;

    
    ExprGrammar() {
        Number = NUMBER >> [](auto x) {
                    return std::stod(x);
               }
               ;
        
        Primary = (L_PAREN, _, Expr, _, R_PAREN) >> [](auto x) {
                    return get<2>(x);
                }
                | Number
                ;
    
        Unary = (ADD | SUB, _, Unary) >> [](auto t) {
                    return get<0>(t) == "+" ? get<2>(t) : - get<2>(t);
              }
              | Primary
              ;
    
        Term = (Unary, *(_, MUL | DIV, _, Unary)) >> [](auto t) {
                auto ans = get<0>(t);
                for (auto& term : get<1>(t)) {
                    if (get<1>(term) == "*") {
                        ans *= get<3>(term);
                    } else {
                        ans /= get<3>(term);
                    }
                }
                return ans;
             }
             ;
    
        Expr = (Term , *(_ , ADD | SUB, _, Term)) >> [](auto t) {
                auto ans = get<0>(t);
                for (auto& term : get<1>(t)) {
                    if (get<1>(term) == "+") {
                        ans += get<3>(term);
                    } else {
                        ans -= get<3>(term);
                    }
                }
                return ans;
             }
             ;
        
        this->setStartRule(Expr);
    }
};


int main () {
    // initialize parser
    ExprGrammar grammar;
    
    // expression
    string expr = "12 - 3 * (2 + 1) - 4";
    
    // parse
    auto result = grammar.parse(expr);
    double ans = result ? *result : NAN;

    // print result
    cout << ans << endl;
    
    return 0;
}

 
