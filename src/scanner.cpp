//fileName: TinyCompiler // src // scanner.cpp
//coding: UTF-8 with BOM

//this Program is for SCNU_Compiler 2026 Experiment3
//@auther Cai Renbin


/****************************************************/
/* scanner.cpp                                      */
/* 扩充TINY语言的词法分析器实现                       */
/* 基于原TINY scanner的DFA方式进行扩展               */
/****************************************************/

#include "scanner.h"
#include <cctype>

/*=============================================*/
/*  保留字表                                    */
/*=============================================*/
static struct {
    std::string word;
    TokenType token;
} reservedWords[] = {
    {"if",       TokenType::IF},
    {"then",     TokenType::THEN},
    {"else",     TokenType::ELSE},
    {"end",      TokenType::END},
    {"repeat",   TokenType::REPEAT},
    {"until",    TokenType::UNTIL},
    {"read",     TokenType::READ},
    {"write",    TokenType::WRITE},
    {"while",    TokenType::WHILE},
    {"endwhile", TokenType::ENDWHILE},
    {"for",      TokenType::FOR},
    {"endfor",   TokenType::ENDFOR},
    {"endif",    TokenType::ENDIF},
};

constexpr int NUM_RESERVED = sizeof(reservedWords) / sizeof(reservedWords[0]);

Scanner::Scanner() : pos_(0), lineno_(1) {}

void Scanner::setSource(const std::string& source)
{
    source_ = source;
    pos_ = 0;
    lineno_ = 1;
    errors_.clear();
}

char Scanner::getNextChar()
{
    if (pos_ >= static_cast<int>(source_.size()))
        return '\0'; // EOF
    char c = source_[pos_++];
    if (c == '\n') lineno_++;
    return c;
}

void Scanner::ungetChar()
{
    if (pos_ > 0) {
        pos_--;
        if (source_[pos_] == '\n') lineno_--;
    }
}

bool Scanner::isEOF() const
{
    return pos_ >= static_cast<int>(source_.size());
}

TokenType Scanner::reservedLookup(const std::string& s)
{
    for (int i = 0; i < NUM_RESERVED; i++) {
        if (reservedWords[i].word == s)
            return reservedWords[i].token;
    }
    return TokenType::ID;
}

