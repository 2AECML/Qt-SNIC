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

        // ʹ�øĽ��ı߽�����㷨�Ե��������
        for (auto& pair : pointMap) {
            std::vector<OGRPoint>& points = pair.second;
            if (points.empty()) continue;

            std::vector<OGRPoint> sortedPoints;

            sortedPoints = ClockwiseSortPoints(points);
            //OGRPoint startPoint = points[0];
            //sortedPoints.push_back(startPoint);
            //points.erase(std::remove(points.begin(), points.end(), startPoint), points.end());

            //OGRPoint currentPoint = startPoint;
            //int direction = 3; // ��ʼ����Ϊ���Ͻ�

            //while (!points.empty()) {
            //    bool foundNextPoint = false;
            //    for (int i = 0; i < 8; ++i) {
            //        OGRPoint nextPoint;
            //        switch ((direction + i) % 8) {
            //        case 0: nextPoint = OGRPoint(currentPoint.getX() + 1, currentPoint.getY()); break; // �ҷ�
            //        case 1: nextPoint = OGRPoint(currentPoint.getX() + 1, currentPoint.getY() + 1); break; // ���·�
            //        case 2: nextPoint = OGRPoint(currentPoint.getX(), currentPoint.getY() + 1); break; // �·�
            //        case 3: nextPoint = OGRPoint(currentPoint.getX() - 1, currentPoint.getY() + 1); break; // ���·�
            //        case 4: nextPoint = OGRPoint(currentPoint.getX() - 1, currentPoint.getY()); break; // ��
            //        case 5: nextPoint = OGRPoint(currentPoint.getX() - 1, currentPoint.getY() - 1); break; // ���Ϸ�
            //        case 6: nextPoint = OGRPoint(currentPoint.getX(), currentPoint.getY() - 1); break; // �Ϸ�
            //        case 7: nextPoint = OGRPoint(currentPoint.getX() + 1, currentPoint.getY() - 1); break; // ���Ϸ�
            //        }

            //        auto it = std::find(points.begin(), points.end(), nextPoint);
            //        if (it != points.end()) {
            //            sortedPoints.push_back(*it);
            //            currentPoint = *it;
            //            points.erase(it);
            //            direction = (direction + i + 5) % 8; // ���·���
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
    //����a���ڵ�b,����a�ڵ�b˳ʱ�뷽��,����true,���򷵻�false
    bool PointCmp(const OGRPoint& a, const OGRPoint& b, const OGRPoint& center)
    {
        if (a.getX() >= 0 && b.getX() < 0)
            return true;
        if (a.getX() == 0 && b.getX() == 0)
            return a.getY() > b.getY();
        //����OA������OB�Ĳ��
        int det = (a.getX() - center.getX()) * (b.getY() - center.getY()) - (b.getX() - center.getX()) * (a.getY() - center.getY());
        if (det < 0)
            return true;
        if (det > 0)
            return false;
        //����OA������OB���ߣ��Ծ����жϴ�С
        int d1 = (a.getX() - center.getX()) * (a.getX() - center.getX()) + (a.getY() - center.getY()) * (a.getY() - center.getY());
        int d2 = (b.getX() - center.getX()) * (b.getX() - center.getY()) + (b.getY() - center.getY()) * (b.getY() - center.getY());
        return d1 > d2;
    }

    std::vector<OGRPoint> ClockwiseSortPoints(std::vector<OGRPoint> vPoints)
    {
        //��������
        OGRPoint center;
        double x = 0, y = 0;
        for (int i = 0; i < vPoints.size(); i++)
        {
            x += vPoints[i].getX();
            y += vPoints[i].getY();
        }
        center.setX((int)x / vPoints.size());
        center.setY((int)y / vPoints.size());

        //ð������
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