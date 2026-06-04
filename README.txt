========================================================================
项目名称：基于C++的C语言子集编译器 (支持Struct/Pointer/Float)
小组成员：
1. sdy - 负责: 词法/语法分析器核心, Struct类型系统设计
2. gj - 负责: 中间代码生成 (四元式), 控制流 (If/While) 实现
========================================================================

一、 参考资料与致谢 (Reference)
本项目的基础词法分析思路参考了以下开源项目：
1. https://github.com/rothso/c-minus (C- 递归下降编译器，参考了递归下降解析框架)
2. https://github.com/fauzanzaid/Quadruple-Three-Address-Code-in-C (四元式中间代码生成参考)

【本项目与参考代码的区别】：
1. 类型系统重写：参考代码仅支持 int。本项目独立设计了 Type 基类，实现了"结构体(Struct)"、"多级指针(Pointer)"、"数组(Array)"和"函数(Function)"的递归定义与解析。
2. 左值处理机制：为了支持 `*p = 10;` 这种指针赋值，我们重写了语法分析层，区分了 L-value 和 R-value，引入了 ADDR 和 DEREF 指令。
3. 实型扩展：词法和语法层完整扩展了 float 类型支持。
4. 高级特性：实现了数组（一维/多维）和函数调用机制，这是参考代码中没有的。

二、 项目架构
1. Lexer (词法): 支持 int/float/struct 关键字，识别 >=, &&, ||, [] 等复杂运算符.
2. Parser (语法): 采用递归下降分析法，实现了完整的运算符优先级控制.
3. CodeGen (生成): 生成标准四元式，支持 Label 跳转、函数调用、数组索引等机制.
4. Type System (类型系统): 支持基本类型、指针、结构体、数组、函数类型的完整层次结构.

三、 运行方式
编译：g++ main.cpp -o compiler
运行：./compiler input.txt

四、 功能演示 (详见附图)
1. 基础运算与Float支持 (test_basic.txt + result_basic.txt)
2. 流程控制 If/While/For (test_flow.txt + result_flow.txt)
3. 高级特性 Struct/Pointer (test_advanced.txt + result_advanced.txt) - *核心创新点*
4. 数组功能 (test_array.txt) - 一维数组声明、索引、遍历
5. 函数调用 (test_function.txt) - 函数定义、参数传递、递归调用
6. 综合应用 (test_array_function.txt) - 数组作为函数参数

五、 完成的改进思路 (满分5项)
 1. 实型数据扩展 (float)
 2. 函数调用支持 (定义、调用、参数、返回值、递归)
 3. 数组运算 (一维数组声明、索引访问、作为函数参数)
 4. 指针运算 (多级指针、取地址&、解引用*)
 5. 结构体支持 (定义、成员访问 . 和 ->)
 
