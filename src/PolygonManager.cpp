
#include "SegmentationResult.cpp"
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <QPolygon>
#include <set>

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

    void getPolygons() {
        
    }

    void printLayerContents() {

        // ��ȡͼ�㶨��
        OGRFeatureDefn* layerDefn = mLayer->GetLayerDefn();

        // ����ͼ���е�����Ҫ��
        OGRFeature* feature;
        mLayer->ResetReading();
        while ((feature = mLayer->GetNextFeature()) != nullptr) {
            // ��ȡ��������
            OGRGeometry* geometry = feature->GetGeometryRef();
            if (geometry != nullptr && wkbFlatten(geometry->getGeometryType()) == wkbPolygon) {
                OGRPolygon* polygon = dynamic_cast<OGRPolygon*>(geometry);
                // ��ӡ����εĶ�������
                OGRLinearRing* ring = polygon->getExteriorRing();
                for (int i = 0; i < ring->getNumPoints(); ++i) {
                    OGRPoint point;
                    ring->getPoint(i, &point);
                    std::cout << "Point " << i << ": (" << point.getX() << ", " << point.getY() << ")" << std::endl;
                }
            }

            // �ͷ�Ҫ�ض���
            OGRFeature::DestroyFeature(feature);
        }
    }

private:
    void createPolygons() {

        if (mDataset == nullptr) {
            std::cerr << "�����ڴ�����Դʧ�ܣ�" << std::endl;
            return;
        }

        // ����ͼ��
        mLayer = mDataset->CreateLayer("labels", nullptr, wkbPolygon, nullptr);
        if (mLayer == nullptr) {
            std::cerr << "����ͼ��ʧ�ܣ�" << std::endl;
            GDALClose(mDataset);
            return;
        }

        std::cout << "ͼ�㴴���ɹ���mLayer ��ַ: " << mLayer << std::endl;

        // ����ֶ�
        OGRFieldDefn fieldDefn("Label", OFTInteger);
        if (mLayer->CreateField(&fieldDefn) != OGRERR_NONE) {
            std::cerr << "�����ֶ�ʧ�ܣ�" << std::endl;
            GDALClose(mDataset);
            return;
        }

        std::vector<std::vector<int>> labels = mSegResult.getLabels();

        // ����mLabels����Ҫ��
        int rows = labels.size();
        int cols = labels[0].size();
        std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));

        std::cout << "����labels��..." << std::endl;

        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                if (!visited[i][j]) {
                    int label = labels[i][j];
                    std::vector<std::pair<int, int>> region;
                    std::set<std::pair<int, int>> boundary;
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
                            if (nx >= 0 && nx < rows && ny >= 0 && ny < cols) {
                                if (!visited[nx][ny]) {
                                    if (labels[nx][ny] == label) {
                                        visited[nx][ny] = true;
                                        queue.push_back(std::make_pair(nx, ny));
                                    }
                                    else {
                                        boundary.insert(std::make_pair(x, y));
                                    }
                                }
                            }
                            else {
                                boundary.insert(std::make_pair(x, y));
                            }
                            
                        }
                    }

                    OGRFeature* feature = OGRFeature::CreateFeature(mLayer->GetLayerDefn());
                    feature->SetField("label", label);

                    OGRPolygon polygon;
                    OGRLinearRing ring;

                    // ��ȡ�߽綥��
                    for (const auto& [x, y] : boundary) {
                        ring.addPoint(y, x);
                    }
                    ring.closeRings();
                    polygon.addRing(&ring);

                    // ��ӡ ring �������Ϣ
                    int numPoints = ring.getNumPoints();
                    std::cout << feature->GetFID() << " Ring �Ķ�����: " << numPoints << std::endl;

                    for (int i = 0; i < numPoints; ++i) {
                        OGRPoint point;
                        ring.getPoint(i, &point);
                        std::cout << "���� " << i << ": (" << point.getX() << ", " << point.getY() << ")" << std::endl;
                    }

                    feature->SetGeometry(&polygon);
                    if (mLayer->CreateFeature(feature) != OGRERR_NONE) {
                        std::cerr << "����Ҫ��ʧ�ܣ�" << std::endl;
                    }
                    OGRFeature::DestroyFeature(feature);
                }
            }
        }
        std::cout << "����δ������" << std::endl;

        std::cout << "ͼ������: " << mLayer->GetName() << std::endl;
        std::cout << "ͼ��Ҫ������: " << mLayer->GetFeatureCount(FALSE) << std::endl;
    }
    


private:
    GDALDriver* mDriver;
    GDALDataset* mDataset;
    SegmentationResult mSegResult;
    OGRLayer* mLayer;
    std::vector<QPolygon> mPolygons;
};