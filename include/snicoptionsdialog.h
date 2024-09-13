#ifndef SNICOptionsDialog_H
#define SNICOptionsDialog_H

#include <QDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>

class SNICOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SNICOptionsDialog(QWidget* parent = nullptr);

    int getSuperpixelNumber() const;
    double getCompactness() const;

private slots:
    void onAccept();

private:
    void initUI();

private:
    QSpinBox* superpixelNumberSpinBox;
    QDoubleSpinBox* compactnessDoubleSpinBox;

    static int superpixelNumberValue;
    static double compactnessValue;
};

#endif // SNICOptionsDialog_H
