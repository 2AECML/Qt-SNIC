#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include "snic.h"
#include "snicoptions.h"
#include "SegmentationResult.h"
#include <string>
#include <memory>
#include <QObject>

struct GDALDatasetDeleter {
    void operator()(GDALDataset* dataset) const {
        if (dataset) {
            GDALClose(dataset);
        }
    }
};

extern "C" {
    void SNIC_main(double* pinp, int w, int h, int c, int numsuperpixels, double compactness, bool doRGBtoLAB, int* plabels, int* pnumlabels);
}

class ImageProcessor : public QObject{
    Q_OBJECT

public:
    ImageProcessor();
    explicit ImageProcessor(const std::string& fileName);
    void setImageFileName(const std::string& fileName);
    void executeSNIC();
    std::shared_ptr<SegmentationResult> getSegmentationResult() const;
    void setSNICOptions(const SNICOptions& options);

private:
    void getDataSet();

private:
    std::string mImageFileName;
    std::shared_ptr<SegmentationResult> mSegResult;
    std::unique_ptr<GDALDataset, GDALDatasetDeleter> mDataset;
    SNICOptions mSNICOptions;

signals:
    void doingGetDataset();
    void doingSegment();
    void doingInitResult();
};

#endif // IMAGEPROCESSOR_H
