#pragma once

#include <vector>
#include <fstream>
#include <iostream>
#include <map>
#include <ogrsf_frmts.h>
#include <algorithm>
#include <opencv2/opencv.hpp>

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

        // 将 mLabels 转换为 OpenCV 的 Mat 对象
        cv::Mat labelsMat(height, width, CV_32SC1);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                labelsMat.at<int>(y, x) = mLabels[y][x];
            }
        }

        // 将0值替换为一个非零的背景值
        cv::Mat nonZeroLabelsMat = labelsMat.clone();
        nonZeroLabelsMat.setTo(mLabelCount + 1, labelsMat == 0);

        // 找到轮廓
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(nonZeroLabelsMat, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);

        // 将轮廓转换为 OGRPolygon 对象
        for (size_t i = 0; i < contours.size(); ++i) {
            const std::vector<cv::Point>& contour = contours[i];
            int label = nonZeroLabelsMat.at<int>(contour[0].y, contour[0].x);

            // 将非零的背景值转换回0
            if (label == mLabelCount + 1) {
                label = 0;
            }

            if (mPolygons.find(label) == mPolygons.end()) {
                mPolygons[label] = new OGRPolygon();
            }

            OGRLinearRing* ring = new OGRLinearRing();
            for (const auto& point : contour) {
                ring->addPoint(point.x, point.y);
            }
            ring->closeRings();

            mPolygons[label]->addRingDirectly(ring);
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
    

private:
    int mLabelCount;
    std::vector<std::vector<int>> mLabels;
    std::map<int, OGRPolygon*> mPolygons;
};