#ifndef TIFFANDBOXGENERATOR_H
#define TIFFANDBOXGENERATOR_H

#include <QtCore/QList>
#include <QtGui/QImage>
#include <QtGui/QPainter>

class TiffAndBoxGenerator
{
public:
    TiffAndBoxGenerator(int width, int height, int dpi, const QString &text, const QStringList &ligatures, const QFont &font);
    void applyBoxesToImage();
    void saveTiffAndBoxFile(const QString &fileName);
    const QString &error() { return m_error; }

private:
    struct BoxDataItem
    {
        QString glyph;
        QRect boundingBox;
    };
    QList<BoxDataItem> boxData;
    QImage image;
    QPoint pos;
    QPainter painter;
    QRect textBoundingBox;
    int lineSpacing;
    QString m_error;
    QRect fitToImage(const QRect &rect);
    bool drawGlyph(const QString &text);
    bool generateImageAndBoxData(const QString &text, const QStringList &ligatures);
};

#endif // TIFFANDBOXGENERATOR_H
