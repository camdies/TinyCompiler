//fileName: TinyCompiler // src // mainwindow.cpp
//coding: UTF-8 with BOM

//this Program is for SCNU_Compiler 2026 Experiment3
//@auther Cai Renbin


/****************************************************/
/* mainwindow.cpp                                   */
/* 主窗口界面实现                                     */
/* 实现源代码编辑、文件操作、词法/语法分析及结果展示   */
/****************************************************/

#include "mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QHeaderView>
#include <QFont>
#include <QApplication>
#include <QDialog>       // 新增
#include <QTimer>        // 新增
#include <memory>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), syntaxTree_(nullptr), showingTreeView_(true), showingCodeTable_(true)
{
    setWindowTitle("TINY扩充语言 - 语法树生成器");
    resize(1200, 750);

    setupUI();
    setupToolBar();
    setupMenuBar();

    statusBar()->showMessage("就绪");
}

MainWindow::~MainWindow()
{
    // 释放语法树内存
    if (syntaxTree_) {
        deleteTree(syntaxTree_);
        syntaxTree_ = nullptr;
    }
}

/*=============================================*/
/*  初始化界面布局                              */
/*=============================================*/
void MainWindow::setupUI()
{
    // ===== 主分割器：左右分割 =====
    mainSplitter_ = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter_);

    // ===== 左侧：源代码编辑区 =====
    editorGroup_ = new QGroupBox("TINY源代码", this);
    QVBoxLayout* editorLayout = new QVBoxLayout(editorGroup_);

    sourceEditor_ = new QTextEdit(this);
    sourceEditor_->setFont(QFont("Consolas", 11));
    sourceEditor_->setTabStopDistance(30);
    sourceEditor_->setLineWrapMode(QTextEdit::NoWrap);
    sourceEditor_->setStyleSheet(
        "QTextEdit { background-color: #f0fff0; "
        "border: 1px solid #ccc; }"
    );

    // 设置语法高亮
    highlighter_ = new TinyHighlighter(sourceEditor_->document());

    editorLayout->addWidget(sourceEditor_);
    editorGroup_->setLayout(editorLayout);

    // ===== 右侧：分析结果Tab =====
    QWidget* rightWidget = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);

    resultTabs_ = new QTabWidget(this);

    // ----- 词法分析Tab -----
    lexTab_ = new QWidget(this);
    QVBoxLayout* lexLayout = new QVBoxLayout(lexTab_);

    tokenTable_ = new QTableWidget(this);
    tokenTable_->setColumnCount(3);
    tokenTable_->setHorizontalHeaderLabels({ "行号", "Token类型", "词素" });
    tokenTable_->horizontalHeader()->setStretchLastSection(true);
    tokenTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tokenTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tokenTable_->setFont(QFont("Consolas", 10));

    lexLayout->addWidget(tokenTable_);
    lexTab_->setLayout(lexLayout);

    // ----- 语法分析Tab -----
    syntaxTab_ = new QWidget(this);
    QVBoxLayout* syntaxLayout = new QVBoxLayout(syntaxTab_);

    // 语法树控制按钮区
    QHBoxLayout* btnLayout = new QHBoxLayout();

    expandAllBtn_ = new QPushButton("展开全部", this);
    collapseAllBtn_ = new QPushButton("折叠全部", this);
    expandSelBtn_ = new QPushButton("展开选中", this);
    collapseSelBtn_ = new QPushButton("折叠选中", this);
    switchViewBtn_ = new QPushButton("切换为多叉树", this);  // 新增
    popupViewBtn_ = new QPushButton("弹窗查看", this);  // 新增

    expandAllBtn_->setIcon(QApplication::style()->standardIcon(
        QStyle::SP_ArrowDown));
    collapseAllBtn_->setIcon(QApplication::style()->standardIcon(
        QStyle::SP_ArrowUp));
    popupViewBtn_->setIcon(QApplication::style()->standardIcon(   // 新增
        QStyle::SP_TitleBarMaxButton));

    btnLayout->addWidget(expandAllBtn_);
    btnLayout->addWidget(collapseAllBtn_);
    btnLayout->addWidget(expandSelBtn_);
    btnLayout->addWidget(collapseSelBtn_);
    btnLayout->addStretch();
    btnLayout->addWidget(popupViewBtn_);   // 新增，放在切换按钮左边
    btnLayout->addWidget(switchViewBtn_);  // 新增，放右侧

    // 使用 QStackedWidget 切换两种视图
    treeStack_ = new QStackedWidget(this);
    showingTreeView_ = true;

    // 目录树视图(index 0)
    syntaxTreeView_ = new QTreeView(this);
    treeModel_ = new SyntaxTreeModel(this);
    syntaxTreeView_->setModel(treeModel_);
    syntaxTreeView_->setFont(QFont("Consolas", 10));
    syntaxTreeView_->setAnimated(true);
    syntaxTreeView_->setIndentation(25);
    syntaxTreeView_->setHeaderHidden(false);
    syntaxTreeView_->setStyleSheet(
        "QTreeView { background-color: #f0fff0; "
        "border: 1px solid #ccc; }"
    );

    // 多叉树图形视图（index 1）
    graphicsTreeView_ = new SyntaxTreeGraphicsView(this);

    treeStack_->addWidget(syntaxTreeView_);    // index 0
    treeStack_->addWidget(graphicsTreeView_);  // index 1
    treeStack_->setCurrentIndex(0);

    syntaxLayout->addLayout(btnLayout);
    syntaxLayout->addWidget(treeStack_);
    syntaxTab_->setLayout(syntaxLayout);

    // ----- 中间代码Tab-----
    codeTab_ = new QWidget(this);
    QVBoxLayout* codeLayout = new QVBoxLayout(codeTab_);
    // 切换按钮
    QHBoxLayout* codeBtnLayout = new QHBoxLayout();
    switchCodeViewBtn_ = new QPushButton("切换为文本形式", this);
    codeBtnLayout->addStretch();
    codeBtnLayout->addWidget(switchCodeViewBtn_);
    codeLayout->addLayout(codeBtnLayout);
    // 使用 QStackedWidget 切换两种视图
    codeStack_ = new QStackedWidget(this);
    showingCodeTable_ = true;
    // 表格视图(index 0)
    codeTable_ = new QTableWidget(this);
    codeTable_->setColumnCount(5);
    codeTable_->setHorizontalHeaderLabels(
        { "序号", "操作码", "操作数1", "操作数2", "结果" });
    codeTable_->horizontalHeader()->setStretchLastSection(true);
    codeTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    codeTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    codeTable_->setFont(QFont("Consolas", 10));
    codeTable_->setStyleSheet(
        "QTableWidget { background-color: #f0fff0; "
        "border: 1px solid #ccc; }"
    );
    // 文本视图(index 1)
    codeTextView_ = new QPlainTextEdit(this);
    codeTextView_->setReadOnly(true);
    codeTextView_->setFont(QFont("Consolas", 10));
    codeTextView_->setStyleSheet(
        "QPlainTextEdit { background-color: #f0fff0; "
        "border: 1px solid #ccc; }"
    );
    codeStack_->addWidget(codeTable_);     // index 0
    codeStack_->addWidget(codeTextView_);  // index 1
    codeStack_->setCurrentIndex(0);
    codeLayout->addWidget(codeStack_);
    codeTab_->setLayout(codeLayout);
    // 连接切换按钮
    connect(switchCodeViewBtn_, &QPushButton::clicked,
        this, &MainWindow::onSwitchCodeView);

    // 添加Tabs
    resultTabs_->addTab(lexTab_, "词法分析");
    resultTabs_->addTab(syntaxTab_, "语法分析");
    resultTabs_->addTab(codeTab_, "中间代码生成");

    // ----- 错误信息表 -----
    errorTable_ = new QTableWidget(this);
    errorTable_->setColumnCount(2);
    errorTable_->setHorizontalHeaderLabels({ "类型", "错误信息" });
    errorTable_->horizontalHeader()->setStretchLastSection(true);
    errorTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    errorTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    errorTable_->setMaximumHeight(150);
    errorTable_->setFont(QFont("Consolas", 9));

    rightLayout->addWidget(resultTabs_);
    rightLayout->addWidget(new QLabel("错误信息:", this));
    rightLayout->addWidget(errorTable_);
    rightWidget->setLayout(rightLayout);

    // 将左右两部分添加到分割器
    mainSplitter_->addWidget(editorGroup_);
    mainSplitter_->addWidget(rightWidget);
    mainSplitter_->setSizes({ 450, 700 });

    // 连接按钮信号
    connect(expandAllBtn_, &QPushButton::clicked, this, &MainWindow::onExpandAll);
    connect(collapseAllBtn_, &QPushButton::clicked, this, &MainWindow::onCollapseAll);
    connect(expandSelBtn_, &QPushButton::clicked, this, &MainWindow::onExpandSelected);
    connect(collapseSelBtn_, &QPushButton::clicked, this, &MainWindow::onCollapseSelected);
    connect(switchViewBtn_, &QPushButton::clicked, this, &MainWindow::onSwitchView);
    connect(popupViewBtn_, &QPushButton::clicked, this, &MainWindow::onPopupView); // 新增

    // 初始化按钮状态（目录树模式下展开选中/折叠选中可用）
    updateButtonStates();
}

