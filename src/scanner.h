//fileName: TinyCompiler // src // scanner.h
//coding: UTF-8 with BOM

//this Program is for SCNU_Compiler 2026 Experiment3
//@auther Cai Renbin


/****************************************************/
/* scanner.h                                        */
/* 扩充TINY语言的词法分析器接口                       */
/* 在原TINY scanner基础上扩展:                        */
/*   新增保留字: while, endwhile, for, endfor, endif */
/*   新增运算符: <=, >=, <>, =>, ++, --, %, ^       */
/*   新增正则运算符: |, &, #, ?                      */
/*   新增赋值符: ::= (正则赋值)                        */
/****************************************************/

#ifndef SCANNER_H
#define SCANNER_H

#include "globals.h"
#include <string>
#include <vector>

/*=============================================*/
/*              Token结构体                     */
/*=============================================*/
struct Token {
    TokenType type;
    std::string lexeme;     // token的词素
    int lineno;             // 所在行号

    Token() : type(TokenType::ENDFILE), lineno(0) {}
    Token(TokenType t, const std::string& lex, int line)
        : type(t), lexeme(lex), lineno(line) {
    }
};

/*=============================================*/
/*              Scanner类                       */
/*=============================================*/
class Scanner {
public:
    Scanner();

    // 设置源代码
    void setSource(const std::string& source);

    // 获取下一个token
    Token getToken();

    // 获取所有token（用于词法分析结果展示）
    std::vector<Token> tokenize();

    // 获取错误信息
    const std::vector<std::string>& getErrors() const { return errors_; }

    // 获取token类型的显示名
    static std::string tokenTypeName(TokenType type);

private:
    std::string source_;        // 源代码
    int pos_;                   // 当前位置
    int lineno_;                // 当前行号
    std::vector<std::string> errors_;

    // DFA状态
    enum class State {
        START, INASSIGN, INCOMMENT, INNUM, INID,
        INLT, INGT, INEQ, INPLUS, INMINUS,
        INCOLON,
        INDOUBLECOLON,  // 新增：读到 :: 后，等待 =
        INLETTER,
        INSCI, INSCI_SIGN, INSCI_NUM,
        INFLOAT,          // 新增：读到小数点后
        INFLOAT_SCI,      // 新增：浮点数的科学计数法 e/E
        INFLOAT_SCI_SIGN, // 新增：浮点数科学计数法的符号
        INFLOAT_SCI_NUM,  // 新增：浮点数科学计数法的指数数字
        DONE
    };

    // 辅助函数
    char getNextChar();             // 获取下一个字符
    void ungetChar();               // 回退一个字符
    bool isEOF() const;             // 是否到达末尾
    TokenType reservedLookup(const std::string& s); // 保留字查找
};

#endif // SCANNER_H