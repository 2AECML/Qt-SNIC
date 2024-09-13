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

    // ��ȡͼ������
    double* pinp = new double[w * h * c];
    for (int i = 0; i < c; ++i) {
        GDALRasterBand* band = mDataset->GetRasterBand(i + 1);
        CPLErr err = band->RasterIO(GF_Read, 0, 0, w, h, pinp + i * w * h, w, h, GDT_Float64, 0, 0);
        if (err) {
            std::cout << "��ȡդ������ʱ����: " << CPLGetLastErrorMsg() << std::endl;
        }
    }

    emit doingSegment();

    int numsuperpixels = mSNICOptions.superpixelNumber; // ����������
    double compactness = mSNICOptions.compactness; // ���ն�
    bool doRGBtoLAB = true; // �Ƿ����RGB��LAB��ת��
    int* plabels = new int[w * h]; // �洢�ָ����ı�ǩ
    int* pnumlabels = new int[1]; // �洢��ǩ����

    std::cout << "��ʼ����ͼ��ָ�..." << std::endl;

    SNIC_main(pinp, w, h, c, numsuperpixels, compactness, doRGBtoLAB, plabels, pnumlabels);

    std::cout << "ͼ��ָ����" << std::endl;

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
        std::cout << "�޷����ļ�" << std::endl;
        return;
    }

    // ��ȡͼ��Ļ�����Ϣ
    int nRasterXSize = mDataset->GetRasterXSize();
    int nRasterYSize = mDataset->GetRasterYSize();
    int nBands = mDataset->GetRasterCount();

    std::cout << "ͼ����: " << nRasterXSize << std::endl;
    std::cout << "ͼ��߶�: " << nRasterYSize << std::endl;
    std::cout << "��������: " << nBands << std::endl;
}
