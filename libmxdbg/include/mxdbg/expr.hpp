/* 
    MXDBG - Debugger with AI 
    coded by Jared Bruni (jaredbruni@protonmail.com)
    https://lostsidedead.biz
*/
#ifndef __EXPR__H_
#define __EXPR__H_
#include<iostream>
#include<string>
#include<unordered_map>
#include"mxdbg/scanner.hpp"

 namespace expr_parser {
        
        enum class ExprTokenType {
            END, NUMBER, PLUS, MINUS, MUL, DIV, MOD, LPAREN, RPAREN,
            PLUSEQ, MINUSEQ, XOR, OR, AND,
            LOGICAL_AND, LOGICAL_OR, NOT,
            EQ, NEQ, GT, GTE, LT, LTE,
            VARIABLE
        };

        struct ExprToken {
            ExprTokenType type;
            uint64_t value; 
        };

    
        inline std::unordered_map<std::string,  uint64_t> vars;

        class ExprLexer {
        public:
            std::string variable;
            ExprLexer(const std::string& input) : scanner(input), pos(0) {
                scanner.scan();
                next();
            }

            void next() {
                if (pos >= scanner.size()) {
                    current = {ExprTokenType::END, 0};
                    return;
                }
                auto t = scanner[pos++];
                auto val = t.getTokenValue();
                if (t.getTokenType() == types::TokenType::TT_NUM) {
                    current = {ExprTokenType::NUMBER, std::stoull(val)};
                } else if(t.getTokenType() == types::TokenType::TT_HEX) {
                    current = {ExprTokenType::NUMBER, std::stoull(val, nullptr, 16)};
                } else if (t.getTokenType() == types::TokenType::TT_ID) { 
                    current = {ExprTokenType::VARIABLE, 0};
                    variable = val;
                } else if (val == "+") {
                    current = {ExprTokenType::PLUS, 0};
                } else if (val == "-") {
                    current = {ExprTokenType::MINUS, 0};
                } else if (val == "*") {
                    current = {ExprTokenType::MUL, 0};
                } else if (val == "/") {
                    current = {ExprTokenType::DIV, 0};
                } else if (val == "%") {
                    current = {ExprTokenType::MOD, 0};
                } else if (val == "(") {
                    current = {ExprTokenType::LPAREN, 0};
                } else if (val == ")") {
                    current = {ExprTokenType::RPAREN, 0};
                } else if (val == "+=") {
                    current = {ExprTokenType::PLUSEQ, 0};
                } else if (val == "-=") {
                    current = {ExprTokenType::MINUSEQ, 0};
                } else if (val == "^") {
                    current = {ExprTokenType::XOR, 0};
                } else if (val == "|") {
                    current = {ExprTokenType::OR, 0};
                } else if (val == "&") {
                    current = {ExprTokenType::AND, 0};
                } else if (val == "&&") {
                    current = {ExprTokenType::LOGICAL_AND, 0};
                } else if (val == "||") {
                    current = {ExprTokenType::LOGICAL_OR, 0};
                } else if (val == "!") {
                    current = {ExprTokenType::NOT, 0};
                } 
                else if (val == "==") {
                    current = {ExprTokenType::EQ, 0};
                } else if (val == "!=") {
                    current = {ExprTokenType::NEQ, 0};
                } else if (val == ">") {
                    current = {ExprTokenType::GT, 0};
                } else if (val == ">=") {
                    current = {ExprTokenType::GTE, 0};
                } else if (val == "<") {
                    current = {ExprTokenType::LT, 0};
                } else if (val == "<=") {
                    current = {ExprTokenType::LTE, 0};
                }
                else {
                    current = {ExprTokenType::END, 0};
                }
            }

            ExprToken peek() const { return current; }
            void consume() { next(); }

        private:
            scan::Scanner scanner;
            size_t pos;
            ExprToken current;
        };

        class ExprParser {
        public:
            ExprParser(ExprLexer& lexer) : lexer(lexer) {}
            uint64_t parse() { return parseLogical(); }

        private:
            ExprLexer& lexer;

            uint64_t parseAdd() {
                uint64_t val = parseTerm();
                while (lexer.peek().type == ExprTokenType::PLUS ||
                    lexer.peek().type == ExprTokenType::MINUS) {
                    if (lexer.peek().type == ExprTokenType::PLUS) {
                        lexer.consume();
                        val += parseTerm();
                    } else {
                        lexer.consume();
                        val -= parseTerm();
                    }
                }
                return val;
            }

            uint64_t parseBitwise() {
                uint64_t val = parseAdd();  
                while (lexer.peek().type == ExprTokenType::XOR ||
                    lexer.peek().type == ExprTokenType::OR  ||
                    lexer.peek().type == ExprTokenType::AND) {
                    if (lexer.peek().type == ExprTokenType::XOR) {
                        lexer.consume();
                        val ^= parseAdd();
                    } else if (lexer.peek().type == ExprTokenType::OR) {
                        lexer.consume();
                        val |= parseAdd();
                    } else {  
                        lexer.consume();
                        val &= parseAdd();
                    }
                }
                return val;
            }

            uint64_t parseComparison() {
                uint64_t val = parseBitwise();
                while (lexer.peek().type == ExprTokenType::EQ ||
                       lexer.peek().type == ExprTokenType::NEQ ||
                       lexer.peek().type == ExprTokenType::GT ||
                       lexer.peek().type == ExprTokenType::GTE ||
                       lexer.peek().type == ExprTokenType::LT ||
                       lexer.peek().type == ExprTokenType::LTE) {
                    if (lexer.peek().type == ExprTokenType::EQ) {
                        lexer.consume();
                        val = (val == parseBitwise());
                    } else if (lexer.peek().type == ExprTokenType::NEQ) {
                        lexer.consume();
                        val = (val != parseBitwise());
                    } else if (lexer.peek().type == ExprTokenType::GT) {
                        lexer.consume();
                        val = (val > parseBitwise());
                    } else if (lexer.peek().type == ExprTokenType::GTE) {
                        lexer.consume();
                        val = (val >= parseBitwise());
                    } else if (lexer.peek().type == ExprTokenType::LT) {
                        lexer.consume();
                        val = (val < parseBitwise());
                    } else {  
                        lexer.consume();
                        val = (val <= parseBitwise());
                    }
                }
                return val;
            }

            uint64_t parseLogical() {
                uint64_t val = parseComparison();
                while (lexer.peek().type == ExprTokenType::LOGICAL_OR ||
                       lexer.peek().type == ExprTokenType::LOGICAL_AND) {
                    if (lexer.peek().type == ExprTokenType::LOGICAL_OR) {
                        lexer.consume();
                        val = val || parseComparison();
                    } else {
                        lexer.consume();
                        val = val && parseComparison();
                    }
                }
                return val;
            }

            uint64_t parseTerm() {
                uint64_t val = parseFactor();
                while (lexer.peek().type == ExprTokenType::MUL ||
                    lexer.peek().type == ExprTokenType::DIV ||
                    lexer.peek().type == ExprTokenType::MOD) {
                    if (lexer.peek().type == ExprTokenType::MUL) {
                        lexer.consume();
                        val *= parseFactor();
                    } else if (lexer.peek().type == ExprTokenType::DIV) {
                        lexer.consume();
                        uint64_t d = parseFactor();
                        if (d == 0) throw std::runtime_error("Division by zero");
                        val /= d;
                    } else {
                        lexer.consume();
                        uint64_t d = parseFactor();
                        if (d == 0) throw std::runtime_error("Modulo by zero");
                        val %= d;
                    }
                }
                return val;
            }

            uint64_t parseFactor() {
                if (lexer.peek().type == ExprTokenType::NOT) {
                    lexer.consume();
                    return !parseFactor();
                } else if (lexer.peek().type == ExprTokenType::MINUS) {
                    lexer.consume();
                    return -parseFactor();
                } else if (lexer.peek().type == ExprTokenType::NUMBER) {
                    uint64_t v = lexer.peek().value;
                    lexer.consume();
                    return v;
                } else if (lexer.peek().type == ExprTokenType::VARIABLE) {
                    std::string var_name = lexer.variable;
                    lexer.consume();
                    auto v = vars.find(var_name);
                    if(v == vars.end()) {
                        throw mx::Exception("Error variable name not found...\n");
                    }
                    return vars[var_name];
                } else if (lexer.peek().type == ExprTokenType::LPAREN) {
                    lexer.consume();
                    uint64_t v = parseLogical();
                    if (lexer.peek().type != ExprTokenType::RPAREN)
                        throw std::runtime_error("Expected ')'");
                    lexer.consume();
                    return v;
                }
                throw std::runtime_error("Unexpected token");
            }
        };
    }

#endif