/*=============================================*/
/*  初始化工具栏                               */
/*=============================================*/
void MainWindow::setupToolBar()
{
    QToolBar* toolbar = addToolBar("工具栏");
    toolbar->setIconSize(QSize(24, 24));

    newAction_ = new QAction(
        QApplication::style()->standardIcon(QStyle::SP_FileIcon),
        "新建", this);
    newAction_->setShortcut(QKeySequence::New);
    connect(newAction_, &QAction::triggered, this, &MainWindow::onNewFile);

    openAction_ = new QAction(
        QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton),
        "打开", this);
    openAction_->setShortcut(QKeySequence::Open);
    connect(openAction_, &QAction::triggered, this, &MainWindow::onOpenFile);

    saveAction_ = new QAction(
        QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton),
        "保存", this);
    saveAction_->setShortcut(QKeySequence::Save);
    connect(saveAction_, &QAction::triggered, this, &MainWindow::onSaveFile);

    saveAsAction_ = new QAction(
        QApplication::style()->standardIcon(QStyle::SP_DriveFDIcon),
        "另存为", this);
    saveAsAction_->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction_, &QAction::triggered, this, &MainWindow::onSaveAsFile);

    analyzeAction_ = new QAction(
        QApplication::style()->standardIcon(QStyle::SP_MediaPlay),
        "语法分析", this);
    analyzeAction_->setShortcut(QKeySequence(Qt::Key_F5));
    connect(analyzeAction_, &QAction::triggered, this, &MainWindow::onAnalyze);

    toolbar->addAction(newAction_);
    toolbar->addAction(openAction_);
    toolbar->addAction(saveAction_);
    toolbar->addAction(saveAsAction_);
    toolbar->addSeparator();
    toolbar->addAction(analyzeAction_);
}

