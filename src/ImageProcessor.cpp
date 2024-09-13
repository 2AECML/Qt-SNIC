#include "ImageProcessor.h"

ImageProcessor::ImageProcessor() : mSegResult(nullptr) {
}

void ImageProcessor::setImageFileName(const std::string& fileName) {
	mImageFileName = fileName;
}

void ImageProcessor::executeSNIC() {
    if (mImageFileName.empty()) {
        std::cout << "The split could not be completed because the image path is invalid" << std::endl;
        return;
    }

    emit doingGetDataset();

    getDataSet();

    if (mDataset == nullptr) {
        std::cout << "The split could not be completed because there is no data in dataset" << std::endl;
        return;
    }

    int w = mDataset->GetRasterXSize();
    int h = mDataset->GetRasterYSize();
    int c = mDataset->GetRasterCount();

    // 读取图像数据
    double* pinp = new double[w * h * c];
    for (int i = 0; i < c; ++i) {
        GDALRasterBand* band = mDataset->GetRasterBand(i + 1);
        CPLErr err = band->RasterIO(GF_Read, 0, 0, w, h, pinp + i * w * h, w, h, GDT_Float64, 0, 0);
        if (err) {
            qDebug() << "读取栅格数据时出错: " << CPLGetLastErrorMsg();
        }
    }

    emit doingSegment();

    int numsuperpixels = 500; // 超像素数量
    double compactness = 20.0; // 紧凑度
    bool doRGBtoLAB = true; // 是否进行RGB到LAB的转换
    int* plabels = new int[w * h]; // 存储分割结果的标签
    int* pnumlabels = new int[1]; // 存储标签数量

    qDebug() << "开始进行图像分割...";

    SNIC_main(pinp, w, h, c, numsuperpixels, compactness, doRGBtoLAB, plabels, pnumlabels);

    qDebug() << "图像分割完成";

    emit doingInitResult();

    mSegResult.reset(new SegmentationResult(plabels, pnumlabels, w, h));

    delete[] pinp;
    delete[] plabels;
    delete[] pnumlabels;
}

std::unique_ptr<SegmentationResult> ImageProcessor::getSegmentationResult() const {
	return std::make_unique<SegmentationResult>(*mSegResult);
}

void ImageProcessor::getDataSet() {
    const char* cFileName = mImageFileName.c_str();
    GDALAllRegister();
    mDataset.reset(static_cast<GDALDataset*>(GDALOpen(cFileName, GA_ReadOnly)));
    if (mDataset == nullptr) {
        qDebug() << "无法打开文件";
        return;
    }

    // 获取图像的基本信息
    int nRasterXSize = mDataset->GetRasterXSize();
    int nRasterYSize = mDataset->GetRasterYSize();
    int nBands = mDataset->GetRasterCount();

    qDebug() << "图像宽度: " << nRasterXSize;
    qDebug() << "图像高度: " << nRasterYSize;
    qDebug() << "波段数量: " << nBands;
}
