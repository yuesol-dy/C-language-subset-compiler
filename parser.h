#ifndef PARSER_H
#define PARSER_H

#include "global.h"
#include "lexer.h"
#include "ast.h"

class Parser {
    Lexer& lexer;
    Token currTok;
    void eat(TokenType t) {
        if (currTok.type == t) currTok = lexer.getNextToken();
        else error("Expected token type " + std::to_string(t) + ", got " + currTok.lexeme, currTok.line);
    }
public:
    std::vector<ASTNode*> programAST;
    // Stack for loop labels: <breakLabel, continueLabel>
    std::vector<std::pair<std::string, std::string>> loopStack;
    std::map<std::string, StructType*> structRegistry;

    Parser(Lexer& l) : lexer(l) { currTok = lexer.getNextToken(); }

    // Helper to parse types (int, float, struct A, pointers, arrays)
    Type* parseType() {
        Type* type = nullptr;
        if (currTok.type == TOK_STRUCT) {
            eat(TOK_STRUCT); 
            std::string n = currTok.lexeme; 
            eat(TOK_ID);
            // Check registry
            if (structRegistry.count(n)) {
                type = structRegistry[n];
            } else {
                type = new StructType(n);
                structRegistry[n] = type;
            }
        } 
        else if (currTok.type == TOK_INT) { eat(TOK_INT); type = new PrimitiveType(Type::T_INT,"int",4); }
        else if (currTok.type == TOK_FLOAT) { eat(TOK_FLOAT); type = new PrimitiveType(Type::T_FLOAT,"float",4); }
        else {
            return nullptr; // Not a type start
        }
        
        // Handle Pointers
        while(currTok.type == TOK_STAR) { eat(TOK_STAR); type = new PointerType(type); }
        
        return type;
    }

    // 因子
    ExprNode* parseFactor() {
        if (currTok.type == TOK_NOT) {
            eat(TOK_NOT); return new BinaryExpr("!", parseFactor(), nullptr);
        }
        if (currTok.type == TOK_INT_LIT) {
            std::string val = currTok.lexeme; eat(TOK_INT_LIT);
            return new LiteralNode(val, new PrimitiveType(Type::T_INT, "int", 4));
        }
        if (currTok.type == TOK_FLOAT_LIT) {
            std::string val = currTok.lexeme; eat(TOK_FLOAT_LIT);
            return new LiteralNode(val, new PrimitiveType(Type::T_FLOAT, "float", 4));
        }
        if (currTok.type == TOK_ID) {
            std::string name = currTok.lexeme; eat(TOK_ID);
            Symbol* s = symTable.lookup(name);
            if (!s) error("Undefined: " + name, currTok.line);
            return new IDNode(name, s->uniqueName, s->type);
        }
        if (currTok.type == TOK_LPAREN) {
            eat(TOK_LPAREN); ExprNode* e = parseExpr(); eat(TOK_RPAREN); return e;
        }
        return nullptr;
    }

    ExprNode* parsePostfix() {
        ExprNode* node = parseFactor();
        while (true) {
            if (currTok.type == TOK_DOT) {
                eat(TOK_DOT); std::string m = currTok.lexeme; eat(TOK_ID);
                node = new MemberExpr(node, m, false);
            } else if (currTok.type == TOK_ARROW) {
                eat(TOK_ARROW); std::string m = currTok.lexeme; eat(TOK_ID);
                node = new MemberExpr(node, m, true);
            } else if (currTok.type == TOK_LBRACKET) {
                eat(TOK_LBRACKET);
                ExprNode* index = parseExpr();
                eat(TOK_RBRACKET);
                node = new ArrayIndexExpr(node, index);
            } else if (currTok.type == TOK_LPAREN && dynamic_cast<IDNode*>(node)) {
                std::string funcName = ((IDNode*)node)->name;
                eat(TOK_LPAREN);
                std::vector<ExprNode*> args;
                if (currTok.type != TOK_RPAREN) {
                    args.push_back(parseExpr());
                    while (currTok.type == TOK_COMMA) {
                        eat(TOK_COMMA);
                        args.push_back(parseExpr());
                    }
                }
                eat(TOK_RPAREN);
                node = new FunctionCallExpr(funcName, args);
            } else break;
        }
        return node;
    }

    ExprNode* parseUnary() {
        if (currTok.type == TOK_STAR) { eat(TOK_STAR); return new DerefExpr(parseUnary()); }
        if (currTok.type == TOK_ADDR) { eat(TOK_ADDR); return new AddrExpr(parseUnary()); }
        return parsePostfix();
    }
    
