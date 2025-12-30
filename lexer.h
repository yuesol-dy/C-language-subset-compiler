#ifndef LEXER_H
#define LEXER_H

#include "global.h"
#include <cctype>

class Lexer {
    std::string src;
    int pos = 0;
    int line = 1;
public:
    Lexer(std::string source) : src(source) {}

    char peek(int offset = 0) { 
        if (pos + offset >= src.length()) return 0;
        return src[pos + offset]; 
    }
    char advance() { 
        char c = peek(); 
        if(c) pos++; if(c=='\n') line++; 
        return c; 
    }

    Token getNextToken() {
        while (true) {
            while (isspace(peek())) advance();
            if (peek() == '/' && peek(1) == '/') { // 注释处理
                while (peek() != '\n' && peek() != 0) advance();
                continue;
            }
            break; 
        }

        if (peek() == 0) return {TOK_EOF, "", line, ""};

        char c = peek();

        if (isalpha(c) || c == '_') {
            std::string lexeme;
            while (isalnum(peek()) || peek() == '_') lexeme += advance();
            
            if (lexeme == "int") return {TOK_INT, lexeme, line, ""};
            if (lexeme == "float") return {TOK_FLOAT, lexeme, line, ""};
            if (lexeme == "struct") return {TOK_STRUCT, lexeme, line, ""};
            if (lexeme == "if") return {TOK_IF, lexeme, line, ""};
            if (lexeme == "else") return {TOK_ELSE, lexeme, line, ""};
            if (lexeme == "while") return {TOK_WHILE, lexeme, line, ""};
            if (lexeme == "for") return {TOK_FOR, lexeme, line, ""};
            if (lexeme == "break") return {TOK_BREAK, lexeme, line, ""};
            if (lexeme == "continue") return {TOK_CONTINUE, lexeme, line, ""};
            if (lexeme == "read") return {TOK_READ, lexeme, line, ""};
            if (lexeme == "write") return {TOK_WRITE, lexeme, line, ""};
            if (lexeme == "return") return {TOK_RETURN, lexeme, line, ""};
            
            return {TOK_ID, lexeme, line, lexeme};
        }

        if (isdigit(c)) {
            std::string lexeme;
            bool isFloat = false;
            while (isdigit(peek())) lexeme += advance();
            if (peek() == '.') {
                isFloat = true; lexeme += advance();
                while (isdigit(peek())) lexeme += advance();
            }
            return {isFloat ? TOK_FLOAT_LIT : TOK_INT_LIT, lexeme, line, lexeme};
        }

        advance();
        switch (c) {
            case '+': return {TOK_PLUS, "+", line, ""};
            case '-': return (peek() == '>') ? (advance(), Token{TOK_ARROW, "->", line, ""}) : Token{TOK_MINUS, "-", line, ""};
            case '*': return {TOK_STAR, "*", line, ""};
            case '/': return {TOK_DIV, "/", line, ""};
            case '%': return {TOK_MOD, "%", line, ""};
            case '^': return {TOK_XOR, "^", line, ""};
            case '!': return (peek()=='=') ? (advance(), Token{TOK_NEQ, "!=", line,""}) : Token{TOK_NOT, "!", line, ""};
            case '=': return (peek()=='=') ? (advance(), Token{TOK_EQ, "==", line,""}) : Token{TOK_ASSIGN, "=", line, ""};
            case '>': return (peek()=='=') ? (advance(), Token{TOK_GEQ, ">=", line,""}) : Token{TOK_GT, ">", line, ""};
            case '<': return (peek()=='=') ? (advance(), Token{TOK_LEQ, "<=", line,""}) : Token{TOK_LT, "<", line, ""};
            case '&': return (peek()=='&') ? (advance(), Token{TOK_AND, "&&", line,""}) : Token{TOK_ADDR, "&", line, ""};
            case '|': if(peek()=='|') { advance(); return {TOK_OR, "||", line, ""}; } break;
            case ';': return {TOK_SEMI, ";", line, ""};
            case ',': return {TOK_COMMA, ",", line, ""};
            case '(': return {TOK_LPAREN, "(", line, ""};
            case ')': return {TOK_RPAREN, ")", line, ""};
            case '{': return {TOK_LBRACE, "{", line, ""};
            case '}': return {TOK_RBRACE, "}", line, ""};
            case '[': return {TOK_LBRACKET, "[", line, ""};
            case ']': return {TOK_RBRACKET, "]", line, ""};
            case '.': return {TOK_DOT, ".", line, ""};
        }
        return {TOK_ERROR, "Unknown", line, ""};
    }
};
#endif