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
    QRect fitToImage(const QImage &image, const QRect &rect);
    bool drawGlyph(QPainter &painter, const QString &text, const QRect &textBoundingBox, QPoint &pos, int lineSpacing, QList<BoxDataItem> &boxData);
    bool generateImageAndBoxData(QPainter &painter, const QString &text, const QStringList &ligatures, const QRect &textBoundingBox, QList<BoxDataItem> &boxData);
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
