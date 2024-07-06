#pragma once

#include <vector>
#include <fstream>
#include <iostream>
#include <map>
#include <ogrsf_frmts.h>

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

    ~SegmentationResult() {
        for (auto& pair : mPolygons) {
            delete pair.second;
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

    const std::vector<std::vector<int>>& getLabels() const {
        return mLabels;
    }

    int getLabelByPixel(int x, int y) {
        return mLabels[y][x];
    }

    void generatePolygons() {
        OGRRegisterAll();

        std::cout << "��ʼ���ɷָ����Ķ����..." << std::endl;

        int width = mLabels[0].size();
        int height = mLabels.size();

        std::map<int, OGRLinearRing*> ringMap;

        // ����ÿ�����أ�����ÿ����ǩ����Ӧ�Ķ���α߽�ring
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int label = mLabels[y][x];
                
                if (mPolygons.find(label) == mPolygons.end()) {
                    mPolygons[label] = new OGRPolygon();
                }

                if (ringMap.find(label) == ringMap.end()) {
                    ringMap[label] = new OGRLinearRing();
                }

                if (x == 0 || y == 0 || x == width - 1 || y == height - 1) {
                    ringMap[label]->addPoint(x, y);
                }
                else {
                    if (mLabels[y - 1][x] != label || mLabels[y][x - 1] != label || mLabels[y + 1][x] != label || mLabels[y][x + 1] != label) {     
                        ringMap[label]->addPoint(x, y);
                    }
                }
            }
        }

        // ����ÿ����ǩ����Ӧ��ring�����ɶ����
        for (auto& pair : ringMap) {
            pair.second->closeRings();

            mPolygons[pair.first]->addRingDirectly(pair.second->clone());
            delete pair.second;
        }

        std::cout << "���ɶ�������" << std::endl;

        //for (auto& pair : mPolygons) {
        //    OGRLinearRing* ring = pair.second->getExteriorRing();
        //    std::cout << pair.first << "�ű�ǩ�Ķ���α߽磺" << std::endl;
        //    std::cout << "����" << ring->getNumPoints() << "���㣺" << std::endl;
        //    for (int i = 0; i < ring->getNumPoints(); ++i) {
        //        OGRPoint point;
        //        ring->getPoint(i, &point);
        //        std::cout << point.getX() << "," << point.getY() << std::endl;
        //    }
        //}

    }

    int getPolygonCount() {
        return mPolygons.size();
    }

    const std::map<int, OGRPolygon*>& getPolygons() {
        return mPolygons;
    }

    const OGRPolygon* getPolygonByLabel(int label) {
        return mPolygons[label];
    }

private:
    int mLabelCount;
    std::vector<std::vector<int>> mLabels;
    std::map<int, OGRPolygon*> mPolygons;
};