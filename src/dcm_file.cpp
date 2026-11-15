
#include "dcm_file.hpp"

#include <dcmtk/dcmdata/dcdeftag.h>

#include <QDebug>

#include <climits>
#include <vector>
#include <algorithm>

static const std::pair<DcmTagKey, const char8_t*> dataset_metadata[] = {
    { DCM_PatientName,                  u8"Nome do Paciente" },
    { DCM_PatientID,                    u8"ID do Paciente" },
    { DCM_PatientBirthDate,             u8"Data de Nascimento do Paciente" },
    { DCM_PatientSex,                   u8"Sexo do Paciente" },
    { DCM_StudyDate,                    u8"Data do Estudo" },
    { DCM_StudyTime,                    u8"Hora do Estudo" },
    { DCM_AccessionNumber,              u8"Número de Acesso" },
    { DCM_StudyDescription,             u8"Descrição do Estudo" },
    { DCM_StudyInstanceUID,             u8"UID da Instância do Estudo" },
    { DCM_Modality,                     u8"Modalidade" },
    { DCM_SeriesDescription,            u8"Descrição da Série" },
    { DCM_SeriesInstanceUID,            u8"UID da Instância da Série" },
    { DCM_SeriesNumber,                 u8"Número da Série" },
    { DCM_SOPClassUID,                  u8"UID da Classe SOP" },
    { DCM_SOPInstanceUID,               u8"UID da Instância SOP" },
    { DCM_Rows,                         u8"Linhas" },
    { DCM_Columns,                      u8"Colunas" },
    { DCM_BitsAllocated,                u8"Bits Alocados" },
    { DCM_BitsStored,                   u8"Bits Armazenados" },
    { DCM_SamplesPerPixel,              u8"Amostras por Pixel" },
    { DCM_PhotometricInterpretation,    u8"Interpretação Fotométrica" },
    { DCM_PixelRepresentation,          u8"Representação de Pixel" },
    { DCM_Manufacturer,                 u8"Fabricante" },
    { DCM_InstitutionName,              u8"Nome da Instituição" },
};

static const std::pair<DcmTagKey, const char8_t*> metainfo_metadata[] = {
    { DCM_TransferSyntaxUID,            u8"Transfer Syntax UID" }
};


bool DCMFile::open(const QString& filename) {
    close();

    auto file = std::make_unique<DcmFileFormat>();

    const QByteArray filename_utf8 = filename.toLocal8Bit();
    const OFCondition status = file->loadFile(filename_utf8.constData());

    if (status.bad()) {
        qWarning().noquote()
            << "DCMTK: Failed to open DICOM file:"
            << status.text();

        return false;
    }

    m_file_name = filename;
    m_file = std::move(file);

    return true;
}

void DCMFile::close() {
    m_dicom.reset();

    m_dicom_window_presets = 0;
    m_metadata_cache.clear();

    m_file.reset();
    m_file_name.clear();
}

bool DCMFile::isOpen() const noexcept {
    return m_file != nullptr;
}

QString DCMFile::fileName() const {
    return m_file_name;
}

