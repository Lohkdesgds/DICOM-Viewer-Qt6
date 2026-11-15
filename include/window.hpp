#pragma once

#include "dcm_file.hpp"

#include <QMainWindow>
#include <QPixmap>
#include <QLabel>
#include <QDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QMenu>
#include <QAction>

class QResizeEvent;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void showImageProperties();
    void openImage();
    void updateImage();

    QAction* imagePropertiesAction_ = nullptr;
    QLabel imageLabel_;
    QPixmap originalImage_;

    DCMFile dcmFile_;
};