//fileName: TinyCompiler // src // highlighter.h
//coding: UTF-8 with BOM

//this Program is for SCNU_Compiler 2026 Experiment3
//@auther Cai Renbin


/****************************************************/
/* highlighter.h                                    */
/* TINY语言的语法高亮器                              */
/* 为源代码编辑器提供关键字、数字、注释等高亮         */
/****************************************************/

#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <vector>

class TinyHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit TinyHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    std::vector<HighlightRule> rules_;

    // 各种格式
    QTextCharFormat keywordFormat_;
    QTextCharFormat numberFormat_;
    QTextCharFormat operatorFormat_;
    QTextCharFormat commentFormat_;
    QTextCharFormat stringFormat_;

    // 多行注释的起止正则
    QRegularExpression commentStartExpression_;
    QRegularExpression commentEndExpression_;
};

#endif // HIGHLIGHTER_H