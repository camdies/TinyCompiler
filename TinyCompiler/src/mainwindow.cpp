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

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), syntaxTree_(nullptr), showingTreeView_(true)
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
    switchViewBtn_ = new QPushButton("切换为多叉树", this);  // ★ 新增

    expandAllBtn_->setIcon(QApplication::style()->standardIcon(
        QStyle::SP_ArrowDown));
    collapseAllBtn_->setIcon(QApplication::style()->standardIcon(
        QStyle::SP_ArrowUp));

    btnLayout->addWidget(expandAllBtn_);
    btnLayout->addWidget(collapseAllBtn_);
    btnLayout->addWidget(expandSelBtn_);
    btnLayout->addWidget(collapseSelBtn_);
    btnLayout->addStretch();
    btnLayout->addWidget(switchViewBtn_);  // ★ 新增，放右侧

    // ★ 使用 QStackedWidget 切换两种视图
    treeStack_ = new QStackedWidget(this);
    showingTreeView_ = true;

    // 目录树视图（index 0）
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

    // 添加Tabs
    resultTabs_->addTab(lexTab_, "词法分析");
    resultTabs_->addTab(syntaxTab_, "语法分析");

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
            "- 词法分析与语法分析\n"
            "- 语法树可视化(可展开/折叠)\n"
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
    syntaxTreeView_->expandAll();   // 默认展开所有节点
    // 如果当前显示的是多叉树，也要更新
    if (!showingTreeView_) {
        graphicsTreeView_->buildFromModel(treeModel_);
    }

    // 收集所有错误
    std::vector<std::string> allErrors;
    for (auto& e : parser_.getScanErrors())
        allErrors.push_back("[词法] " + e);
    for (auto& e : parser_.getErrors())
        allErrors.push_back("[语法] " + e);

    showErrors(allErrors);

    // 切换到语法分析Tab
    resultTabs_->setCurrentIndex(1);

    if (allErrors.empty()) {
        statusBar()->showMessage("语法分析完成，无错误");
    }
    else {
        statusBar()->showMessage("语法分析完成，发现 "
            + QString::number(allErrors.size()) + " 个错误");
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
        else {
            typeItem = new QTableWidgetItem("语法错误");
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
    }
}