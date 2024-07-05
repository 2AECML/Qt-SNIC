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
		std::cout << "无法打开文件" << std::endl;
		return nullptr;
	}

	// 获取图像的基本信息
	int nRasterXSize = dataset->GetRasterXSize();
	int nRasterYSize = dataset->GetRasterYSize();
	int nBands = dataset->GetRasterCount();

	std::cout << "图像宽度: " << nRasterXSize << std::endl;
	std::cout << "图像高度: " << nRasterYSize << std::endl;
	std::cout << "波段数量: " << nBands << std::endl;


	return dataset;
}

SegmentationResult getSegmentationResult(GDALDataset* dataset) {
    if (dataset == nullptr) {
        std::cout << "无法获取数据集" << std::endl;
        return SegmentationResult();
    }

    int w = dataset->GetRasterXSize();
    int h = dataset->GetRasterYSize();
    int c = dataset->GetRasterCount();

    // 读取图像数据
    double* pinp = new double[w * h * c];
    for (int i = 0; i < c; ++i) {
        GDALRasterBand* band = dataset->GetRasterBand(i + 1);
        band->RasterIO(GF_Read, 0, 0, w, h, pinp + i * w * h, w, h, GDT_Float64, 0, 0);
    }

    int numsuperpixels = 500; // 超像素数量
    double compactness = 20.0; // 紧凑度
    bool doRGBtoLAB = true; // 是否进行RGB到LAB的转换
    int* plabels = new int[w * h]; // 存储分割结果的标签
    int* pnumlabels = new int[1]; // 存储标签数量

    std::cout << "开始进行图像分割..." << std::endl;

    SNIC_main(pinp, w, h, c, numsuperpixels, compactness, doRGBtoLAB, plabels, pnumlabels);

    std::cout << "图像分割完成" << std::endl;

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
            std::cout << "分割结果标签数量: " << result.getLabelCount() << std::endl;
            GDALClose(dataset);
            result.exportToCSV();   // 导出结果到CSV文件
        }

        CPLFree(recodedFilename); 
    }

    return 0;
}

