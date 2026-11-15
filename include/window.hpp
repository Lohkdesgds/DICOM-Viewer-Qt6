#pragma once

#include "dcm_file.hpp"
#include "graphic_view.hpp"

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
#include <QDockWidget>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QComboBox>

class QResizeEvent;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void showImageProperties();
    void openImage();
    void quitApplication();
    void setupWindowLevelDock();
    void updateWindowLevelControls();
    void applyWindowLevel();
    void refreshDicomImage();
    void populateWindowPresets();

    QAction* m_properties_action{};
    GraphicView* m_image_view{};
    QDockWidget* m_window_dock{};
    QSlider* m_center_slider{};
    QSlider* m_width_slider{};
    QDoubleSpinBox* m_center_spin{};
    QDoubleSpinBox* m_width_spin{};
    QComboBox* m_window_presets{};

    DCMFile m_dcm_file;

    double m_center_min = 0.0;
    double m_center_max = 1.0;
    double m_width_max = 1.0;
    bool m_is_fullscreen = false;
};