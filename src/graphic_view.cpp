#include "graphic_view.hpp"

#include <QWheelEvent>
#include <QScrollBar>

#include <algorithm>
#include <stdexcept>

GraphicView::GraphicView(QWidget* parent)
    : QGraphicsView(parent)
{
    if (!parent)
        throw std::invalid_argument("No parent on GraphicsView");

    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    setRenderHint(QPainter::Antialiasing, false);
    setRenderHint(QPainter::SmoothPixmapTransform, true);

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);

    setDragMode(QGraphicsView::ScrollHandDrag);

    setBackgroundBrush(QColor(16, 16, 16));

    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_pixelmap = m_scene->addPixmap(QPixmap{});
}

void GraphicView::setImage(const QPixmap& pixmap) {
    m_pixelmap->setPixmap(pixmap);
    m_scene->setSceneRect(m_pixelmap->boundingRect());
}

void GraphicView::clearImage() {
    m_pixelmap->setPixmap(QPixmap{});
    m_scene->setSceneRect(QRectF{});
    resetTransform();
    m_zoom = 1.0;
}

void GraphicView::fitImage() {
    if (m_pixelmap->pixmap().isNull())
        return;

    resetTransform();
    fitInView(m_pixelmap, Qt::KeepAspectRatio);
    m_zoom = transform().m11();
}

void GraphicView::resetZoom() {
    fitImage();
}

void GraphicView::wheelEvent(QWheelEvent* event) {
    if (m_pixelmap->pixmap().isNull())
        return;

    const int delta = event->angleDelta().y();

    if (delta == 0) 
        return;

    const qreal factor = std::pow(ZOOM_STEP, static_cast<qreal>(delta) / 120.0);
    const qreal new_zoom = std::clamp(m_zoom * factor, MIN_ZOOM, MAX_ZOOM);

    resetTransform();
    scale(new_zoom, new_zoom);
    m_zoom = new_zoom;

    event->accept();
}

void GraphicView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);

    if (m_zoom == 0.0)
        fitImage();
}