/*=============================================*/
/*  初始化菜单栏                                */
/*=============================================*/
void MainWindow::setupMenuBar()
{
    QMenu* fileMenu = menuBar()->addMenu("文件(&F)");
    fileMenu->addAction(newAction_);
    fileMenu->addAction(openAction_);
    fileMenu->addAction(saveAction_);
    fileMenu->addAction(saveAsAction_);
    fileMenu->addSeparator();
    fileMenu->addAction("退出(&Q)", QKeySequence::Quit, this, &QWidget::close);

    QMenu* analyzeMenu = menuBar()->addMenu("分析(&A)");
    analyzeMenu->addAction(analyzeAction_);

    QMenu* helpMenu = menuBar()->addMenu("帮助(&H)");
    helpMenu->addAction("关于(&A)", this, [this]() {
        QMessageBox::about(this, "关于",
            "TINY扩充语言语法树生成器\n\n"
            "功能:\n"
            "- 支持扩充的TINY语言文法\n"
            "- 词法分析、语法分析和中间代码生成\n"
            "- 语法树可视化(可展开/折叠)\n"
			"- 中间代码生成以四元组形式表现\n"
            "- 支持 if/while/for/repeat 语句\n"
            "- 支持正则表达式赋值\n"
            "- 支持前置自增(++)、自减(--)\n"
            "- 支持 %、^ 运算符\n"
            "- 支持扩充比较运算符\n\n"
            "基于Kenneth C. Louden的TINY编译器扩展");
        });
}

/*=============================================*/
/*  新建文件                                    */
/*=============================================*/
void MainWindow::onNewFile()
{
    sourceEditor_->clear();
    currentFilePath_.clear();
    setWindowTitle("TINY扩充语言 - 语法树生成器 - [新文件]");
    statusBar()->showMessage("新建文件");
}

/*=============================================*/
/*  打开文件                                    */
/*=============================================*/
void MainWindow::onOpenFile()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, "打开TINY源文件", "",
        "TINY源文件 (*.tny *.txt);;所有文件 (*.*)");

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法打开文件: " + filePath);
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    sourceEditor_->setPlainText(in.readAll());
    file.close();

    currentFilePath_ = filePath;
    setWindowTitle("TINY扩充语言 - 语法树生成器 - [" + filePath + "]");
    statusBar()->showMessage("已打开: " + filePath);
}

