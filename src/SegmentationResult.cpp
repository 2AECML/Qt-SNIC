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
        // 打开文件用于写入
        std::ofstream file("分割结果.csv");

        if (!file.is_open()) {
            std::cerr << "无法打开文件进行写入" << std::endl;
            return;
        }

        std::cout << "开始写入分割结果到CSV文件..." << std::endl;

        // 写入标签数据
        for (const auto& row : mLabels) {
            for (size_t i = 0; i < row.size(); ++i) {
                file << row[i];
                if (i != row.size() - 1) {
                    file << ","; // 每个标签之间用逗号分隔
                }
            }
            file << "\n"; // 每行结束换行
        }

        std::cout << "写入分割结果到CSV文件完成" << std::endl;

        // 关闭文件
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

        std::cout << "开始生成分割结果的多边形..." << std::endl;

        int width = mLabels[0].size();
        int height = mLabels.size();

        std::map<int, std::vector<OGRPoint>> pointMap;

        // 遍历每个像素，生成每个标签所对应的多边形边界ring
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

        std::cout << "生成多边形完成" << std::endl;

        int index = 0;
        for (auto& pair : mPolygons) {
            if (index++ >= 10) {
                break;
            }
            OGRLinearRing* ring = pair.second->getExteriorRing();
            std::cout << pair.first << "号标签的多边形边界：" << std::endl;
            std::cout << "共有" << ring->getNumPoints() << "个点：" << std::endl;
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

    // 计算两点之间的距离
    double distance(const OGRPoint& p1, const OGRPoint& p2) {
        return std::sqrt(std::pow(p2.getX() - p1.getX(), 2) + std::pow(p2.getY() - p1.getY(), 2));
    }

    // 临近点简化算法
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

        // 确保最后一个点和第一个点之间的距离也大于容忍度
        if (distance(simplifiedPoints.back(), simplifiedPoints.front()) <= tolerance) {
            simplifiedPoints.pop_back();
        }

        return simplifiedPoints;
    }

    // 找到最近的点
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

    // 多边形顶点排序算法
    std::vector<OGRPoint> sortPolygonVertices(std::vector<OGRPoint>& points) {
        if (points.empty()) {
            return points;
        }

        std::vector<OGRPoint> sortedPoints;
        std::vector<bool> used(points.size(), false);

        // 选择第一个点
        size_t currentIndex = 0;
        sortedPoints.push_back(points[currentIndex]);
        used[currentIndex] = true;

        // 找到最近的点并添加到排序后的点集中
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