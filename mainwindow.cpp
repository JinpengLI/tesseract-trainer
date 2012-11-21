#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QFileDialog>
#include <QtCore/QSettings>
#include <QtGui/QMessageBox>
#include <QtGui/QDesktopServices>
#include <QtCore/QUrl>
#include <QtGui/QProgressDialog>
#include <QtCore/QDebug>
#include <QtCore/QFile>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->checkBoxAntiAliasing, SIGNAL(toggled(bool)), this, SLOT(updatePlainTextEditFontSettings()));
    connect(ui->checkBoxBold, SIGNAL(toggled(bool)), this, SLOT(updatePlainTextEditFontSettings()));
    connect(ui->checkBoxItalic, SIGNAL(toggled(bool)), this, SLOT(updatePlainTextEditFontSettings()));
    connect(ui->checkBoxMonospace, SIGNAL(toggled(bool)), this, SLOT(updatePlainTextEditFontSettings()));
    connect(ui->fontComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updatePlainTextEditFontSettings()));
    connect(ui->spinBox, SIGNAL(valueChanged(int)), this, SLOT(updatePlainTextEditFontSettings()));
    ui->lineEditImageWidth->setValidator(new QIntValidator(1, 99999));
    ui->lineEditImageHeight->setValidator(new QIntValidator(1, 99999));
    ui->lineEditImageDPI->setValidator(new QIntValidator(0, 99999));

    QSettings settings;
    ui->plainTextEdit->setPlainText(settings.value("Last Text").toString());
    ui->plainTextEdit_2->setPlainText(settings.value("Last Text 2").toString());
    ui->checkBox->setChecked(settings.value("Last Checked", true).toBool());
    ui->spinBox->setValue(settings.value("Font Size", 8).toInt());
    ui->checkBoxBold->setChecked(settings.value("Bold", false).toBool());
    ui->checkBoxItalic->setChecked(settings.value("Italic", false).toBool());
    ui->checkBoxMonospace->setChecked(settings.value("Monospace", false).toBool());
    ui->checkBoxAntiAliasing->setChecked(settings.value("Antialiasing", true).toBool());
    ui->lineEditImageWidth->setText(settings.value("Image Width", 612).toString());
    ui->lineEditImageHeight->setText(settings.value("Image Height", 792).toString());
    ui->lineEditImageDPI->setText(settings.value("Image DPI", 600).toString());
    QFont font;
    if (font.fromString(settings.value("Font").toString()))
        ui->fontComboBox->setCurrentFont(font);
}

MainWindow::~MainWindow()
{
    QSettings settings;
    settings.setValue("Last Text", ui->plainTextEdit->toPlainText());
    settings.setValue("Last Text 2", ui->plainTextEdit_2->toPlainText());
    settings.setValue("Last Checked", ui->checkBox->isChecked());
    settings.setValue("Font Size", ui->spinBox->value());
    settings.setValue("Font", ui->fontComboBox->currentFont().toString());
    settings.setValue("Bold", ui->checkBoxBold->isChecked());
    settings.setValue("Italic", ui->checkBoxItalic->isChecked());
    settings.setValue("Monospace", ui->checkBoxMonospace->isChecked());
    settings.setValue("Antialiasing", ui->checkBoxAntiAliasing->isChecked());
    settings.setValue("Image Width", ui->lineEditImageWidth->text());
    settings.setValue("Image Height", ui->lineEditImageHeight->text());
    settings.setValue("Image DPI", ui->lineEditImageDPI->text());
    delete ui;
}

struct BoxDataItem
{
    QString glyph;
    QRect boundingBox;
};

