//fileName: TinyCompiler // src // treenode.cpp
//coding: UTF-8 with BOM

//this Program is for SCNU_Compiler 2026 Experiment3
//@auther Cai Renbin


/****************************************************/
/* treenode.cpp                                     */
/* 语法树节点实现                                     */
/****************************************************/

#include "treenode.h"

TreeNode::TreeNode()
    : sibling(nullptr), lineno(0),
    nodekind(NodeKind::StmtK), type(ExpType::Void)
{
    for (int i = 0; i < MAXCHILDREN; i++)
        child[i] = nullptr;
    kind.stmt = StmtKind::IfK;
    attr.op = TokenType::ERROR_TOKEN;
    attr.val = 0;
}

TreeNode::~TreeNode()
{
    // 注意：不在析构中递归删除，使用deleteTree统一管理
}

/*=============================================*/
/* 获取节点的显示文本，用于语法树可视化            */
/*=============================================*/
std::string TreeNode::getDisplayText() const
{
    switch (nodekind) {
    case NodeKind::StmtK:
        switch (kind.stmt) {
        case StmtKind::IfK:        return "if";
        case StmtKind::RepeatK:    return "repeat";
        case StmtKind::AssignK: {
            switch (attr.op) {
            case TokenType::ASSIGN:        return ":=";
            case TokenType::PLUS_ASSIGN:   return "+=";
            case TokenType::MINUS_ASSIGN:  return "-=";
            default:                       return "assign";
            }
        }
        case StmtKind::RegAssignK: return "::=";
        case StmtKind::ReadK:      return "read";
        case StmtKind::WriteK:     return "write";
        case StmtKind::WhileK:     return "while";
        case StmtKind::ForK:       return "for";
        }
        break;

    case NodeKind::ExpK:
        switch (kind.exp) {
        case ExpKind::OpK: {
            switch (attr.op) {
            case TokenType::PLUS:   return "+";
            case TokenType::MINUS:  return "-";
            case TokenType::TIMES:  return "*";
            case TokenType::OVER:   return "/";
            case TokenType::MOD:    return "%";
            case TokenType::POWER:  return "^";
            case TokenType::LT:     return "<";
            case TokenType::GT:     return ">";
            case TokenType::EQ:     return "=";
            case TokenType::LE:     return "<=";
            case TokenType::GE:     return ">=";
            case TokenType::NE:     return "<>";
            case TokenType::ARROW:  return "=>";
            default:                return "op(?)";
            }
        }
        case ExpKind::ConstK:
            // 统一用 attr.name 存储原始数字字符串
            return "number(" + attr.name + ")";

        /*
        case ExpKind::ConstK:
            if (!attr.name.empty()) {
                // 浮点数或科学计数法，显示原始字符串
                return "number(" + attr.name + ")";
            }
            // 整数也用name存储，统一走上面分支
            return "number(" + attr.name + ")";
        */

        case ExpKind::IdK:
            return "id(" + attr.name + ")";
        case ExpKind::SelfIncK:
            return "++";
        case ExpKind::SelfDecK:
            return "--";
        case ExpKind::LetterK:
            return "char('" + attr.name + "')";
        }
        break;

    case NodeKind::RegexK:
        switch (kind.regex) {
        case RegexKind::OrK:       return "|";
        case RegexKind::ConcatK:   return "&";
        case RegexKind::ClosureK:  return "#";
        case RegexKind::OptionalK: return "?";
        case RegexKind::LetterK:   return "char('" + attr.name + "')";
        case RegexKind::IdK:       return "id(" + attr.name + ")";
        }
        break;
    }
    return "unknown";
}

/*=============================================*/
/* 工厂函数：创建语句节点                        */
/*=============================================*/
TreeNode* newStmtNode(StmtKind kind, int lineno)
{
    TreeNode* t = new TreeNode();
    t->nodekind = NodeKind::StmtK;
    t->kind.stmt = kind;
    t->lineno = lineno;
    return t;
}

/*=============================================*/
/* 工厂函数：创建表达式节点                      */
/*=============================================*/
TreeNode* newExpNode(ExpKind kind, int lineno)
{
    TreeNode* t = new TreeNode();
    t->nodekind = NodeKind::ExpK;
    t->kind.exp = kind;
    t->lineno = lineno;
    t->type = ExpType::Void;
    return t;
}

/*=============================================*/
/* 工厂函数：创建正则表达式节点                   */
/*=============================================*/
TreeNode* newRegexNode(RegexKind kind, int lineno)
{
    TreeNode* t = new TreeNode();
    t->nodekind = NodeKind::RegexK;
    t->kind.regex = kind;
    t->lineno = lineno;
    return t;
}

/*=============================================*/
/* 递归释放语法树内存                            */
/*=============================================*/
void deleteTree(TreeNode* tree)
{
    if (tree == nullptr) return;
    for (int i = 0; i < MAXCHILDREN; i++)
        deleteTree(tree->child[i]);
    deleteTree(tree->sibling);
    delete tree;
}