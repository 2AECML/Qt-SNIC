#include <vector>
#include <fstream>
#include <iostream>

class SegmentationResult {
public:
    SegmentationResult() :
        mLabelCount(0),
        mLabels() {
    }

    SegmentationResult(int* plabels, int* pnumlabels, int width, int height) {
        mLabelCount = *pnumlabels;
        mLabels.resize(height, std::vector<int>(width));
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                mLabels[i][j] = plabels[i * width + j];
            }
        }
    }

    void exportToCSV() {
        // ���ļ�����д��
        std::ofstream file("�ָ���.csv");

        if (!file.is_open()) {
            std::cerr << "�޷����ļ�����д��" << std::endl;
            return;
        }

        std::cout << "��ʼд��ָ�����CSV�ļ�..." << std::endl;

        // д���ǩ����
        for (const auto& row : mLabels) {
            for (size_t i = 0; i < row.size(); ++i) {
                file << row[i];
                if (i != row.size() - 1) {
                    file << ","; // ÿ����ǩ֮���ö��ŷָ�
                }
            }
            file << "\n"; // ÿ�н�������
        }

        std::cout << "д��ָ�����CSV�ļ����" << std::endl;

        // �ر��ļ�
        file.close();

    }

    int getLabelCount() {
        return mLabelCount;
    }

    int getLabel(int x, int y) {
        return mLabels[y][x];
    }

    std::vector<std::vector<int>> getLabels() {
        return mLabels;
    }

private:
    std::vector<std::vector<int>> mLabels;
    int mLabelCount;
};