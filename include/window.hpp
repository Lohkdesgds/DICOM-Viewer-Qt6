#pragma once

#include <QMainWindow>
#include <QPixmap>
#include <QLabel>

#include <memory>

class QLabel;
class QResizeEvent;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void openImage();
    void updateImage();

    QLabel imageLabel_;
    QPixmap originalImage_;
};