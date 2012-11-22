#include "tiffandboxgenerator.h"
#include <QtGui/QMessageBox>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QDebug>

TiffAndBoxGenerator::TiffAndBoxGenerator(int width, int height, int dpi, const QString &text, const QStringList &ligatures, const QFont &font)
{
    image = QImage(width, height, QImage::Format_RGB32);
    image.fill(Qt::white);
    if (dpi) {
        image.setDotsPerMeterX(dpi * 39.3701);
        image.setDotsPerMeterY(dpi * 39.3701);
    }
    painter.begin(&image);
    painter.setFont(font);
    lineSpacing = qMax(painter.fontMetrics().lineSpacing(), painter.fontMetrics().boundingRect('0').height()) * 3 / 2;
    textBoundingBox = image.rect().adjusted(lineSpacing, lineSpacing, -lineSpacing, -lineSpacing);
    generateImageAndBoxData(text, ligatures);
}

void TiffAndBoxGenerator::applyBoxesToImage()
{
    painter.setPen(Qt::red);
    foreach (BoxDataItem box, boxData) {
        painter.drawRect(box.boundingBox);
    }
}

void TiffAndBoxGenerator::saveTiffAndBoxFile(const QString &fileName)
{
    qDebug() << fileName;
    image.save(fileName);
    QFile file(QString(fileName).replace(QRegExp("\\.[^\\.]*$"), ".box"));
    file.open(QIODevice::WriteOnly);
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    foreach (BoxDataItem box, boxData) {
        stream << QString("%1 %2 %3 %4 %5 0").arg(box.glyph).arg(box.boundingBox.left()).arg(image.height()-box.boundingBox.top()).arg(box.boundingBox.left() + box.boundingBox.width()).arg(image.height()-(box.boundingBox.top()+box.boundingBox.height())) << endl;
    }
}

QRect TiffAndBoxGenerator::fitToImage(const QRect &rect)
{
    bool initialized = false;
    int left, right, top, bottom;
    for (int ii = rect.left(); ii <= rect.right(); ii++)
        for (int jj = rect.top(); jj <= rect.bottom(); jj++) {
            if (image.pixel(ii, jj) != qRgb(255, 255, 255)) {
                if (!initialized) {
                    left = right = ii;
                    top = bottom = jj;
                    initialized = true;
                } else {
                    if (ii < left)
                        left = ii;
                    if (ii > right)
                        right = ii;
                    if (jj < top)
                        top = jj;
                    if (jj > bottom)
                        bottom = jj;
                }
            }
        }
    if (!initialized)
        return rect;
    else
        return QRect(left, top, right-left, bottom-top);
}

bool TiffAndBoxGenerator::drawGlyph(const QString &text)
{
    if (text != "\n") {
        foreach (QChar ch, text)
            if (!painter.fontMetrics().inFont(ch)) {
                m_error = "Invalid character - character missing from font";
                return false;
            }

        QRect r;
        painter.drawText(QRect(pos.x(), pos.y(), image.width()-pos.x(), image.height()-pos.y()), 0, text, &r);
        pos.rx() += r.width() * 3 / 2;
        r = fitToImage(r);

        if (!r.isValid()) {
            m_error = "Invalid rectangle";
            return false;
        }

        if (text != " ") {
            BoxDataItem item = { text, r };
            boxData.append(item);
        }
    }

    if (text == "\n" || pos.x() >= textBoundingBox.right()) {
        pos.rx() = textBoundingBox.left();
        pos.ry() += lineSpacing;
    }

    if (pos.y() >= textBoundingBox.bottom() - lineSpacing) {
        m_error = "Text is too big for the picture";
        return false;
    }

    return true;
}

bool TiffAndBoxGenerator::generateImageAndBoxData(const QString &text, const QStringList &ligatures)
{
    pos = textBoundingBox.topLeft();

    foreach (QChar ch, text)
        if (!drawGlyph(ch))
            return false;

    foreach (QString ligature, ligatures)
        if (!drawGlyph(ligature))
            return false;

    return true;
}