/*=============================================*/
/*  保存文件                                    */
/*=============================================*/
void MainWindow::onSaveFile()
{
    if (currentFilePath_.isEmpty()) {
        onSaveAsFile();
        return;
    }

    QFile file(currentFilePath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法保存文件: " + currentFilePath_);
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << sourceEditor_->toPlainText();
    file.close();

    statusBar()->showMessage("已保存: " + currentFilePath_);
}

/*=============================================*/
/*  另存为文件                                  */
/*=============================================*/
void MainWindow::onSaveAsFile()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, "保存TINY源文件", "",
        "TINY源文件 (*.tny);;文本文件 (*.txt);;所有文件 (*.*)");

    if (filePath.isEmpty()) return;

    currentFilePath_ = filePath;
    onSaveFile();

    setWindowTitle("TINY扩充语言 - 语法树生成器 - [" + filePath + "]");
}

/*=============================================*/
/*  执行语法分析                                */
/*=============================================*/
void MainWindow::onAnalyze()
{
    QString source = sourceEditor_->toPlainText();
    if (source.trimmed().isEmpty()) {
        QMessageBox::information(this, "提示", "请先输入或打开TINY源代码");
        return;
    }

    // 释放旧的语法树
    if (syntaxTree_) {
        deleteTree(syntaxTree_);
        syntaxTree_ = nullptr;
    }

    // 执行解析
    syntaxTree_ = parser_.parse(source.toStdString());

    // 显示词法分析结果
    showTokens(parser_.getTokens());

    // 显示语法树
    treeModel_->setSyntaxTree(syntaxTree_);
    syntaxTreeView_->expandAll();

    // 中间代码生成
    codeGen_.generate(syntaxTree_);
    showQuadruples(codeGen_.getQuadruples());

    // 修复：如果当前是多叉树模式，立即构建并延迟刷新布局
    if (!showingTreeView_) {
        graphicsTreeView_->buildFromModel(treeModel_);
        // 延迟执行 fitInView，确保视图几何尺寸已更新
        QTimer::singleShot(50, this, [this]() {
            graphicsTreeView_->refreshLayout();
            });
    }

    // 收集所有错误
    std::vector<std::string> allErrors;
    for (auto& e : parser_.getScanErrors())
        allErrors.push_back("[词法] " + e);
    for (auto& e : parser_.getErrors())
        allErrors.push_back("[语法] " + e);
    for (auto& e : codeGen_.getErrors())
        allErrors.push_back("[代码生成] " + e);

    showErrors(allErrors);

    // 切换到语法分析Tab
    resultTabs_->setCurrentIndex(1);

    // 同步刷新所有已打开的弹窗
    refreshAllPopups();

    if (allErrors.empty()) {
        statusBar()->showMessage("语法分析完成，无错误");
    }
    else {
        statusBar()->showMessage("语法分析完成，发现 "
            + QString::number(allErrors.size()) + " 个错误");
    }
}

/*=============================================*/
/*  新增：同步刷新所有已打开的弹窗           */
/*=============================================*/
void MainWindow::refreshAllPopups()
{
    // 清理已关闭的弹窗
    openPopups_.removeIf([](QDialog* d) { return !d->isVisible(); });

    // 通知每个弹窗刷新（通过发出自定义信号或直接调用slot）
    // 弹窗在创建时已连接 treeModel_ 的 modelReset 信号，会自动刷新
    // 这里额外触发一次以确保图形视图同步
    for (QDialog* dlg : openPopups_) {
        // 找到弹窗中的 SyntaxTreeGraphicsView 并刷新
        // 弹窗通过 objectName 标记
        auto* gv = dlg->findChild<SyntaxTreeGraphicsView*>("popupGraphicsView");
        auto* tv = dlg->findChild<QTreeView*>("popupTreeView");
        auto* sw = dlg->findChild<QStackedWidget*>("popupStack");

        if (tv) {
            tv->expandAll();
        }
        if (sw && gv && sw->currentWidget() == gv) {
            gv->buildFromModel(treeModel_);
            QTimer::singleShot(50, gv, [gv]() {
                gv->refreshLayout();
                });
        }
    }
}

/*=============================================*/
/*  显示词法分析Token列表                       */
/*=============================================*/
void MainWindow::showTokens(const std::vector<Token>& tokens)
{
    tokenTable_->setRowCount(0);
    tokenTable_->setRowCount(static_cast<int>(tokens.size()));

    for (int i = 0; i < static_cast<int>(tokens.size()); i++) {
        const Token& tok = tokens[i];

        QTableWidgetItem* lineItem = new QTableWidgetItem(
            QString::number(tok.lineno));
        QTableWidgetItem* typeItem = new QTableWidgetItem(
            QString::fromStdString(Scanner::tokenTypeName(tok.type)));
        QTableWidgetItem* lexItem = new QTableWidgetItem(
            QString::fromStdString(tok.lexeme));

        tokenTable_->setItem(i, 0, lineItem);
        tokenTable_->setItem(i, 1, typeItem);
        tokenTable_->setItem(i, 2, lexItem);
    }

    tokenTable_->resizeColumnsToContents();
}

