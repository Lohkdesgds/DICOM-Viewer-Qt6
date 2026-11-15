#pragma once

#include <QGraphicsView>
#include <QGraphicsPixmapItem>

class GraphicView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit GraphicView(QWidget* parent = nullptr);

    void setImage(const QPixmap& pixmap);
    void clearImage();

    void fitImage();
    void resetZoom();

protected:
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QGraphicsScene* m_scene{};
    QGraphicsPixmapItem* m_pixelmap{};

    qreal m_zoom = 1.0;

    static constexpr qreal MIN_ZOOM = 0.01;
    static constexpr qreal MAX_ZOOM = 20.0;
    static constexpr qreal ZOOM_STEP = 1.15;
};