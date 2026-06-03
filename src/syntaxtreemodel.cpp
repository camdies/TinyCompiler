//fileName: TinyCompiler // src // syntaxtreemodel.h
//coding: UTF-8 with BOM

//this Program is for SCNU_Compiler 2026 Experiment3
//@auther Cai Renbin


/****************************************************/
/* syntaxtreemodel.cpp                              */
/* 语法树Qt Model实现                                */
/* 将语法树的TreeNode结构转换为QTreeView可展示的模型  */
/****************************************************/

#include "syntaxtreemodel.h"
#include <QIcon>
#include <QFont>

/*=============================================*/
/*  TreeItem 实现                               */
/*=============================================*/

TreeItem::TreeItem(const QString& data, TreeItem* parent)
    : data_(data), parent_(parent)
{
}

TreeItem::~TreeItem()
{
    for (auto* child : children_)
        delete child;
}

void TreeItem::appendChild(TreeItem* child)
{
    children_.push_back(child);
}

TreeItem* TreeItem::child(int row)
{
    if (row >= 0 && row < static_cast<int>(children_.size()))
        return children_[row];
    return nullptr;
}

int TreeItem::childCount() const
{
    return static_cast<int>(children_.size());
}

int TreeItem::columnCount() const
{
    return 1; // 只有一列
}

QVariant TreeItem::data(int column) const
{
    if (column == 0)
        return data_;
    return QVariant();
}

int TreeItem::row() const
{
    if (parent_) {
        for (int i = 0; i < static_cast<int>(parent_->children_.size()); i++) {
            if (parent_->children_[i] == const_cast<TreeItem*>(this))
                return i;
        }
    }
    return 0;
}

TreeItem* TreeItem::parentItem()
{
    return parent_;
}

/*=============================================*/
/*  SyntaxTreeModel 实现                        */
/*=============================================*/

SyntaxTreeModel::SyntaxTreeModel(QObject* parent)
    : QAbstractItemModel(parent)
{
    rootItem_ = new TreeItem("语法树");
}

SyntaxTreeModel::~SyntaxTreeModel()
{
    delete rootItem_;
}

void SyntaxTreeModel::clear()
{
    beginResetModel();
    delete rootItem_;
    rootItem_ = new TreeItem("语法树");
    endResetModel();
}

/*=============================================*/
/* 设置语法树,将TreeNode转换为TreeItem          */
/* 添加 start 根节点                            */
/*=============================================*/
void SyntaxTreeModel::setSyntaxTree(TreeNode* root)
{
    beginResetModel();
    delete rootItem_;
    rootItem_ = new TreeItem("语法树结点");

    if (root != nullptr) {
        // [新增] 创建 start 节点作为顶层
        TreeItem* startItem = new TreeItem("start", rootItem_);
        rootItem_->appendChild(startItem);

        // 处理根节点及其兄弟节点(stmt_sequence中的多条语句)
        TreeNode* current = root;
        while (current != nullptr) {
            buildTreeItems(current, startItem);  // 挂在 start 下面
            current = current->sibling;
        }
    }

    endResetModel();
}

/*=============================================*/
/* 递归构建TreeItem树                           */
/* 注意：兄弟节点(sibling)只在顶层处理           */
/* 子节点(child[])需要递归构建                   */
/*=============================================*/
void SyntaxTreeModel::buildTreeItems(TreeNode* syntaxNode, TreeItem* parentItem)
{
    if (syntaxNode == nullptr) return;

    // 创建当前节点对应的TreeItem
    QString displayText = QString::fromStdString(syntaxNode->getDisplayText());
    TreeItem* currentItem = new TreeItem(displayText, parentItem);
    parentItem->appendChild(currentItem);

    // 递归添加子节点
    for (int i = 0; i < MAXCHILDREN; i++) {
        if (syntaxNode->child[i] != nullptr) {
            // 子节点本身及其兄弟节点都要处理
            TreeNode* childNode = syntaxNode->child[i];
            while (childNode != nullptr) {
                buildTreeItems(childNode, currentItem);
                childNode = childNode->sibling;
            }
        }
    }
}

/*=============================================*/
/*  QAbstractItemModel 接口实现                 */
/*=============================================*/

QVariant SyntaxTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    TreeItem* item = static_cast<TreeItem*>(index.internalPointer());

    if (role == Qt::DisplayRole) {
        return item->data(index.column());
    }
    else if (role == Qt::FontRole) {
        QFont font("Consolas", 10);
        return font;
    }

    return QVariant();
}

Qt::ItemFlags SyntaxTreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return QAbstractItemModel::flags(index);
}

QVariant SyntaxTreeModel::headerData(int section, Qt::Orientation orientation,
    int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem_->data(section);
    return QVariant();
}

QModelIndex SyntaxTreeModel::index(int row, int column,
    const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem* parentItem;
    if (!parent.isValid())
        parentItem = rootItem_;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    TreeItem* childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}

QModelIndex SyntaxTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem* childItem = static_cast<TreeItem*>(index.internalPointer());
    TreeItem* parentItem = childItem->parentItem();

    if (parentItem == rootItem_)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int SyntaxTreeModel::rowCount(const QModelIndex& parent) const
{
    TreeItem* parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem_;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}

int SyntaxTreeModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 1;
}