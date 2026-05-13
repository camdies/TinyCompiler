//fileName: TinyCompiler // src // globals.h
//coding: UTF-8 with BOM

//this Program is for SCNU_Compiler 2026 Experiment3
//@auther Cai Renbin


/****************************************************/
/* 扩充TINY语言编译器的全局类型和常量定义           */
/* 基于Kenneth C. Louden的TINY编译器进行扩展        */
/****************************************************/

#ifndef GLOBALS_H
#define GLOBALS_H

#include <string>
#include <vector>
#include <memory>

/*=============================================*/
/*           Token类型枚举定义                 */
/*=============================================*/
enum class TokenType {
    // 文件结束和错误
    ENDFILE, ERROR_TOKEN,

    // ---- 保留字 ----
    IF, THEN, ELSE, END,           // 原TINY保留字(THEN/END保留但不再用于新if)
    REPEAT, UNTIL,                  // repeat...until
    READ, WRITE,                    // I/O
    WHILE, ENDWHILE,                // while...endwhile (新增)
    FOR, ENDFOR,                    // for...endfor (新增)
    ENDIF,                          // endif (新增，用于新if语句)

    // ---- 多字符token ----
    ID, NUM, LETTER,                // 标识符、数字、字母(正则表达式中用)

    // ---- 赋值运算符 ----
    ASSIGN,                         // :=  (算术表达式赋值)
    REGEX_ASSIGN,                   // ::= (正则表达式赋值)

    // ---- 比较运算符 ----
    EQ,                             // =   (等于)
    LT,                             // <   (小于)
    GT,                             // >   (大于)
    LE,                             // <=  (小于等于)
    GE,                             // >=  (大于等于)
    NE,                             // <>  (不等于)
    ARROW,                          // =>  (蕴含/大于等于的另一种写法)

    // ---- 算术运算符 ----
    PLUS, MINUS, TIMES, OVER,       // + - * /
    MOD,                            // %   (求余)
    POWER,                          // ^   (乘方)

    // ---- 自增自减 ----
    INC,                            // ++  (前置自增)
    DEC,                            // --  (前置自减)

    // ---- 正则表达式运算符 ----
    REGEX_OR,                       // |   (正则或) - 注意：在re产生式中用 /
    REGEX_CONCAT,                   // &   (正则连接)
    REGEX_CLOSURE,                  // #   (正则闭包)
    REGEX_OPTIONAL,                 // ?   (正则可选)
    REGEX_SLASH,                    // /   (正则或，在你的文法中re用/分隔)

    // ---- 分隔符 ----
    LPAREN, RPAREN,                 // ( )
    SEMI,                           // ;   (分号)
};

/*=============================================*/
/*           语法树节点类型定义                */
/*=============================================*/

// 节点大类
enum class NodeKind {
    StmtK,      // 语句节点
    ExpK,       // 表达式节点
    RegexK      // 正则表达式节点 (新增)
};

// 语句子类型
enum class StmtKind {
    IfK,        // if语句
    RepeatK,    // repeat语句
    AssignK,    // 赋值语句 (:=)
    RegAssignK, // 正则赋值语句 (==)
    ReadK,      // read语句
    WriteK,     // write语句
    WhileK,     // while语句 (新增)
    ForK,       // for语句 (新增)
};

// 表达式子类型
enum class ExpKind {
    OpK,        // 运算符节点
    ConstK,     // 常量节点
    IdK,        // 标识符节点
    SelfIncK,   // 前置自增 ++id
    SelfDecK,   // 前置自减 --id
    LetterK     // 字母常量(正则/write中)
};

// 正则表达式子类型
enum class RegexKind {
    OrK,        // 正则或 (/)
    ConcatK,    // 正则连接 (&)
    ClosureK,   // 正则闭包 (#)
    OptionalK,  // 正则可选 (?)
    LetterK,    // 正则字母
    IdK         // 正则标识符
};

// 表达式值类型（用于类型检查）
enum class ExpType {
    Void,
    Integer,
    Boolean,
    Regex
};

/*=============================================*/
/*         保留字数量                          */
/*=============================================*/
constexpr int MAXRESERVED = 13;
constexpr int MAXCHILDREN = 4;  // 语法树最大子节点数(for语句需要4个)
constexpr int MAXTOKENLEN = 256;

#endif // GLOBALS_H