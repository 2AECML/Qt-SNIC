﻿#include "SegmentationResult.h"

SegmentationResult::SegmentationResult() 
    : mLabelCount(0) {
}

SegmentationResult::SegmentationResult(int* plabels, int* pnumlabels, int width, int height) {
    mLabelCount = *pnumlabels;
    mLabels.resize(height, std::vector<int>(width));
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            mLabels[i][j] = plabels[i * width + j];
        }
    }
    calculateBoundingBoxes();
}

SegmentationResult::~SegmentationResult() {
}

void SegmentationResult::exportToCSV() {
    if (mLabels.empty()) {
        return;
    }

    adjustLabels();

    std::ofstream file("分割结果.csv");

    if (!file.is_open()) {
        std::cerr << "无法打开文件进行写入" << std::endl;
        return;
    }

    std::cout << "开始写入分割结果到CSV文件..." << std::endl;

    for (const auto& row : mLabels) {
        for (size_t i = 0; i < row.size(); ++i) {
            file << row[i];
            if (i != row.size() - 1) {
                file << ",";
            }
        }
        file << "\n";
    }

    std::cout << "写入分割结果到CSV文件完成" << std::endl;

    file.close();
}

int SegmentationResult::getLabelCount() {
    return mLabelCount;
}

const std::vector<std::vector<int>>& SegmentationResult::getLabels() const {
    return mLabels;
}

int SegmentationResult::getLabelByPixel(int x, int y) {
    if (x < 0 || x >= mLabels[0].size() || y < 0 || y >= mLabels.size()) {
        std::cerr << "坐标越界" << std::endl;
        return -1;
    }
    return mLabels[y][x];
}

const std::map<int, cv::Rect>& SegmentationResult::getBoundingBoxes() {
    return mBoundingBoxes;
}

const cv::Rect& SegmentationResult::getBoundingBoxByLabel(int label) {
    if (mBoundingBoxes.find(label) == mBoundingBoxes.end()) {
        std::cerr << "没有找到标签 " << label << " 的外包矩形" << std::endl;
        static cv::Rect emptyRect;
        return emptyRect;
    }
    return mBoundingBoxes[label];
}

int SegmentationResult::mergeLabels(std::vector<int>& labels) {
    int targetLabel = labels[0];

    mLabelCount = mLabelCount - labels.size() + 1;

    int startX = mLabels[0].size();
    int startY = mLabels.size();
    int endX = 0;
    int endY = 0;

    for (int i = 0; i < labels.size(); ++i) {
        int label = labels[i];
        cv::Rect& rect = mBoundingBoxes[label];
        startX = std::min(startX, rect.x);
        startY = std::min(startY, rect.y);
        endX = std::max(endX, rect.x + rect.width);
        endY = std::max(endY, rect.y + rect.height);
    }

    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            if (std::find(labels.begin() + 1, labels.end(), mLabels[y][x]) != labels.end()) {
                //std::cout << "合并标签 " << mLabels[y][x] << " 到 " << targetLabel << std::endl;
                mLabels[y][x] = targetLabel;
            }
        }
    }

    std::cout << "label合并完成" << std::endl;

    for (int i = 0; i < labels.size(); ++i) {
        std::cout << "删除标签 " << labels[i] << " 的外包矩形" << std::endl;
        mBoundingBoxes.erase(labels[i]);
    }

    mBoundingBoxes[targetLabel] = cv::Rect(startX, startY, endX - startX, endY - startY);

    std::cout << "合并后的外包矩形: " << mBoundingBoxes[targetLabel] << std::endl;

    return targetLabel;
}

void SegmentationResult::adjustLabels() {
    std::map<int, int> labelMap;

    int width = mLabels[0].size();
    int height = mLabels.size();

    int newLabel = 0;
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {

            int& curLabel = mLabels[i][j];

            if (labelMap.find(curLabel) == labelMap.end()) {
                labelMap[curLabel] = newLabel;
                ++newLabel;
            }

            if (curLabel != labelMap[curLabel]) {
                curLabel = labelMap[curLabel];
            }

        }
    }
}

void SegmentationResult::calculateBoundingBoxes() {

    std::cout << "计算每个label的外包矩形中..." << std::endl;

    struct Box{
        int minX;
        int minY;
        int maxX;
        int maxY;
    };

    int width = mLabels[0].size();
    int height = mLabels.size();

    std::map<int, Box> boxes;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int label = mLabels[y][x];
            if (mBoundingBoxes.find(label) == mBoundingBoxes.end()) {
                boxes[label].minX = width;
                boxes[label].minY = height;
                boxes[label].maxX = 0;
                boxes[label].maxY = 0;
                mBoundingBoxes[label] = cv::Rect(x, y, 1, 1);
            }

            boxes[label].minX = std::min(boxes[label].minX, x);
            boxes[label].minY = std::min(boxes[label].minY, y);
            boxes[label].maxX = std::max(boxes[label].maxX, x);
            boxes[label].maxY = std::max(boxes[label].maxY, y);
        
            cv::Rect& rect = mBoundingBoxes[label];
            rect.x = boxes[label].minX;
            rect.y = boxes[label].minY;
            rect.width = boxes[label].maxX - boxes[label].minX + 1;
            rect.height = boxes[label].maxY - boxes[label].minY + 1;
        }
    }

    //for (const auto& pair : mBoundingBoxes) {
    //    std::cout << "标签 " << pair.first << " 的外包矩形: " << pair.second << std::endl;
    //}
}