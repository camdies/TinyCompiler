//fileName: TinyCompiler // src // codegen.cpp
//coding: UTF-8 with BOM

//this Program is for SCNU_Compiler 2026 Experiment5
//@auther Cai Renbin

/****************************************************/
/* codegen.cpp                                      */
/* 扩充TINY语言的中间代码（四元组）生成器实现         */
/* 遍历语法树，生成三地址四元组形式中间代码           */
/* 条件跳转采用回填（Backpatch）技术                  */
/****************************************************/

#include "codegen.h"
#include <stdexcept>

namespace {
    /*=============================================*/
/*  获取比较运算符字符串                       */
/*=============================================*/
    std::string getOpStr(TokenType op)
    {
        switch (op) {
        case TokenType::LT:    return "<";
        case TokenType::GT:    return ">";
        case TokenType::EQ:    return "=";
        case TokenType::LE:    return "<=";
        case TokenType::GE:    return ">=";
        case TokenType::NE:    return "<>";
        case TokenType::ARROW: return "=>";
        default:               return "?";
        }
    }

    /*=============================================*/
    /*  获取算术运算符字符串                       */
    /*=============================================*/
    std::string getArithOpStr(TokenType op)
    {
        switch (op) {
        case TokenType::PLUS:  return "+";
        case TokenType::MINUS: return "-";
        case TokenType::TIMES: return "*";
        case TokenType::OVER:  return "/";
        case TokenType::MOD:   return "%";
        case TokenType::POWER: return "^";
        default:               return "?";
        }
    }
}  // anonymous namespace

CodeGenerator::CodeGenerator()
    : tempCount_(0), quadCount_(0)
{
}

void CodeGenerator::clear()
{
    quads_.clear();
    errors_.clear();
    tempCount_ = 0;
    quadCount_ = 0;
}

/*=============================================*/
/*  入口                                        */
/*=============================================*/
void CodeGenerator::generate(TreeNode* root)
{
    clear();
    if (root == nullptr) return;
    genStmtSequence(root);
    finalize();
}

/*=============================================*/
/*  辅助函数                                    */
/*=============================================*/
std::string CodeGenerator::newTemp()
{
    return "T" + std::to_string(++tempCount_);
}

int CodeGenerator::nextQuad() const
{
    return quadCount_ + 1;
}

int CodeGenerator::emitQuad(const std::string& op,
    const std::string& arg1,
    const std::string& arg2,
    const std::string& result)
{
    ++quadCount_;
    quads_.emplace_back(quadCount_, op, arg1, arg2, result);
    return quadCount_;
}

void CodeGenerator::backpatch(const std::vector<int>& patchList, int target)
{
    std::string tgt = std::to_string(target);
    for (int idx : patchList) {
        // idx 是四元组序号（1-based），对应 quads_[idx-1]
        if (idx >= 1 && idx <= static_cast<int>(quads_.size())) {
            quads_[idx - 1].result = tgt;
        }
    }
}

std::vector<int> CodeGenerator::merge(const std::vector<int>& a,
    const std::vector<int>& b)
{
    std::vector<int> res = a;
    res.insert(res.end(), b.begin(), b.end());
    return res;
}

/*=============================================*/
/*  语句序列                                    */
/*=============================================*/
void CodeGenerator::genStmtSequence(TreeNode* node)
{
    TreeNode* cur = node;
    while (cur != nullptr) {
        genStatement(cur);
        cur = cur->sibling;
    }
}

/*=============================================*/
/*  单条语句分发                                */
/*=============================================*/
void CodeGenerator::genStatement(TreeNode* node)
{
    if (node == nullptr) return;
    if (node->nodekind != NodeKind::StmtK) {
        // 表达式作为独立语句（selfexp等）
        genExp(node);
        return;
    }

    switch (node->kind.stmt) {
    case StmtKind::IfK:
        genIfStmt(node);
        break;
    case StmtKind::RepeatK:
        genRepeatStmt(node);
        break;
    case StmtKind::WhileK:
        genWhileStmt(node);
        break;
    case StmtKind::ForK:
        genForStmt(node);
        break;
    case StmtKind::AssignK:
        genAssignStmt(node);
        break;
    case StmtKind::RegAssignK:
        genRegAssign(node);
        break;
    case StmtKind::ReadK:
        genReadStmt(node);
        break;
    case StmtKind::WriteK:
        genWriteStmt(node);
        break;
    default:
        errors_.push_back("未知语句类型");
        break;
    }
}

