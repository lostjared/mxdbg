/* 
    MXDBG - Debugger with AI 
    coded by Jared Bruni (jaredbruni@protonmail.com)
    https://lostsidedead.biz
*/
#ifndef _TYPES_X__
#define _TYPES_X__
  
#include<iostream>
#include<vector>
#include<string>
#include<string_view>
#include<array>
#include<optional>

namespace types {
    enum class TokenType { TT_ID, TT_ARG, TT_SYM, TT_STR, TT_NUM, TT_HEX, TT_NULL };
    enum class CharType { TT_CHAR, TT_DIGIT, TT_SYMBOL, TT_STRING, TT_SINGLE, TT_SPACE, TT_NULL };
    enum class OperatorType {
        OP_INC,             // '++'
        OP_DEC,             // '--'
        OP_LSHIFT_ASSIGN,   // '<<='
        OP_RSHIFT_ASSIGN,   // '>>='
        OP_PLUS_ASSIGN,     // '+='
        OP_MINUS_ASSIGN,    // '-='
        OP_MUL_ASSIGN,      // '*='
        OP_DIV_ASSIGN,      // '/='
        OP_MOD_ASSIGN,      // '%='
        OP_AND_ASSIGN,      // '&='
        OP_OR_ASSIGN,       // '|='
        OP_XOR_ASSIGN,      // '^='
        OP_LSHIFT,          // '<<'
        OP_RSHIFT,          // '>>'
        OP_EQ,              // '=='
        OP_NEQ,             // '!='
        OP_LE,              // '<='
        OP_GE,              // '>='
        OP_AND_AND,         // '&&'
        OP_OR_OR,           // '||'
        OP_ARROW,           // '->'
        OP_PLUS,            // '+'
        OP_MINUS,           // '-'
        OP_MUL,             // '*'
        OP_DIV,             // '/'
        OP_MOD,             // '%'
        OP_ASSIGN,          // '='
        OP_AND,             // '&'
        OP_OR,              // '|'
        OP_XOR,             // '^'
        OP_NOT,             // '!'
        OP_LT,              // '<'
        OP_GT,              // '>'
        OP_LPAREN,          // '('
        OP_RPAREN,          // ')'
        OP_LBRACKET,        // '['
        OP_RBRACKET,        // ']'
        OP_LBRACE,          // '{'
        OP_RBRACE,          // '}'
        OP_COMMA,           // ','
        OP_SEMICOLON,       // ';'
        OP_COLON,           // ':'
        OP_DOT,             // '.'
        OP_QUESTION,        // '?'
        OP_HASH,            // '#' 
        OP_SCOPE,           // '::' 
        OP_TILDE,            // '~'
        OP_DOLLAR,          // '$'
        OP_AT,              // '@'
    };

    inline constexpr std::array opName = {
        std::string_view{"Increment"},           // OP_INC             -> '++'
        std::string_view{"Decrement"},           // OP_DEC             -> '--'
        std::string_view{"Left Shift Assign"},   // OP_LSHIFT_ASSIGN   -> '<<='
        std::string_view{"Right Shift Assign"},  // OP_RSHIFT_ASSIGN   -> '>>='
        std::string_view{"Plus Assign"},         // OP_PLUS_ASSIGN     -> '+='
        std::string_view{"Minus Assign"},        // OP_MINUS_ASSIGN    -> '-='
        std::string_view{"Multiply Assign"},     // OP_MUL_ASSIGN      -> '*='
        std::string_view{"Divide Assign"},       // OP_DIV_ASSIGN      -> '/='
        std::string_view{"Modulo Assign"},       // OP_MOD_ASSIGN      -> '%='
        std::string_view{"Bitwise And Assign"},  // OP_AND_ASSIGN      -> '&='
        std::string_view{"Bitwise Or Assign"},   // OP_OR_ASSIGN       -> '|='
        std::string_view{"Bitwise Xor Assign"},  // OP_XOR_ASSIGN      -> '^='
        std::string_view{"Left Shift"},          // OP_LSHIFT          -> '<<'
        std::string_view{"Right Shift"},         // OP_RSHIFT          -> '>>'
        std::string_view{"Equal"},               // OP_EQ              -> '=='
        std::string_view{"Not Equal"},           // OP_NEQ             -> '!='
        std::string_view{"Less Than Or Equal"},  // OP_LE              -> '<='
        std::string_view{"Greater Than Or Equal"}, // OP_GE            -> '>='
        std::string_view{"Logical And"},         // OP_AND_AND         -> '&&'
        std::string_view{"Logical Or"},          // OP_OR_OR           -> '||'
        std::string_view{"Arrow"},               // OP_ARROW           -> '->'
        std::string_view{"Plus"},                // OP_PLUS            -> '+'
        std::string_view{"Minus"},               // OP_MINUS           -> '-'
        std::string_view{"Multiply"},            // OP_MUL             -> '*'
        std::string_view{"Divide"},              // OP_DIV             -> '/'
        std::string_view{"Modulo"},              // OP_MOD             -> '%'
        std::string_view{"Assign"},              // OP_ASSIGN          -> '='
        std::string_view{"Bitwise And"},         // OP_AND             -> '&'
        std::string_view{"Bitwise Or"},          // OP_OR              -> '|'
        std::string_view{"Bitwise Xor"},         // OP_XOR             -> '^'
        std::string_view{"Not"},                 // OP_NOT             -> '!'
        std::string_view{"Less Than"},           // OP_LT              -> '<'
        std::string_view{"Greater Than"},        // OP_GT              -> '>'
        std::string_view{"Left Parenthesis"},    // OP_LPAREN          -> '('
        std::string_view{"Right Parenthesis"},   // OP_RPAREN          -> ')'
        std::string_view{"Left Bracket"},        // OP_LBRACKET        -> '['
        std::string_view{"Right Bracket"},       // OP_RBRACKET        -> ']'
        std::string_view{"Left Brace"},          // OP_LBRACE          -> '{'
        std::string_view{"Right Brace"},         // OP_RBRACE          -> '}'
        std::string_view{"Comma"},               // OP_COMMA           -> ','
        std::string_view{"Semicolon"},           // OP_SEMICOLON       -> ';'
        std::string_view{"Colon"},               // OP_COLON           -> ':'
        std::string_view{"Dot"},                 // OP_DOT             -> '.'
        std::string_view{"Question Mark"},       // OP_QUESTION        -> '?'
        std::string_view{"Hash"},                // OP_HASH            -> '#'
        std::string_view{"Scope"},               // OP_SCOPE           -> '::'
        std::string_view{"Tilde"},               // OP_TILDE           -> '~'
        std::string_view{"Dollar"},              // OP_DOLLAR          -> '$'
        std::string_view{"AT"},                  // OP_AT              -> '@'
    };

