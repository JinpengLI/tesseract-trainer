#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

struct BoxDataItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_fontComboBox_currentFontChanged(const QFont &f);

    void on_pushButton_clicked();

    void on_spinBox_valueChanged(int arg1);

    void on_checkBoxBold_toggled(bool checked);

    void on_checkBoxItalic_toggled(bool checked);

    void on_checkBoxMonospace_toggled(bool checked);

    void on_checkBoxAntiAliasing_toggled(bool checked);

private:
    bool generateImageAndBoxData(const QString &text, const QFont &font, QImage &image, QList<BoxDataItem> &boxData);
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