/*=============================================*/
/*  显示错误信息                                */
/*=============================================*/
void MainWindow::showErrors(const std::vector<std::string>& errors)
{
    errorTable_->setRowCount(0);

    if (errors.empty()) {
        errorTable_->setRowCount(1);
        QTableWidgetItem* typeItem = new QTableWidgetItem("Right!");
        typeItem->setForeground(Qt::darkGreen);
        QTableWidgetItem* msgItem = new QTableWidgetItem("无错误");
        msgItem->setForeground(Qt::darkGreen);
        errorTable_->setItem(0, 0, typeItem);
        errorTable_->setItem(0, 1, msgItem);
        return;
    }

    errorTable_->setRowCount(static_cast<int>(errors.size()));
    for (int i = 0; i < static_cast<int>(errors.size()); i++) {
        const std::string& err = errors[i];
        QTableWidgetItem* typeItem;
        if (err.find("[词法]") != std::string::npos) {
            typeItem = new QTableWidgetItem("词法错误");
        }
        else if (err.find("[语法]") != std::string::npos) {
            typeItem = new QTableWidgetItem("语法错误");
        }
        else if (err.find("[代码生成]") != std::string::npos) {
            typeItem = new QTableWidgetItem("代码生成错误");
        }
        else {
            typeItem = new QTableWidgetItem("未知错误");
        }
        typeItem->setForeground(Qt::red);

        QTableWidgetItem* msgItem = new QTableWidgetItem(
            QString::fromStdString(err));
        msgItem->setForeground(Qt::red);

        errorTable_->setItem(i, 0, typeItem);
        errorTable_->setItem(i, 1, msgItem);
    }

    errorTable_->resizeColumnsToContents();
}

/*=============================================*/
/*  显示中间代码四元组列表                    */
/*=============================================*/
void MainWindow::showQuadruples(const std::vector<Quadruple>& quads)
{
    // ========== 1. 填充表格视图 ==========
    codeTable_->setRowCount(0);
    if (quads.empty()) {
        codeTable_->setRowCount(1);
        QTableWidgetItem* item = new QTableWidgetItem("（无四元组生成）");
        item->setForeground(Qt::gray);
        codeTable_->setItem(0, 0, item);
        codeTextView_->clear();
        return;
    }
    codeTable_->setRowCount(static_cast<int>(quads.size()));
    for (int i = 0; i < static_cast<int>(quads.size()); i++) {
        const Quadruple& q = quads[i];
        // --- 判断是否为占位空行（假出口行） ---
        bool isPlaceholder = q.op.empty();
        // 序号
        auto* idxItem = new QTableWidgetItem(
            QString("(%1)").arg(q.index));
        idxItem->setTextAlignment(Qt::AlignCenter);
        if (isPlaceholder) {
            // 假出口占位行：其余列留空
            codeTable_->setItem(i, 0, idxItem);
            // 操作码、操作数1、操作数2、结果都留空
            for (int col = 1; col <= 4; col++) {
                codeTable_->setItem(i, col, new QTableWidgetItem(""));
            }
            continue;
        }
        // 操作码
        auto* opItem = new QTableWidgetItem(
            QString::fromStdString(q.op));
        opItem->setTextAlignment(Qt::AlignCenter);
        // 操作数1
        std::string a1 = q.arg1.empty() ? "_" : q.arg1;
        auto* arg1Item = new QTableWidgetItem(
            QString::fromStdString(a1));
        arg1Item->setTextAlignment(Qt::AlignCenter);
        // 操作数2
        std::string a2 = q.arg2.empty() ? "_" : q.arg2;
        auto* arg2Item = new QTableWidgetItem(
            QString::fromStdString(a2));
        arg2Item->setTextAlignment(Qt::AlignCenter);
        // 结果：若为纯数字跳转目标，显示为 (num) 格式
        std::string resultStr = q.result;
        QString resultDisplay;
        bool isJumpTarget = false;
        if (!q.op.empty() && q.op[0] == 'j' && !resultStr.empty() && resultStr != "0") {
            // 尝试解析为整数
            try {
                int tgt = std::stoi(resultStr);
                resultDisplay = QString("(%1)").arg(tgt);
                isJumpTarget = true;
            }
            catch (...) {
                resultDisplay = QString::fromStdString(resultStr);
            }
        }
        else {
            resultDisplay = QString::fromStdString(resultStr);
        }
        auto* resItem = new QTableWidgetItem(resultDisplay);
        resItem->setTextAlignment(Qt::AlignCenter);
        codeTable_->setItem(i, 0, idxItem);
        codeTable_->setItem(i, 1, opItem);
        codeTable_->setItem(i, 2, arg1Item);
        codeTable_->setItem(i, 3, arg2Item);
        codeTable_->setItem(i, 4, resItem);
    }
    codeTable_->resizeColumnsToContents();
    // ========== 2. 填充文本视图 ==========
    QString textOutput;
    for (int i = 0; i < static_cast<int>(quads.size()); i++) {
        const Quadruple& q = quads[i];
        bool isPlaceholder = q.op.empty();
        // 序号 (固定格式)
        QString idxPart = QString("(%1) ").arg(q.index);
        textOutput += idxPart;
        if (isPlaceholder) {
            // 假出口占位行：仅有序号，换行
            textOutput += "\n";
            continue;
        }
        // 操作码
        QString opPart = QString::fromStdString(q.op);
        // 操作数1
        std::string a1 = q.arg1.empty() ? "_" : q.arg1;
        QString arg1Part = QString::fromStdString(a1);
        // 操作数2
        std::string a2 = q.arg2.empty() ? "_" : q.arg2;
        QString arg2Part = QString::fromStdString(a2);
        // 结果：跳转目标加括号
        std::string resultStr = q.result;
        QString resPart;
        if (!q.op.empty() && q.op[0] == 'j' && !resultStr.empty() && resultStr != "0") {
            try {
                int tgt = std::stoi(resultStr);
                resPart = QString("(%1)").arg(tgt);
            }
            catch (...) {
                resPart = QString::fromStdString(resultStr);
            }
        }
        else {
            resPart = QString::fromStdString(resultStr);
        }
        // 组合为 (op, arg1, arg2, result) 形式
        textOutput += QString("(%1 , %2 , %3 , %4)\n")
            .arg(opPart, arg1Part, arg2Part, resPart);
    }
    codeTextView_->setPlainText(textOutput);
}