    // 异或/幂运算 ^ (优先级高于乘法)
    ExprNode* parsePower() {
        ExprNode* left = parseUnary();
        while(currTok.type == TOK_XOR) {
            eat(TOK_XOR);
            left = new BinaryExpr("^", left, parseUnary());
        }
        return left;
    }

    ExprNode* parseMultiplicative() {
        ExprNode* left = parsePower();
        while (currTok.type == TOK_STAR || currTok.type == TOK_DIV || currTok.type == TOK_MOD) {
            std::string op = currTok.lexeme; eat(currTok.type);
            left = new BinaryExpr(op, left, parsePower());
        }
        return left;
    }

    ExprNode* parseAdditive() {
        ExprNode* left = parseMultiplicative();
        while (currTok.type == TOK_PLUS || currTok.type == TOK_MINUS) {
            std::string op = currTok.lexeme; eat(currTok.type);
            left = new BinaryExpr(op, left, parseMultiplicative());
        }
        return left;
    }

    ExprNode* parseRelational() {
        ExprNode* left = parseAdditive();
        while (currTok.type >= TOK_GT && currTok.type <= TOK_LEQ) {
            std::string op = currTok.lexeme; eat(currTok.type);
            left = new BinaryExpr(op, left, parseAdditive());
        }
        return left;
    }
    
    ExprNode* parseEquality() {
        ExprNode* left = parseRelational();
        while (currTok.type == TOK_EQ || currTok.type == TOK_NEQ) {
            std::string op = currTok.lexeme; eat(currTok.type);
            left = new BinaryExpr(op, left, parseRelational());
        }
        return left;
    }
    
    ExprNode* parseLogic() {
        ExprNode* left = parseEquality();
        while(currTok.type == TOK_AND || currTok.type == TOK_OR) {
            std::string op = currTok.lexeme; eat(currTok.type);
            left = new BinaryExpr(op, left, parseEquality());
        }
        return left;
    }

    ExprNode* parseAssignment() {
        ExprNode* left = parseLogic();
        if (currTok.type == TOK_ASSIGN) {
            if (!left->isLValue) {
                error("Expression is not assignable (not an l-value)", currTok.line);
            }
            eat(TOK_ASSIGN);
            ExprNode* right = parseAssignment();
            
            // Basic Type Checking
            if (left->type && right->type) {
                if (left->type->kind != right->type->kind) {
                    // Allow int <-> float implicit conversion (warning)
                    if ((left->type->kind == Type::T_INT && right->type->kind == Type::T_FLOAT) ||
                        (left->type->kind == Type::T_FLOAT && right->type->kind == Type::T_INT)) {
                         // Warning only
                    } else {
                        std::cerr << "[Warning] Line " << currTok.line << ": Type mismatch in assignment (" 
                                  << left->type->toString() << " = " << right->type->toString() << ")\n";
                    }
                }
            }
            
            return new AssignExpr(left, right);
        }
        return left;
    }
    
    ExprNode* parseExpr() { return parseAssignment(); }

    // --- 语句与声明 ---
    
    ASTNode* parseDeclaration() {
        Type* type = parseType();
        if (!type) {
             // Should not happen if called correctly
        }
        
        // Handle Struct Definition: struct A { ... }
        if (type->kind == Type::T_STRUCT && currTok.type == TOK_LBRACE) {
             eat(TOK_LBRACE); 
             StructType* st = dynamic_cast<StructType*>(type);
             while(currTok.type != TOK_RBRACE) {
                 Type* memberType = parseType();
                 std::string m = currTok.lexeme; eat(TOK_ID); eat(TOK_SEMI);
                 st->addMember(m, memberType);
             }
             eat(TOK_RBRACE); eat(TOK_SEMI);
             
             // Register defined struct
             structRegistry[st->name] = st;
             std::cout << "[Parser] Defined struct " << st->name << std::endl;
             return new BlockStmt();
        }

        std::string name = currTok.lexeme; eat(TOK_ID);
        
        // 检查是否是函数定义
        if(currTok.type == TOK_LPAREN) {
            eat(TOK_LPAREN);
            std::vector<std::pair<std::string, Type*>> params;
            
            // Set current function for name mangling of locals/params
            symTable.setCurrentFunc(name);
            
            if(currTok.type != TOK_RPAREN) {
                Type* ptype = parseType(); // 参数类型
                std::string pname = currTok.lexeme; eat(TOK_ID);
                params.push_back({pname, ptype});
                while(currTok.type == TOK_COMMA) {
                    eat(TOK_COMMA);
                    ptype = parseType();
                    pname = currTok.lexeme; eat(TOK_ID);
                    params.push_back({pname, ptype});
                }
            }
            eat(TOK_RPAREN);
            
            symTable.enterScope();
            // Reset param offset for this new function
            symTable.resetParamOffset();
            
            for(auto& p : params) {
                // Use insertParam to assign [ebp+X] offsets
                symTable.insertParam(p.first, p.second);
            }
            
            ASTNode* body = parseBlock();
            
            // Capture frame size
            int frameSize = symTable.getCurrentScopeSize();
            
            symTable.exitScope();
            
            // Reset current function
            symTable.setCurrentFunc("");
            
            FunctionType* ftype = new FunctionType(type);
            for(auto& p : params) ftype->addParam(p.second);
            symTable.insert(name, ftype);
            return new FunctionDef(name, type, params, body, frameSize);
        }
        
        // 检查是否是数组声明
        if(currTok.type == TOK_LBRACKET) {
            eat(TOK_LBRACKET);
            int size = std::stoi(currTok.lexeme);
            eat(TOK_INT_LIT);
            eat(TOK_RBRACKET);
            type = new ArrayType(type, size);
        }
        
        if (!symTable.insert(name, type)) {
            error("Redefinition of variable '" + name + "'", currTok.line);
        }
        eat(TOK_SEMI);
        return new BlockStmt();
    }

