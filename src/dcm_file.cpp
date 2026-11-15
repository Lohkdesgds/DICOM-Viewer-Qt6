
#include "dcm_file.hpp"

#include <dcmtk/dcmimgle/dcmimage.h>
#include <dcmtk/dcmdata/dcdeftag.h>

#include <QDebug>

#include <climits>
#include <vector>

bool DCMFile::open(const QString& fileName)
{
    close();

    auto file = std::make_unique<DcmFileFormat>();

    const QByteArray encodedFileName = fileName.toLocal8Bit();

    const OFCondition status = file->loadFile(
        encodedFileName.constData()
    );

    if (status.bad())
    {
        qWarning().noquote()
            << "DCMTK: Failed to open DICOM file:"
            << status.text();

        return false;
    }

    m_file_name = fileName;
    m_file = std::move(file);

    printMetadata();

    return true;
}

void DCMFile::close()
{
    m_file.reset();
    m_file_name.clear();
}

bool DCMFile::isOpen() const noexcept
{
    return m_file != nullptr;
}

QString DCMFile::fileName() const
{
    return m_file_name;
}

DcmDataset* DCMFile::dataset() noexcept
{
    return m_file ? m_file->getDataset() : nullptr;
}

const DcmDataset* DCMFile::dataset() const noexcept
{
    return m_file ? m_file->getDataset() : nullptr;
}

QImage DCMFile::readImage(QString* error)
{
    if (!m_file)
    {
        if (error)
            *error = "No DICOM file is currently open.";

        return {};
    }

    DcmDataset* ds = dataset();

    const auto printAttribute = [ds](const DcmTagKey& tag, const char* name) {
        OFString value;

        if (ds->findAndGetOFString(tag, value).good())
        {
            qInfo().noquote()
                << name << ":" << value.c_str();
        }
        else
        {
            qInfo().noquote()
                << name << ": MISSING";
        }
    };

    printAttribute(DCM_SOPClassUID, "SOP Class UID");
    printAttribute(DCM_Modality, "Modality");
    printAttribute(DCM_Rows, "Rows");
    printAttribute(DCM_Columns, "Columns");
    printAttribute(DCM_BitsAllocated, "Bits Allocated");
    printAttribute(DCM_BitsStored, "Bits Stored");
    printAttribute(DCM_SamplesPerPixel, "Samples Per Pixel");
    printAttribute(DCM_PhotometricInterpretation, "Photometric Interpretation");
    printAttribute(DCM_PixelRepresentation, "Pixel Representation");

    if (!ds)
    {
        if (error)
            *error = "DICOM dataset is unavailable.";

        return {};
    }

    if (!ds->tagExists(DCM_PixelData))
    {
        if (error)
            *error = "DICOM file does not contain PixelData.";

        return {};
    }

    // Create the DCMTK image representation from our loaded dataset.
    // EXS_Unknown is appropriate when passing a DcmFileFormat.
    DicomImage dicomImage(m_file.get(), EXS_Unknown);

    if (dicomImage.getStatus() != EIS_Normal)
    {
        qWarning() << "DCMTK image status:"
                << dicomImage.getStatus();

        qWarning() << "DCMTK image status description:"
                << DicomImage::getString(dicomImage.getStatus());

        if (error)
        {
            *error = QString::fromLocal8Bit(
                DicomImage::getString(dicomImage.getStatus())
            );
        }

        return {};
    }

    const unsigned long dicomWidth = dicomImage.getWidth();
    const unsigned long dicomHeight = dicomImage.getHeight();

    if (dicomWidth == 0 || dicomHeight == 0 ||
        dicomWidth > static_cast<unsigned long>(INT_MAX) ||
        dicomHeight > static_cast<unsigned long>(INT_MAX))
    {
        if (error)
            *error = "Invalid or unsupported DICOM image dimensions.";

        return {};
    }

    const int width = static_cast<int>(dicomWidth);
    const int height = static_cast<int>(dicomHeight);

    // Request 8-bit output for convenient QImage conversion.
    constexpr int outputBits = 8;

    const unsigned long outputSize =
        dicomImage.getOutputDataSize(outputBits);

    if (outputSize == 0)
    {
        if (error)
            *error = "Could not determine DICOM pixel data size.";

        return {};
    }

    // Allocate a buffer owned by our application.
    std::vector<Uint8> pixelData(outputSize);

    // Render the first frame (frame index 0).
    // planar = 0 means interleaved RGB for color images.
    if (!dicomImage.getOutputData(
            pixelData.data(),
            outputSize,
            outputBits,
            0,
            0))
    {
        if (error)
            *error = "DCMTK failed to render the pixel data.";

        return {};
    }

    const bool monochrome = dicomImage.isMonochrome();

    const QImage::Format format = monochrome
        ? QImage::Format_Grayscale8
        : QImage::Format_RGB888;

    const qsizetype bytesPerLine = monochrome
        ? static_cast<qsizetype>(width)
        : static_cast<qsizetype>(width) * 3;

    // Construct a QImage referencing our temporary buffer.
    // copy() is important: the returned image must own its pixels.
    const QImage image(
        pixelData.data(),
        width,
        height,
        bytesPerLine,
        format
    );

    if (image.isNull())
    {
        if (error)
            *error = "Could not create the Qt image.";

        return {};
    }

    return image.copy();
}

