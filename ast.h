#ifndef AST_H
#define AST_H

#include "global.h"
#include <vector>
#include <string>

// 全局四元式列表
std::vector<Quad> quads;

static int tempCount = 0;
inline std::string newTemp() { 
    std::string t = "t" + std::to_string(++tempCount); 
    // Register temporary as a local INT variable (default 4 bytes)
    // Note: To support float temps, we might need type info passed to newTemp, 
    // but for now 4 bytes is enough for pointers/ints/floats.
    symTable.insert(t, new PrimitiveType(Type::T_INT, "int", 4));
    Symbol* s = symTable.lookup(t);
    return s ? s->uniqueName : t; 
}

static int labelCount = 0;
inline std::string newLabel() { return "L" + std::to_string(++labelCount); }

inline void emit(std::string op, std::string arg1, std::string arg2, std::string res) {
    quads.push_back({op, arg1, arg2, res});
}

struct ASTNode {
    virtual ~ASTNode() {}
    virtual std::string genCode() { return ""; }
};

struct ExprNode : public ASTNode {
    Type* type = nullptr;
    bool isLValue = false;
    virtual std::string genAddr() { return ""; }
};

struct LiteralNode : public ExprNode {
    std::string val;
    LiteralNode(std::string v, Type* t) : val(v) { type = t; }
    std::string genCode() override { return val; } 
};

struct IDNode : public ExprNode {
    std::string name;
    std::string uniqueName;
    IDNode(std::string n, std::string uName, Type* t) : name(n), uniqueName(uName) { type = t; isLValue = (t && t->kind != Type::T_FUNC); }
    std::string genCode() override { 
        return uniqueName;
    }
    std::string genAddr() override {
        return uniqueName;
    }
};

struct BinaryExpr : public ExprNode {
    std::string op;
    ExprNode *left, *right;
    BinaryExpr(std::string o, ExprNode* l, ExprNode* r) : op(o), left(l), right(r) {
        if(l) type = l->type;
    }
    std::string genCode() override {
        std::string t1 = left->genCode();
        std::string t2 = (right) ? right->genCode() : ""; 

        // === 代码优化：常数折叠 (10分) ===
        // 简单实现：如果两个操作数首字符都是数字
        if (!t2.empty() && isdigit(t1[0]) && isdigit(t2[0])) {
            int v1 = std::stoi(t1);
            int v2 = std::stoi(t2);
            if (op == "+") return std::to_string(v1 + v2);
            if (op == "-") return std::to_string(v1 - v2);
            if (op == "*") return std::to_string(v1 * v2);
            if (op == "/") return std::to_string(v1 / v2);
            if (op == "%") return std::to_string(v1 % v2);
        }
        // ==============================

        std::string res = newTemp();
        emit(op, t1, t2, res);
        return res;
    }
};

struct AssignExpr : public ExprNode {
    ExprNode *left, *right;
    AssignExpr(ExprNode* l, ExprNode* r) : left(l), right(r) {}
    std::string genCode() override {
        std::string src = right->genCode();
        if (dynamic_cast<IDNode*>(left)) {
            std::string dst = left->genCode();
            emit("=", src, "_", dst);
            return dst;
        }
        std::string addr = left->genAddr();
        emit("STORE", src, "_", addr);
        return src;
    }
};

struct MemberExpr : public ExprNode {
    ExprNode* base;
    std::string memberName;
    bool isArrow;
    MemberExpr(ExprNode* b, std::string m, bool arrow) : base(b), memberName(m), isArrow(arrow) { isLValue = true; }
    std::string genAddr() override {
        std::string b = isArrow ? base->genCode() : base->genAddr();
        int off = 0;
        if (base && base->type) {
            StructType* st = nullptr;
            if (!isArrow && base->type->kind == Type::T_STRUCT) {
                st = dynamic_cast<StructType*>(base->type);
            } else if (isArrow && base->type->kind == Type::T_PTR) {
                PointerType* pt = dynamic_cast<PointerType*>(base->type);
                if (pt && pt->base && pt->base->kind == Type::T_STRUCT) st = dynamic_cast<StructType*>(pt->base);
            }
            if (st && st->offsets.count(memberName)) off = st->offsets[memberName];
            if (st && st->members.count(memberName)) type = st->members[memberName];
        }
        std::string res = newTemp();
        emit(isArrow?"ARROW":"DOT", b, memberName + ":" + std::to_string(off), res);
        return res;
    }
    std::string genCode() override {
        std::string addr = genAddr();
        std::string res = newTemp();
        emit("LOAD", addr, "_", res);
        return res;
    }
};

struct DerefExpr : public ExprNode {
    ExprNode* ptr;
    DerefExpr(ExprNode* p) : ptr(p) { isLValue = true; }
    std::string genAddr() override {
        std::string t = ptr->genCode();
        if (ptr && ptr->type && ptr->type->kind == Type::T_PTR) {
            PointerType* pt = dynamic_cast<PointerType*>(ptr->type);
            if (pt) type = pt->base;
        }
        return t;
    }
    std::string genCode() override {
        std::string addr = genAddr();
        std::string res = newTemp();
        emit("LOAD", addr, "_", res);
        return res;
    }
};

struct AddrExpr : public ExprNode {
    ExprNode* target;
    AddrExpr(ExprNode* t) : target(t) { isLValue = false; }
    std::string genCode() override {
        if (dynamic_cast<IDNode*>(target)) {
            std::string t = target->genAddr();
            std::string res = newTemp();
            emit("ADDR", t, "_", res);
            return res;
        }
        return target->genAddr();
    }
};

