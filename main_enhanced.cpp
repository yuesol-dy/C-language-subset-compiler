#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "global.h"
#include "lexer.h"
#include "parser.h"
#include "optimizer.h"

using namespace std;

// 全局变量已在ast.h中定义，这里不需要重复定义
// 但需要定义符号表
SymbolTable symTable;

// Token类型名称映射
string getTokenTypeName(TokenType type) {
    switch(type) {
        case TOK_INT: return "TYPE";
        case TOK_FLOAT: return "TYPE";
        case TOK_STRUCT: return "STRUCT";
        case TOK_IF: return "IF";
        case TOK_ELSE: return "ELSE";
        case TOK_WHILE: return "WHILE";
        case TOK_FOR: return "FOR";
        case TOK_READ: return "READ";
        case TOK_WRITE: return "WRITE";
        case TOK_RETURN: return "RETURN";
        case TOK_ID: return "ID";
        case TOK_INT_LIT: return "NUMBER";
        case TOK_FLOAT_LIT: return "FLOAT_NUM";
        case TOK_PLUS: return "ADD";
        case TOK_MINUS: return "MINUS";
        case TOK_STAR: return "MULTIPLY";
        case TOK_DIV: return "DIVIDE";
        case TOK_MOD: return "MOD";
        case TOK_XOR: return "XOR";
        case TOK_ASSIGN: return "ASSIGN";
        case TOK_EQ: return "EQUAL";
        case TOK_NEQ: return "NOT_EQUAL";
        case TOK_LT: return "LESS";
        case TOK_GT: return "GREATER";
        case TOK_LEQ: return "LESS_EQUAL";
        case TOK_GEQ: return "GREATER_EQUAL";
        case TOK_AND: return "AND";
        case TOK_OR: return "OR";
        case TOK_NOT: return "NOT";
        case TOK_ADDR: return "ADDRESS";
        case TOK_SEMI: return "SEMICOLON";
        case TOK_COMMA: return "COMMA";
        case TOK_LPAREN: return "LP";
        case TOK_RPAREN: return "RP";
        case TOK_LBRACE: return "LBRACE";
        case TOK_RBRACE: return "RBRACE";
        case TOK_LBRACKET: return "LBRACKET";
        case TOK_RBRACKET: return "RBRACKET";
        case TOK_DOT: return "DOT";
        case TOK_ARROW: return "ARROW";
        default: return "UNKNOWN";
    }
}

// 词法分析输出
void printLexicalAnalysis(const string& source) {
    cout << "\n========================================" << endl;
    cout << "  词法分析结果 (Lexical Analysis)" << endl;
    cout << "========================================\n" << endl;
    
    cout << left << setw(20) << "单词(Token)" 
         << setw(15) << "词素(Lexeme)" 
         << setw(20) << "属性(Attribute)" << endl;
    cout << string(55, '-') << endl;
    
    Lexer lexer(source);
    Token tok;
    
    while ((tok = lexer.getNextToken()).type != TOK_EOF) {
        string tokenName = getTokenTypeName(tok.type);
        string attribute = "";
        
        // 为标识符添加符号表指针（模拟地址）
        if (tok.type == TOK_ID) {
            stringstream ss;
            ss << "0x" << hex << (hash<string>{}(tok.lexeme) & 0xFFFFFF);
            attribute = ss.str();
        }
        // 为数字添加数值属性
        else if (tok.type == TOK_INT_LIT || tok.type == TOK_FLOAT_LIT) {
            attribute = tok.lexeme;
        }
        // 关键字显示词素
        else if (tok.type >= TOK_INT && tok.type <= TOK_RETURN) {
            attribute = tok.lexeme;
        }
        
        cout << left << setw(20) << tokenName 
             << setw(15) << tok.lexeme 
             << setw(20) << attribute << endl;
    }
    cout << endl;
}

