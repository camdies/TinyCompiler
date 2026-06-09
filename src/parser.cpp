//fileName: TinyCompiler // src // scanner.cpp
//coding: UTF-8 with BOM

//this Program is for SCNU_Compiler 2026 Experiment3
//@auther Cai Renbin


/****************************************************/
/* parser.cpp                                       */
/* 扩充TINY语言的语法分析器实现                       */
/* 采用递归下降法，基于原TINY parser扩展              */
/****************************************************/

#include "parser.h"

Parser::Parser() {}

/*=============================================*/
/*  辅助函数                                    */
/*=============================================*/

void Parser::nextToken()
{
    currentToken_ = scanner_.getToken();
}

void Parser::match(TokenType expected)
{
    if (currentToken_.type == expected) {
        nextToken();
    }
    else {
        syntaxError("期望 " + Scanner::tokenTypeName(expected)
            + ",但得到 " + Scanner::tokenTypeName(currentToken_.type)
            + " ('" + currentToken_.lexeme + "')");
        // [关键修复]失败时也要消耗当前 token，否则会死循环
        nextToken();
    }
}

void Parser::syntaxError(const std::string& msg)
{
    errors_.push_back("语法错误 (行 " + std::to_string(currentToken_.lineno)
        + "): " + msg);
}

/*=============================================*/
/*  主解析函数                                  */
/*=============================================*/
TreeNode* Parser::parse(const std::string& source)
{
    errors_.clear();
    scanErrors_.clear();

    // 先做一遍完整的词法分析用于界面展示
    Scanner tempScanner;
    tempScanner.setSource(source);
    tokens_ = tempScanner.tokenize();
    scanErrors_ = tempScanner.getErrors();

    // 重新初始化scanner进行语法分析
    scanner_.setSource(source);
    nextToken();

    TreeNode* t = stmt_sequence();

    if (currentToken_.type != TokenType::ENDFILE) {
        syntaxError("代码在文件结束前终止");
    }

    // 收集scanner的错误
    for (auto& e : scanner_.getErrors()) {
        errors_.push_back(e);
    }

    return t;
}

/*=============================================*/
/* stmt_sequence -> statement { ; statement }   */
/*=============================================*/
TreeNode* Parser::stmt_sequence()
{
    // 如果当前就是结束标记，直接返回空
    if (currentToken_.type == TokenType::ENDFILE ||
        currentToken_.type == TokenType::ELSE ||
        currentToken_.type == TokenType::ENDIF ||
        currentToken_.type == TokenType::UNTIL ||
        currentToken_.type == TokenType::ENDWHILE ||
        currentToken_.type == TokenType::ENDFOR ||
        currentToken_.type == TokenType::END) {
        return nullptr;
    }

    TreeNode* t = statement();
    TreeNode* p = t;

    while (currentToken_.type == TokenType::SEMI) {
        match(TokenType::SEMI);

        // 分号后遇到结束标记，允许尾部可选分号
        if (currentToken_.type == TokenType::ENDFILE ||
            currentToken_.type == TokenType::ELSE ||
            currentToken_.type == TokenType::ENDIF ||
            currentToken_.type == TokenType::UNTIL ||
            currentToken_.type == TokenType::ENDWHILE ||
            currentToken_.type == TokenType::ENDFOR ||
            currentToken_.type == TokenType::END)
            break;

        TreeNode* q = statement();
        if (q != nullptr) {
            if (t == nullptr) {
                t = p = q;
            }
            else {
                p->sibling = q;
                p = q;
            }
        }
    }
    return t;
}

