#include "ImageProcessor.h"

ImageProcessor::ImageProcessor() 
    : mSegResult(nullptr) {
}

ImageProcessor::ImageProcessor(const std::string& fileName) 
    : mImageFileName(fileName)
    , mSegResult(nullptr) {
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
            std::cout << "读取栅格数据时出错: " << CPLGetLastErrorMsg() << std::endl;
        }
    }

    emit doingSegment();

    int numsuperpixels = mSNICOptions.superpixelNumber; // 超像素数量
    double compactness = mSNICOptions.compactness; // 紧凑度
    bool doRGBtoLAB = true; // 是否进行RGB到LAB的转换
    int* plabels = new int[w * h]; // 存储分割结果的标签
    int* pnumlabels = new int[1]; // 存储标签数量

    std::cout << "开始进行图像分割..." << std::endl;

    SNIC_main(pinp, w, h, c, numsuperpixels, compactness, doRGBtoLAB, plabels, pnumlabels);

    std::cout << "图像分割完成" << std::endl;

    emit doingInitResult();

    mSegResult.reset(new SegmentationResult(plabels, pnumlabels, w, h));

    delete[] pinp;
    delete[] plabels;
    delete[] pnumlabels;
}

std::shared_ptr<SegmentationResult> ImageProcessor::getSegmentationResult() const {
	return mSegResult;
}

void ImageProcessor::setSNICOptions(const SNICOptions& options) {
    mSNICOptions = options;
}

void ImageProcessor::getDataSet() {
    const char* cFileName = mImageFileName.c_str();
    GDALAllRegister();
    mDataset.reset(static_cast<GDALDataset*>(GDALOpen(cFileName, GA_ReadOnly)));
    if (mDataset == nullptr) {
        std::cout << "无法打开文件" << std::endl;
        return;
    }

    // 获取图像的基本信息
    int nRasterXSize = mDataset->GetRasterXSize();
    int nRasterYSize = mDataset->GetRasterYSize();
    int nBands = mDataset->GetRasterCount();

    std::cout << "图像宽度: " << nRasterXSize << std::endl;
    std::cout << "图像高度: " << nRasterYSize << std::endl;
    std::cout << "波段数量: " << nBands << std::endl;
}
