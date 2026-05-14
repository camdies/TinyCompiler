//fileName: TinyCompiler // src // syntaxtreeview.cpp

#include "syntaxtreeview.h"
#include <QAbstractItemModel>
#include <QFontMetrics>
#include <QFont>
#include <cmath>

SyntaxTreeGraphicsView::SyntaxTreeGraphicsView(QWidget* parent)
    : QGraphicsView(parent), root_(nullptr), currentScale_(1.0)
{
    scene_ = new QGraphicsScene(this);
    setScene(scene_);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setStyleSheet(
        "QGraphicsView { background-color: #f8f8f0; "
        "border: 1px solid #ccc; }"
    );
}

SyntaxTreeGraphicsView::~SyntaxTreeGraphicsView()
{
    delete root_;
}

void SyntaxTreeGraphicsView::clear()
{
    scene_->clear();
    delete root_;
    root_ = nullptr;
}

/*=============================================*/
/*  从 QAbstractItemModel 构建可视化树          */
/*=============================================*/
void SyntaxTreeGraphicsView::buildFromModel(QAbstractItemModel* model)
{
    clear();
    if (!model || model->rowCount() == 0) return;

    // model的结构：rootItem_ -> "start" -> 各语句节点
    // 我们从 model 的第一层开始构建
    QModelIndex rootIndex = model->index(0, 0); // "start"节点
    if (!rootIndex.isValid()) return;

    root_ = buildNode(model, rootIndex, nullptr);

    if (root_) {
        calculateSubtreeWidth(root_);
        layoutTree(root_, 0, 0);
        drawTree();

        // 自适应缩放到视图
        QRectF bounds = scene_->itemsBoundingRect();
        bounds.adjust(-30, -30, 30, 30);
        scene_->setSceneRect(bounds);
        fitInView(bounds, Qt::KeepAspectRatio);
        currentScale_ = transform().m11();
    }
}

VisualTreeNode* SyntaxTreeGraphicsView::buildNode(
    QAbstractItemModel* model,
    const QModelIndex& index,
    VisualTreeNode* parent)
{
    if (!index.isValid()) return nullptr;

    QString text = model->data(index, Qt::DisplayRole).toString();
    VisualTreeNode* node = new VisualTreeNode(text, parent);

    // 计算节点宽度
    QFont font("Consolas", 9);
    QFontMetrics fm(font);
    double textWidth = fm.horizontalAdvance(text);
    node->width = std::max(MIN_NODE_WIDTH, textWidth + NODE_PADDING * 2);

    // 递归构建子节点
    int rows = model->rowCount(index);
    for (int i = 0; i < rows; i++) {
        QModelIndex childIndex = model->index(i, 0, index);
        VisualTreeNode* child = buildNode(model, childIndex, node);
        if (child) {
            node->children.push_back(child);
        }
    }

    return node;
}

/*=============================================*/
/*  布局算法                                    */
/*=============================================*/
void SyntaxTreeGraphicsView::calculateSubtreeWidth(VisualTreeNode* node)
{
    if (!node) return;

    if (node->children.empty() || node->collapsed) {
        node->subtreeWidth = node->width;
        return;
    }

    double totalChildWidth = 0;
    for (auto* child : node->children) {
        calculateSubtreeWidth(child);
        totalChildWidth += child->subtreeWidth;
    }
    // 子节点间间距
    totalChildWidth += NODE_H_GAP * (node->children.size() - 1);

    node->subtreeWidth = std::max(node->width, totalChildWidth);
}

void SyntaxTreeGraphicsView::layoutTree(
    VisualTreeNode* node, double x, double y)
{
    if (!node) return;

    // 节点居中于其子树宽度
    node->x = x + node->subtreeWidth / 2.0;
    node->y = y;

    if (node->collapsed || node->children.empty()) return;

    // 子节点从左到右排列
    double childX = x;
    double childY = y + NODE_HEIGHT + NODE_V_GAP;

    for (auto* child : node->children) {
        layoutTree(child, childX, childY);
        childX += child->subtreeWidth + NODE_H_GAP;
    }
}

/*=============================================*/
/*  绘制                                        */
/*=============================================*/
void SyntaxTreeGraphicsView::drawTree()
{
    scene_->clear();
    if (root_) {
        drawNode(root_);
    }
}

