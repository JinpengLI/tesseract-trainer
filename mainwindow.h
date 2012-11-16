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
    void on_pushButton_clicked();
    void updatePlainTextEditFontSettings();

private:
    bool generateImageAndBoxData(const QString &text, const QFont &font, QImage &image, QList<BoxDataItem> &boxData);
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
