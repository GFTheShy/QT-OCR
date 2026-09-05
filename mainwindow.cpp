#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_ocrProcess(nullptr)
{
    setupUi();
    initOcrEngine(); // 初始化常驻管道进程
}

MainWindow::~MainWindow()
{
    // 退出程序时优雅关闭后台进程
    if (m_ocrProcess && m_ocrProcess->state() == QProcess::Running) {
        m_ocrProcess->kill();
        m_ocrProcess->waitForFinished(1000);
    }
}

void MainWindow::setupUi()
{
    this->setWindowTitle("OCR 识别上位机 ");
    this->resize(900, 600);

    QWidget *centralWidget = new QWidget(this);
    this->setCentralWidget(centralWidget);

    // 顶部文件选择
    QHBoxLayout *topLayout = new QHBoxLayout();
    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setPlaceholderText("请选择需要识别的图像文件...");
    m_pathEdit->setReadOnly(true);

    m_selectBtn = new QPushButton("选择图片", this);
    m_ocrBtn = new QPushButton("开始极速识别", this);
    m_ocrBtn->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");

    topLayout->addWidget(m_pathEdit);
    topLayout->addWidget(m_selectBtn);
    topLayout->addWidget(m_ocrBtn);

    // 中间显示区域
    QHBoxLayout *contentLayout = new QHBoxLayout();

    m_imgLabel = new QLabel("图片预览区", this);
    m_imgLabel->setAlignment(Qt::AlignCenter);
    m_imgLabel->setStyleSheet("border: 2px dashed #CCCCCC; background-color: #F9F9F9;");
    m_imgLabel->setMinimumSize(400, 400);

    QVBoxLayout *rightLayout = new QVBoxLayout();

    m_timeLabel = new QLabel("识别耗时：0 ms", this);
    m_timeLabel->setStyleSheet("color: #007ACC; font-weight: bold; font-size: 14px;");

    QLabel *lblResult = new QLabel("<b>原始识别文本：</b>", this);
    m_resultEdit = new QTextEdit(this);
    m_resultEdit->setReadOnly(true);

    // 会员 ID 提取栏
    QHBoxLayout *field1Layout = new QHBoxLayout();
    QLabel *lblMember = new QLabel("<b>会员 ID：</b>", this);
    lblMember->setStyleSheet("color: #D32F2F; font-size: 13px;");
    m_memberIdEdit = new QLineEdit(this);
    m_memberIdEdit->setReadOnly(true);
    m_memberIdEdit->setStyleSheet("font-weight: bold; color: #D32F2F;");
    field1Layout->addWidget(lblMember);
    field1Layout->addWidget(m_memberIdEdit);

    // 运单号提取栏
    QHBoxLayout *field2Layout = new QHBoxLayout();
    field2Layout->addWidget(new QLabel("提取运单号：", this));
    m_trackingEdit = new QLineEdit(this);
    m_trackingEdit->setReadOnly(true);
    field2Layout->addWidget(m_trackingEdit);

    rightLayout->addWidget(m_timeLabel);
    rightLayout->addWidget(lblResult);
    rightLayout->addWidget(m_resultEdit);
    rightLayout->addLayout(field1Layout);
    rightLayout->addLayout(field2Layout);

    contentLayout->addWidget(m_imgLabel, 1);
    contentLayout->addLayout(rightLayout, 1);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(contentLayout);

    connect(m_selectBtn, &QPushButton::clicked, this, &MainWindow::onSelectImage);
    connect(m_ocrBtn, &QPushButton::clicked, this, &MainWindow::onRunOcr);
}

// 初始化常驻后台的 OCR 进程
void MainWindow::initOcrEngine()
{
    QString exePath = QCoreApplication::applicationDirPath() + "/OCR/PaddleOCR-json.exe";

    if (!QFile::exists(exePath)) {
        m_resultEdit->setText("错误：找不到 PaddleOCR-json.exe 文件！");
        return;
    }

    m_ocrProcess = new QProcess(this);

    // 设置工作目录，确保模型加载正确
    QFileInfo exeInfo(exePath);
    m_ocrProcess->setWorkingDirectory(exeInfo.absolutePath());

    // 绑定管道读取信号
    connect(m_ocrProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::onOcrReadyRead);

    // 启动引擎（不加 --image_path 参数，使其进入常驻管道交互模式）
    // --use_angle_cls=1 开启方向自动纠正
    QStringList args;
    args << "--use_angle_cls=1";

    m_ocrProcess->start(exePath, args);

    if (!m_ocrProcess->waitForStarted(3000)) {
        m_resultEdit->setText("引擎启动失败！");
    } else {
        qDebug() << "PaddleOCR 后台常驻进程启动成功，等待管道指令...";
    }
}

