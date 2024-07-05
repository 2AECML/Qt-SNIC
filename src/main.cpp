#include "main.h"
#include "snic.h"
#include <iostream>
#include <gdal_priv.h>
#include "SegmentationResult.cpp"

extern "C" {
    void SNIC_main(double* pinp, int w, int h, int c, int numsuperpixels, double compactness, bool doRGBtoLAB, int* plabels, int* pnumlabels);
}


GDALDataset* getDataSet(const char* fileName) {
	GDALAllRegister();

	
	GDALDataset* dataset = (GDALDataset*)GDALOpen(fileName, GA_ReadOnly);
	if (dataset == NULL) {
		std::cout << "�޷����ļ�" << std::endl;
		return nullptr;
	}

	// ��ȡͼ��Ļ�����Ϣ
	int nRasterXSize = dataset->GetRasterXSize();
	int nRasterYSize = dataset->GetRasterYSize();
	int nBands = dataset->GetRasterCount();

	std::cout << "ͼ����: " << nRasterXSize << std::endl;
	std::cout << "ͼ��߶�: " << nRasterYSize << std::endl;
	std::cout << "��������: " << nBands << std::endl;


	return dataset;
}

SegmentationResult getSegmentationResult(GDALDataset* dataset) {
    if (dataset == nullptr) {
        std::cout << "�޷���ȡ���ݼ�" << std::endl;
        return SegmentationResult();
    }

    int w = dataset->GetRasterXSize();
    int h = dataset->GetRasterYSize();
    int c = dataset->GetRasterCount();

    // ��ȡͼ������
    double* pinp = new double[w * h * c];
    for (int i = 0; i < c; ++i) {
        GDALRasterBand* band = dataset->GetRasterBand(i + 1);
        band->RasterIO(GF_Read, 0, 0, w, h, pinp + i * w * h, w, h, GDT_Float64, 0, 0);
    }

    int numsuperpixels = 500; // ����������
    double compactness = 20.0; // ���ն�
    bool doRGBtoLAB = true; // �Ƿ����RGB��LAB��ת��
    int* plabels = new int[w * h]; // �洢�ָ����ı�ǩ
    int* pnumlabels = new int[1]; // �洢��ǩ����

    std::cout << "��ʼ����ͼ��ָ�..." << std::endl;

    SNIC_main(pinp, w, h, c, numsuperpixels, compactness, doRGBtoLAB, plabels, pnumlabels);

    std::cout << "ͼ��ָ����" << std::endl;

    SegmentationResult result(plabels, pnumlabels, w, h);

    delete[] pinp;
    delete[] plabels;
    delete[] pnumlabels;

    return result;
}

int main() {
    const char* fileName = "C:\\Users\\HC8052\\Pictures\\123.png";
    char* recodedFilename = CPLRecode(fileName, "GBK", "UTF-8");

    if (recodedFilename != nullptr) {
        GDALDataset* dataset = getDataSet(recodedFilename);

        if (dataset != nullptr) {
            SegmentationResult result = getSegmentationResult(dataset);
            std::cout << "�ָ�����ǩ����: " << result.getLabelCount() << std::endl;
            GDALClose(dataset);
            result.exportToCSV();   // ���������CSV�ļ�
        }

        CPLFree(recodedFilename); 
    }

    return 0;
}

