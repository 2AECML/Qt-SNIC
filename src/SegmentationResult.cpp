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
        

        // �� mLabels ת��Ϊ OpenCV �� Mat ����
        cv::Mat labelsMat(height, width, CV_32SC1);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                labelsMat.at<int>(y, x) = mLabels[y][x];

                int label = mLabels[y][x];
                if (mBoundingBoxes.find(label) == mBoundingBoxes.end()) {
                    mBoundingBoxes[label] = cv::Rect(x, y, 1, 1);
                }
                else {
                    cv::Rect& rect = mBoundingBoxes[label];
                    rect.x = std::min(rect.x, x);
                    rect.y = std::min(rect.y, y);
                    rect.width = std::max(rect.width, x - rect.x + 1);
                    rect.height = std::max(rect.height, y - rect.y + 1);
                }
            }
        }

        //for (const auto& pair : mBoundingBoxes) {
        //    std::cout << "��ǩ " << pair.first << " ���������: " << pair.second << std::endl;
        //}

        // ��0ֵ�滻Ϊһ������ı���ֵ
        cv::Mat nonZeroLabelsMat = labelsMat.clone();
        nonZeroLabelsMat.setTo(mLabelCount + 1, labelsMat == 0);

        // �ҵ�����
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(nonZeroLabelsMat, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);

        // ������ת��Ϊ OGRPolygon ����
        for (size_t i = 0; i < contours.size(); ++i) {
            const std::vector<cv::Point>& contour = contours[i];
            int label = nonZeroLabelsMat.at<int>(contour[0].y, contour[0].x);

            // ������ı���ֵת����0
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
    if (mPolygons.find(label) == mPolygons.end()) {
            return nullptr;
        }
        return mPolygons[label];
    }

    void mergeLabels(std::vector<int>& labels) {
        int targetLabel = labels[0];

        int startX = mLabels[0].size();
        int startY = mLabels.size();
        int endX = 0;
        int endY = 0;

        // ����ϲ�����������
        for (int i = 0; i < labels.size(); ++i) {
            int label = labels[i];
            cv::Rect& rect = mBoundingBoxes[label];
            startX = std::min(startX, rect.x);
            startY = std::min(startY, rect.y);
            endX = std::max(endX, rect.x + rect.width);
            endY = std::max(endY, rect.y + rect.height);
        }

        std::cout << "�ϲ�����������: " << cv::Rect(startX, startY, endX - startX, endY - startY) << std::endl;

        // �ϲ���ǩ
        for (int y = startY; y < endY; ++y) {
            for (int x = startX; x < endX; ++x) {
                if (std::find(labels.begin() + 1, labels.end(), mLabels[y][x]) != labels.end()) {
                    std::cout << "�ϲ���ǩ " << mLabels[y][x] << " �� " << targetLabel << std::endl;
                    mLabels[y][x] = targetLabel;
                }
            }
        }

        // ɾ���ϲ�ǰ�Ķ�������������
        for (int i = 0; i < labels.size(); ++i) {
            int label = labels[i];
            if (mPolygons.find(label) != mPolygons.end()) {
                delete mPolygons[label];
                mPolygons.erase(label);
            }
            mBoundingBoxes.erase(label);

        }

        // ���ɺϲ���Ķ����
        int width = mLabels[0].size();
        int height = mLabels.size();
        cv::Mat labelsMat(height, width, CV_32SC1);
        for (int y = startY; y < endY; ++y) {
            for (int x = startX; x < endX; ++x) {
                labelsMat.at<int>(y, x) = mLabels[y][x];
            }
        }

        // ��0ֵ�滻Ϊһ������ı���ֵ
        cv::Mat nonZeroLabelsMat = labelsMat.clone();
        nonZeroLabelsMat.setTo(mLabelCount + 1, labelsMat == 0);

        // �ҵ�����
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(nonZeroLabelsMat, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);

        // ������ת��Ϊ OGRPolygon ����
        for (size_t i = 0; i < contours.size(); ++i) {
            const std::vector<cv::Point>& contour = contours[i];
            int label = nonZeroLabelsMat.at<int>(contour[0].y, contour[0].x);

            if (std::find(labels.begin(), labels.end(), label) == labels.end()) {
                continue;
            }

            // ������ı���ֵת����0
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

        std::cout << "���ɺϲ���Ķ�������" << std::endl;

        // �����������
        mBoundingBoxes[targetLabel] = cv::Rect(startX, startY, endX - startX, endY - startY);
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
    

private:
    int mLabelCount;
    std::vector<std::vector<int>> mLabels;
    std::map<int, OGRPolygon*> mPolygons;
    std::map<int, cv::Rect> mBoundingBoxes;
};