#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "global.h"
#include <vector>
#include <string>
#include <set>
#include <algorithm>
#include <sstream>

struct OptimizationStats {
    int algebraicSimplifications = 0;
    int deadCodeEliminations = 0;
    int constantFoldings = 0; // 已经在 AST 中统计了，但如果四元式再做一次也可以
};

// 代数简化
void algebraicSimplification(OptimizationStats& stats) {
    for (auto& q : quads) {
        if (q.op == "+") {
            if (q.arg2 == "0") { // x + 0 -> x
                q.op = "=";
                q.arg2 = "_";
                stats.algebraicSimplifications++;
            } else if (q.arg1 == "0") { // 0 + x -> x
                q.op = "=";
                q.arg1 = q.arg2;
                q.arg2 = "_";
                stats.algebraicSimplifications++;
            }
        } else if (q.op == "-") {
            if (q.arg2 == "0") { // x - 0 -> x
                q.op = "=";
                q.arg2 = "_";
                stats.algebraicSimplifications++;
            }
        } else if (q.op == "*") {
            if (q.arg2 == "1") { // x * 1 -> x
                q.op = "=";
                q.arg2 = "_";
                stats.algebraicSimplifications++;
            } else if (q.arg1 == "1") { // 1 * x -> x
                q.op = "=";
                q.arg1 = q.arg2;
                q.arg2 = "_";
                stats.algebraicSimplifications++;
            } else if (q.arg2 == "0" || q.arg1 == "0") { // x * 0 -> 0
                q.op = "=";
                q.arg1 = "0";
                q.arg2 = "_";
                stats.algebraicSimplifications++;
            }
        } else if (q.op == "/") {
            if (q.arg2 == "1") { // x / 1 -> x
                q.op = "=";
                q.arg2 = "_";
                stats.algebraicSimplifications++;
            }
        }
    }
}

// 死代码消除
void deadCodeElimination(OptimizationStats& stats) {
    bool changed = true;
    while (changed) {
        changed = false;
        std::set<std::string> usedTemps;
        
        // 统计所有作为 arg 使用的 temps，以及有副作用的指令使用的 temps
        for (const auto& q : quads) {
            if (q.op == "WRITE" || q.op == "READ" || q.op == "PARAM" || q.op == "RETURN" || q.op == "JZ" || q.op == "JMP" || q.op == "CALL" || q.op == "FUNC_BEGIN" || q.op == "FUNC_END") {
                if (q.arg1.length() > 0 && q.arg1[0] == 't') usedTemps.insert(q.arg1);
                if (q.res.length() > 0 && q.op == "JZ" && q.res[0] == 't') usedTemps.insert(q.res); 
            } else {
                if (q.arg1.length() > 0 && q.arg1[0] == 't') usedTemps.insert(q.arg1);
                if (q.arg2.length() > 0 && q.arg2[0] == 't') usedTemps.insert(q.arg2);
            }
        }
        
        std::vector<Quad> newQuads;
        int removedCount = 0;
        for (const auto& q : quads) {
            bool keep = true;
            // 检查是否是赋值给临时变量
            if ((q.op == "=" || q.op == "+" || q.op == "-" || q.op == "*" || q.op == "/" || q.op == "ARRAY_INDEX" || q.op == "DEREF" || q.op == "ADDR" || q.op == "DOT" || q.op == "ARROW") 
                && q.res.length() > 0 && q.res[0] == 't') {
                
                if (usedTemps.find(q.res) == usedTemps.end()) {
                    keep = false;
                    removedCount++;
                }
            }
            
            if (keep) {
                newQuads.push_back(q);
            }
        }
        
        if (removedCount > 0) {
            quads = newQuads;
            stats.deadCodeEliminations += removedCount;
            changed = true;
        }
    }
}

// 综合优化入口
OptimizationStats optimizeQuads() {
    OptimizationStats stats;
    algebraicSimplification(stats);
    deadCodeElimination(stats);
    return stats;
}

#endif