/*=============================================*/
/*  获取下一个Token (核心DFA)                   */
/*=============================================*/
Token Scanner::getToken()
{
    std::string tokenString;
    TokenType currentToken = TokenType::ENDFILE;
    State state = State::START;

    while (state != State::DONE) {
        char c = getNextChar();
        bool save = true;

        switch (state) {
        case State::START:
            if (std::isdigit(c)) {
                state = State::INNUM;
            }
            else if (c == '.') {
                // 可能是 .5 这样以小数点开头的浮点数
                state = State::INFLOAT;
            }
            else if (std::isalpha(c)) {
                state = State::INID;
            }
            else if (c == ':') {
                // 可能是 :=
                state = State::INCOLON;
            }
            else if (c == '=') {
                // 可能是 = 或 == 或 =>
                state = State::INEQ;
            }
            else if (c == '<') {
                // 可能是 < 或 <= 或 <>
                state = State::INLT;
            }
            else if (c == '>') {
                // 可能是 > 或 >=
                state = State::INGT;
            }
            else if (c == '+') {
                // 可能是 + 或 ++
                state = State::INPLUS;
            }
            else if (c == '-') {
                // 可能是 - 或 --
                state = State::INMINUS;
            }
            else if (c == '\'') {
                // 字母常量开始，如 'a'
                save = false;
                state = State::INLETTER;
            }
            else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                save = false;
            }
            else if (c == '{') {
                // 注释开始
                save = false;
                state = State::INCOMMENT;
            }
            else {
                state = State::DONE;
                switch (c) {
                case '\0':
                    save = false;
                    currentToken = TokenType::ENDFILE;
                    break;
                case '*':
                    currentToken = TokenType::TIMES;
                    break;
                case '/':
                    currentToken = TokenType::REGEX_SLASH;
                    break;
                case '%':
                    currentToken = TokenType::MOD;
                    break;
                case '^':
                    currentToken = TokenType::POWER;
                    break;
                case '(':
                    currentToken = TokenType::LPAREN;
                    break;
                case ')':
                    currentToken = TokenType::RPAREN;
                    break;
                case ';':
                    currentToken = TokenType::SEMI;
                    break;
                case '&':
                    currentToken = TokenType::REGEX_CONCAT;
                    break;
                case '#':
                    currentToken = TokenType::REGEX_CLOSURE;
                    break;
                case '?':
                    currentToken = TokenType::REGEX_OPTIONAL;
                    break;
                case '|':
                    currentToken = TokenType::REGEX_OR;
                    break;
                default:
                    currentToken = TokenType::ERROR_TOKEN;
                    errors_.push_back("Line " + std::to_string(lineno_)
                        + ": 非法字符 '" + std::string(1, c) + "'");
                    break;
                }
            }
            break;

        case State::INCOMMENT:
            // 注释处理：{ ... }
            save = false;
            if (c == '\0') {
                state = State::DONE;
                currentToken = TokenType::ENDFILE;
                errors_.push_back("Line " + std::to_string(lineno_)
                    + ": 注释未关闭");
            }
            else if (c == '}') {
                state = State::START;
            }
            break;

        case State::INCOLON:
            // : 后面可能是 = (:=) 或 : (::=的开始)
            if (c == '=') {
                state = State::DONE;
                currentToken = TokenType::ASSIGN;   // :=
            }
            else if (c == ':') {
                state = State::INDOUBLECOLON;       // :: 继续等待 =
            }
            else {
                ungetChar();
                save = false;
                state = State::DONE;
                currentToken = TokenType::ERROR_TOKEN;
                errors_.push_back("Line " + std::to_string(lineno_)
                    + ": 非法字符 ':'，期望 ':=' 或 '::='");
            }
            break;

        case State::INDOUBLECOLON:
            // :: 后面必须是 =
            state = State::DONE;
            if (c == '=') {
                currentToken = TokenType::REGEX_ASSIGN;  // ::=
            }
            else {
                ungetChar();
                save = false;
                currentToken = TokenType::ERROR_TOKEN;
                errors_.push_back("Line " + std::to_string(lineno_)
                    + ": 非法字符 '::'，期望 '::='");
            }
            break;

        case State::INEQ:
            state = State::DONE;
            if (c == '>') {
                currentToken = TokenType::ARROW;    // =>
            }
            else {
                ungetChar();
                save = false;
                currentToken = TokenType::EQ;       // 单独的 =
            }
            break;

        case State::INLT:
            // < 后面可能是 = (<=) 或 > (<>)
            state = State::DONE;
            if (c == '=') {
                currentToken = TokenType::LE;   // <=
            }
            else if (c == '>') {
                currentToken = TokenType::NE;   // <>
            }
            else {
                ungetChar();
                save = false;
                currentToken = TokenType::LT;   // 单独的 <
            }
            break;

        case State::INGT:
            // > 后面可能是 = (>=)
            state = State::DONE;
            if (c == '=') {
                currentToken = TokenType::GE;   // >=
            }
            else {
                ungetChar();
                save = false;
                currentToken = TokenType::GT;   // 单独的 >
            }
            break;

        case State::INPLUS:
            // + 后面可能是 + (++) 或 = (+=)
            state = State::DONE;
            if (c == '+') {
                currentToken = TokenType::INC;  // ++
            }
            else if (c == '=') {
                currentToken = TokenType::PLUS_ASSIGN;  // +=
            }
            else {
                ungetChar();
                save = false;
                currentToken = TokenType::PLUS; // 单独的 +
            }
            break;

        case State::INMINUS:
            // - 后面可能是 - (--) 或 = (-=)
            state = State::DONE;
            if (c == '-') {
                currentToken = TokenType::DEC;  // --
            }
            else if (c == '=') {
                currentToken = TokenType::MINUS_ASSIGN;  // -=
            }
            else {
                ungetChar();
                save = false;
                currentToken = TokenType::MINUS; // 单独的 -
            }
            break;

        case State::INNUM:
            if (std::isdigit(c)) {
                // 继续读数字
            }
            else if (c == '.') {
                // 小数点：转为浮点数状态
                state = State::INFLOAT;
            }
            else if (c == 'e' || c == 'E') {
                state = State::INSCI;
            }
            else {
                if (c != '\0')
                    ungetChar();
                save = false;
                state = State::DONE;
                currentToken = TokenType::NUM;
            }
            break;

        case State::INFLOAT:
            // 读到小数点后，必须跟数字
            if (std::isdigit(c)) {
                // 继续读小数部分
            }
            else if (c == 'e' || c == 'E') {
                state = State::INFLOAT_SCI;
            }
            else {
                if (c != '\0')
                    ungetChar();
                save = false;
                state = State::DONE;
                currentToken = TokenType::FLOAT;
            }
            break;

        case State::INFLOAT_SCI:
            if (std::isdigit(c)) {
                state = State::INFLOAT_SCI_NUM;
            }
            else if (c == '+' || c == '-') {
                state = State::INFLOAT_SCI_SIGN;
            }
            else {
                save = false;
                state = State::DONE;
                currentToken = TokenType::ERROR_TOKEN;
                errors_.push_back("Line " + std::to_string(lineno_)
                    + ": 浮点科学计数法格式错误,'e'后需要数字或正负号");
            }
            break;

        case State::INFLOAT_SCI_SIGN:
            if (std::isdigit(c)) {
                state = State::INFLOAT_SCI_NUM;
            }
            else {
                save = false;
                state = State::DONE;
                currentToken = TokenType::ERROR_TOKEN;
                errors_.push_back("Line " + std::to_string(lineno_)
                    + ": 浮点科学计数法格式错误,符号后需要数字");
            }
            break;

        case State::INFLOAT_SCI_NUM:
            if (!std::isdigit(c)) {
                if (c != '\0')
                    ungetChar();
                save = false;
                state = State::DONE;
                currentToken = TokenType::FLOAT;
            }
            break;

        case State::INSCI:
            if (std::isdigit(c)) {
                state = State::INSCI_NUM;
            }
            else if (c == '+' || c == '-') {
                state = State::INSCI_SIGN;
            }
            else {
                save = false;
                state = State::DONE;
                currentToken = TokenType::ERROR_TOKEN;
                errors_.push_back("Line " + std::to_string(lineno_)
                    + ": 科学计数法格式错误,'e'后需要数字或正负号");
            }
            break;

        case State::INSCI_SIGN:
            if (std::isdigit(c)) {
                state = State::INSCI_NUM;
            }
            else {
                save = false;
                state = State::DONE;
                currentToken = TokenType::ERROR_TOKEN;
                errors_.push_back("Line " + std::to_string(lineno_)
                    + ": 科学计数法格式错误,符号后需要数字");
            }
            break;

        case State::INSCI_NUM:
            if (!std::isdigit(c)) {
                if (c != '\0')
                    ungetChar();
                save = false;
                state = State::DONE;
                currentToken = TokenType::FLOAT;  // 科学计数法也归为FLOAT
            }
            break;

        case State::INID:
            // 读取标识符(字母+数字)
            if (!std::isalpha(c) && !std::isdigit(c)) {
                if (c != '\0')        //新增：\0 不需要回退
                    ungetChar();
                save = false;
                state = State::DONE;
                currentToken = TokenType::ID;
            }
            break;

        case State::INLETTER:
            // 读取字母常量 'x'
            if (c == '\'') {
                // 结束
                save = false;
                state = State::DONE;
                currentToken = TokenType::LETTER;
            }
            else if (c == '\0') {
                state = State::DONE;
                currentToken = TokenType::ERROR_TOKEN;
                errors_.push_back("Line " + std::to_string(lineno_)
                    + ": 字母常量未关闭");
            }
            // 否则继续读取字母内容
            break;

        case State::DONE:
        default:
            state = State::DONE;
            currentToken = TokenType::ERROR_TOKEN;
            break;
        }

        if (save && c != '\0') {
            tokenString += c;
        }

        if (state == State::DONE) {
            // 如果是ID，检查是否为保留字
            if (currentToken == TokenType::ID) {
                currentToken = reservedLookup(tokenString);
            }
        }
    }

    return Token(currentToken, tokenString, lineno_);
}

