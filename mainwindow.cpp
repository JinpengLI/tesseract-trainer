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
    QSettings settings;
    ui->plainTextEdit->setPlainText(settings.value("Last Text").toString());
    ui->checkBox->setChecked(settings.value("Last Checked", true).toBool());
    ui->spinBox->setValue(settings.value("Font Size", 8).toInt());
    ui->checkBoxBold->setChecked(settings.value("Bold", false).toBool());
    ui->checkBoxItalic->setChecked(settings.value("Italic", false).toBool());
    ui->checkBoxMonospace->setChecked(settings.value("Monospace", false).toBool());
    ui->checkBoxAntiAliasing->setChecked(settings.value("Antialiasing", true).toBool());
    ui->lineEditImageWidth->setText(settings.value("Image Width", 612).toString());
    ui->lineEditImageHeight->setText(settings.value("Image Height", 792).toString());
    QFont font;
    if (font.fromString(settings.value("Font").toString()))
        ui->fontComboBox->setCurrentFont(font);
}

MainWindow::~MainWindow()
{
    QSettings settings;
    settings.setValue("Last Text", ui->plainTextEdit->toPlainText());
    settings.setValue("Last Checked", ui->checkBox->isChecked());
    settings.setValue("Font Size", ui->spinBox->value());
    settings.setValue("Font", ui->fontComboBox->currentFont().toString());
    settings.setValue("Bold", ui->checkBoxBold->isChecked());
    settings.setValue("Italic", ui->checkBoxItalic->isChecked());
    settings.setValue("Monospace", ui->checkBoxMonospace->isChecked());
    settings.setValue("Antialiasing", ui->checkBoxAntiAliasing->isChecked());
    settings.setValue("Image Width", ui->lineEditImageWidth->text());
    settings.setValue("Image Height", ui->lineEditImageHeight->text());
    delete ui;
}

void MainWindow::on_fontComboBox_currentFontChanged(const QFont &f)
{
    QFont font = f;
    font.setPointSize(ui->spinBox->value());
    font.setBold(ui->checkBoxBold->isChecked());
    font.setItalic(ui->checkBoxItalic->isChecked());
    font.setFixedPitch(ui->checkBoxMonospace->isChecked());
    if (!ui->checkBoxAntiAliasing->isChecked())
        font.setStyleStrategy(QFont::NoAntialias);
    ui->plainTextEdit->setFont(font);
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
    QFont font = ui->plainTextEdit->font();
//    font.setPixelSize(font.pointSize() * 300 / 72);
    if (!generateImageAndBoxData(ui->plainTextEdit->toPlainText(), font, image, boxData))
        return;

    if (ui->checkBox->isChecked()) {
        QPainter painter(&image);
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

bool MainWindow::generateImageAndBoxData(const QString &text, const QFont &font, QImage &image, QList<BoxDataItem> &boxData)
{
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setFont(font);

    QFontMetrics fm(font);

    QRect textBoundingBox = image.rect().adjusted(fm.lineSpacing()*2, fm.lineSpacing()*2, -fm.lineSpacing()*2, -fm.lineSpacing()*2);

    int x = textBoundingBox.left(), y = textBoundingBox.top();

    foreach (QChar ch, text) {
        if (ch != '\n') {
            QRect r;
            painter.drawText(QRect(x, y, image.width()-x, image.height()-y), 0, QString(ch), &r);
            x += r.width() * 3 / 2;

            if (!r.isValid()) {
                QMessageBox::critical(this, "", tr("Invalid rectangle"));
                return false;
            }

            if (!fm.inFont(ch)) {
                QMessageBox::critical(this, "", tr("Invalid character - character missing from font"));
                return false;
            }

            if (ch != ' ') {
                BoxDataItem item = { ch, r };
                boxData.append(item);
            }
        }

        if (ch == '\n' || x >= textBoundingBox.right()) {
            x = textBoundingBox.left();
            y += fm.lineSpacing() * 3 / 2;
        }

        if (y >= textBoundingBox.bottom() - fm.lineSpacing()) {
            QMessageBox::critical(this, "", tr("Text is too big for the picture"));
            return false;
        }
    }

    for (int ii = 10; ii < 20; ii++) {
        QRect r;
        painter.drawText(QRect(x, y, image.width()-x, image.height()-y), 0, QString("%1").arg(ii), &r);

        x += r.width() * 3 / 2;
        if (x >= textBoundingBox.right()) {
            x = textBoundingBox.left();
            y += fm.lineSpacing() * 3 / 2;

            if (y >= textBoundingBox.bottom() - fm.lineSpacing()) {
                QMessageBox::critical(this, "", tr("Text is too big for the picture"));
                return false;
            }
        }

        BoxDataItem item = { QString("%1").arg(ii), r };
        boxData.append(item);
    }

    return true;
}

void MainWindow::on_spinBox_valueChanged(int /*arg1*/)
{
    on_fontComboBox_currentFontChanged(ui->fontComboBox->currentFont());
}

void MainWindow::on_checkBoxBold_toggled(bool /*checked*/)
{
    on_fontComboBox_currentFontChanged(ui->fontComboBox->currentFont());
}

void MainWindow::on_checkBoxItalic_toggled(bool /*checked*/)
{
    on_fontComboBox_currentFontChanged(ui->fontComboBox->currentFont());
}

void MainWindow::on_checkBoxMonospace_toggled(bool /*checked*/)
{
    on_fontComboBox_currentFontChanged(ui->fontComboBox->currentFont());
}

void MainWindow::on_checkBoxAntiAliasing_toggled(bool /*checked*/)
{
    on_fontComboBox_currentFontChanged(ui->fontComboBox->currentFont());
}