/*=============================================*/
/* statement -> if-stmt | repeat-stmt |         */
/*   for-stmt | while-stmt | assign-stmt |      */
/*   read-stmt | write-stmt                     */
/*=============================================*/
TreeNode* Parser::statement()
{
    TreeNode* t = nullptr;

    switch (currentToken_.type) {
    case TokenType::IF:
        t = if_stmt();
        break;
    case TokenType::REPEAT:
        t = repeat_stmt();
        break;
    case TokenType::WHILE:
        t = while_stmt();
        break;
    case TokenType::FOR:
        t = for_stmt();
        break;
    case TokenType::ID:
        t = assign_stmt();
        break;
    case TokenType::READ:
        t = read_stmt();
        break;
    case TokenType::WRITE:
        t = write_stmt();
        break;

    // 新增：处理 ++x 和 --x 作为独立语句
    case TokenType::INC:
    case TokenType::DEC:
        t = selfexp();
        break;

    default:
        if (currentToken_.type == TokenType::ENDFILE ||
            currentToken_.type == TokenType::ELSE ||
            currentToken_.type == TokenType::ENDIF ||
            currentToken_.type == TokenType::UNTIL ||
            currentToken_.type == TokenType::ENDWHILE ||
            currentToken_.type == TokenType::ENDFOR ||
            currentToken_.type == TokenType::END) {
            return nullptr;
        }
        syntaxError("意外的token: " + currentToken_.lexeme);
        nextToken();
        break;
    }
    return t;
}

/*=============================================*/
/* if-stmt -> if ( exp ) stmt-sequence          */
/*            [else stmt-sequence] endif        */
/*=============================================*/
TreeNode* Parser::if_stmt()
{
    TreeNode* t = newStmtNode(StmtKind::IfK, currentToken_.lineno);

    match(TokenType::IF);           // 匹配 if
    match(TokenType::LPAREN);       // 匹配 (

    if (t != nullptr)
        t->child[0] = exp();       // 条件表达式

    match(TokenType::RPAREN);       // 匹配 )

    if (t != nullptr)
        t->child[1] = stmt_sequence();  // then 分支

    if (currentToken_.type == TokenType::ELSE) {
        match(TokenType::ELSE);     // 匹配 else
        if (t != nullptr)
            t->child[2] = stmt_sequence();  // else 分支
    }

    match(TokenType::ENDIF);        // 匹配 endif

    return t;
}

/*=============================================*/
/* repeat-stmt -> repeat stmt-sequence until exp*/
/*=============================================*/
TreeNode* Parser::repeat_stmt()
{
    TreeNode* t = newStmtNode(StmtKind::RepeatK, currentToken_.lineno);

    match(TokenType::REPEAT);       // 匹配 repeat

    if (t != nullptr)
        t->child[0] = stmt_sequence();  // 循环体

    match(TokenType::UNTIL);        // 匹配 until

    if (t != nullptr)
        t->child[1] = exp();       // 终止条件

    return t;
}

/*=============================================*/
/* while-stmt -> while ( exp ) stmt-sequence    */
/*               endwhile                       */
/*=============================================*/
TreeNode* Parser::while_stmt()
{
    TreeNode* t = newStmtNode(StmtKind::WhileK, currentToken_.lineno);

    match(TokenType::WHILE);        // 匹配 while
    match(TokenType::LPAREN);       // 匹配 (

    if (t != nullptr)
        t->child[0] = exp();       // 循环条件

    match(TokenType::RPAREN);       // 匹配 )

    if (t != nullptr)
        t->child[1] = stmt_sequence();  // 循环体

    match(TokenType::ENDWHILE);     // 匹配 endwhile

    return t;
}

/*=============================================*/
/* for-stmt -> for ( assign-stmt ; exp ;        */
/*              selfexp ) stmt-sequence endfor  */
/*=============================================*/
TreeNode* Parser::for_stmt()
{
    TreeNode* t = newStmtNode(StmtKind::ForK, currentToken_.lineno);

    match(TokenType::FOR);
    match(TokenType::LPAREN);

    if (t != nullptr)
        t->child[0] = assign_stmt();   // 初始化赋值

    match(TokenType::SEMI);

    if (t != nullptr)
        t->child[1] = exp();           // 循环条件

    match(TokenType::SEMI);

    // 先解析 selfexp，但暂存
    TreeNode* selfexpNode = selfexp();

    match(TokenType::RPAREN);

    if (t != nullptr)
        t->child[2] = stmt_sequence(); // 循环体

    if (t != nullptr)
        t->child[3] = selfexpNode;     // 步进表达式放最后

    match(TokenType::ENDFOR);

    return t;
}

