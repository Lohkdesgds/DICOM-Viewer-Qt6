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
#include <QFormLayout>
#include <QPushButton>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_image_view(new GraphicView(this))
{
    setWindowTitle("Image Viewer");
    setMinimumSize(400, 300);
    resize(900, 600);

    QImageReader::setAllocationLimit(2048); 

    setCentralWidget(m_image_view);
    setupWindowLevelDock();

    QMenu* file_menu = menuBar()->addMenu(u8"&Arquivo");
    QMenu* view_menu = menuBar()->addMenu(u8"&Mais informações");


    QAction* open_action = file_menu->addAction("A&brir imagem...");
    open_action->setShortcut(QKeySequence::Open);
    connect(open_action, &QAction::triggered, this, &MainWindow::openImage);

    QAction* quit_action = file_menu->addAction("&Sair");
    connect(quit_action, &QAction::triggered, this, &MainWindow::quitApplication);

    
    m_properties_action = view_menu->addAction("&Propriedades da imagem");
    m_properties_action->setEnabled(false);
    connect(m_properties_action, &QAction::triggered, this, &MainWindow::showImageProperties);

    view_menu->addAction(m_window_dock->toggleViewAction());


    statusBar()->showMessage("Ready.");
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    QMainWindow::keyPressEvent(event);

    if (event->type() != QEvent::Type::KeyPress)
        return;

    switch(event->key()) {
    case Qt::Key_F11:
        if ((m_is_fullscreen = !m_is_fullscreen)) {
            showFullScreen();
        } else {
            showNormal();
        }
        break;
    default:
        break;
    }
}

void MainWindow::showImageProperties() {
    if (!m_dcm_file.isOpen())
        return;

    QDialog dialog(this);
    dialog.setWindowTitle("Propriedades da imagem");
    dialog.resize(650, 500);

    auto* layout = new QVBoxLayout(&dialog);
    auto* table = new QTableWidget(&dialog);

    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({"Propriedade", "Valor"});

    const auto properties = m_dcm_file.metadata();

    table->setRowCount(static_cast<int>(properties.size()));

    for (int row = 0; row < static_cast<int>(properties.size()); ++row)
    {
        const auto& [name, value] = properties[row];

        auto* item_name = new QTableWidgetItem(name);
        auto* item_value = new QTableWidgetItem(value);

        item_name->setFlags(item_name->flags() & ~Qt::ItemIsEditable);
        item_value->setFlags(item_value->flags() & ~Qt::ItemIsEditable);

        table->setItem(row, 0, item_name);
        table->setItem(row, 1, item_value);
    }

    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);

    layout->addWidget(table);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    layout->addWidget(buttons);
    dialog.exec();
}

void MainWindow::openImage() {
    const QString filename = QFileDialog::getOpenFileName(
        this,
        "Abrir imagem",
        QString(),
        "Imagens DICOM (*.dcm *.dicom);;"
        "Todos os arquivos (*)"
    );

    if (filename.isEmpty()) {
        m_dcm_file.close();
        m_properties_action->setEnabled(false);
        m_window_dock->setEnabled(false);
        m_window_dock->hide();
        return;
    }

    if (!m_dcm_file.open(filename)) {
        QMessageBox::warning(
            this,
            u8"Erro",
            u8"Não consegui abrir o arquivo:\n" + filename + 
            u8"\n\nTalvez o formato não seja compatível."
        );
        m_dcm_file.close();
        m_properties_action->setEnabled(false);
        m_window_dock->setEnabled(false);
        m_window_dock->hide();
        return;
    }

    QString error;
    const QImage image = m_dcm_file.readImage(error);

    if (image.isNull()) {
        QMessageBox::warning(
            this,
            u8"Erro de renderização",
            u8"Falha ao abrir imagem:\n" + error
        );
        m_dcm_file.close();
        m_properties_action->setEnabled(false);
        m_window_dock->setEnabled(false);
        m_window_dock->hide();
        return;
    }

    m_image_view->setImage(QPixmap::fromImage(image));
    m_image_view->fitImage();
    setWindowTitle("Image Viewer - " + filename);

    statusBar()->showMessage(
        QString("DICOM | %1 x %2 | %3 (%4)")
            .arg(image.width())
            .arg(image.height())
            .arg(m_dcm_file.getPatientName())
            .arg(m_dcm_file.getDate())
    );
    
    m_properties_action->setEnabled(true);

    const bool supports_windowing = m_dcm_file.supportWindowing();

    m_window_dock->setEnabled(supports_windowing);
    m_window_dock->setVisible(supports_windowing);

    if (supports_windowing) {
        populateWindowPresets();
        updateWindowLevelControls();
    }
}

void MainWindow::quitApplication() {
    QCoreApplication::quit();
}

