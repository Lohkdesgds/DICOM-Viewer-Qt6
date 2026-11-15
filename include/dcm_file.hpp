
#pragma once

#include <dcmtk/dcmdata/dctk.h>

#include <QString>
#include <QImage>

#include <memory>
#include <utility>
#include <vector>

class DCMFile {
public:
    using MetadataEntry = std::pair<QString, QString>;

    DCMFile() = default;

    bool open(const QString& fileName);
    void close();

    [[nodiscard]] bool isOpen() const noexcept;

    [[nodiscard]] QString fileName() const;

    [[nodiscard]] DcmDataset* dataset() noexcept;
    [[nodiscard]] const DcmDataset* dataset() const noexcept;

    [[nodiscard]] QImage readImage(QString* error = nullptr);

    [[nodiscard]] std::vector<MetadataEntry> metadata();
private:
    void printMetadata();

    QString m_file_name;
    std::unique_ptr<DcmFileFormat> m_file;
};