// 语法树可视化
void printSyntaxTree(ASTNode* node, int depth = 0, const string& prefix = "") {
    if (!node) return;
    
    string indent(depth * 2, ' ');
    
    // 表达式节点
    if (auto lit = dynamic_cast<LiteralNode*>(node)) {
        cout << indent << prefix << "Literal: " << lit->val << endl;
    }
    else if (auto id = dynamic_cast<IDNode*>(node)) {
        cout << indent << prefix << "ID: " << id->name << endl;
    }
    else if (auto bin = dynamic_cast<BinaryExpr*>(node)) {
        cout << indent << prefix << "BinaryOp: " << bin->op << endl;
        printSyntaxTree(bin->left, depth + 1, "├─ Left: ");
        printSyntaxTree(bin->right, depth + 1, "└─ Right: ");
    }
    else if (auto assign = dynamic_cast<AssignExpr*>(node)) {
        cout << indent << prefix << "Assignment" << endl;
        printSyntaxTree(assign->left, depth + 1, "├─ LHS: ");
        printSyntaxTree(assign->right, depth + 1, "└─ RHS: ");
    }
    else if (auto arr = dynamic_cast<ArrayIndexExpr*>(node)) {
        cout << indent << prefix << "ArrayIndex" << endl;
        printSyntaxTree(arr->array, depth + 1, "├─ Array: ");
        printSyntaxTree(arr->index, depth + 1, "└─ Index: ");
    }
    else if (auto call = dynamic_cast<FunctionCallExpr*>(node)) {
        cout << indent << prefix << "FunctionCall: " << call->funcName << endl;
        for (size_t i = 0; i < call->args.size(); i++) {
            string argPrefix = (i == call->args.size() - 1) ? "└─ Arg" : "├─ Arg";
            printSyntaxTree(call->args[i], depth + 1, argPrefix + to_string(i) + ": ");
        }
    }
    else if (auto mem = dynamic_cast<MemberExpr*>(node)) {
        cout << indent << prefix << "Member: " << mem->memberName 
             << (mem->isArrow ? " (->)" : " (.)") << endl;
        printSyntaxTree(mem->base, depth + 1, "└─ Object: ");
    }
    else if (auto deref = dynamic_cast<DerefExpr*>(node)) {
        cout << indent << prefix << "Dereference (*)" << endl;
        printSyntaxTree(deref->ptr, depth + 1, "└─ Expr: ");
    }
    else if (auto addr = dynamic_cast<AddrExpr*>(node)) {
        cout << indent << prefix << "AddressOf (&)" << endl;
        printSyntaxTree(addr->target, depth + 1, "└─ Expr: ");
    }
    // 语句节点
    else if (auto block = dynamic_cast<BlockStmt*>(node)) {
        cout << indent << prefix << "Block {" << endl;
        for (auto stmt : block->stmts) {
            printSyntaxTree(stmt, depth + 1, "");
        }
        cout << indent << "}" << endl;
    }
    else if (auto ifStmt = dynamic_cast<IfStmt*>(node)) {
        cout << indent << prefix << "If Statement" << endl;
        printSyntaxTree(ifStmt->cond, depth + 1, "├─ Condition: ");
        printSyntaxTree(ifStmt->thenStmt, depth + 1, "├─ Then: ");
        if (ifStmt->elseStmt) {
            printSyntaxTree(ifStmt->elseStmt, depth + 1, "└─ Else: ");
        }
    }
    else if (auto whileStmt = dynamic_cast<WhileStmt*>(node)) {
        cout << indent << prefix << "While Loop" << endl;
        printSyntaxTree(whileStmt->cond, depth + 1, "├─ Condition: ");
        printSyntaxTree(whileStmt->body, depth + 1, "└─ Body: ");
    }
    else if (auto forStmt = dynamic_cast<ForStmt*>(node)) {
        cout << indent << prefix << "For Loop" << endl;
        printSyntaxTree(forStmt->init, depth + 1, "├─ Init: ");
        printSyntaxTree(forStmt->cond, depth + 1, "├─ Condition: ");
        if (forStmt->inc) {
            printSyntaxTree(forStmt->inc, depth + 1, "├─ Increment: ");
        }
        printSyntaxTree(forStmt->body, depth + 1, "└─ Body: ");
    }
    else if (auto ioStmt = dynamic_cast<IOStmt*>(node)) {
        cout << indent << prefix << "IO: " << ioStmt->op << endl;
        printSyntaxTree(ioStmt->expr, depth + 1, "└─ Expr: ");
    }
    else if (auto retStmt = dynamic_cast<ReturnStmt*>(node)) {
        cout << indent << prefix << "Return" << endl;
        if (retStmt->expr) {
            printSyntaxTree(retStmt->expr, depth + 1, "└─ Value: ");
        }
    }
    else if (auto funcDef = dynamic_cast<FunctionDef*>(node)) {
        cout << indent << prefix << "Function: " << funcDef->name << endl;
        cout << indent << "  Parameters: ";
        for (size_t i = 0; i < funcDef->params.size(); i++) {
            cout << funcDef->params[i].first;
            if (i < funcDef->params.size() - 1) cout << ", ";
        }
        cout << endl;
        printSyntaxTree(funcDef->body, depth + 1, "└─ Body: ");
    }
}

