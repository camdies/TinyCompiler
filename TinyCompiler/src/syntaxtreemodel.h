//fileName: TinyCompiler // src // syntaxtreemodel.h
//coding: UTF-8 with BOM

//this Program is for SCNU_Compiler 2026 Experiment3
//@auther Cai Renbin


/****************************************************/
/* syntaxtreemodel.h                                */
/* Qt TreeView的Model，用于展示语法树                 */
/* 支持展开/折叠分支(QTreeView自带功能)               */
/****************************************************/

#ifndef SYNTAXTREEMODEL_H
#define SYNTAXTREEMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QString>
#include <vector>
#include <memory>
#include "treenode.h"

/*=============================================*/
/*  内部树项目类，将语法树转换为Qt模型数据         */
/*=============================================*/
class TreeItem {
public:
    explicit TreeItem(const QString& data, TreeItem* parent = nullptr);
    ~TreeItem();

    void appendChild(TreeItem* child);

    TreeItem* child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    TreeItem* parentItem();

private:
    std::vector<TreeItem*> children_;
    QString data_;
    TreeItem* parent_;
};

/*=============================================*/
/*  Qt Model类                                  */
/*=============================================*/
class SyntaxTreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit SyntaxTreeModel(QObject* parent = nullptr);
    ~SyntaxTreeModel();

    // 设置语法树数据
    void setSyntaxTree(TreeNode* root);

    // 清空数据
    void clear();

    // QAbstractItemModel接口实现
    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
        const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

private:
    TreeItem* rootItem_;

    // 递归构建TreeItem
    void buildTreeItems(TreeNode* syntaxNode, TreeItem* parentItem);
};

#endif // SYNTAXTREEMODEL_H