/*=============================================*/
/*  if 语句                                    */
/*  if(exp) stmt-seq [else stmt-seq] endif     */
/*  生成：                                      */
/*   genBoolExp(cond) -> {trueList, falseList}  */
/*   backpatch(trueList, nextQuad)              */
/*   genStmtSeq(then-branch)                    */
/*   if else: emit(j, _, _, ?)   // 跳过else   */
/*   backpatch(falseList, nextQuad)             */
/*   genStmtSeq(else-branch)                    */
/*   backpatch(跳过else的四元组, nextQuad)       */
/*=============================================*/
void CodeGenerator::genIfStmt(TreeNode* node)
{
    // child[0]=条件, child[1]=then分支, child[2]=else分支(可选)
    TreeNode* condNode = node->child[0];
    TreeNode* thenNode = node->child[1];
    TreeNode* elseNode = node->child[2];

    // 生成条件布尔表达式
    BoolResult boolRes = genBoolExp(condNode);

    // 真入口：回填 trueList
    int trueEntry = nextQuad();
    backpatch(boolRes.trueList, trueEntry);

    // 生成 then 分支
    genStmtSequence(thenNode);

    if (elseNode != nullptr) {
        // 生成无条件跳转（跳过 else），目标待回填
        int jmpIdx = emitQuad("j", "_", "_", "0");
        std::vector<int> jumpList = { jmpIdx };

        // 假入口：回填 falseList
        int falseEntry = nextQuad();
        backpatch(boolRes.falseList, falseEntry);

        // 生成 else 分支
        genStmtSequence(elseNode);

        // 跳过else的无条件跳转目标 = 当前下一条
        int afterElse = nextQuad();
        backpatch(jumpList, afterElse);
    }
    else {
        // 无 else：回填 falseList 到 if 结束后
        int afterThen = nextQuad();
        backpatch(boolRes.falseList, afterThen);
    }
}

/*=============================================*/
/*  repeat 语句                                */
/*  repeat stmt-seq until exp                  */
/*  生成：                                     */
/*   loopStart = nextQuad                      */
/*   genStmtSeq(body)                          */
/*   genBoolExp(cond) -> {trueList, falseList} */
/*   backpatch(falseList, loopStart)  // 假则继续循环 */
/*   backpatch(trueList, nextQuad)   // 真则退出  */
/*=============================================*/
void CodeGenerator::genRepeatStmt(TreeNode* node)
{
    // child[0]=循环体, child[1]=终止条件
    TreeNode* bodyNode = node->child[0];
    TreeNode* condNode = node->child[1];

    int loopStart = nextQuad();

    // 生成循环体
    genStmtSequence(bodyNode);

    // 生成终止条件（布尔表达式）
    BoolResult boolRes = genBoolExp(condNode);

    // 条件为真 => 退出循环（回填 trueList 到循环后）
    int afterLoop = nextQuad();
    backpatch(boolRes.trueList, afterLoop);

    // 条件为假 => 继续循环（回填 falseList 到 loopStart）
    backpatch(boolRes.falseList, loopStart);
}

/*=============================================*/
/*  while 语句                                 */
/*  while(exp) stmt-seq endwhile               */
/*  生成：                                     */
/*   loopStart = nextQuad                      */
/*   genBoolExp(cond) -> {trueList, falseList} */
/*   backpatch(trueList, nextQuad)             */
/*   genStmtSeq(body)                          */
/*   emit(j, _, _, loopStart)                  */
/*   backpatch(falseList, nextQuad)            */
/*=============================================*/
void CodeGenerator::genWhileStmt(TreeNode* node)
{
    // child[0]=循环条件, child[1]=循环体
    TreeNode* condNode = node->child[0];
    TreeNode* bodyNode = node->child[1];

    int loopStart = nextQuad();

    // 生成条件
    BoolResult boolRes = genBoolExp(condNode);

    // 真入口
    int trueEntry = nextQuad();
    backpatch(boolRes.trueList, trueEntry);

    // 生成循环体
    genStmtSequence(bodyNode);

    // 无条件跳回循环头
    emitQuad("j", "_", "_", std::to_string(loopStart));

    // 假出口 => 循环后
    int afterLoop = nextQuad();
    backpatch(boolRes.falseList, afterLoop);
}

