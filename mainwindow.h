#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_pushButtonGenerate_clicked();
    void updatePlainTextEditFontSettings();

    void on_pushButtonPreview_clicked();

    void on_pushButtonGenerateAndInstall_clicked();

private:
    QString tempDir;
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
