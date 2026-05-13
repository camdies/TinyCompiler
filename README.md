# TinyCompiler

一个基于 **Qt6 + C++17** 的 TINY 扩充语言语法树生成器，可视化展示词法/语法分析结果。

## 软件介绍

TinyCompiler 提供图形化界面来编辑与分析 TINY 扩充语言源代码，并支持语法树可视化与错误信息提示，适用于教学与实验场景。

## 支持的功能

- 词法分析与语法分析
- 语法树可视化（目录树 / 多叉树切换）
- Token 表格展示
- 错误信息提示
- 支持扩充语法：if/while/for/repeat、正则表达式赋值、前置自增/自减、% 与 ^ 运算符等

## 快速开始

### 1. 环境准备
- Visual Studio 2022（MSVC）
- Qt 6.9.3
- CMake ≥ 3.16
- Ninja（推荐）

### 2. 构建

```bash
# 在 TinyCompiler 目录下执行
cmake --preset x64-debug
cmake --build --preset x64-debug
```

### 3. 运行

```bash
# Windows
out/build/x64-debug/TinyCompiler.exe
```