void SyntaxTreeGraphicsView::drawNode(VisualTreeNode* node)
{
    if (!node) return;

    QFont font("Consolas", 9);

    // 绘制节点矩形(圆角)
    double rx = node->x - node->width / 2.0;
    double ry = node->y;

    QGraphicsRectItem* rect = scene_->addRect(
        rx, ry, node->width, NODE_HEIGHT,
        QPen(Qt::black, 1.5),
        QBrush(QColor(220, 240, 255)));
    rect->setData(0, QVariant::fromValue(reinterpret_cast<quintptr>(node)));
    node->rectItem = rect;

    // 绘制文字
    QGraphicsTextItem* text = scene_->addText(node->label, font);
    text->setDefaultTextColor(Qt::black);
    // 文字居中
    QRectF textBound = text->boundingRect();
    text->setPos(node->x - textBound.width() / 2.0,
        node->y + (NODE_HEIGHT - textBound.height()) / 2.0);
    node->textItem = text;

    // 如果有折叠的子节点，显示 [+] 标记
    if (node->collapsed && !node->children.empty()) {
        QGraphicsTextItem* mark = scene_->addText("[+]",
            QFont("Consolas", 7, QFont::Bold));
        mark->setDefaultTextColor(Qt::red);
        mark->setPos(rx + node->width - 18, ry + 1);
    }

    // 绘制到子节点的连线
    if (!node->collapsed) {
        for (auto* child : node->children) {
            QGraphicsLineItem* line = scene_->addLine(
                node->x, node->y + NODE_HEIGHT,
                child->x, child->y,
                QPen(QColor(100, 100, 100), 1.2));
            node->lineItems.push_back(line);

            drawNode(child);
        }
    }
}

/*=============================================*/
/*  交互：缩放、点击展开/折叠                    */
/*=============================================*/
void SyntaxTreeGraphicsView::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        double factor = 1.15;
        if (event->angleDelta().y() < 0)
            factor = 1.0 / factor;

        double newScale = currentScale_ * factor;
        // 限制缩放范围
        if (newScale > 0.1 && newScale < 10.0) {
            scale(factor, factor);
            currentScale_ = newScale;
        }
        event->accept();
    }
    else {
        QGraphicsView::wheelEvent(event);
    }
}

void SyntaxTreeGraphicsView::mouseDoubleClickEvent(QMouseEvent* event)
{
    QPointF scenePos = mapToScene(event->pos());
    if (root_) {
        VisualTreeNode* clicked = findNodeAt(scenePos, root_);
        if (clicked && !clicked->children.empty()) {
            clicked->collapsed = !clicked->collapsed;
            // 重新布局和绘制
            calculateSubtreeWidth(root_);
            layoutTree(root_, 0, 0);
            drawTree();
            // 更新场景范围
            QRectF bounds = scene_->itemsBoundingRect();
            bounds.adjust(-30, -30, 30, 30);
            scene_->setSceneRect(bounds);
        }
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void SyntaxTreeGraphicsView::mousePressEvent(QMouseEvent* event)
{
    QGraphicsView::mousePressEvent(event);
}

VisualTreeNode* SyntaxTreeGraphicsView::findNodeAt(
    const QPointF& scenePos, VisualTreeNode* node)
{
    if (!node) return nullptr;

    double rx = node->x - node->width / 2.0;
    QRectF nodeRect(rx, node->y, node->width, NODE_HEIGHT);
    if (nodeRect.contains(scenePos)) {
        return node;
    }

    if (!node->collapsed) {
        for (auto* child : node->children) {
            VisualTreeNode* found = findNodeAt(scenePos, child);
            if (found) return found;
        }
    }

    return nullptr;
}

void SyntaxTreeGraphicsView::expandAll()
{
    if (root_) {
        setCollapseRecursive(root_, false);
        calculateSubtreeWidth(root_);
        layoutTree(root_, 0, 0);
        drawTree();
        QRectF bounds = scene_->itemsBoundingRect();
        bounds.adjust(-30, -30, 30, 30);
        scene_->setSceneRect(bounds);
    }
}

void SyntaxTreeGraphicsView::collapseAll()
{
    if (root_) {
        // 折叠除根节点外的所有节点
        for (auto* child : root_->children) {
            setCollapseRecursive(child, true);
        }
        calculateSubtreeWidth(root_);
        layoutTree(root_, 0, 0);
        drawTree();
        QRectF bounds = scene_->itemsBoundingRect();
        bounds.adjust(-30, -30, 30, 30);
        scene_->setSceneRect(bounds);
    }
}

void SyntaxTreeGraphicsView::setCollapseRecursive(
    VisualTreeNode* node, bool collapse)
{
    if (!node) return;
    node->collapsed = collapse;
    for (auto* child : node->children) {
        setCollapseRecursive(child, collapse);
    }
}

void SyntaxTreeGraphicsView::clearGraphics(VisualTreeNode* node)
{
    if (!node) return;
    node->rectItem = nullptr;
    node->textItem = nullptr;
    node->lineItems.clear();
    for (auto* child : node->children)
        clearGraphics(child);
}

/*=============================================*/
/*    新增：刷新布局，解决视图尺寸未就绪导致    */
/*    fitInView 计算错误的问题                   */
/*=============================================*/
void SyntaxTreeGraphicsView::refreshLayout()
{
    if (root_) {
        calculateSubtreeWidth(root_);
        layoutTree(root_, 0, 0);
        drawTree();

        QRectF bounds = scene_->itemsBoundingRect();
        bounds.adjust(-30, -30, 30, 30);
        scene_->setSceneRect(bounds);

        // 只有在视图有有效尺寸时才执行 fitInView
        if (viewport()->width() > 1 && viewport()->height() > 1) {
            fitInView(bounds, Qt::KeepAspectRatio);
            currentScale_ = transform().m11();
        }
    }
}