#include "mainwindow.h"

#include "common/log4j.h"

#include <QApplication>
#include <QMessageBox>
#include <QApplication>
#include <QTextCodec>
int main(int argc, char *argv[])
{
    //protobuf启动
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    //使用日志
    Log4j::userLog(Log4j::CONSOLE);

    QApplication app(argc, argv);

    app.setQuitOnLastWindowClosed(false);

    //以下代码为适配windows下输入中文不能获得正确格式
    app.setFont(QFont("Microsoft Yahei", 9));
   #if (QT_VERSION <= QT_VERSION_CHECK(5,0,0))
   #if _MSC_VER
       QTextCodec *codec = QTextCodec::codecForName("GBK");
   #else
       QTextCodec *codec = QTextCodec::codecForName("UTF-8");
   #endif
       QTextCodec::setCodecForLocale(codec);
       QTextCodec::setCodecForCStrings(codec);
       QTextCodec::setCodecForTr(codec);
   #else
       QTextCodec *codec = QTextCodec::codecForName("UTF-8");
       QTextCodec::setCodecForLocale(codec);
   #endif

    MainWindow mw ;
    mw.show();
    int returnCode = app.exec();
    //protobuf关闭
    google::protobuf::ShutdownProtobufLibrary();
    return returnCode;
}