// 选择图片
void MainWindow::onSelectImage()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择测试图片", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (filePath.isEmpty()) return;

    m_currentImgPath = filePath;
    m_pathEdit->setText(filePath);

    QPixmap pixmap(filePath);
    m_imgLabel->setPixmap(pixmap.scaled(m_imgLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

// 通过管道发送识别请求（毫秒级响应）
void MainWindow::onRunOcr()
{
    if (m_currentImgPath.isEmpty()) {
        m_resultEdit->setText("请先选择图片！");
        return;
    }

    if (!m_ocrProcess || m_ocrProcess->state() != QProcess::Running) {
        m_resultEdit->setText("后台 OCR 引擎未正常运行，尝试重新初始化...");
        initOcrEngine();
        return;
    }

    m_ocrBtn->setEnabled(false);
    m_memberIdEdit->clear();
    m_trackingEdit->clear();

    // 格式化图片路径，替换 Windows 的反斜杠为正斜杠，防止 JSON 转义出错
    QString formattedPath = m_currentImgPath;
    formattedPath.replace("\\", "/");

    // 构造发送给 PaddleOCR-json 的管道 JSON 指令
    // 格式：{"image_path": "C:/test.jpg"}\n
    QJsonObject requestObj;
    requestObj["image_path"] = formattedPath;
    QByteArray requestData = QJsonDocument(requestObj).toJson(QJsonDocument::Compact) + "\n";

    // 启动高精度计时器
    m_timer.start();

    // 向子进程管道写入数据
    m_ocrProcess->write(requestData);
}

// 异步接收管道返回的数据
void MainWindow::onOcrReadyRead()
{
    // 读取管道发回的一行完整的 JSON 输出
    while (m_ocrProcess->canReadLine()) {
        QByteArray line = m_ocrProcess->readLine().trimmed();
        QString lineStr = QString::fromUtf8(line);

        // PaddleOCR 启动时会打印一些初始化日志，忽略非 JSON 的数据
        if (lineStr.startsWith("{\"code\":")) {
            qint64 elapsed = m_timer.elapsed(); // 获取本次识别消耗的毫秒数
            m_timeLabel->setText(QString("识别耗时：%1 ms").arg(elapsed));

            parseOcrJson(lineStr);

            m_ocrBtn->setEnabled(true);
            break;
        }
    }
}

// 解析 JSON 与正则匹配会员 ID
void MainWindow::parseOcrJson(const QString &jsonStr)
{
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;

    QJsonObject rootObj = doc.object();
    int code = rootObj["code"].toInt();

    if (code != 100) {
        m_resultEdit->setText(QString("未检测到有效文字 (错误码：%1)").arg(code));
        return;
    }

    QJsonArray dataArray = rootObj["data"].toArray();
    QString fullText = "";

    for (const QJsonValue &val : dataArray) {
        QJsonObject item = val.toObject();
        QString text = item["text"].toString();
        double score = item["score"].toDouble();

        if (score > 0.3) {
            fullText += text + "\n";
        }
    }

    m_resultEdit->setText(fullText);

    // ================= 正则表达式精准提取“会员ID” =================
    // 兼容 “会员ID:XXXX”、“会员ID：XXXX”、“会员 ID : XXXX” 等带空格或中文冒号的情况
    // 捕获组匹配冒号后面的连续字母和数字：[a-zA-Z0-9]+

    // 正则表达式拆解分析：
    // 1. (?:会员|會員|会1员|會1員) : 兼容"会员"二字
    // 2. [\s\n]*                   : 容忍空格和换行
    // 3. (?:ID|1D|lD|!D|I0|10|Id|1d) : 兼容 ID, 1D, lD, !D, I0, 10 等错别字
    // 4. [\s\n]*[:：=]?[s\n]*      : 兼容冒号、等号、空格和换行（跨行关键）
    // 5. ([a-zA-Z0-9_-]+)          : 捕获真正的 ID 账号字符串

    QRegularExpression reMemberId(
        "(?:会员|會員)[\\s\\n]*(?:ID|1D|lD|!D|I0|10|Id|1d)[\\s\\n]*[:：=]?[\\s\\n]*([a-zA-Z0-9_-]+)",
        QRegularExpression::CaseInsensitiveOption
    );

    QRegularExpressionMatch memberMatch = reMemberId.match(fullText);

    if (memberMatch.hasMatch()) {
        m_memberIdEdit->setText(memberMatch.captured(1));
    } else {
        // 备用正则：如果不带冒号，直接是“会员ID”后面紧跟字母数字
        QRegularExpression reBackup("(?:会员|會員)\\s*ID\\s*([a-zA-Z0-9]+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch backupMatch = reBackup.match(fullText);
        if (backupMatch.hasMatch()) {
            m_memberIdEdit->setText(backupMatch.captured(1));
        } else {
            m_memberIdEdit->setText("未匹配到会员ID");
        }
    }

    // 正则提取运单号
    QRegularExpression reTracking("(YT\\d{13}|\\d{12,16})");
    QRegularExpressionMatch trackMatch = reTracking.match(fullText);
    if (trackMatch.hasMatch()) {
        m_trackingEdit->setText(trackMatch.captured(1));
    } else {
        m_trackingEdit->setText("未匹配到");
    }
}