    inline constexpr std::array opStrings = {
        std::string_view{"++"},       // OP_INC
        std::string_view{"--"},       // OP_DEC
        std::string_view{"<<="},      // OP_LSHIFT_ASSIGN
        std::string_view{">>="},      // OP_RSHIFT_ASSIGN
        std::string_view{"+="},       // OP_PLUS_ASSIGN
        std::string_view{"-="},       // OP_MINUS_ASSIGN
        std::string_view{"*="},       // OP_MUL_ASSIGN
        std::string_view{"/="},       // OP_DIV_ASSIGN
        std::string_view{"%="},       // OP_MOD_ASSIGN
        std::string_view{"&="},       // OP_AND_ASSIGN
        std::string_view{"|="},       // OP_OR_ASSIGN
        std::string_view{"^="},       // OP_XOR_ASSIGN
        std::string_view{"<<"},       // OP_LSHIFT
        std::string_view{">>"},       // OP_RSHIFT
        std::string_view{"=="},       // OP_EQ
        std::string_view{"!="},       // OP_NEQ
        std::string_view{"<="},       // OP_LE
        std::string_view{">="},       // OP_GE
        std::string_view{"&&"},       // OP_AND_AND
        std::string_view{"||"},       // OP_OR_OR
        std::string_view{"->"},       // OP_ARROW
        std::string_view{"+"},        // OP_PLUS
        std::string_view{"-"},        // OP_MINUS
        std::string_view{"*"},        // OP_MUL
        std::string_view{"/"},        // OP_DIV
        std::string_view{"%"},        // OP_MOD
        std::string_view{"="},        // OP_ASSIGN
        std::string_view{"&"},        // OP_AND
        std::string_view{"|"},        // OP_OR
        std::string_view{"^"},        // OP_XOR
        std::string_view{"!"},        // OP_NOT
        std::string_view{"<"},        // OP_LT
        std::string_view{">"},        // OP_GT
        std::string_view{"("},        // OP_LPAREN
        std::string_view{")"},        // OP_RPAREN
        std::string_view{"["},        // OP_LBRACKET
        std::string_view{"]"},        // OP_RBRACKET
        std::string_view{"{"},        // OP_LBRACE
        std::string_view{"}"},        // OP_RBRACE
        std::string_view{","},        // OP_COMMA
        std::string_view{";"},        // OP_SEMICOLON
        std::string_view{":"},        // OP_COLON
        std::string_view{"."},        // OP_DOT
        std::string_view{"?"},        // OP_QUESTION
        std::string_view{"#"},        // OP_HASH
        std::string_view{"::"},       // OP_SCOPE
        std::string_view{"~"},        // OP_TILDE
        std::string_view{"$"},        // OP_DOLLAR
        std::string_view{"@"},        // OP_AT
    };

    enum class KeywordType { KW_LET, KW_PROC, KW_DEFINE, KW_IF, KW_ELSE, KW_SWITCH, KW_WHILE, KW_FOR, KW_RETURN, KW_BREAK, KW_CONTINUE };
    inline constexpr std::array kwStr = {
        std::string_view{"let"}, std::string_view{"proc"}, std::string_view{"define"}, 
        std::string_view{"if"}, std::string_view{"else"}, std::string_view{"switch"}, 
        std::string_view{"while"}, std::string_view{"for"}, std::string_view{"return"}, 
        std::string_view{"break"}, std::string_view{"continue"}
    };

    
    extern std::vector<std::string> strTokenType;
    extern std::vector<std::string> strCharType;
    void print_type_TokenType(std::ostream &out, const TokenType &tt);
    void print_type_CharType(std::ostream &out, const CharType &c);
    [[nodiscard]] std::optional<OperatorType> lookUp(const std::string &op);
    [[nodiscard]] std::optional<KeywordType> lookUp_Keyword(const std::string &op);
}

#endif