/*=============================================*/
/*  一次性获取所有Token                         */
/*=============================================*/
std::vector<Token> Scanner::tokenize()
{
    std::vector<Token> tokens;
    pos_ = 0;
    lineno_ = 1;
    errors_.clear();

    Token tok;
    do {
        tok = getToken();
        tokens.push_back(tok);
    } while (tok.type != TokenType::ENDFILE);

    return tokens;
}

/*=============================================*/
/*  Token类型名称                               */
/*=============================================*/
std::string Scanner::tokenTypeName(TokenType type)
{
    switch (type) {
    case TokenType::ENDFILE:        return "EOF";
    case TokenType::ERROR_TOKEN:    return "ERROR";
    case TokenType::IF:             return "IF('if')";
    case TokenType::THEN:           return "THEN('then')";
    case TokenType::ELSE:           return "ELSE('else')";
    case TokenType::END:            return "END('end')";
    case TokenType::REPEAT:         return "REPEAT('repeat')";
    case TokenType::UNTIL:          return "UNTIL('until')";
    case TokenType::READ:           return "READ('read')";
    case TokenType::WRITE:          return "WRITE('write')";
    case TokenType::WHILE:          return "WHILE('while')";
    case TokenType::ENDWHILE:       return "ENDWHILE('endwhile')";
    case TokenType::FOR:            return "FOR('for')";
    case TokenType::ENDFOR:         return "ENDFOR('endfor')";
    case TokenType::ENDIF:          return "ENDIF('endif')";
    case TokenType::ID:             return "ID";
    case TokenType::NUM:            return "NUM";
    case TokenType::FLOAT:          return "FLOAT";
    case TokenType::LETTER:         return "LETTER";
    case TokenType::ASSIGN:         return "ASSIGN(':=')";
    case TokenType::PLUS_ASSIGN:   return "PLUS_ASSIGN('+=')";
    case TokenType::MINUS_ASSIGN:  return "MINUS_ASSIGN('-=')";
    case TokenType::REGEX_ASSIGN:   return "REGEX_ASSIGN('::=')";
    case TokenType::EQ:             return "EQ('=')";
    case TokenType::LT:             return "LT('<')";
    case TokenType::GT:             return "GT('>')";
    case TokenType::LE:             return "LE('<=')";
    case TokenType::GE:             return "GE('>=')";
    case TokenType::NE:             return "NE('<>')";
    case TokenType::ARROW:          return "ARROW('=>')";
    case TokenType::PLUS:           return "PLUS('+')";
    case TokenType::MINUS:          return "MINUS('-')";
    case TokenType::TIMES:          return "TIMES('*')";
    case TokenType::OVER:           return "OVER('/')";
    case TokenType::MOD:            return "MOD('%')";
    case TokenType::POWER:          return "POWER('^')";
    case TokenType::INC:            return "INC('++')";
    case TokenType::DEC:            return "DEC('--')";
    case TokenType::REGEX_OR:       return "RE_OR('|')";
    case TokenType::REGEX_CONCAT:   return "RE_CONCAT('&')";
    case TokenType::REGEX_CLOSURE:  return "RE_CLOSURE('#')";
    case TokenType::REGEX_OPTIONAL: return "RE_OPTIONAL('?')";
    case TokenType::REGEX_SLASH:    return "RE_SLASH('/')";
    case TokenType::LPAREN:         return "LPAREN('(')";
    case TokenType::RPAREN:         return "RPAREN(')')";
    case TokenType::SEMI:           return "SEMI(';')";
    default:                        return "UNKNOWN";
    }
}