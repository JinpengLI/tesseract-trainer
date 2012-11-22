#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tiffandboxgenerator.h"
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
#include <QtCore/QProcess>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->checkBoxAntiAliasing, SIGNAL(toggled(bool)), this, SLOT(updatePlainTextEditFontSettings()));
    connect(ui->checkBoxBold, SIGNAL(toggled(bool)), this, SLOT(updatePlainTextEditFontSettings()));
    connect(ui->checkBoxItalic, SIGNAL(toggled(bool)), this, SLOT(updatePlainTextEditFontSettings()));
    connect(ui->checkBoxMonospace, SIGNAL(toggled(bool)), this, SLOT(updatePlainTextEditFontSettings()));
    connect(ui->fontComboBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(updatePlainTextEditFontSettings()));
    connect(ui->spinBox, SIGNAL(valueChanged(int)), this, SLOT(updatePlainTextEditFontSettings()));
    ui->lineEditImageWidth->setValidator(new QIntValidator(1, 99999));
    ui->lineEditImageHeight->setValidator(new QIntValidator(1, 99999));
    ui->lineEditImageDPI->setValidator(new QIntValidator(0, 99999));

    QSettings settings;
    ui->plainTextEdit->setPlainText(settings.value("Last Text").toString());
    ui->plainTextEdit_2->setPlainText(settings.value("Last Text 2").toString());
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
    ui->plainTextEditInstallScript->setPlainText(settings.value("Install Script").toString());
    ui->lineEditOutputFile->setText(settings.value("Output File").toString());

    tempDir = QDir::cleanPath(QDir::tempPath() + "/TesseractTrainerInstall");
    if (!QDir().mkpath(tempDir)) {
        QMessageBox::critical(this, "", tr("Failed to create temporary directory"));
        qApp->exit(1);
    }
}

MainWindow::~MainWindow()
{
    QSettings settings;
    settings.setValue("Last Text", ui->plainTextEdit->toPlainText());
    settings.setValue("Last Text 2", ui->plainTextEdit_2->toPlainText());
    settings.setValue("Font Size", ui->spinBox->value());
    settings.setValue("Font", ui->fontComboBox->currentFont().toString());
    settings.setValue("Bold", ui->checkBoxBold->isChecked());
    settings.setValue("Italic", ui->checkBoxItalic->isChecked());
    settings.setValue("Monospace", ui->checkBoxMonospace->isChecked());
    settings.setValue("Antialiasing", ui->checkBoxAntiAliasing->isChecked());
    settings.setValue("Image Width", ui->lineEditImageWidth->text());
    settings.setValue("Image Height", ui->lineEditImageHeight->text());
    settings.setValue("Image DPI", ui->lineEditImageDPI->text());
    settings.setValue("Install Script", ui->plainTextEditInstallScript->toPlainText());
    settings.setValue("Output File", ui->lineEditOutputFile->text());
    delete ui;
}

struct BoxDataItem
{
    QString glyph;
    QRect boundingBox;
};

void MainWindow::on_pushButtonGenerate_clicked()
{
    TiffAndBoxGenerator generator(ui->lineEditImageWidth->text().toInt(), ui->lineEditImageHeight->text().toInt(), ui->lineEditImageDPI->text().toInt(), ui->plainTextEdit->toPlainText(), ui->plainTextEdit_2->toPlainText().split(' ', QString::SkipEmptyParts), ui->plainTextEdit->font());
    if (!generator.error().isEmpty()) {
        QMessageBox::critical(this, "", generator.error());
        return;
    }
    QSettings settings;
    QString fileName = QFileDialog::getSaveFileName(this, "", settings.value("Last Saved File").toString(), "Images (*.tif *.png *.jpg)", 0);
    if (!fileName.isEmpty()) {
        settings.setValue("Last Saved File", fileName);
        generator.saveTiffAndBoxFile(fileName);
    }
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

void MainWindow::on_pushButtonPreview_clicked()
{
    TiffAndBoxGenerator generator(ui->lineEditImageWidth->text().toInt(), ui->lineEditImageHeight->text().toInt(), ui->lineEditImageDPI->text().toInt(), ui->plainTextEdit->toPlainText(), ui->plainTextEdit_2->toPlainText().split(' ', QString::SkipEmptyParts), ui->plainTextEdit->font());
    if (!generator.error().isEmpty()) {
        QMessageBox::critical(this, "", generator.error());
        return;
    }
    generator.applyBoxesToImage();
    QString fileName = QDir::tempPath() + "/output.tif";
    generator.saveTiffAndBoxFile(fileName);
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
}

void MainWindow::on_pushButtonGenerateAndInstall_clicked()
{
    TiffAndBoxGenerator generator(ui->lineEditImageWidth->text().toInt(), ui->lineEditImageHeight->text().toInt(), ui->lineEditImageDPI->text().toInt(), ui->plainTextEdit->toPlainText(), ui->plainTextEdit_2->toPlainText().split(' ', QString::SkipEmptyParts), ui->plainTextEdit->font());
    if (!generator.error().isEmpty()) {
        QMessageBox::critical(this, "", generator.error());
        return;
    }

    QString tiffFileName = tempDir + "/" + ui->lineEditOutputFile->text();
    generator.saveTiffAndBoxFile(tiffFileName);
    QString text = ui->plainTextEditInstallScript->toPlainText();
    if (text.isEmpty())
        return;

#ifdef Q_OS_LINUX
    QString scriptFileName = tempDir + "/a.sh";
#elif defined(Q_OS_WIN)
    QString scriptFileName = tempDir + "/a.bat";
#endif

    {
        QFile file(scriptFileName);
        if (!file.open(QIODevice::WriteOnly))
            return;
        QTextStream stream(&file);
        stream << text;
    }

    QProcess process;
#ifdef Q_OS_WIN
    process.start("cmd", QStringList() << "/c" << batFileName);
#elif defined(Q_OS_LINUX)
    process.start("sh", QStringList() << scriptFileName);
#endif
    process.waitForFinished();
    qDebug() << process.readAll();
}