QImage DCMFile::readImage(QString& error) {    
    if (!m_file) {
        error = u8"Nenhuma imagem está aberta neste momento";
        return {};
    }

    if (!m_dicom) {
        loadMetadata();

        DcmDataset* ds = m_file->getDataset();

        if (m_metadata_cache.empty()) {
            error = u8"Dados esperados não encontrados";
            return {};
        }

        if (!ds || !ds->tagExists(DCM_PixelData)) {
            error = u8"O arquivo não tem dados de imagem.";
            return {};
        }

        m_dicom = std::make_unique<DicomImage>(m_file.get(), EXS_Unknown);

        if (m_dicom->getStatus() != EIS_Normal) {
            qWarning() << "DCMTK image status:"
                       << m_dicom->getStatus();

            qWarning() << "DCMTK image status description:"
                       << DicomImage::getString(m_dicom->getStatus());

            error = QString::fromLocal8Bit(
                DicomImage::getString(m_dicom->getStatus())
            );

            m_dicom.reset();
            m_dicom_window_presets = 0;
            return {};
        }

        m_dicom_window_presets = m_dicom->getWindowCount();

        double center{}, width{};

        if (!m_dicom->getWindow(center, width))
            m_dicom->setMinMaxWindow();
    }

    const unsigned long dicom_width = m_dicom->getWidth();
    const unsigned long dicom_height = m_dicom->getHeight();

    if (dicom_width == 0 || dicom_height == 0 ||
        dicom_width > static_cast<unsigned long>(INT_MAX) ||
        dicom_height > static_cast<unsigned long>(INT_MAX)) {
        error = u8"Dimensões da imagem inválidas ou desconhecidas.";
        return {};
    }

    const int width = static_cast<int>(dicom_width);
    const int height = static_cast<int>(dicom_height);

    constexpr int output_bits = 8;

    const unsigned long output_size = m_dicom->getOutputDataSize(output_bits);

    if (output_size == 0) {
        error = u8"Falha ao determinar dados sobre os pixels da imagem";
        return {};
    }

    std::vector<Uint8> pixel_data(output_size);

    // Currently rendering the first frame.
    if (!m_dicom->getOutputData(
            pixel_data.data(),
            output_size,
            output_bits,
            0,
            0)) {
        error = u8"Biblioteca falhou em renderizar imagem.";
        return {};
    }

    const bool monochrome = m_dicom->isMonochrome();

    const QImage::Format format = monochrome
        ? QImage::Format_Grayscale8
        : QImage::Format_RGB888;

    const qsizetype bytes_per_line = monochrome
        ? static_cast<qsizetype>(width)
        : static_cast<qsizetype>(width) * 3;

    const QImage image(
        pixel_data.data(),
        width,
        height,
        bytes_per_line,
        format
    );

    if (image.isNull()) {
        error = u8"Falha ao criar a imagem em memória.";
        return {};
    }

    return image.copy();
}

std::vector<DCMFile::MetadataEntry> DCMFile::metadata() {
    std::vector<DCMFile::MetadataEntry> temp;
    for(const auto& i : m_metadata_cache)
        temp.push_back(i.second);
    return temp;
}

QString DCMFile::getPatientName() const {
    const auto it = std::find_if(m_metadata_cache.begin(), m_metadata_cache.end(), [](auto& p){ return p.first == DCM_PatientName;});
    if (it != m_metadata_cache.end()) return it->second.second;
    return u8"Desconhecido";
}

QString DCMFile::getDate() const {
    const auto it = std::find_if(m_metadata_cache.begin(), m_metadata_cache.end(), [](auto& p){ return p.first == DCM_StudyDate;});
    if (it != m_metadata_cache.end()) return it->second.second;
    return u8"Desconhecido";
}

bool DCMFile::supportWindowing() const noexcept {
    return m_dicom && m_dicom->isMonochrome();
}

bool DCMFile::setWindow(double center, double width)
{
    if (!m_dicom || !m_dicom->isMonochrome())
        return false;

    if (!std::isfinite(center) ||
        !std::isfinite(width) ||
        width < 1.0) {
        return false;
    }

    return m_dicom->setWindow(center, width) != 0;
}

double DCMFile::getWindowCenter() const noexcept {
    double center{}, width{};

    if (m_dicom)
        m_dicom->getWindow(center, width);

    return center;
}

double DCMFile::getWindowWidth() const noexcept {
    double center{}, width{};

    if (m_dicom)
        m_dicom->getWindow(center, width);

    return width;
}

unsigned long DCMFile::windowCount() const noexcept {
    return m_dicom_window_presets;
}

bool DCMFile::setDicomWindow(unsigned long index) {
    return m_dicom &&
           index < m_dicom->getWindowCount() &&
           m_dicom->setWindow(index) != 0;
}

bool DCMFile::resetWindow() {
    if (!m_dicom || !m_dicom->isMonochrome())
        return false;

    m_dicom->setMinMaxWindow();

    return true;
}

void DCMFile::loadMetadata() {
    m_metadata_cache.clear();

    if (!m_file)
        return;

    DcmDataset* ds = m_file->getDataset();
    DcmMetaInfo* meta = m_file->getMetaInfo();

    if (!ds) return;

    const auto add = [this](DcmItem* item, const DcmTagKey& tag, const QString& label) {
        OFString value;
        if (item->findAndGetOFString(tag, value).good())
            m_metadata_cache.push_back({
                tag,
                MetadataEntry{label, QString::fromLocal8Bit(value.c_str())}
            });
    };

    for(const auto& [tag, name] : dataset_metadata)
        add(ds, tag, name);

    for(const auto& [tag, name] : metainfo_metadata)
        add(meta, tag, name);
}