// 代码优化分析报告
void printOptimizationReport(const OptimizationStats& stats) {
    cout << "\n========================================" << endl;
    cout << "  代码优化分析 (Optimizations)" << endl;
    cout << "========================================\n" << endl;
    
    cout << "✓ 常数折叠 (Constant Folding): 在AST生成阶段自动执行 (如 10+20 -> 30)" << endl;
    
    cout << "✓ 代数简化 (Algebraic Simplification): " << stats.algebraicSimplifications << " 处" << endl;
    if (stats.algebraicSimplifications > 0) {
        cout << "  (已应用: x+0->x, x*1->x, x*0->0 等)" << endl;
    } else {
        cout << "  (未检测到可优化的代数模式)" << endl;
    }
    
    cout << "✓ 死代码消除 (Dead Code Elimination): " << stats.deadCodeEliminations << " 条指令" << endl;
    if (stats.deadCodeEliminations > 0) {
        cout << "  (已移除未使用的临时变量赋值)" << endl;
    } else {
        cout << "  (未检测到死代码)" << endl;
    }
    cout << endl;
}

// 错误分析
void printErrorAnalysis() {
    cout << "\n========================================" << endl;
    cout << "  错误分析能力 (Error Analysis)" << endl;
    cout << "========================================\n" << endl;
    
    cout << "支持的错误检测类型：" << endl;
    cout << "1. 词法错误 - 非法字符、非法标识符" << endl;
    cout << "2. 语法错误 - 缺少分号、括号不匹配" << endl;
    cout << "3. 语义错误 - 未定义变量、类型不匹配" << endl;
    cout << "4. 作用域错误 - 变量重定义、作用域外访问" << endl;
    cout << "\n错误信息格式: [Error] Line X: 错误描述" << endl;
    cout << endl;
}