/*=============================================*/
/*  for 语句                                   */
/*  for(assign; exp; selfexp) stmt-seq endfor  */
/*  child[0]=init-assign, child[1]=cond,       */
/*  child[2]=body, child[3]=step               */
/*  生成：                                     */
/*   genAssignStmt(init)                       */
/*   loopStart = nextQuad                      */
/*   genBoolExp(cond) -> {trueList, falseList} */
/*   backpatch(trueList, nextQuad)             */
/*   genStmtSeq(body)                          */
/*   genExp(step)                              */
/*   emit(j, _, _, loopStart)                  */
/*   backpatch(falseList, nextQuad)            */
/*=============================================*/
void CodeGenerator::genForStmt(TreeNode* node)
{
    TreeNode* initNode = node->child[0];
    TreeNode* condNode = node->child[1];
    TreeNode* bodyNode = node->child[2];
    TreeNode* stepNode = node->child[3];

    // 初始化
    genStatement(initNode);

    int loopStart = nextQuad();

    // 条件
    BoolResult boolRes = genBoolExp(condNode);
    int trueEntry = nextQuad();
    backpatch(boolRes.trueList, trueEntry);

    // 循环体
    genStmtSequence(bodyNode);

    // 步进
    if (stepNode != nullptr) {
        genExp(stepNode);
    }

    // 无条件跳回
    emitQuad("j", "_", "_", std::to_string(loopStart));

    // 假出口
    int afterLoop = nextQuad();
    backpatch(boolRes.falseList, afterLoop);
}

/*=============================================*/
/*  赋值语句  id := exp | id += exp | id -= exp */
/*  child[0]=id节点, child[1]=exp节点          */
/*  := 生成：(:=, expResult, _, id)             */
/*  += 生成：(+, id, expResult, Tn)             */
/*          (:=, Tn, _, id)                     */
/*  -= 生成：(-, id, expResult, Tn)             */
/*          (:=, Tn, _, id)                     */
/*=============================================*/
void CodeGenerator::genAssignStmt(TreeNode* node)
{
    // child[0] = IdK(变量名), child[1] = 表达式
    TreeNode* idNode = node->child[0];
    TreeNode* expNode = node->child[1];

    if (idNode == nullptr || expNode == nullptr) {
        errors_.push_back("赋值语句结构错误");
        return;
    }

    std::string varName = idNode->attr.name;
    TokenType assignOp = node->attr.op;

    if (assignOp == TokenType::PLUS_ASSIGN) {
        // id += exp => T1 = id + exp; id = T1
        Operand val = genExp(expNode);
        std::string tmp = newTemp();
        emitQuad("+", varName, val.str(), tmp);
        emitQuad(":=", tmp, "_", varName);
    }
    else if (assignOp == TokenType::MINUS_ASSIGN) {
        // id -= exp => T1 = id - exp; id = T1
        Operand val = genExp(expNode);
        std::string tmp = newTemp();
        emitQuad("-", varName, val.str(), tmp);
        emitQuad(":=", tmp, "_", varName);
    }
    else {
        // := 普通赋值
        Operand val = genExp(expNode);
        emitQuad(":=", val.str(), "_", varName);
    }
}

/*=============================================*/
/*  read 语句  read id                         */
/*  child[0] = IdK 节点                        */
/*  生成：(rd, _, _, id)                       */
/*=============================================*/
void CodeGenerator::genReadStmt(TreeNode* node)
{
    TreeNode* idNode = node->child[0];
    if (idNode == nullptr) {
        errors_.push_back("read语句缺少标识符");
        return;
    }
    emitQuad("rd", "_", "_", idNode->attr.name);
}

