//fileName: TinyCompiler // src // scanner.cpp
//coding: UTF-8 with BOM

//this Program is for SCNU_Compiler 2026 Experiment3
//@auther Cai Renbin


/****************************************************/
/* parser.h                                         */
/* 扩充TINY语言的语法分析器接口                       */
/* 递归下降分析法，生成语法树                         */
/*                                                   */
/* 文法规则:                                         */
/* program -> stmt-sequence                          */
/* stmt-sequence -> statement { ; statement }        */
/* statement -> if-stmt | repeat-stmt | for-stmt     */
/*            | while-stmt | assign-stmt             */
/*            | read-stmt | write-stmt               */
/* if-stmt -> if(exp) stmt-seq [else stmt-seq] endif */
/* repeat-stmt -> repeat stmt-seq until exp          */
/* for-stmt -> for(assign; exp; selfexp)             */
/*             stmt-seq endfor                       */
/* while-stmt -> while(exp) stmt-seq endwhile        */
/* assign-stmt -> id assign-sub-stmt                 */
/* assign-sub-stmt -> := exp | += exp | -= exp       */
/*                  | ::= re                         */
/* read-stmt -> read id                              */
/* write-stmt -> write (exp | letter)                */
/* exp -> simple-exp [comp-op simple-exp]            */
/* simple-exp -> term { addop term }                 */
/* term -> factor { mulop factor }                   */
/* factor -> resexp { ^ resexp }                     */
/* resexp -> (exp) | number | id | selfexp           */
/* selfexp -> selfop id                              */
/* selfop -> ++ | --                                 */
/* re -> orre { | orre }                             */
/* orre -> conre { & conre }                         */
/* conre -> repre [ repop ]                          */
/* repre -> (re) | letter | id                       */
/* repop -> # | ?                                    */
/****************************************************/

#ifndef PARSER_H
#define PARSER_H

#include "globals.h"
#include "scanner.h"
#include "treenode.h"
#include <string>
#include <vector>

class Parser {
public:
    Parser();

    // 解析源代码，返回语法树根节点
    TreeNode* parse(const std::string& source);

    // 获取错误信息
    const std::vector<std::string>& getErrors() const { return errors_; }

    // 获取词法分析的token列表（供界面展示）
    const std::vector<Token>& getTokens() const { return tokens_; }

    // 获取词法分析错误
    const std::vector<std::string>& getScanErrors() const { return scanErrors_; }

private:
    Scanner scanner_;
    Token currentToken_;
    std::vector<std::string> errors_;
    std::vector<std::string> scanErrors_;
    std::vector<Token> tokens_;

    // 辅助函数
    void nextToken();                       // 获取下一个token
    void match(TokenType expected);         // 匹配当前token
    void syntaxError(const std::string& msg); // 报告语法错误

    // 各非终结符对应的递归下降函数
    TreeNode* stmt_sequence();
    TreeNode* statement();
    TreeNode* if_stmt();
    TreeNode* repeat_stmt();
    TreeNode* while_stmt();
    TreeNode* for_stmt();
    TreeNode* assign_stmt();
    TreeNode* read_stmt();
    TreeNode* write_stmt();

    // 算术表达式相关
    TreeNode* exp();
    TreeNode* simple_exp();
    TreeNode* term();
    TreeNode* factor();
    TreeNode* resexp();
    TreeNode* selfexp();

    // 正则表达式相关
    TreeNode* re();
    TreeNode* orre();
    TreeNode* conre();
    TreeNode* repre();
};

#endif // PARSER_H