//fileName: TinyCompiler // src // mainwindow.h
//coding: UTF-8 with BOM

//this Program is for SCNU_Compiler 2026 Experiment3
//@auther Cai Renbin


/****************************************************/
/* mainwindow.h                                     */
/* 主窗口界面定义                                     */
/* 包含: 源代码编辑区、工具栏(新建/打开/保存)         */
/*       词法分析结果Tab、语法树Tab                   */
/*       展开全部/折叠全部按钮                        */
/****************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QTreeView>
#include <QTableWidget>
#include <QTabWidget>
#include <QPushButton>
#include <QToolBar>
#include <QAction>
#include <QSplitter>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QDialog>
#include <QStackedWidget>

#include "parser.h"
#include "syntaxtreemodel.h"
#include "syntaxtreeview.h"
#include "highlighter.h"
#include "codegen.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onNewFile();           // 新建文件
    void onOpenFile();          // 打开文件
    void onSaveFile();          // 保存文件
    void onSaveAsFile();        // 另存为
    void onAnalyze();           // 执行语法分析
    void onExpandAll();         // 展开全部节点
    void onCollapseAll();       // 折叠全部节点
    void onExpandSelected();    // 展开选中节点
    void onCollapseSelected();  // 折叠选中节点

    void onSwitchView();               // 切换目录树/多叉树
    void onPopupView();          // 新增：弹窗查看语法树
    void updateButtonStates();   // 新增：根据当前视图模式更新按钮状态

private:
    void setupUI();             // 初始化界面
    void setupToolBar();        // 初始化工具栏
    void setupMenuBar();        // 初始化菜单栏
    void showTokens(const std::vector<Token>& tokens);
    void showErrors(const std::vector<std::string>& errors);
    void showQuadruples(const std::vector<Quadruple>& quads);

    // 新增：同步刷新所有已打开的弹窗
    void refreshAllPopups();

    // UI组件
    QSplitter* mainSplitter_;           // 主分割器（左右分割）

    // 左侧：源代码编辑区
    QGroupBox* editorGroup_;
    QTextEdit* sourceEditor_;           // 源代码编辑器
    TinyHighlighter* highlighter_;      // 语法高亮器

    // 右侧：分析结果区(Tab)
    QTabWidget* resultTabs_;

    // 词法分析Tab
    QWidget* lexTab_;
    QTableWidget* tokenTable_;

    // 语法分析Tab
    QWidget* syntaxTab_;
    QTreeView* syntaxTreeView_;         // 语法树视图
    SyntaxTreeModel* treeModel_;        // 语法树模型
    QPushButton* expandAllBtn_;         // 展开全部按钮
    QPushButton* collapseAllBtn_;       // 折叠全部按钮
    QPushButton* expandSelBtn_;         // 展开选中按钮
    QPushButton* collapseSelBtn_;       // 折叠选中按钮

    // 多叉树视图
    SyntaxTreeGraphicsView* graphicsTreeView_;
    QPushButton* switchViewBtn_;       // 切换按钮
    QPushButton* popupViewBtn_;    // 新增：弹窗查看按钮
    bool showingTreeView_;             // true=目录树, false=多叉树
    QStackedWidget* treeStack_;        // 用于切换两个视图

    // 中间代码Tab
    QWidget* codeTab_;
    QTableWidget* codeTable_;

    // 中间代码生成器
    CodeGenerator codeGen_;

    // 错误显示
    QTableWidget* errorTable_;

    // 工具栏动作
    QAction* newAction_;
    QAction* openAction_;
    QAction* saveAction_;
    QAction* saveAsAction_;
    QAction* analyzeAction_;

    // 当前文件路径
    QString currentFilePath_;

    // 解析器
    Parser parser_;

    // 语法树根节点（需要手动管理内存）
    TreeNode* syntaxTree_;

    // 新增：跟踪所有已打开的弹窗，用于同步刷新
    QList<QDialog*> openPopups_;
};

#endif // MAINWINDOW_H