/*=============================================*/
/* assign-stmt -> identifier assign-sub-stmt    */
/* assign-sub-stmt -> := exp | += exp |         */
/*                    -= exp | ::= re           */
/*=============================================*/
TreeNode* Parser::assign_stmt()
{
    std::string name = currentToken_.lexeme;
    int line = currentToken_.lineno;

    match(TokenType::ID);

    if (currentToken_.type == TokenType::ASSIGN ||
        currentToken_.type == TokenType::PLUS_ASSIGN ||
        currentToken_.type == TokenType::MINUS_ASSIGN)
    {
        TokenType assignOp = currentToken_.type;
        TreeNode* t = newStmtNode(StmtKind::AssignK, line);
        t->attr.op = assignOp;   // 存储赋值操作符类型
        match(assignOp);
        if (t != nullptr) {
            TreeNode* idNode = newExpNode(ExpKind::IdK, line);
            if (idNode) idNode->attr.name = name;
            t->child[0] = idNode;
            t->child[1] = exp();
        }
        return t;
    }
    else if (currentToken_.type == TokenType::REGEX_ASSIGN) {
        // ::= re
        TreeNode* t = newStmtNode(StmtKind::RegAssignK, line);
        match(TokenType::REGEX_ASSIGN);
        if (t != nullptr) {
            TreeNode* idNode = newExpNode(ExpKind::IdK, line);
            if (idNode) idNode->attr.name = name;
            t->child[0] = idNode;
            t->child[1] = re();
        }
        return t;
    }
    else {
        syntaxError("赋值语句中期望 ':='、'+='、'-=' 或 '::='，但得到 '"
            + currentToken_.lexeme + "'");
        TreeNode* t = newStmtNode(StmtKind::AssignK, line);
        if (t != nullptr) t->attr.name = name;
        return t;
    }
}

/*=============================================*/
/* read-stmt -> read identifier                 */
/*=============================================*/
TreeNode* Parser::read_stmt()
{
    TreeNode* t = newStmtNode(StmtKind::ReadK, currentToken_.lineno);

    match(TokenType::READ);

    if (t != nullptr && currentToken_.type == TokenType::ID) {
        // 创建标识符子节点
        TreeNode* idNode = newExpNode(ExpKind::IdK, currentToken_.lineno);
        if (idNode) idNode->attr.name = currentToken_.lexeme;
        t->child[0] = idNode;
    }

    match(TokenType::ID);

    return t;
}

/*=============================================*/
/* write-stmt -> write (exp | letter)           */
/*=============================================*/
TreeNode* Parser::write_stmt()
{
    TreeNode* t = newStmtNode(StmtKind::WriteK, currentToken_.lineno);

    match(TokenType::WRITE);

    if (currentToken_.type == TokenType::LETTER) {
        // write letter
        TreeNode* letterNode = newExpNode(ExpKind::LetterK, currentToken_.lineno);
        if (letterNode != nullptr)
            letterNode->attr.name = currentToken_.lexeme;
        if (t != nullptr)
            t->child[0] = letterNode;
        match(TokenType::LETTER);
    }
    else {
        // write exp
        if (t != nullptr)
            t->child[0] = exp();
    }

    return t;
}

/*=============================================*/
/* exp -> simple-exp [comparison-op simple-exp] */
/* comparison-op: < | = | > | <= | >= | <> | =>*/
/*=============================================*/
TreeNode* Parser::exp()
{
    TreeNode* t = simple_exp();

    if (currentToken_.type == TokenType::LT ||
        currentToken_.type == TokenType::EQ ||
        currentToken_.type == TokenType::GT ||
        currentToken_.type == TokenType::LE ||
        currentToken_.type == TokenType::GE ||
        currentToken_.type == TokenType::NE ||
        currentToken_.type == TokenType::ARROW)
    {
        TreeNode* p = newExpNode(ExpKind::OpK, currentToken_.lineno);
        if (p != nullptr) {
            p->child[0] = t;
            p->attr.op = currentToken_.type;
            t = p;
        }
        nextToken();
        TreeNode* right = simple_exp();  // 始终调用
        if (t != nullptr)
            t->child[1] = right;
    }

    return t;
}

