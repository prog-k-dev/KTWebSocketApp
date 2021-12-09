#ifndef IMAGEWIDGET_H
#define IMAGEWIDGET_H

#include "WebSocketApp.h"
#include <QWidget>

class ImageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ImageWidget(QWidget *parent = nullptr);

    bool SetImage(const std::string& imagePath);
    bool SetImage(const QByteArray& imageData);
    bool SetImage(const QImage& image);

signals:

private:
    QImage _originalImage;

    void paintEvent( QPaintEvent *event ) override;

};

#endif // IMAGEWIDGET_H
