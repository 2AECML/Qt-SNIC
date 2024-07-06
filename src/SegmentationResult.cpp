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

        // 使用改进的边界跟踪算法对点进行排序
        for (auto& pair : pointMap) {
            std::vector<OGRPoint>& points = pair.second;
            if (points.empty()) continue;

            std::vector<OGRPoint> sortedPoints;

            sortedPoints = ClockwiseSortPoints(points);
            //OGRPoint startPoint = points[0];
            //sortedPoints.push_back(startPoint);
            //points.erase(std::remove(points.begin(), points.end(), startPoint), points.end());

            //OGRPoint currentPoint = startPoint;
            //int direction = 3; // 初始方向为左上角

            //while (!points.empty()) {
            //    bool foundNextPoint = false;
            //    for (int i = 0; i < 8; ++i) {
            //        OGRPoint nextPoint;
            //        switch ((direction + i) % 8) {
            //        case 0: nextPoint = OGRPoint(currentPoint.getX() + 1, currentPoint.getY()); break; // 右方
            //        case 1: nextPoint = OGRPoint(currentPoint.getX() + 1, currentPoint.getY() + 1); break; // 右下方
            //        case 2: nextPoint = OGRPoint(currentPoint.getX(), currentPoint.getY() + 1); break; // 下方
            //        case 3: nextPoint = OGRPoint(currentPoint.getX() - 1, currentPoint.getY() + 1); break; // 左下方
            //        case 4: nextPoint = OGRPoint(currentPoint.getX() - 1, currentPoint.getY()); break; // 左方
            //        case 5: nextPoint = OGRPoint(currentPoint.getX() - 1, currentPoint.getY() - 1); break; // 左上方
            //        case 6: nextPoint = OGRPoint(currentPoint.getX(), currentPoint.getY() - 1); break; // 上方
            //        case 7: nextPoint = OGRPoint(currentPoint.getX() + 1, currentPoint.getY() - 1); break; // 右上方
            //        }

            //        auto it = std::find(points.begin(), points.end(), nextPoint);
            //        if (it != points.end()) {
            //            sortedPoints.push_back(*it);
            //            currentPoint = *it;
            //            points.erase(it);
            //            direction = (direction + i + 5) % 8; // 更新方向
            //            foundNextPoint = true;
            //            break;
            //        }
            //    }
            //    if (!foundNextPoint) {
            //        break;
            //    }
            //}

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
    //若点a大于点b,即点a在点b顺时针方向,返回true,否则返回false
    bool PointCmp(const OGRPoint& a, const OGRPoint& b, const OGRPoint& center)
    {
        if (a.getX() >= 0 && b.getX() < 0)
            return true;
        if (a.getX() == 0 && b.getX() == 0)
            return a.getY() > b.getY();
        //向量OA和向量OB的叉积
        int det = (a.getX() - center.getX()) * (b.getY() - center.getY()) - (b.getX() - center.getX()) * (a.getY() - center.getY());
        if (det < 0)
            return true;
        if (det > 0)
            return false;
        //向量OA和向量OB共线，以距离判断大小
        int d1 = (a.getX() - center.getX()) * (a.getX() - center.getX()) + (a.getY() - center.getY()) * (a.getY() - center.getY());
        int d2 = (b.getX() - center.getX()) * (b.getX() - center.getY()) + (b.getY() - center.getY()) * (b.getY() - center.getY());
        return d1 > d2;
    }

    std::vector<OGRPoint> ClockwiseSortPoints(std::vector<OGRPoint> vPoints)
    {
        //计算重心
        OGRPoint center;
        double x = 0, y = 0;
        for (int i = 0; i < vPoints.size(); i++)
        {
            x += vPoints[i].getX();
            y += vPoints[i].getY();
        }
        center.setX((int)x / vPoints.size());
        center.setY((int)y / vPoints.size());

        //冒泡排序
        for (int i = 0; i < vPoints.size() - 1; i++)
        {
            for (int j = 0; j < vPoints.size() - i - 1; j++)
            {
                if (PointCmp(vPoints[j], vPoints[j + 1], center))
                {
                    OGRPoint tmp = vPoints[j];
                    vPoints[j] = vPoints[j + 1];
                    vPoints[j + 1] = tmp;
                }
            }
        }
        return vPoints;
    }

private:
    int mLabelCount;
    std::vector<std::vector<int>> mLabels;
    std::map<int, OGRPolygon*> mPolygons;
};