void MainWindow::setupWindowLevelDock() {
    m_window_dock = new QDockWidget("Ajuste de imagem", this);
    m_window_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto* panel = new QWidget(m_window_dock);
    auto* layout = new QVBoxLayout(panel);
    auto* form = new QFormLayout();

    m_window_presets = new QComboBox(panel);
    m_window_presets->addItem("Manual", -1);

    m_center_slider = new QSlider(Qt::Horizontal, panel);
    m_center_slider->setRange(0, 1000);

    m_center_spin = new QDoubleSpinBox(panel);
    m_center_spin->setDecimals(2);
    m_center_spin->setRange(-1'000'000'000.0, 1'000'000'000.0);

    m_width_slider = new QSlider(Qt::Horizontal, panel);
    m_width_slider->setRange(0, 1000);

    m_width_spin = new QDoubleSpinBox(panel);
    m_width_spin->setDecimals(2);
    m_width_spin->setRange(1.0, 1'000'000'000.0);

    form->addRow(u8"Predefinição:", m_window_presets);
    form->addRow("Centro:", m_center_spin);
    form->addRow(m_center_slider);
    form->addRow("Largura:", m_width_spin);
    form->addRow(m_width_slider);

    layout->addLayout(form);

    auto* reset_button = new QPushButton("Redefinir", panel);
    layout->addWidget(reset_button);
    layout->addStretch();

    m_window_dock->setWidget(panel);
    addDockWidget(Qt::RightDockWidgetArea, m_window_dock);

    m_window_dock->setEnabled(false);
    m_window_dock->hide();

    connect(m_center_slider, &QSlider::valueChanged, this,
        [this](int value) {
            const double center =
                m_center_min +
                (m_center_max - m_center_min) * value / 1000.0;

            m_center_spin->setValue(center);
        }
    );

    connect(m_width_slider, &QSlider::valueChanged, this, 
        [this](int value) {
            const double width =
                1.0 + (m_width_max - 1.0) * value / 1000.0;

            m_width_spin->setValue(width);
        }
    );

    connect(m_center_spin, &QDoubleSpinBox::valueChanged, this, [this](double) { applyWindowLevel(); });

    connect(m_width_spin, &QDoubleSpinBox::valueChanged, this, [this](double) { applyWindowLevel(); });

    connect(m_window_presets, &QComboBox::currentIndexChanged, this,
        [this](int index) {
            if (index <= 0)
                return;

            const auto preset_index =
                m_window_presets->itemData(index).toULongLong();

            if (m_dcm_file.setDicomWindow(static_cast<unsigned long>(preset_index))) {
                refreshDicomImage();
                updateWindowLevelControls();
            }
        }
    );

    connect(reset_button, &QPushButton::clicked, this,
        [this] {
            if (m_dcm_file.resetWindow()) {
                refreshDicomImage();
                updateWindowLevelControls();

                const QSignalBlocker blocker(m_window_presets);
                m_window_presets->setCurrentIndex(0);
            }
        }
    );
}

void MainWindow::updateWindowLevelControls() {
    const double center = m_dcm_file.getWindowCenter();
    const double width = std::max(1.0, m_dcm_file.getWindowWidth());

    m_center_min = center - width * 2.0;
    m_center_max = center + width * 2.0;

    m_width_max = std::max(2.0, width * 4.0);

    const QSignalBlocker b1(m_center_spin);
    const QSignalBlocker b2(m_width_spin);
    const QSignalBlocker b3(m_center_slider);
    const QSignalBlocker b4(m_width_slider);

    m_center_spin->setValue(center);
    m_width_spin->setValue(width);

    m_center_slider->setValue(
        static_cast<int>(
            (center - m_center_min) /
            (m_center_max - m_center_min) * 1000.0
        )
    );

    m_width_slider->setValue(
        static_cast<int>(
            (width - 1.0) /
            (m_width_max - 1.0) * 1000.0
        )
    );
}

void MainWindow::applyWindowLevel() {
    const double center = m_center_spin->value();
    const double width = m_width_spin->value();

    if (!m_dcm_file.setWindow(center, width))
        return;

    refreshDicomImage();

    {
        const QSignalBlocker blocker(m_center_slider);

        const int value = static_cast<int>(
            (center - m_center_min) /
            (m_center_max - m_center_min) * 1000.0
        );

        m_center_slider->setValue(
            std::clamp(value, 0, 1000)
        );
    }

    {
        const QSignalBlocker blocker(m_width_slider);

        const int value = static_cast<int>(
            (width - 1.0) /
            (m_width_max - 1.0) * 1000.0
        );

        m_width_slider->setValue(std::clamp(value, 0, 1000));
    }

    const QSignalBlocker blocker(m_window_presets);
    m_window_presets->setCurrentIndex(0);
}

void MainWindow::refreshDicomImage() {
    QString error;
    const QImage image = m_dcm_file.readImage(error);

    if (image.isNull()) {
        statusBar()->showMessage("Falha: " + error);
        return;
    }

    m_image_view->setImage(QPixmap::fromImage(image));
}

void MainWindow::populateWindowPresets() {
    const QSignalBlocker blocker(m_window_presets);

    m_window_presets->clear();
    m_window_presets->addItem("Manual", -1);

    for (unsigned long i = 0; i < m_dcm_file.windowCount(); ++i) {
        m_window_presets->addItem(
            QString("DICOM predef %1").arg(i + 1),
            QVariant::fromValue<qulonglong>(i)
        );
    }

    m_window_presets->setCurrentIndex(0);
}