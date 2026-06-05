//fileName: TinyCompiler // src // codegen.h
//coding: UTF-8 with BOM

//this Program is for SCNU_Compiler 2026 Experiment5
//@auther Cai Renbin

/****************************************************/
/* codegen.h                                        */
/* 扩充TINY语言的中间代码(四元组)生成器接口         */
/* 基于语法树(TreeNode)遍历生成四元组               */
/****************************************************/

#ifndef CODEGEN_H
#define CODEGEN_H

#include "globals.h"
#include "treenode.h"
#include <string>
#include <vector>

/*=============================================*/
/*  四元组结构体                               */
/*=============================================*/
struct Quadruple {
    int    index;
    std::string op;
    std::string arg1;
    std::string arg2;
    std::string result;

    Quadruple(int idx, const std::string& o,
        const std::string& a1, const std::string& a2,
        const std::string& r)
        : index(idx), op(o), arg1(a1), arg2(a2), result(r) {
    }
};

/*=============================================*/
/*  操作数描述(用于表达式生成)               */
/*=============================================*/
struct Operand {
    bool isTemp;
    std::string name;
    std::string numVal;
    bool isConst;

    Operand() : isTemp(false), isConst(false) {}

    static Operand makeVar(const std::string& n) {
        Operand o; o.name = n; o.isTemp = false; o.isConst = false;
        return o;
    }
    static Operand makeTemp(const std::string& n) {
        Operand o; o.name = n; o.isTemp = true; o.isConst = false;
        return o;
    }
    static Operand makeConst(const std::string& v) {
        Operand o; o.numVal = v; o.isConst = true; o.isTemp = false;
        return o;
    }
    std::string str() const {
        if (isConst) return numVal;
        return name;
    }
};

/*=============================================*/
/*  布尔结果(必须在类外部定义,否则MSVC报错)  */
/*=============================================*/
struct BoolResult {
    std::vector<int> trueList;
    std::vector<int> falseList;
};

/*=============================================*/
/*  中间代码生成器                             */
/*=============================================*/
class CodeGenerator {
public:
    CodeGenerator();

    void generate(TreeNode* root);

    const std::vector<Quadruple>& getQuadruples() const { return quads_; }
    const std::vector<std::string>& getErrors() const { return errors_; }

    void clear();

private:
    std::vector<Quadruple> quads_;
    std::vector<std::string> errors_;
    int tempCount_;
    int quadCount_;

    /*--- 辅助函数 ---*/
    std::string newTemp();
    int nextQuad() const;
    int emitQuad(const std::string& op,
        const std::string& arg1,
        const std::string& arg2,
        const std::string& result);
    void backpatch(const std::vector<int>& patchList, int target);
    std::vector<int> merge(const std::vector<int>& a, const std::vector<int>& b);

    /*--- 各类语句生成 ---*/
    void genStmtSequence(TreeNode* node);
    void genStatement(TreeNode* node);
    void genIfStmt(TreeNode* node);
    void genRepeatStmt(TreeNode* node);
    void genWhileStmt(TreeNode* node);
    void genForStmt(TreeNode* node);
    void genAssignStmt(TreeNode* node);
    void genReadStmt(TreeNode* node);
    void genWriteStmt(TreeNode* node);
    void genRegAssign(TreeNode* node);

    /*--- 表达式生成,返回操作数描述 ---*/
    Operand genExp(TreeNode* node);
    Operand genOpExp(TreeNode* node);
    Operand genSelfExp(TreeNode* node);

    /*--- 布尔条件生成(回填式) ---*/
    BoolResult genBoolExp(TreeNode* node);
};

#endif // CODEGEN_H