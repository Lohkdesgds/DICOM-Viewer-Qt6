#include "window.hpp"

#include <QAction>
#include <QFileDialog>
#include <QImageReader>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QResizeEvent>
#include <QStatusBar>
#include <QKeySequence>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      imageLabel_()
{
    setWindowTitle("Image Viewer");
    setMinimumSize(400, 300);
    resize(900, 600);

    imageLabel_.setAlignment(Qt::AlignCenter);
    imageLabel_.setText("Open an image to get started");
    imageLabel_.setStyleSheet("QLabel { background-color: #202020; color: #aaaaaa; }");

    setCentralWidget(&imageLabel_);

    QMenu* fileMenu = menuBar()->addMenu("&File");

    QAction* openAction = fileMenu->addAction("&Open Image...");
    openAction->setShortcut(QKeySequence::Open);

    connect(openAction, &QAction::triggered, this, &MainWindow::openImage);

    statusBar()->showMessage("Ready");
}

void MainWindow::openImage()
{
    const QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open Image",
        QString(),
        "Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.tif *.tiff);;"
        "All Files (*)"
    );

    // User cancelled the dialog.
    if (fileName.isEmpty())
        return;

    // Read the image, respecting its orientation metadata.
    QImageReader reader(fileName);
    reader.setAutoTransform(true);

    const QImage image = reader.read();

    if (image.isNull())
    {
        QMessageBox::warning(
            this,
            "Image Error",
            "Could not load image:\n" + reader.errorString()
        );

        return;
    }

    // Keep the original image.
    originalImage_ = QPixmap::fromImage(image);

    // Update the window.
    setWindowTitle("Image Viewer - " + fileName);

    statusBar()->showMessage(
        QString("%1 x %2")
            .arg(image.width())
            .arg(image.height())
    );

    updateImage();
}

void MainWindow::updateImage()
{
    if (originalImage_.isNull())
        return;

    const QPixmap scaledImage = originalImage_.scaled(
        imageLabel_.contentsRect().size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    imageLabel_.setPixmap(scaledImage);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    updateImage();
}