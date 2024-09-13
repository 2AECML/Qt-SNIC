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

    // ��ȡͼ������
    double* pinp = new double[w * h * c];
    for (int i = 0; i < c; ++i) {
        GDALRasterBand* band = mDataset->GetRasterBand(i + 1);
        CPLErr err = band->RasterIO(GF_Read, 0, 0, w, h, pinp + i * w * h, w, h, GDT_Float64, 0, 0);
        if (err) {
            qDebug() << "��ȡդ������ʱ����: " << CPLGetLastErrorMsg();
        }
    }

    emit doingSegment();

    int numsuperpixels = 500; // ����������
    double compactness = 20.0; // ���ն�
    bool doRGBtoLAB = true; // �Ƿ����RGB��LAB��ת��
    int* plabels = new int[w * h]; // �洢�ָ����ı�ǩ
    int* pnumlabels = new int[1]; // �洢��ǩ����

    qDebug() << "��ʼ����ͼ��ָ�...";

    SNIC_main(pinp, w, h, c, numsuperpixels, compactness, doRGBtoLAB, plabels, pnumlabels);

    qDebug() << "ͼ��ָ����";

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
        qDebug() << "�޷����ļ�";
        return;
    }

    // ��ȡͼ��Ļ�����Ϣ
    int nRasterXSize = mDataset->GetRasterXSize();
    int nRasterYSize = mDataset->GetRasterYSize();
    int nBands = mDataset->GetRasterCount();

    qDebug() << "ͼ����: " << nRasterXSize;
    qDebug() << "ͼ��߶�: " << nRasterYSize;
    qDebug() << "��������: " << nBands;
}
