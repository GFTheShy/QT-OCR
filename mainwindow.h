#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QProcess>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QElapsedTimer>
#include <QDebug>
#include <QCoreApplication>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSelectImage();    // 选择图片
    void onRunOcr();         // 发送识别请求
    void onOcrReadyRead();   // 管道读取 OCR 返回数据

private:
    void setupUi();
    void initOcrEngine();    // 初始化常驻后台的引擎
    void parseOcrJson(const QString &jsonStr); // 解析结果

    // UI 控件
    QLabel *m_imgLabel;
    QLineEdit *m_pathEdit;
    QPushButton *m_selectBtn;
    QPushButton *m_ocrBtn;
    QTextEdit *m_resultEdit;
    QLineEdit *m_memberIdEdit;  // 会员ID
    QLineEdit *m_trackingEdit;  // 运单号
    QLabel *m_timeLabel;        // 耗时显示

    // 后台常驻进程与耗时计时器
    QProcess *m_ocrProcess;
    QElapsedTimer m_timer;
    QString m_currentImgPath;
};

#endif // MAINWINDOW_H
