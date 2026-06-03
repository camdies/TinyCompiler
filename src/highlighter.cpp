//fileName: TinyCompiler // src // highlighter.cpp
//coding: UTF-8 with BOM

//this Program is for SCNU_Compiler 2026 Experiment3
//@auther Cai Renbin


/****************************************************/
/* highlighter.cpp                                  */
/* TINY语言语法高亮器实现                             */
/****************************************************/

#include "highlighter.h"

TinyHighlighter::TinyHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    // 关键字格式：蓝色粗体
    keywordFormat_.setForeground(Qt::blue);
    keywordFormat_.setFontWeight(QFont::Bold);

    // TINY保留字列表
    QStringList keywords;
    keywords << "\\bif\\b" << "\\bthen\\b" << "\\belse\\b" << "\\bend\\b"
        << "\\brepeat\\b" << "\\buntil\\b" << "\\bread\\b" << "\\bwrite\\b"
        << "\\bwhile\\b" << "\\bendwhile\\b" << "\\bfor\\b" << "\\bendfor\\b"
        << "\\bendif\\b";

    for (const auto& keyword : keywords) {
        HighlightRule rule;
        rule.pattern = QRegularExpression(keyword);
        rule.format = keywordFormat_;
        rules_.push_back(rule);
    }

    // 数字格式：红色
    numberFormat_.setForeground(QColor(180, 0, 0));
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression("\\b[0-9]+\\b");
        rule.format = numberFormat_;
        rules_.push_back(rule);
    }

    // 运算符格式：深绿色
    operatorFormat_.setForeground(QColor(0, 128, 0));
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression("[+\\-*/%^<>=!&|#?:;()]+");
        rule.format = operatorFormat_;
        rules_.push_back(rule);
    }

    // 注释格式：灰色斜体 { ... }
    // 注释格式（注意：不在 rules_ 中添加单行注释规则，改为统一处理多行注释）
    commentFormat_.setForeground(Qt::gray);
    commentFormat_.setFontItalic(true);

    // 多行注释起止标记
    commentStartExpression_ = QRegularExpression("\\{");
    commentEndExpression_ = QRegularExpression("\\}");


    // 字母常量格式：紫色 'x'
    stringFormat_.setForeground(QColor(128, 0, 128));
    {
        HighlightRule rule;
        rule.pattern = QRegularExpression("'[^']*'");
        rule.format = stringFormat_;
        rules_.push_back(rule);
    }
}

void TinyHighlighter::highlightBlock(const QString& text)
{
    // 先应用普通规则
    for (const auto& rule : rules_) {
        QRegularExpressionMatchIterator matchIterator =
            rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // 处理多行注释 { ... }
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1) {
        // 上一个 block 不在注释中，寻找注释开始位置
        QRegularExpressionMatch startMatch = commentStartExpression_.match(text);
        startIndex = startMatch.hasMatch() ? startMatch.capturedStart() : -1;
    }

    while (startIndex >= 0) {
        QRegularExpressionMatch endMatch;
        int searchFrom = (previousBlockState() == 1 && startIndex == 0)
            ? 0 : startIndex + 1;
        endMatch = commentEndExpression_.match(text, searchFrom);

        int endIndex = endMatch.hasMatch() ? endMatch.capturedStart() : -1;
        int commentLength;

        if (endIndex == -1) {
            // 注释延续到下一行
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        }
        else {
            // 注释在本行结束
            commentLength = endIndex - startIndex + endMatch.capturedLength();
        }

        setFormat(startIndex, commentLength, commentFormat_);

        // 继续查找本行是否有更多注释
        QRegularExpressionMatch nextStart =
            commentStartExpression_.match(text, startIndex + commentLength);
        startIndex = nextStart.hasMatch() ? nextStart.capturedStart() : -1;
    }
}