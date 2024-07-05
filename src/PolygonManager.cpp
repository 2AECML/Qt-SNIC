
#include "SegmentationResult.cpp"
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <QPolygonF>

class PolygonManager {
public:
    PolygonManager() : mDriver(nullptr), mDataset(nullptr), mLayer(nullptr) {}
    PolygonManager(const SegmentationResult& result) : mSegResult(result), mLayer(nullptr) {
        GDALAllRegister();
        OGRRegisterAll();

        mDriver = GetGDALDriverManager()->GetDriverByName("Memory");

        mDataset = mDriver->Create("", 0, 0, 0, GDT_Unknown, nullptr);

        createPolygons();
    }
    ~PolygonManager() {
        if (mDataset != nullptr) {
            GDALClose(mDataset);
        }
    }

    std::vector<QPolygonF> getPolygonData() {

        std::vector<QPolygonF> polygonData;

        if (mLayer == nullptr) {
            std::cerr << "图层未初始化！" << std::endl;
            return polygonData;
        }

        std::cout << "mLayer 地址: " << mLayer << std::endl;
        std::cout << "图层名称: " << mLayer->GetName() << std::endl;
        std::cout << "图层要素数量: " << mLayer->GetFeatureCount(FALSE) << std::endl;


        // 遍历图层中的所有要素
        OGRFeature* feature;
        mLayer->ResetReading();
        while ((feature = mLayer->GetNextFeature()) != nullptr) {
            // 获取几何属性
            OGRGeometry* geometry = feature->GetGeometryRef();
            if (geometry != nullptr && wkbFlatten(geometry->getGeometryType()) == wkbPolygon) {
                OGRPolygon* polygon = dynamic_cast<OGRPolygon*>(geometry);
                // 将 OGRPolygon 转换为 QPolygonF
                QPolygonF qPolygon;
                OGRLinearRing* ring = polygon->getExteriorRing();
                for (int i = 0; i < ring->getNumPoints(); ++i) {
                    OGRPoint point;
                    ring->getPoint(i, &point);
                    qPolygon << QPointF(point.getX(), point.getY());
                }
                polygonData.push_back(qPolygon);
            }

            // 释放要素对象
            OGRFeature::DestroyFeature(feature);
        }

        return polygonData;
    }

    void printLayerContents() {

        // 获取图层定义
        OGRFeatureDefn* layerDefn = mLayer->GetLayerDefn();

        // 遍历图层中的所有要素
        OGRFeature* feature;
        mLayer->ResetReading();
        while ((feature = mLayer->GetNextFeature()) != nullptr) {
            // 获取几何属性
            OGRGeometry* geometry = feature->GetGeometryRef();
            if (geometry != nullptr && wkbFlatten(geometry->getGeometryType()) == wkbPolygon) {
                OGRPolygon* polygon = dynamic_cast<OGRPolygon*>(geometry);
                // 打印多边形的顶点坐标
                OGRLinearRing* ring = polygon->getExteriorRing();
                for (int i = 0; i < ring->getNumPoints(); ++i) {
                    OGRPoint point;
                    ring->getPoint(i, &point);
                    std::cout << "Point " << i << ": (" << point.getX() << ", " << point.getY() << ")" << std::endl;
                }
            }

            // 释放要素对象
            OGRFeature::DestroyFeature(feature);
        }
    }



private:
    void createPolygons() {

        if (mDataset == nullptr) {
            std::cerr << "创建内存数据源失败！" << std::endl;
            return;
        }

        // 创建图层
        mLayer = mDataset->CreateLayer("labels", nullptr, wkbPolygon, nullptr);
        if (mLayer == nullptr) {
            std::cerr << "创建图层失败！" << std::endl;
            GDALClose(mDataset);
            return;
        }

        std::cout << "图层创建成功，mLayer 地址: " << mLayer << std::endl;

        // 添加字段
        OGRFieldDefn fieldDefn("Label", OFTInteger);
        if (mLayer->CreateField(&fieldDefn) != OGRERR_NONE) {
            std::cerr << "创建字段失败！" << std::endl;
            GDALClose(mDataset);
            return;
        }

        std::vector<std::vector<int>> labels = mSegResult.getLabels();

        // 遍历mLabels生成要素
        int rows = labels.size();
        int cols = labels[0].size();
        std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));

        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                if (!visited[i][j]) {
                    int label = labels[i][j];
                    std::vector<std::pair<int, int>> region;
                    std::vector<std::pair<int, int>> queue;
                    queue.push_back(std::make_pair(i, j));
                    visited[i][j] = true;

                    while (!queue.empty()) {
                        auto [x, y] = queue.back();
                        queue.pop_back();
                        region.push_back(std::make_pair(x, y));

                        static const int dx[] = { -1, 1, 0, 0 };
                        static const int dy[] = { 0, 0, -1, 1 };

                        for (int k = 0; k < 4; ++k) {
                            int nx = x + dx[k];
                            int ny = y + dy[k];
                            if (nx >= 0 && nx < rows && ny >= 0 && ny < cols && 
                                !visited[nx][ny] && labels[nx][ny] == label) {
                                visited[nx][ny] = true;
                                queue.push_back(std::make_pair(nx, ny));
                            }
                        }
                    }

                    OGRFeature* feature = OGRFeature::CreateFeature(mLayer->GetLayerDefn());
                    feature->SetField("label", label);

                    OGRPolygon polygon;
                    OGRLinearRing ring;

                    for (const auto& [x, y] : region) {
                        ring.addPoint(y, x);
                    }
                    ring.closeRings();
                    polygon.addRing(&ring);

                    feature->SetGeometry(&polygon);
                    if (mLayer->CreateFeature(feature) != OGRERR_NONE) {
                        std::cerr << "创建要素失败！" << std::endl;
                    }
                    OGRFeature::DestroyFeature(feature);
                }
            }
        }
        std::cout << "多边形创建完成" << std::endl;

        std::cout << "图层名称: " << mLayer->GetName() << std::endl;
        std::cout << "图层要素数量: " << mLayer->GetFeatureCount(FALSE) << std::endl;
    }
    

private:
    GDALDriver* mDriver;
    GDALDataset* mDataset;
    SegmentationResult mSegResult;
    OGRLayer* mLayer;
};