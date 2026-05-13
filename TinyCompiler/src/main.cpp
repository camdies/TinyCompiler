//fileName: TinyCompiler // src // main.cpp
//coding: UTF-8 with BOM

//this Program is for SCNU_Compiler 2026 Experiment3
//@auther Cai Renbin


/****************************************************/
/* main.cpp                                         */
/* 程序入口                                          */
/****************************************************/

#include <QApplication>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // 设置应用程序信息
    app.setApplicationName("TINY扩充语言语法树生成器");
    app.setApplicationVersion("1.0");

    // 设置全局字体
    QFont appFont("Microsoft YaHei", 9);
    app.setFont(appFont);

    // 创建并显示主窗口
    MainWindow window;
    window.show();

    return app.exec();
}