/*=============================================*/
/*  write 语句  write exp | write letter       */
/*  child[0] = exp 或 LetterK                  */
/*  生成：(WR, _, _, result)                   */
/*=============================================*/
void CodeGenerator::genWriteStmt(TreeNode* node)
{
    TreeNode* child = node->child[0];
    if (child == nullptr) {
        errors_.push_back("write语句缺少参数");
        return;
    }

    if (child->nodekind == NodeKind::ExpK &&
        child->kind.exp == ExpKind::LetterK)
    {
        // write letter: 直接输出字符常量
        emitQuad("WR", "_", "_", "'" + child->attr.name + "'");
    }
    else {
        Operand val = genExp(child);
        emitQuad("WR", "_", "_", val.str());
    }
}

/*=============================================*/
/*  正则赋值语句  id ::= re                    */
/*  仅生成占位注释四元组，不做实际RE计算        */
/*=============================================*/
void CodeGenerator::genRegAssign(TreeNode* node)
{
    TreeNode* idNode = node->child[0];
    if (idNode == nullptr) return;
    // 正则赋值：生成注释性四元组
    emitQuad("::=", "re", "_", idNode->attr.name);
}

/*=============================================*/
/*  表达式生成（返回操作数）                   */
/*  根据节点类型分发                            */
/*=============================================*/
Operand CodeGenerator::genExp(TreeNode* node)
{
    if (node == nullptr) return Operand::makeConst("0");

    if (node->nodekind == NodeKind::ExpK) {
        switch (node->kind.exp) {
        case ExpKind::ConstK:
            return Operand::makeConst(node->attr.name);

        case ExpKind::IdK:
            return Operand::makeVar(node->attr.name);

        case ExpKind::OpK:
            return genOpExp(node);

        case ExpKind::SelfIncK:
        case ExpKind::SelfDecK:
            return genSelfExp(node);

        case ExpKind::LetterK:
            return Operand::makeConst("'" + node->attr.name + "'");

        default:
            break;
        }
    }

    // StmtK作为表达式（不应出现，但兜底）
    if (node->nodekind == NodeKind::StmtK) {
        genStatement(node);
        return Operand::makeConst("_");
    }

    return Operand::makeConst("0");
}

/*=============================================*/
/*  运算符表达式生成                            */
/*  对比较运算符：生成条件跳转四元组            */
/*  对算术运算符：生成计算四元组并返回临时变量  */
/*=============================================*/
Operand CodeGenerator::genOpExp(TreeNode* node)
{
    // 判断是否为比较运算符
    TokenType op = node->attr.op;
    bool isCmp = (op == TokenType::LT || op == TokenType::GT ||
        op == TokenType::EQ || op == TokenType::LE ||
        op == TokenType::GE || op == TokenType::NE ||
        op == TokenType::ARROW);

    Operand left = genExp(node->child[0]);
    Operand right = genExp(node->child[1]);

    if (isCmp) {
        // 比较结果存入临时变量（布尔值用0/1表示）
        // 在布尔语境下通过 genBoolExp 调用，但这里作为表达式值处理
        // 生成：(j<, left, right, ?)(j, _, _, ?)，再回填
        // 简化：生成 (cmp, left, right, Tn) 并返回 Tn
        // 与 genBoolExp 区分：作为右值时只生成比较并存布尔到临时变量
        std::string opStr = getOpStr(op);
        std::string tmp = newTemp();
        // 真时 Tn=1，假时 Tn=0
        // (j<, left, right, L1)  -- 真跳 L1
        int jTrue = emitQuad("j" + opStr, left.str(), right.str(), "0");
        int jFalse = emitQuad("j", "_", "_", "0");
        // L1: Tn := 1
        backpatch({ jTrue }, nextQuad());
        emitQuad(":=", "1", "_", tmp);
        int jEnd = emitQuad("j", "_", "_", "0");
        // L2: Tn := 0
        backpatch({ jFalse }, nextQuad());
        emitQuad(":=", "0", "_", tmp);
        backpatch({ jEnd }, nextQuad());
        return Operand::makeTemp(tmp);
    }

    // 算术运算
    std::string opStr = getArithOpStr(op);
    std::string tmp = newTemp();
    emitQuad(opStr, left.str(), right.str(), tmp);
    return Operand::makeTemp(tmp);
}

