#ifndef GLOBAL_H
#define GLOBAL_H

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cstdlib>

// ==========================================
// 1. Token 定义
// ==========================================
enum TokenType {
    TOK_EOF, TOK_ERROR,
    // 关键字
    TOK_INT, TOK_FLOAT, TOK_STRUCT, 
    TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR, TOK_RETURN, TOK_BREAK, TOK_CONTINUE,
    TOK_READ, TOK_WRITE, // IO
    
    // 标识符与字面量
    TOK_ID, TOK_INT_LIT, TOK_FLOAT_LIT,
    
    // 运算符
    TOK_ASSIGN, TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_DIV, TOK_MOD,
    TOK_XOR, // ^
    
    // 关系与逻辑
    TOK_GT, TOK_LT, TOK_EQ, TOK_NEQ, TOK_GEQ, TOK_LEQ,
    TOK_AND, TOK_OR, TOK_NOT,
    
    // 辅助
    TOK_ADDR, TOK_DOT, TOK_ARROW,
    
    // 分隔符
    TOK_SEMI, TOK_COMMA, TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE, TOK_LBRACKET, TOK_RBRACKET
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    std::string attribute; 
};

inline void error(const std::string& msg, int line) {
    throw std::runtime_error("[Error] Line " + std::to_string(line) + ": " + msg);
}

// ==========================================
// 2. 中间代码结构 (四元式) 
// ==========================================
struct Quad {
    std::string op;
    std::string arg1;
    std::string arg2;
    std::string res;
};
// 全局存储四元式，供汇编生成使用
extern std::vector<Quad> quads; 

// ==========================================
// 3. 类型系统
// ==========================================
struct Type {
    enum Kind { T_INT, T_FLOAT, T_PTR, T_STRUCT, T_ARRAY, T_FUNC } kind;
    int size; 
    virtual ~Type() {}
    virtual std::string toString() = 0;
};

struct PrimitiveType : public Type {
    std::string name;
    PrimitiveType(Kind k, std::string n, int s) : name(n) { kind = k; size = s; }
    std::string toString() override { return name; }
};

struct PointerType : public Type {
    Type* base;
    PointerType(Type* b) : base(b) { kind = T_PTR; size = 4; }
    std::string toString() override { return base->toString() + "*"; }
};

struct StructType : public Type {
    std::string name;
    std::map<std::string, Type*> members;
    std::map<std::string, int> offsets;
    StructType(std::string n) : name(n) { kind = T_STRUCT; size = 0; }
    void addMember(std::string memName, Type* memType) {
        members[memName] = memType;
        offsets[memName] = size;
        size += memType->size;
    }
    std::string toString() override { return "struct " + name; }
};

struct ArrayType : public Type {
    Type* elementType;
    int length;
    ArrayType(Type* elem, int len) : elementType(elem), length(len) { 
        kind = T_ARRAY; 
        size = elem->size * len; 
    }
    std::string toString() override { 
        return elementType->toString() + "[" + std::to_string(length) + "]"; 
    }
};

struct FunctionType : public Type {
    Type* returnType;
    std::vector<Type*> paramTypes;
    FunctionType(Type* ret) : returnType(ret) { kind = T_FUNC; size = 0; }
    void addParam(Type* paramType) { paramTypes.push_back(paramType); }
    std::string toString() override { 
        std::string s = returnType->toString() + "(";
        for(size_t i = 0; i < paramTypes.size(); i++) {
            if(i > 0) s += ", ";
            s += paramTypes[i]->toString();
        }
        return s + ")";
    }
};

// ==========================================
// 4. 符号表
// ==========================================
extern std::map<std::string, StructType*> structRegistry;

struct Symbol {
    std::string name;
    std::string uniqueName; // mangled name for quads: func_var
    Type* type;
    int offset;
    int scopeLevel;
    bool isParam;
    std::string asmName; // [ebp-4] or global_label

    Symbol(std::string n, std::string uName, Type* t, int off, int level, bool param, std::string assemblyName) 
        : name(n), uniqueName(uName), type(t), offset(off), scopeLevel(level), isParam(param), asmName(assemblyName) {}
};

class SymbolTable {
    std::vector<std::map<std::string, Symbol*>> scopes;
    std::vector<int> currentOffset; 
    std::string currentFunc = ""; // Current function name for mangling
    int paramOffset = 8; // Start of params: ebp + 8
public:
    // Registry of all symbols by uniqueName, for code gen
    std::map<std::string, Symbol*> symbolRegistry;
    std::vector<std::string> allVariables; // Keep for globals logic if needed, but registry is better

    SymbolTable() { enterScope(); }
    
    void setCurrentFunc(std::string name) { currentFunc = name; }
    std::string getCurrentFunc() { return currentFunc; }

    void enterScope() {
        scopes.push_back({});
        currentOffset.push_back(0); // Locals offset starts at 0 (accumulates negative)
    }
    
    void exitScope() {
        if(scopes.size() > 1) { scopes.pop_back(); currentOffset.pop_back(); }
        // Note: We do NOT delete the Symbols, they are referenced in registry
    }
    
    // Reset param offset counter when starting function
    void resetParamOffset() { paramOffset = 8; }

    bool insert(std::string name, Type* type) {
        if(scopes.back().count(name)) return false;
        
        int level = scopes.size() - 1;
        std::string uName;
        std::string aName;
        int off = 0;

        if (level == 0) { // Global
            uName = name;
            aName = name;
            // Align globals? usually just label
        } else { // Local
            uName = (currentFunc.empty() ? "main" : currentFunc) + "_" + name;
            
            // Locals: [ebp - 4], [ebp - 8]...
            // We increase offset by size
            // Align to 4 bytes
            int size = type->size;
            if (size % 4 != 0) size += (4 - size % 4);
            
            currentOffset.back() += size;
            off = currentOffset.back();
            aName = "[ebp-" + std::to_string(off) + "]";
        }

        Symbol* sym = new Symbol(name, uName, type, off, level, false, aName);
        scopes.back()[name] = sym;
        symbolRegistry[uName] = sym;
        
        if (level == 0) allVariables.push_back(name);
        return true;
    }
    
    bool insertParam(std::string name, Type* type) {
        if(scopes.back().count(name)) return false;
        
        int level = scopes.size() - 1;
        std::string uName = (currentFunc.empty() ? "main" : currentFunc) + "_" + name;
        
        // Params: [ebp + 8], [ebp + 12]...
        // Pushed in reverse order? Standard CDECL: pushed right-to-left.
        // Param 1 is at EBP+8.
        int off = paramOffset;
        std::string aName = "[ebp+" + std::to_string(off) + "]";
        
        // Update param offset for next param
        // All params on stack are aligned to 4 bytes in 32-bit mode
        paramOffset += 4; 

        Symbol* sym = new Symbol(name, uName, type, off, level, true, aName);
        scopes.back()[name] = sym;
        symbolRegistry[uName] = sym;
        return true;
    }
    
    Symbol* lookup(std::string name) {
        for(int i=scopes.size()-1; i>=0; --i) {
            if(scopes[i].count(name)) return scopes[i][name];
        }
        return nullptr;
    }
    
    Symbol* getByUniqueName(std::string uName) {
        if (symbolRegistry.count(uName)) return symbolRegistry[uName];
        return nullptr;
    }

    int getCurrentScopeSize() {
        return currentOffset.empty() ? 0 : currentOffset.back();
    }
};

extern SymbolTable symTable;

#endif