/*=============================================*/
/*  展开全部节点                                */
/*=============================================*/
void MainWindow::onExpandAll()
{
    if (showingTreeView_) {
        syntaxTreeView_->expandAll();
    }
    else {
        graphicsTreeView_->expandAll();
    }
}

/*=============================================*/
/*  折叠全部节点                                */
/*=============================================*/
void MainWindow::onCollapseAll()
{
    if (showingTreeView_) {
        syntaxTreeView_->collapseAll();
    }
    else {
        graphicsTreeView_->collapseAll();
    }
}

/*=============================================*/
/*  展开选中节点及其子树                        */
/*=============================================*/
void MainWindow::onExpandSelected()
{
    if (showingTreeView_) {
        QModelIndex index = syntaxTreeView_->currentIndex();
        if (index.isValid()) {
            std::function<void(const QModelIndex&)> expandRecursive;
            expandRecursive = [&](const QModelIndex& idx) {
                syntaxTreeView_->expand(idx);
                int rows = treeModel_->rowCount(idx);
                for (int i = 0; i < rows; i++) {
                    expandRecursive(treeModel_->index(i, 0, idx));
                }
                };
            expandRecursive(index);
        }
    }
    // 多叉树模式下展开选中暂不支持（双击节点展开）
}

/*=============================================*/
/*  折叠选中节点及其子树                        */
/*=============================================*/
void MainWindow::onCollapseSelected()
{
    if (showingTreeView_) {
        QModelIndex index = syntaxTreeView_->currentIndex();
        if (index.isValid()) {
            std::function<void(const QModelIndex&)> collapseRecursive;
            collapseRecursive = [&](const QModelIndex& idx) {
                int rows = treeModel_->rowCount(idx);
                for (int i = 0; i < rows; i++) {
                    collapseRecursive(treeModel_->index(i, 0, idx));
                }
                syntaxTreeView_->collapse(idx);
                };
            collapseRecursive(index);
        }
    }
}

