#include "ImageWidget.h"
#include <QPainter>


ImageWidget::ImageWidget(QWidget *parent)
    : QWidget(parent)
{
}

bool ImageWidget::SetImage(const std::string& imagePath) {
    if (!_originalImage.load(imagePath.c_str())) {
        OUTPUT_ERROR_LOG("イメージ読み込み失敗", imagePath.c_str());
        return false;
    }

    update();
    return true;
}
bool ImageWidget::SetImage(const QByteArray& imageData) {
    if (!_originalImage.loadFromData(imageData)) {
        OUTPUT_ERROR_LOG("イメージ読み込み失敗");
        return false;
    }

    update();
    return true;
}

bool ImageWidget::SetImage(const QImage& image) {
    _originalImage = image;
    update();
    return true;
}


void ImageWidget::paintEvent(QPaintEvent *)
{
    if (_originalImage.isNull()) {
        return;
    }

    QRect rect = this->rect();
    {
        float ratioX = (float)width() / (float)_originalImage.width();
        float ratioY = (float)height() / (float)_originalImage.height();
        if (ratioX < ratioY) {
            rect.setHeight((float)_originalImage.height() * ratioX);
        } else {
            rect.setWidth((float)_originalImage.width() * ratioY);
        }
    }

    QPainter painter(this);
    painter.drawImage(rect, _originalImage);
}