std::vector<DCMFile::MetadataEntry> DCMFile::metadata()
{
    std::vector<MetadataEntry> result;

    if (!m_file)
        return result;

    DcmDataset* ds = m_file->getDataset();
    DcmMetaInfo* meta = m_file->getMetaInfo();

    const auto add = [&result](DcmItem* item, const DcmTagKey& tag, const QString& label) {
        if (!item)
            return;

        OFString value;

        if (item->findAndGetOFString(tag, value).good())
            result.emplace_back(label, QString::fromLocal8Bit(value.c_str()));
    };

    add(ds, DCM_PatientName, "Patient Name");
    add(ds, DCM_PatientID, "Patient ID");
    add(ds, DCM_PatientBirthDate, "Patient Birth Date");
    add(ds, DCM_PatientSex, "Patient Sex");

    add(ds, DCM_StudyDate, "Study Date");
    add(ds, DCM_StudyTime, "Study Time");
    add(ds, DCM_AccessionNumber, "Accession Number");
    add(ds, DCM_StudyDescription, "Study Description");
    add(ds, DCM_StudyInstanceUID, "Study Instance UID");

    add(ds, DCM_Modality, "Modality");
    add(ds, DCM_SeriesDescription, "Series Description");
    add(ds, DCM_SeriesInstanceUID, "Series Instance UID");
    add(ds, DCM_SeriesNumber, "Series Number");

    add(ds, DCM_SOPClassUID, "SOP Class UID");
    add(ds, DCM_SOPInstanceUID, "SOP Instance UID");
    add(ds, DCM_Rows, "Rows");
    add(ds, DCM_Columns, "Columns");
    add(ds, DCM_BitsAllocated, "Bits Allocated");
    add(ds, DCM_BitsStored, "Bits Stored");
    add(ds, DCM_SamplesPerPixel, "Samples Per Pixel");
    add(ds, DCM_PhotometricInterpretation, "Photometric Interpretation");
    add(ds, DCM_PixelRepresentation, "Pixel Representation");

    add(ds, DCM_Manufacturer, "Manufacturer");
    add(ds, DCM_InstitutionName, "Institution Name");

    add(meta, DCM_TransferSyntaxUID, "Transfer Syntax UID");

    return result;
}

void DCMFile::printMetadata()
{
    qInfo() << "\n========== DICOM Metadata ==========";

    for (const auto& [name, value] : metadata())
        qInfo().noquote() << name << ":" << value;

    qInfo() << "====================================\n";
}