    ASTNode* parseBlock() {
        eat(TOK_LBRACE); BlockStmt* b = new BlockStmt();
        while(currTok.type != TOK_RBRACE) b->stmts.push_back(parseStatement());
        eat(TOK_RBRACE); return b;
    }

    ASTNode* parseStatement() {
        if (currTok.type == TOK_LBRACE) return parseBlock();
        if (currTok.type == TOK_INT || currTok.type == TOK_FLOAT || currTok.type == TOK_STRUCT) return parseDeclaration();
        
        if (currTok.type == TOK_IF) {
            eat(TOK_IF); eat(TOK_LPAREN); ExprNode* c = parseExpr(); eat(TOK_RPAREN);
            ASTNode* t = parseStatement();
            ASTNode* e = (currTok.type == TOK_ELSE) ? (eat(TOK_ELSE), parseStatement()) : nullptr;
            return new IfStmt(c, t, e);
        }
        
        if (currTok.type == TOK_WHILE) {
            std::string L_start = newLabel();
            std::string L_end = newLabel();
            loopStack.push_back({L_end, L_start}); // break -> end, continue -> start
            
            eat(TOK_WHILE); eat(TOK_LPAREN); ExprNode* c = parseExpr(); eat(TOK_RPAREN);
            ASTNode* body = parseStatement();
            
            loopStack.pop_back();
            return new WhileStmt(c, body, L_start, L_end);
        }
        
        if (currTok.type == TOK_FOR) {
            std::string L_start = newLabel();
            std::string L_end = newLabel();
            std::string L_inc = newLabel();
            loopStack.push_back({L_end, L_inc}); // break -> end, continue -> inc
            
            eat(TOK_FOR); eat(TOK_LPAREN);
            ASTNode* init = parseStatement();
            ExprNode* cond = parseExpr(); eat(TOK_SEMI);
            ASTNode* inc = (currTok.type != TOK_RPAREN) ? parseAssignment() : nullptr;
            eat(TOK_RPAREN);
            ASTNode* body = parseStatement();
            
            loopStack.pop_back();
            return new ForStmt(init, cond, inc, body, L_start, L_end, L_inc);
        }
        
        if (currTok.type == TOK_BREAK) {
            eat(TOK_BREAK); eat(TOK_SEMI);
            if (loopStack.empty()) error("break statement not within loop", currTok.line);
            return new BreakStmt(loopStack.back().first);
        }
        
        if (currTok.type == TOK_CONTINUE) {
            eat(TOK_CONTINUE); eat(TOK_SEMI);
            if (loopStack.empty()) error("continue statement not within loop", currTok.line);
            return new ContinueStmt(loopStack.back().second);
        }
        
        if (currTok.type == TOK_READ || currTok.type == TOK_WRITE) {
            std::string op = currTok.lexeme; eat(currTok.type); eat(TOK_LPAREN);
            ExprNode* e = parseExpr(); eat(TOK_RPAREN); eat(TOK_SEMI);
            return new IOStmt(op, e);
        }
        
        if (currTok.type == TOK_RETURN) {
            eat(TOK_RETURN);
            ExprNode* e = (currTok.type != TOK_SEMI) ? parseExpr() : nullptr;
            eat(TOK_SEMI);
            return new ReturnStmt(e);
        }
        
        ExprNode* e = parseExpr(); eat(TOK_SEMI);
        return e;
    }

    void parseProgram() {
        while (currTok.type != TOK_EOF) {
            ASTNode* s = parseStatement();
            if(s) {
                s->genCode();
                programAST.push_back(s);
            }
        }
    }
};
#endif