void MainWindow::on_pushButton_clicked()
{
    QList<BoxDataItem> boxData;
    QImage image(ui->lineEditImageWidth->text().toInt(), ui->lineEditImageHeight->text().toInt(), QImage::Format_RGB32);
    image.fill(Qt::white);
    if (int dpi = ui->lineEditImageDPI->text().toInt()) {
        image.setDotsPerMeterX(dpi * 39.3701);
        image.setDotsPerMeterY(dpi * 39.3701);
    }
    QPainter painter(&image);
    painter.setFont(ui->plainTextEdit->font());
    int lineSpacing = qMax(painter.fontMetrics().lineSpacing(), painter.fontMetrics().boundingRect('0').height());
    QRect textBoundingBox = image.rect().adjusted(lineSpacing*2, lineSpacing*2, -lineSpacing*2, -lineSpacing*2);
    if (!generateImageAndBoxData(painter, ui->plainTextEdit->toPlainText(), ui->plainTextEdit_2->toPlainText().split(' ', QString::SkipEmptyParts), textBoundingBox, boxData))
        return;
    for (QList<BoxDataItem>::iterator box = boxData.begin(); box != boxData.end(); box++) {
        box->boundingBox = fitToImage(image, box->boundingBox);
    }
    if (ui->checkBox->isChecked()) {
        painter.setPen(Qt::red);
        foreach (BoxDataItem box, boxData)
            painter.drawRect(box.boundingBox);
        QString fileName = QDir::tempPath() + "/output.tif";
        image.save(fileName);
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
    } else {
        QSettings settings;
        QString fileName = QFileDialog::getSaveFileName(this, "", settings.value("Last Saved File").toString(), "Images (*.tif *.png *.jpg)", 0);
        if (!fileName.isEmpty()) {
            settings.setValue("Last Saved File", fileName);
            image.save(fileName);
            QFile file(fileName.replace(QRegExp("\\.[^\\.]*$"), ".box"));
            file.open(QIODevice::WriteOnly);
            QTextStream stream(&file);
            stream.setCodec("UTF-8");
            foreach (BoxDataItem box, boxData)
                stream << QString("%1 %2 %3 %4 %5 0").arg(box.glyph).arg(box.boundingBox.left()).arg(image.height()-box.boundingBox.top()).arg(box.boundingBox.right()).arg(image.height()-box.boundingBox.bottom()) << endl;
        }
    }
}

bool MainWindow::generateImageAndBoxData(QPainter &painter, const QString &text, const QStringList &ligatures, const QRect &textBoundingBox, QList<BoxDataItem> &boxData)
{
    int lineSpacing = qMax(painter.fontMetrics().lineSpacing(), painter.fontMetrics().boundingRect('0').height());
    QPoint pos = textBoundingBox.topLeft();

    foreach (QChar ch, text)
        if (!drawGlyph(painter, ch, textBoundingBox, pos, lineSpacing, boxData))
            return false;

    foreach (QString ligature, ligatures)
        if (!drawGlyph(painter, ligature, textBoundingBox, pos, lineSpacing, boxData))
            return false;

    return true;
}

void MainWindow::updatePlainTextEditFontSettings()
{
    QFont font = ui->fontComboBox->currentFont();
    font.setPointSize(ui->spinBox->value());
    font.setBold(ui->checkBoxBold->isChecked());
    font.setItalic(ui->checkBoxItalic->isChecked());
    font.setFixedPitch(ui->checkBoxMonospace->isChecked());
    if (!ui->checkBoxAntiAliasing->isChecked())
        font.setStyleStrategy(QFont::NoAntialias);
    ui->plainTextEdit->setFont(font);
    ui->plainTextEdit_2->setFont(font);
}

QRect MainWindow::fitToImage(const QImage &image, const QRect &r)
{
    bool initialized = false;
    int left, right, top, bottom;
    for (int ii = r.left(); ii <= r.right(); ii++)
        for (int jj = r.top(); jj <= r.bottom(); jj++) {
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
    return QRect(left, top, right-left, bottom-top);
}

bool MainWindow::drawGlyph(QPainter &painter, const QString &text, const QRect &textBoundingBox, QPoint &pos, int lineSpacing, QList<BoxDataItem> &boxData)
{
    if (text != "\n") {
        foreach (QChar ch, text)
            if (!painter.fontMetrics().inFont(ch)) {
                QMessageBox::critical(this, "", tr("Invalid character - character missing from font"));
                return false;
            }

        QRect r;
        painter.drawText(QRect(pos.x(), pos.y(), textBoundingBox.width()-pos.x(), textBoundingBox.height()-pos.y()), 0, text, &r);
        pos.rx() += r.width();

        if (!r.isValid()) {
            QMessageBox::critical(this, "", tr("Invalid rectangle"));
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
        QMessageBox::critical(this, "", tr("Text is too big for the picture"));
        return false;
    }

    return true;
}