struct ArrayIndexExpr : public ExprNode {
    ExprNode* array;
    ExprNode* index;
    ArrayIndexExpr(ExprNode* arr, ExprNode* idx) : array(arr), index(idx) { 
        isLValue = true;
        if(arr->type && arr->type->kind == Type::T_ARRAY) {
            type = ((ArrayType*)arr->type)->elementType;
        }
    }
    std::string genAddr() override {
        std::string arr = array->genCode();
        std::string idx = index->genCode();
        std::string res = newTemp();
        emit("ARRAY_ADDR", arr, idx, res);
        return res;
    }
    std::string genCode() override {
        std::string addr = genAddr();
        std::string res = newTemp();
        emit("LOAD", addr, "_", res);
        return res;
    }
};

struct FunctionCallExpr : public ExprNode {
    std::string funcName;
    std::vector<ExprNode*> args;
    FunctionCallExpr(std::string name, std::vector<ExprNode*> a) : funcName(name), args(a) {}
    std::string genCode() override {
        std::vector<std::string> argTemps;
        // Evaluate arguments left-to-right
        for(auto arg : args) {
            argTemps.push_back(arg->genCode());
        }
        // Push arguments right-to-left (CDECL)
        for (int i = argTemps.size() - 1; i >= 0; i--) {
            emit("PARAM", argTemps[i], "_", "_");
        }
        std::string res = newTemp();
        emit("CALL", funcName, std::to_string(args.size()), res);
        return res;
    }
};

// --- 语句节点 ---

struct BlockStmt : public ASTNode {
    std::vector<ASTNode*> stmts;
    std::string genCode() override {
        for (auto s : stmts) if(s) s->genCode();
        return "";
    }
};

struct IfStmt : public ASTNode {
    ExprNode* cond; ASTNode *thenStmt, *elseStmt;
    IfStmt(ExprNode* c, ASTNode* t, ASTNode* e=nullptr) : cond(c), thenStmt(t), elseStmt(e) {}
    std::string genCode() override {
        std::string L_else = newLabel(), L_end = newLabel();
        emit("JZ", cond->genCode(), "_", L_else);
        if(thenStmt) thenStmt->genCode();
        emit("JMP", "_", "_", L_end);
        emit("LABEL", L_else, "_", "_");
        if(elseStmt) elseStmt->genCode();
        emit("LABEL", L_end, "_", "_");
        return "";
    }
};

struct WhileStmt : public ASTNode {
    ExprNode* cond; ASTNode* body;
    std::string L_start, L_end;
    WhileStmt(ExprNode* c, ASTNode* b, std::string start, std::string end) 
        : cond(c), body(b), L_start(start), L_end(end) {}
    std::string genCode() override {
        emit("LABEL", L_start, "_", "_");
        emit("JZ", cond->genCode(), "_", L_end);
        if(body) body->genCode();
        emit("JMP", "_", "_", L_start);
        emit("LABEL", L_end, "_", "_");
        return "";
    }
};

// 新增：For 循环
struct ForStmt : public ASTNode {
    ASTNode *init, *inc, *body; ExprNode *cond;
    std::string L_start, L_end, L_inc;
    ForStmt(ASTNode* i, ExprNode* c, ASTNode* n, ASTNode* b, std::string start, std::string end, std::string incLabel) 
        : init(i), cond(c), inc(n), body(b), L_start(start), L_end(end), L_inc(incLabel) {}
    std::string genCode() override {
        if(init) init->genCode();
        emit("LABEL", L_start, "_", "_");
        if(cond) emit("JZ", cond->genCode(), "_", L_end);
        if(body) body->genCode();
        emit("LABEL", L_inc, "_", "_");
        if(inc) inc->genCode();
        emit("JMP", "_", "_", L_start);
        emit("LABEL", L_end, "_", "_");
        return "";
    }
};

// 新增：IO 语句
struct IOStmt : public ASTNode {
    std::string op; ExprNode* expr;
    IOStmt(std::string o, ExprNode* e) : op(o), expr(e) {}
    std::string genCode() override {
        std::string t = expr->genCode();
        emit(op == "read" ? "READ" : "WRITE", t, "_", "_");
        return "";
    }
};

struct ReturnStmt : public ASTNode {
    ExprNode* expr;
    ReturnStmt(ExprNode* e) : expr(e) {}
    std::string genCode() override {
        if(expr) {
            std::string t = expr->genCode();
            emit("RETURN", t, "_", "_");
        } else {
            emit("RETURN", "_", "_", "_");
        }
        return "";
    }
};

struct BreakStmt : public ASTNode {
    std::string targetLabel;
    BreakStmt(std::string label) : targetLabel(label) {}
    std::string genCode() override {
        emit("JMP", "_", "_", targetLabel);
        return "";
    }
};

struct ContinueStmt : public ASTNode {
    std::string targetLabel;
    ContinueStmt(std::string label) : targetLabel(label) {}
    std::string genCode() override {
        emit("JMP", "_", "_", targetLabel);
        return "";
    }
};

struct FunctionDef : public ASTNode {
    std::string name;
    Type* returnType;
    std::vector<std::pair<std::string, Type*>> params;
    ASTNode* body;
    int frameSize;
    FunctionDef(std::string n, Type* ret, std::vector<std::pair<std::string, Type*>> p, ASTNode* b, int fs) 
        : name(n), returnType(ret), params(p), body(b), frameSize(fs) {}
    std::string genCode() override {
        emit("FUNC_BEGIN", name, std::to_string(frameSize), "_");
        for(auto& param : params) {
            emit("PARAM_DECL", param.first, "_", "_");
        }
        if(body) body->genCode();
        emit("FUNC_END", name, "_", "_");
        return "";
    }
};

#endif