// 生成汇编代码
void genAssembly() {
    cout << "\n========================================" << endl;
    cout << "  x86汇编代码 (Assembly Code)" << endl;
    cout << "========================================\n" << endl;
    
    cout << ".386" << endl;
    cout << ".model flat, stdcall" << endl;
    cout << "option casemap:none" << endl;
    cout << "include \\masm32\\include\\masm32.inc" << endl;
    cout << "include \\masm32\\include\\kernel32.inc" << endl;
    cout << "includelib \\masm32\\lib\\masm32.lib" << endl;
    cout << "includelib \\masm32\\lib\\kernel32.lib\n" << endl;
    
    cout << ".DATA" << endl;
    for (const auto& var : symTable.allVariables) {
        cout << "    " << var << " DWORD 0" << endl;
    }
    cout << "    fmt_in db \"%d\", 0" << endl;
    cout << "    fmt_out db \"%d\", 0ah, 0" << endl;
    cout << "    tmp_buf DWORD ?\n" << endl;
    
    cout << ".CODE" << endl;
    cout << "start:" << endl;
    
    for (const auto& q : quads) {
        cout << "; (" << q.op << ", " << q.arg1 << ", " << q.arg2 << ", " << q.res << ")" << endl;
        
        if (q.op == "=") {
            cout << "    MOV EAX, " << q.arg1 << endl;
            cout << "    MOV " << q.res << ", EAX" << endl;
        }
        else if (q.op == "+") {
            cout << "    MOV EAX, " << q.arg1 << endl;
            cout << "    ADD EAX, " << q.arg2 << endl;
            cout << "    MOV " << q.res << ", EAX" << endl;
        }
        else if (q.op == "-") {
            cout << "    MOV EAX, " << q.arg1 << endl;
            cout << "    SUB EAX, " << q.arg2 << endl;
            cout << "    MOV " << q.res << ", EAX" << endl;
        }
        else if (q.op == "*") {
            cout << "    MOV EAX, " << q.arg1 << endl;
            cout << "    IMUL EAX, " << q.arg2 << endl;
            cout << "    MOV " << q.res << ", EAX" << endl;
        }
        else if (q.op == "/") {
            cout << "    MOV EAX, " << q.arg1 << endl;
            cout << "    CDQ" << endl;
            cout << "    IDIV " << q.arg2 << endl;
            cout << "    MOV " << q.res << ", EAX" << endl;
        }
        else if (q.op == "LABEL") {
            cout << q.arg1 << ":" << endl;
        }
        else if (q.op == "JMP") {
            cout << "    JMP " << q.res << endl;
        }
        else if (q.op == "JZ") {
            cout << "    MOV EAX, " << q.arg1 << endl;
            cout << "    CMP EAX, 0" << endl;
            cout << "    JE " << q.res << endl;
        }
        else if (q.op == "WRITE") {
            cout << "    invoke crt_printf, addr fmt_out, " << q.arg1 << endl;
        }
    }
    
    cout << "    invoke ExitProcess, 0" << endl;
    cout << "end start" << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "用法: " << argv[0] << " <输入文件>" << endl;
        return 1;
    }

    ifstream file(argv[1]);
    if (!file) {
        cerr << "无法打开文件: " << argv[1] << endl;
        return 1;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string source = buffer.str();

    try {
        // 1. 词法分析
        printLexicalAnalysis(source);
        
        // 2. 语法分析
        Lexer lexer(source);
        Parser parser(lexer);
        parser.parseProgram();
        
        cout << "\n========================================" << endl;
        cout << "  语法分析树 (Syntax Tree)" << endl;
        cout << "========================================\n" << endl;
        
        for (ASTNode* node : parser.programAST) {
            printSyntaxTree(node, 0, "");
        }
        
        // 3. 中间代码生成 (原始)
        cout << "\n========================================" << endl;
        cout << "  中间代码 - 原始 (Original Quads)" << endl;
        cout << "========================================\n" << endl;
        
        for (const auto& q : quads) {
            cout << "(" << q.op << ", " << q.arg1 << ", " << q.arg2 << ", " << q.res << ")" << endl;
        }
        
        // 4. 执行优化
        OptimizationStats stats = optimizeQuads();
        printOptimizationReport(stats);
        
        // 5. 中间代码 (优化后)
        cout << "\n========================================" << endl;
        cout << "  中间代码 - 优化后 (Optimized Quads)" << endl;
        cout << "========================================\n" << endl;
        
        for (const auto& q : quads) {
            cout << "(" << q.op << ", " << q.arg1 << ", " << q.arg2 << ", " << q.res << ")" << endl;
        }
        
        // 6. 错误分析能力说明
        printErrorAnalysis();
        
        // 7. 汇编代码生成
        genAssembly();
        
    } catch (const exception& e) {
        string msg = e.what();
        if (msg.find("[Error]") == string::npos) {
            cerr << "\n[Error] " << msg << endl;
        } else {
            cerr << "\n" << msg << endl;
        }
        return 1;
    }

    return 0;
}