/*=============================================*/
/*  自增/自减表达式                            */
/*  ++id => (++, id, _, id)                   */
/*  --id => (--, id, _, id)                   */
/*=============================================*/
Operand CodeGenerator::genSelfExp(TreeNode* node)
{
    TreeNode* idNode = node->child[0];
    if (idNode == nullptr) {
        errors_.push_back("自增/自减缺少操作数");
        return Operand::makeConst("0");
    }
    std::string varName = idNode->attr.name;
    bool isInc = (node->kind.exp == ExpKind::SelfIncK);
    emitQuad(isInc ? "++" : "--", varName, "_", varName);
    return Operand::makeVar(varName);
}

/*=============================================*/
/*  布尔表达式生成（回填技术）                 */
/*  返回 {trueList, falseList}                 */
/*                                             */
/*  对于比较表达式 a op b：                    */
/*   emit(j<op>, a, b, 0)  -> trueList         */
/*   emit(j, _, _, 0)       -> falseList        */
/*                                             */
/*  对于复合布尔（暂不支持and/or，但exp节点     */
/*  可通过 OpK 节点串联）：                     */
/*  由于本文法中 exp 仅含单一比较运算，         */
/*  布尔语境直接处理比较节点                   */
/*=============================================*/
BoolResult CodeGenerator::genBoolExp(TreeNode* node)
{
    BoolResult res;
    if (node == nullptr) return res;

    // 如果是 OpK 且是比较运算符
    if (node->nodekind == NodeKind::ExpK &&
        node->kind.exp == ExpKind::OpK)
    {
        TokenType op = node->attr.op;
        bool isCmp = (op == TokenType::LT || op == TokenType::GT ||
            op == TokenType::EQ || op == TokenType::LE ||
            op == TokenType::GE || op == TokenType::NE ||
            op == TokenType::ARROW);

        if (isCmp) {
            // 生成两个操作数
            Operand left = genExp(node->child[0]);
            Operand right = genExp(node->child[1]);

            std::string opStr = getOpStr(op);

            // 条件为真跳转（目标待回填，填0）
            int jTrue = emitQuad("j" + opStr, left.str(), right.str(), "0");
            res.trueList.push_back(jTrue);

            // 条件为假无条件跳转（目标待回填）
            int jFalse = emitQuad("j", "_", "_", "0");
            res.falseList.push_back(jFalse);

            return res;
        }

        // 算术运算作为布尔条件（非零为真）
        Operand val = genOpExp(node);
        // 生成 j!= val, 0 -> true; j -> false
        int jTrue = emitQuad("j<>", val.str(), "0", "0");
        res.trueList.push_back(jTrue);
        int jFalse = emitQuad("j", "_", "_", "0");
        res.falseList.push_back(jFalse);
        return res;
    }

    // 常量或变量作为布尔值（非零为真）
    Operand val = genExp(node);
    int jTrue = emitQuad("j<>", val.str(), "0", "0");
    res.trueList.push_back(jTrue);
    int jFalse = emitQuad("j", "_", "_", "0");
    res.falseList.push_back(jFalse);
    return res;
}

/*=============================================*/
/*  补齐假出口占位行                            */
/*  扫描所有跳转四元组的result字段，            */
/*  若目标编号超出当前四元组数组范围，          */
/*  则补发空四元组（仅有序号，其余字段留空）    */
/*=============================================*/
void CodeGenerator::finalize()
{
    int maxTarget = static_cast<int>(quads_.size());
    for (const auto& q : quads_) {
        if (q.op.empty()) continue;
        // 所有跳转类指令的result都是目标序号
        if (q.op[0] == 'j') {
            try {
                int t = std::stoi(q.result);
                if (t > maxTarget) maxTarget = t;
            }
            catch (...) {}
        }
    }
    // 补发占位空四元组（假出口行）
    while (static_cast<int>(quads_.size()) < maxTarget) {
        ++quadCount_;
        quads_.emplace_back(quadCount_, "", "", "", "");
    }
}