/*=============================================*/
/*  新增：根据视图模式更新按钮启用/禁用状态    */
/*=============================================*/
void MainWindow::updateButtonStates()
{
    if (showingTreeView_) {
        // 目录树模式：所有按钮可用
        expandSelBtn_->setEnabled(true);
        collapseSelBtn_->setEnabled(true);
        expandSelBtn_->setToolTip("展开选中节点及其子树");
        collapseSelBtn_->setToolTip("折叠选中节点及其子树");
    }
    else {
        // 多叉树模式：展开选中/折叠选中不可用（置灰）
        expandSelBtn_->setEnabled(false);
        collapseSelBtn_->setEnabled(false);
        expandSelBtn_->setToolTip("多叉树模式下不可用（请双击节点展开/折叠）");
        collapseSelBtn_->setToolTip("多叉树模式下不可用（请双击节点展开/折叠）");
    }
}

/*=============================================*/
/*  语法树切换函数                             */
/*=============================================*/
void MainWindow::onSwitchView()
{
    showingTreeView_ = !showingTreeView_;

    if (showingTreeView_) {
        // 切换到目录树
        treeStack_->setCurrentIndex(0);
        switchViewBtn_->setText("切换为多叉树");
    }
    else {
        // 切换到多叉树，从目录树model构建
        graphicsTreeView_->buildFromModel(treeModel_);
        treeStack_->setCurrentIndex(1);
        switchViewBtn_->setText("切换为目录树");

        // 修复：延迟执行 fitInView，等待视图完成布局
        QTimer::singleShot(50, this, [this]() {
            graphicsTreeView_->refreshLayout();
            });
    }

    // 切换后更新按钮状态
    updateButtonStates();
}

/*=============================================*/
/*  中间代码视图切换（表格/文本）              */
/*=============================================*/
void MainWindow::onSwitchCodeView()
{
    showingCodeTable_ = !showingCodeTable_;
    if (showingCodeTable_) {
        codeStack_->setCurrentIndex(0);
        switchCodeViewBtn_->setText("切换为文本形式");
    }
    else {
        codeStack_->setCurrentIndex(1);
        switchCodeViewBtn_->setText("切换为表格形式");
    }
}

