//fileName: TinyCompiler // src // syntaxtreeview.h

#ifndef SYNTAXTREEVIEW_H
#define SYNTAXTREEVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsLineItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QMenu>
#include <vector>
#include <map>
#include <functional>

/*=============================================*/
/*  可视化树节点                                */
/*=============================================*/
class VisualTreeNode {
public:
    QString label;
    std::vector<VisualTreeNode*> children;
    VisualTreeNode* parent;

    // 布局信息
    double x, y;
    double width;
    double subtreeWidth;
    bool collapsed;

    // 图形项
    QGraphicsRectItem* rectItem;
    QGraphicsTextItem* textItem;
    std::vector<QGraphicsLineItem*> lineItems;

    VisualTreeNode(const QString& lbl, VisualTreeNode* par = nullptr)
        : label(lbl), parent(par), x(0), y(0), width(0),
        subtreeWidth(0), collapsed(false),
        rectItem(nullptr), textItem(nullptr) {
    }

    ~VisualTreeNode() {
        for (auto* c : children) delete c;
    }
};

/*=============================================*/
/*  多叉树图形视图                              */
/*=============================================*/
class SyntaxTreeGraphicsView : public QGraphicsView {
    Q_OBJECT

public:
    explicit SyntaxTreeGraphicsView(QWidget* parent = nullptr);
    ~SyntaxTreeGraphicsView();

    // 从QTreeView的model构建可视化树
    void buildFromModel(class QAbstractItemModel* model);

    // 清空
    void clear();

    // 展开/折叠全部
    void expandAll();
    void collapseAll();

    void refreshLayout();

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    QGraphicsScene* scene_;
    VisualTreeNode* root_;
    double currentScale_;

    // 布局常量
    static constexpr double NODE_H_GAP = 30.0;   // 水平间距
    static constexpr double NODE_V_GAP = 60.0;   // 垂直间距
    static constexpr double NODE_HEIGHT = 30.0;   // 节点高度
    static constexpr double NODE_PADDING = 12.0;  // 文字左右内边距
    static constexpr double MIN_NODE_WIDTH = 40.0;

    // 从 model 递归构建
    VisualTreeNode* buildNode(QAbstractItemModel* model,
        const QModelIndex& index,
        VisualTreeNode* parent);

    // 布局算法
    void calculateSubtreeWidth(VisualTreeNode* node);
    void layoutTree(VisualTreeNode* node, double x, double y);

    // 绘制
    void drawTree();
    void drawNode(VisualTreeNode* node);
    void clearGraphics(VisualTreeNode* node);

    // 查找被点击的节点
    VisualTreeNode* findNodeAt(const QPointF& scenePos,
        VisualTreeNode* node);

    // 展开/折叠递归辅助
    void setCollapseRecursive(VisualTreeNode* node, bool collapse);
};

#endif // SYNTAXTREEVIEW_H