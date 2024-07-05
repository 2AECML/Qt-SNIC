
#include "SegmentationResult.cpp"
#include <gdal_priv.h>
#include <ogrsf_frmts.h>

class PolygonManager {
public:
    PolygonManager(const SegmentationResult& result) : mSegResult(result), mLayer(nullptr) {
        createPolygons();
    }
    ~PolygonManager() {}

private:
    void createPolygons() {
        GDALAllRegister();
        OGRRegisterAll();

        GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("Memory");
        
        GDALDataset* dataset = driver->Create("", 0, 0, 0, GDT_Unknown, nullptr);
        if (dataset == nullptr) {
            std::cerr << "�����ڴ�����Դʧ�ܣ�" << std::endl;
            return;
        }

        // ����ͼ��
        mLayer = dataset->CreateLayer("labels", nullptr, wkbPolygon, nullptr);
        if (mLayer == nullptr) {
            std::cerr << "����ͼ��ʧ�ܣ�" << std::endl;
            GDALClose(dataset);
            return;
        }

        // ����ֶ�
        OGRFieldDefn fieldDefn("Label", OFTInteger);
        if (mLayer->CreateField(&fieldDefn) != OGRERR_NONE) {
            std::cerr << "�����ֶ�ʧ�ܣ�" << std::endl;
            GDALClose(dataset);
            return;
        }

        std::vector<std::vector<int>> labels = mSegResult.getLabels();

        // ����mLabels����Ҫ��
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
                        std::cerr << "����Ҫ��ʧ�ܣ�" << std::endl;
                    }
                    OGRFeature::DestroyFeature(feature);
                }
            }
        }
        std::cout << "����δ������" << std::endl;

        GDALClose(dataset);
    }
    

private:
    SegmentationResult mSegResult;
    OGRLayer* mLayer;
};