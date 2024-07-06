#pragma once

#include <vector>
#include <fstream>
#include <iostream>
#include <map>
#include <ogrsf_frmts.h>
#include <algorithm>

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

        std::map<int, std::vector<OGRPoint>> pointMap;

        // ����ÿ�����أ�����ÿ����ǩ����Ӧ�Ķ���α߽�ring
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int label = mLabels[y][x];

                if (mPolygons.find(label) == mPolygons.end()) {
                    mPolygons[label] = new OGRPolygon();
                }

                if (pointMap.find(label) == pointMap.end()) {
                    pointMap[label] = std::vector<OGRPoint>();
                }

                if (x == 0 || y == 0 || x == width - 1 || y == height - 1) {
                    pointMap[label].emplace_back(x, y);
                }
                else {
                    if (mLabels[y - 1][x] != label || mLabels[y][x - 1] != label || mLabels[y + 1][x] != label || mLabels[y][x + 1] != label) {
                        pointMap[label].emplace_back(x, y);
                    }
                }
            }
        }



        
        for (auto& pair : pointMap) {
            std::vector<OGRPoint>& points = pair.second;
            if (points.empty()) continue;

            //points = simplifyPolygon(points, 1.0);

            std::vector<OGRPoint> sortedPoints;

            sortedPoints = sortPolygonVertices(points);


            OGRLinearRing* ring = new OGRLinearRing();
            for (const auto& point : sortedPoints) {
                ring->addPoint(&point);
            }
            ring->closeRings();

            mPolygons[pair.first]->addRingDirectly(ring);
        }

        std::cout << "���ɶ�������" << std::endl;

        int index = 0;
        for (auto& pair : mPolygons) {
            if (index++ >= 10) {
                break;
            }
            OGRLinearRing* ring = pair.second->getExteriorRing();
            std::cout << pair.first << "�ű�ǩ�Ķ���α߽磺" << std::endl;
            std::cout << "����" << ring->getNumPoints() << "���㣺" << std::endl;
            for (int i = 0; i < ring->getNumPoints(); ++i) {
                OGRPoint point;
                ring->getPoint(i, &point);
                std::cout << point.getX() << "," << point.getY() << std::endl;
            }
        }
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

    // ��������֮��ľ���
    double distance(const OGRPoint& p1, const OGRPoint& p2) {
        return std::sqrt(std::pow(p2.getX() - p1.getX(), 2) + std::pow(p2.getY() - p1.getY(), 2));
    }

    // �ٽ�����㷨
    std::vector<OGRPoint> simplifyPolygon(const std::vector<OGRPoint>& points, double tolerance) {
        std::vector<OGRPoint> simplifiedPoints;
        if (points.empty()) {
            return simplifiedPoints;
        }

        simplifiedPoints.push_back(points[0]);
        for (size_t i = 1; i < points.size(); ++i) {
            if (distance(simplifiedPoints.back(), points[i]) > tolerance) {
                simplifiedPoints.push_back(points[i]);
            }
        }

        // ȷ�����һ����͵�һ����֮��ľ���Ҳ�������̶�
        if (distance(simplifiedPoints.back(), simplifiedPoints.front()) <= tolerance) {
            simplifiedPoints.pop_back();
        }

        return simplifiedPoints;
    }

    // �ҵ�����ĵ�
    size_t findNearestPoint(const OGRPoint& point, const std::vector<OGRPoint>& points, const std::vector<bool>& used) {
        double minDistance = std::numeric_limits<double>::max();
        size_t nearestIndex = 0;
        for (size_t i = 0; i < points.size(); ++i) {
            if (!used[i]) {
                double dist = distance(point, points[i]);
                if (dist < minDistance) {
                    minDistance = dist;
                    nearestIndex = i;
                }
            }
        }
        return nearestIndex;
    }

    // ����ζ��������㷨
    std::vector<OGRPoint> sortPolygonVertices(std::vector<OGRPoint>& points) {
        if (points.empty()) {
            return points;
        }

        std::vector<OGRPoint> sortedPoints;
        std::vector<bool> used(points.size(), false);

        // ѡ���һ����
        size_t currentIndex = 0;
        sortedPoints.push_back(points[currentIndex]);
        used[currentIndex] = true;

        // �ҵ�����ĵ㲢��ӵ������ĵ㼯��
        while (sortedPoints.size() < points.size()) {
            currentIndex = findNearestPoint(sortedPoints.back(), points, used);
            sortedPoints.push_back(points[currentIndex]);
            used[currentIndex] = true;
        }

        return sortedPoints;
    }

    std::vector<OGRPoint> sortPointsByBoundaryTracing(const std::vector<OGRPoint>& points) {
        
    }

    

private:
    int mLabelCount;
    std::vector<std::vector<int>> mLabels;
    std::map<int, OGRPolygon*> mPolygons;
};