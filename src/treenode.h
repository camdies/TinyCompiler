//fileName: TinyCompiler // src // treenode.h
//coding: UTF-8 with BOM

//this Program is for SCNU_Compiler 2026 Experiment3
//@auther Cai Renbin


/****************************************************/
/* treenode.h                                       */
/* 语法树节点定义                                   */
/****************************************************/

#ifndef TREENODE_H
#define TREENODE_H

#include "globals.h"
#include <string>
#include <vector>
#include <memory>

/*=============================================*/
/*           语法树节点类                      */
/*=============================================*/
class TreeNode {
public:
    // 子节点 (最多MAXCHILDREN个)
    TreeNode* child[MAXCHILDREN];

    // 兄弟节点（同一层级的下一条语句）
    TreeNode* sibling;

    // 源代码行号
    int lineno;

    // 节点大类
    NodeKind nodekind;

    // 节点子类型（根据nodekind选择使用哪个）
    union {
        StmtKind stmt;
        ExpKind  exp;
        RegexKind regex;
    } kind;

    // 节点属性
    struct {
        TokenType op;       // 运算符类型 (OpK时使用)
        double val;            // 支持浮点数的常量值 (ConstK时使用)
        std::string name;   // 标识符名/字母 (IdK, LetterK, AssignK, ReadK等)
    } attr;

    // 表达式类型 (类型检查时使用)
    ExpType type;

    // 构造函数
    TreeNode();
    ~TreeNode();

    // 获取节点的显示文本
    std::string getDisplayText() const;
};

// 创建语句节点
TreeNode* newStmtNode(StmtKind kind, int lineno);

// 创建表达式节点
TreeNode* newExpNode(ExpKind kind, int lineno);

// 创建正则表达式节点
TreeNode* newRegexNode(RegexKind kind, int lineno);

// 释放语法树内存
void deleteTree(TreeNode* tree);

#endif // TREENODE_H