/*=============================================*/
/*  弹窗查看（弹出当前视图类型，含完整功能）  */
/*=============================================*/
void MainWindow::onPopupView()
{
    QDialog* dialog = new QDialog(this);

    // 弹出与当前主窗口相同的视图类型
    QString title = showingTreeView_ ?
        "语法树 - 目录树视图（弹窗）" : "语法树 - 多叉树视图（弹窗）";
    dialog->setWindowTitle(title);
    dialog->resize(1000, 700);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    // 按钮区（与主窗口一致：展开全部、折叠全部、展开选中、折叠选中、切换视图）
    QHBoxLayout* btnLayout = new QHBoxLayout();
    auto* popExpandAllBtn = new QPushButton("展开全部", dialog);
    auto* popCollapseAllBtn = new QPushButton("折叠全部", dialog);
    auto* popExpandSelBtn = new QPushButton("展开选中", dialog);
    auto* popCollapseSelBtn = new QPushButton("折叠选中", dialog);
    auto* popSwitchBtn = new QPushButton(
        showingTreeView_ ? "切换为多叉树" : "切换为目录树", dialog);

    popExpandAllBtn->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_ArrowDown));
    popCollapseAllBtn->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_ArrowUp));

    btnLayout->addWidget(popExpandAllBtn);
    btnLayout->addWidget(popCollapseAllBtn);
    btnLayout->addWidget(popExpandSelBtn);
    btnLayout->addWidget(popCollapseSelBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(popSwitchBtn);
    layout->addLayout(btnLayout);

    // 视图区
    auto* popStack = new QStackedWidget(dialog);
    popStack->setObjectName("popupStack");

    // 目录树视图
    auto* popTreeView = new QTreeView(dialog);
    popTreeView->setObjectName("popupTreeView");
    popTreeView->setModel(treeModel_);
    popTreeView->setFont(QFont("Consolas", 10));
    popTreeView->setAnimated(true);
    popTreeView->setIndentation(25);
    popTreeView->setHeaderHidden(false);
    popTreeView->setStyleSheet(
        "QTreeView { background-color: #f0fff0; border: 1px solid #ccc; }");
    popTreeView->expandAll();

    // 多叉树视图
    auto* popGraphicsView = new SyntaxTreeGraphicsView(dialog);
    popGraphicsView->setObjectName("popupGraphicsView");

    popStack->addWidget(popTreeView);      // index 0
    popStack->addWidget(popGraphicsView);  // index 1

    // 弹窗初始视图与主窗口一致
    bool popShowingTree = showingTreeView_;
    if (popShowingTree) {
        popStack->setCurrentIndex(0);
        // 置灰展开选中/折叠选中：目录树模式下可用
        popExpandSelBtn->setEnabled(true);
        popCollapseSelBtn->setEnabled(true);
    }
    else {
        popGraphicsView->buildFromModel(treeModel_);
        popStack->setCurrentIndex(1);
        QTimer::singleShot(50, popGraphicsView, [popGraphicsView]() {
            popGraphicsView->refreshLayout();
            });
        popExpandSelBtn->setEnabled(false);
        popCollapseSelBtn->setEnabled(false);
        popExpandSelBtn->setToolTip("多叉树模式下不可用");
        popCollapseSelBtn->setToolTip("多叉树模式下不可用");
    }

    layout->addWidget(popStack);
    dialog->setLayout(layout);

    // 使用 shared_ptr 管理弹窗内状态
    auto statePtr = std::make_shared<bool>(popShowingTree);

    // 更新弹窗按钮状态的辅助 lambda
    auto updatePopBtns = [=]() {
        if (*statePtr) {
            popExpandSelBtn->setEnabled(true);
            popCollapseSelBtn->setEnabled(true);
            popExpandSelBtn->setToolTip("展开选中节点及其子树");
            popCollapseSelBtn->setToolTip("折叠选中节点及其子树");
        }
        else {
            popExpandSelBtn->setEnabled(false);
            popCollapseSelBtn->setEnabled(false);
            popExpandSelBtn->setToolTip("多叉树模式下不可用(请双击节点展开/折叠)");
            popCollapseSelBtn->setToolTip("多叉树模式下不可用(请双击节点展开/折叠)");
        }
        };

    // 展开全部
    connect(popExpandAllBtn, &QPushButton::clicked, dialog,
        [statePtr, popTreeView, popGraphicsView]() {
            if (*statePtr) popTreeView->expandAll();
            else popGraphicsView->expandAll();
        });

    // 折叠全部
    connect(popCollapseAllBtn, &QPushButton::clicked, dialog,
        [statePtr, popTreeView, popGraphicsView]() {
            if (*statePtr) popTreeView->collapseAll();
            else popGraphicsView->collapseAll();
        });

    // 展开选中（仅目录树模式）
    SyntaxTreeModel* modelPtr = treeModel_;
    connect(popExpandSelBtn, &QPushButton::clicked, dialog,
        [statePtr, popTreeView, modelPtr]() {
            if (!*statePtr) return;
            QModelIndex idx = popTreeView->currentIndex();
            if (!idx.isValid()) return;
            std::function<void(const QModelIndex&)> expRec;
            expRec = [&](const QModelIndex& i) {
                popTreeView->expand(i);
                for (int r = 0; r < modelPtr->rowCount(i); r++)
                    expRec(modelPtr->index(r, 0, i));
                };
            expRec(idx);
        });

    // 折叠选中（仅目录树模式）
    connect(popCollapseSelBtn, &QPushButton::clicked, dialog,
        [statePtr, popTreeView, modelPtr]() {
            if (!*statePtr) return;
            QModelIndex idx = popTreeView->currentIndex();
            if (!idx.isValid()) return;
            std::function<void(const QModelIndex&)> colRec;
            colRec = [&](const QModelIndex& i) {
                for (int r = 0; r < modelPtr->rowCount(i); r++)
                    colRec(modelPtr->index(r, 0, i));
                popTreeView->collapse(i);
                };
            colRec(idx);
        });

    // 切换视图
    connect(popSwitchBtn, &QPushButton::clicked, dialog,
        [this, statePtr, popStack, popSwitchBtn,
        popGraphicsView, popTreeView, dialog, updatePopBtns]() {
            *statePtr = !(*statePtr);
            if (*statePtr) {
                popStack->setCurrentIndex(0);
                popSwitchBtn->setText("切换为多叉树");
                dialog->setWindowTitle("语法树 - 目录树视图（弹窗）");
            }
            else {
                popGraphicsView->buildFromModel(treeModel_);
                popStack->setCurrentIndex(1);
                popSwitchBtn->setText("切换为目录树");
                dialog->setWindowTitle("语法树 - 多叉树视图（弹窗）");
                QTimer::singleShot(50, popGraphicsView, [popGraphicsView]() {
                    popGraphicsView->refreshLayout();
                    });
            }
            updatePopBtns();
        });

    // 注册到弹窗列表，用于同步刷新
    openPopups_.append(dialog);

    // 弹窗关闭时从列表移除
    connect(dialog, &QDialog::finished, this, [this, dialog]() {
        openPopups_.removeOne(dialog);
        });

    dialog->show();
}