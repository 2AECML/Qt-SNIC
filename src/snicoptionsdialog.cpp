#include "snicoptionsdialog.h"
#include <QDialogButtonBox>

int SNICOptionsDialog::superpixelNumberValue = 500;
double SNICOptionsDialog::compactnessValue = 20.0;

SNICOptionsDialog::SNICOptionsDialog(QWidget* parent) :
    QDialog(parent),
    superpixelNumberSpinBox(new QSpinBox(this)),
    compactnessDoubleSpinBox(new QDoubleSpinBox(this)) {
    
    initUI();
}

int SNICOptionsDialog::getSuperpixelNumber() const {
    return superpixelNumberSpinBox->value();
}

double SNICOptionsDialog::getCompactness() const {
    return compactnessDoubleSpinBox->value();
}

void SNICOptionsDialog::initUI() {
    
    superpixelNumberSpinBox->setRange(1, 10000); 
    superpixelNumberSpinBox->setValue(superpixelNumberValue);

    compactnessDoubleSpinBox->setRange(0.0, 100.0); 
    compactnessDoubleSpinBox->setValue(compactnessValue);

    QFormLayout* formLayout = new QFormLayout;
    formLayout->addRow("Superpixel Number:", superpixelNumberSpinBox);
    formLayout->addRow("Compactness:", compactnessDoubleSpinBox);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SNICOptionsDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SNICOptionsDialog::reject);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
}

void SNICOptionsDialog::onAccept() {
    superpixelNumberValue = superpixelNumberSpinBox->value();
    compactnessValue = compactnessDoubleSpinBox->value();
    accept();
}
