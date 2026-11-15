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
#include <QFileInfo>

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
    QMenu* viewMenu = menuBar()->addMenu("&View");

    // fileMenu
    QAction* openAction = fileMenu->addAction("&Open Image...");
    openAction->setShortcut(QKeySequence::Open);

    connect(openAction, &QAction::triggered, this, &MainWindow::openImage);

    // viewMenu    
    imagePropertiesAction_ = viewMenu->addAction(
        "Image &Properties"
    );
    imagePropertiesAction_->setEnabled(false);

    connect(imagePropertiesAction_, &QAction::triggered,
            this, &MainWindow::showImageProperties);

    statusBar()->showMessage("Ready");
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    updateImage();
}

void MainWindow::showImageProperties()
{
    if (!dcmFile_.isOpen())
        return;

    QDialog dialog(this);
    dialog.setWindowTitle("Image Properties");
    dialog.resize(650, 500);

    auto* layout = new QVBoxLayout(&dialog);

    auto* table = new QTableWidget(&dialog);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({
        "Property",
        "Value"
    });

    const auto properties = dcmFile_.metadata();

    table->setRowCount(
        static_cast<int>(properties.size())
    );

    for (int row = 0; row < static_cast<int>(properties.size()); ++row)
    {
        const auto& [name, value] = properties[row];

        auto* nameItem = new QTableWidgetItem(name);
        auto* valueItem = new QTableWidgetItem(value);

        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);

        table->setItem(row, 0, nameItem);
        table->setItem(row, 1, valueItem);
    }

    table->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents
    );

    table->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch
    );

    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);

    layout->addWidget(table);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Close,
        &dialog
    );

    connect(buttons, &QDialogButtonBox::rejected,
            &dialog, &QDialog::reject);

    layout->addWidget(buttons);

    dialog.exec();
}

void MainWindow::openImage()
{
    const QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open Image",
        QString(),
        "Supported Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.tif *.tiff *.dcm *.dicom);;"
        "DICOM Images (*.dcm *.dicom);;"
        "Regular Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.tif *.tiff);;"
        "All Files (*)"
    );

    if (fileName.isEmpty())
        return;

    const QString extension =
        QFileInfo(fileName).suffix().toLower();

    // DICOM file
    if (extension == "dcm" || extension == "dicom")
    {
        if (!dcmFile_.open(fileName))
        {
            QMessageBox::warning(
                this,
                "DICOM Error",
                "Could not load DICOM file:\n" + fileName
            );

            return;
        }

        QString error;
        const QImage image = dcmFile_.readImage(&error);

        if (image.isNull())
        {
            QMessageBox::warning(
                this,
                "DICOM Rendering Error",
                "Could not render DICOM image:\n" + error
            );

            return;
        }

        originalImage_ = QPixmap::fromImage(image);

        setWindowTitle("Image Viewer - " + fileName);

        statusBar()->showMessage(
            QString("DICOM | %1 x %2")
                .arg(image.width())
                .arg(image.height())
        );

        updateImage();
        imagePropertiesAction_->setEnabled(true);

        return;
    }

    // Regular image
    dcmFile_.close();
    imagePropertiesAction_->setEnabled(false);

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

    originalImage_ = QPixmap::fromImage(image);

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