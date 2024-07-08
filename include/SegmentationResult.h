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
    SegmentationResult();
    SegmentationResult(int* plabels, int* pnumlabels, int width, int height);
    ~SegmentationResult();

    void exportToCSV();
    int getLabelCount();
    const std::vector<std::vector<int>>& getLabels() const;
    int getLabelByPixel(int x, int y);
    const std::map<int, cv::Rect>& getBoundingBoxes();
    const cv::Rect& getBoundingBoxByLabel(int label);
    void mergeLabels(std::vector<int>& labels);

private:
    void calculateBoundingBoxes();

private:
    int mLabelCount;
    std::vector<std::vector<int>> mLabels;
    std::map<int, cv::Rect> mBoundingBoxes;
};