/*=============================================*/
/* simple-exp -> term { addop term }           */
/* addop: + | -                                */
/*=============================================*/
TreeNode* Parser::simple_exp()
{
    TreeNode* t = term();

    while (currentToken_.type == TokenType::PLUS ||
        currentToken_.type == TokenType::MINUS)
    {
        TreeNode* p = newExpNode(ExpKind::OpK, currentToken_.lineno);
        if (p != nullptr) {
            p->child[0] = t;
            p->attr.op = currentToken_.type;
            t = p;
        }
        nextToken();
        TreeNode* right = term();      // 始终调用
        if (t != nullptr)
            t->child[1] = right;
    }

    return t;
}

/*=============================================*/
/* term -> factor { mulop factor }             */
/* mulop: * | / | %                            */
/*=============================================*/
TreeNode* Parser::term()
{
    TreeNode* t = factor();

    while (currentToken_.type == TokenType::TIMES ||
        currentToken_.type == TokenType::OVER ||
        currentToken_.type == TokenType::MOD)
    {
        TreeNode* p = newExpNode(ExpKind::OpK, currentToken_.lineno);
        if (p != nullptr) {
            p->child[0] = t;
            p->attr.op = currentToken_.type;
            t = p;
        }
        nextToken();
        TreeNode* right = factor();    // 始终调用
        if (t != nullptr)
            t->child[1] = right;
    }

    return t;
}

/*=============================================*/
/* factor -> resexp { ^ resexp }               */
/*=============================================*/
TreeNode* Parser::factor()
{
    TreeNode* t = resexp();

    while (currentToken_.type == TokenType::POWER) {
        TreeNode* p = newExpNode(ExpKind::OpK, currentToken_.lineno);
        if (p != nullptr) {
            p->child[0] = t;
            p->attr.op = TokenType::POWER;
            t = p;
        }
        nextToken();
        TreeNode* right = resexp();    // 始终调用
        if (t != nullptr)
            t->child[1] = right;
    }

    return t;
}

/*=============================================*/
/* resexp -> ( exp ) | number | identifier     */
/*         | selfexp                           */
/*=============================================*/
TreeNode* Parser::resexp()
{
    TreeNode* t = nullptr;

    switch (currentToken_.type) {
    case TokenType::LPAREN:
        match(TokenType::LPAREN);
        t = exp();
        match(TokenType::RPAREN);
        break;

    case TokenType::NUM:
        t = newExpNode(ExpKind::ConstK, currentToken_.lineno);
        if (t != nullptr) {
            t->attr.name = currentToken_.lexeme;  // 统一存入name
            t->attr.val = std::stod(currentToken_.lexeme);
        }
        match(TokenType::NUM);
        break;

        // 新增：浮点数
    case TokenType::FLOAT:
        t = newExpNode(ExpKind::ConstK, currentToken_.lineno);
        if (t != nullptr) {
            t->attr.name = currentToken_.lexeme;  // 存原始字符串
            t->attr.val = std::stod(currentToken_.lexeme);
        }
        match(TokenType::FLOAT);
        break;

    case TokenType::ID:
        t = newExpNode(ExpKind::IdK, currentToken_.lineno);
        if (t != nullptr) {
            t->attr.name = currentToken_.lexeme;
        }
        match(TokenType::ID);
        break;

    case TokenType::INC:
    case TokenType::DEC:
        t = selfexp();
        break;

    default:
        syntaxError("表达式中遇到意外的token: " + currentToken_.lexeme);
        nextToken();
        break;
    }

    return t;
}

