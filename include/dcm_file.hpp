
#pragma once

#include <dcmtk/dcmimgle/dcmimage.h>
#include <dcmtk/dcmdata/dctk.h>

#include <QString>
#include <QImage>

#include <memory>
#include <utility>
#include <vector>
#include <unordered_map>

class DCMFile {
public:
    using MetadataEntry = std::pair<QString, QString>;

    DCMFile() = default;

    bool open(const QString& filename);
    void close();

    bool isOpen() const noexcept;

    QString fileName() const;

    QImage readImage(QString& error);

    std::vector<MetadataEntry> metadata();

    QString getPatientName() const;
    QString getDate() const;

    bool supportWindowing() const noexcept;

    bool setWindow(double center, double width);

    double getWindowCenter() const noexcept;
    double getWindowWidth() const noexcept;

    unsigned long windowCount() const noexcept;
    bool setDicomWindow(unsigned long index);

    bool resetWindow();
private:
    void loadMetadata();

    QString m_file_name;
    std::unique_ptr<DicomImage> m_dicom;
    std::unique_ptr<DcmFileFormat> m_file;
    std::vector<std::pair<DcmTagKey, MetadataEntry>> m_metadata_cache;
    uint32_t m_dicom_window_presets = 0;
};
