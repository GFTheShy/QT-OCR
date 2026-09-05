QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# ==============================================================================
# 使用 Qt 原生 QMake 内置指令拷贝文件夹（无需依赖 Windows 系统命令，绝对生效）
# ==============================================================================
win32 {
    # 1. 确定源目录和目标目录
    OCR_SRC_DIR = $$PWD/OCR

    !isEmpty(DESTDIR) {
        OCR_DST_DIR = $$DESTDIR/OCR
    } else {
        # 匹配标准 Qt 编译路径 (debug/release)
        CONFIG(release, debug|release): OCR_DST_DIR = $$OUT_PWD/release/OCR
        CONFIG(debug, debug|release):   OCR_DST_DIR = $$OUT_PWD/debug/OCR
    }

    # 2. 规范化斜杠格式
    OCR_SRC_DIR = $$clean_path($$OCR_SRC_DIR)
    OCR_DST_DIR = $$clean_path($$OCR_DST_DIR)

    # 3. 利用 QMake 自带的内部复制工具生成编译规则 (COPY_DIR)
    # 将复制动作挂载到 Extra Targets 中，确保构建时必然触发
    copy_ocr.target = copy_ocr_files
    copy_ocr.commands = $(COPY_DIR) \"$$OCR_SRC_DIR\" \"$$OCR_DST_DIR\"

    # 强制在主工程构建完成后执行
    QMAKE_EXTRA_TARGETS += copy_ocr
    POST_TARGETDEPS += copy_ocr_files
}