/*=============================================*/
/* selfexp -> selfop identifier                */
/* selfop -> ++ | --                           */
/*=============================================*/
TreeNode* Parser::selfexp()
{
    TreeNode* t = nullptr;

    if (currentToken_.type == TokenType::INC) {
        t = newExpNode(ExpKind::SelfIncK, currentToken_.lineno);
        match(TokenType::INC);
        if (t != nullptr && currentToken_.type == TokenType::ID) {
            TreeNode* idNode = newExpNode(ExpKind::IdK, currentToken_.lineno);
            if (idNode) idNode->attr.name = currentToken_.lexeme;
            t->child[0] = idNode;
        }
        match(TokenType::ID);
    }
    else if (currentToken_.type == TokenType::DEC) {
        t = newExpNode(ExpKind::SelfDecK, currentToken_.lineno);
        match(TokenType::DEC);
        if (t != nullptr && currentToken_.type == TokenType::ID) {
            TreeNode* idNode = newExpNode(ExpKind::IdK, currentToken_.lineno);
            if (idNode) idNode->attr.name = currentToken_.lexeme;
            t->child[0] = idNode;
        }
        match(TokenType::ID);
    }
    else {
        syntaxError("期望 '++' 或 '--',但得到 '" + currentToken_.lexeme + "'");
        nextToken();
    }

    return t;
}

/*=============================================*/
/*  ===== 正则表达式解析函数 =====              */
/*=============================================*/

/*=============================================*/
/* re -> orre { | orre }                       */
/*=============================================*/
TreeNode* Parser::re()
{
    TreeNode* t = orre();

    while (currentToken_.type == TokenType::REGEX_OR) {
        TreeNode* p = newRegexNode(RegexKind::OrK, currentToken_.lineno);
        if (p != nullptr) {
            p->child[0] = t;
            t = p;
        }
        match(TokenType::REGEX_OR);
        if (t != nullptr)
            t->child[1] = orre();
    }

    return t;
}

/*=============================================*/
/* orre -> conre { & conre }                   */
/*=============================================*/
TreeNode* Parser::orre()
{
    TreeNode* t = conre();

    while (currentToken_.type == TokenType::REGEX_CONCAT) {
        TreeNode* p = newRegexNode(RegexKind::ConcatK, currentToken_.lineno);
        if (p != nullptr) {
            p->child[0] = t;
            t = p;
        }
        match(TokenType::REGEX_CONCAT);
        if (t != nullptr)
            t->child[1] = conre();
    }

    return t;
}

/*=============================================*/
/* conre -> repre [ repop ]                    */
/* repop -> # | ?                              */
/*=============================================*/
TreeNode* Parser::conre()
{
    TreeNode* t = repre();

    if (currentToken_.type == TokenType::REGEX_CLOSURE) {
        TreeNode* p = newRegexNode(RegexKind::ClosureK, currentToken_.lineno);
        if (p != nullptr) {
            p->child[0] = t;
            t = p;
        }
        match(TokenType::REGEX_CLOSURE);
    }
    else if (currentToken_.type == TokenType::REGEX_OPTIONAL) {
        TreeNode* p = newRegexNode(RegexKind::OptionalK, currentToken_.lineno);
        if (p != nullptr) {
            p->child[0] = t;
            t = p;
        }
        match(TokenType::REGEX_OPTIONAL);
    }

    return t;
}

/*=============================================*/
/* repre -> ( re ) | letter | identifier       */
/*=============================================*/
TreeNode* Parser::repre()
{
    TreeNode* t = nullptr;

    switch (currentToken_.type) {
    case TokenType::LPAREN:
        match(TokenType::LPAREN);
        t = re();
        match(TokenType::RPAREN);
        break;

    case TokenType::LETTER:
        t = newRegexNode(RegexKind::LetterK, currentToken_.lineno);
        if (t != nullptr)
            t->attr.name = currentToken_.lexeme;
        match(TokenType::LETTER);
        break;

    case TokenType::ID:
        t = newRegexNode(RegexKind::IdK, currentToken_.lineno);
        if (t != nullptr)
            t->attr.name = currentToken_.lexeme;
        match(TokenType::ID);
        break;

    default:
        syntaxError("正则表达式中遇到意外的token: " + currentToken_.lexeme);
        nextToken();
        break;
    }

    return t;
}