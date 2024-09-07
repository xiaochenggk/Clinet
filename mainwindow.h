#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "net/passivehandler.h"
#include "ui/dialog/dlgchangepasswd.h"

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QStackedWidget>
#include <QWidget>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QPushButton>
#include "apiwidget.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    //传入QString数据 获取MD5返回
    QString getHashData(QString);
    //传入输入字符串，拼接随机数后格式化数据并返回
    QString getSplicingData(QString);
    //使用get方法完成 数据的获取
    //具体流程 发送get消息，服务器响应后返回，得到返回消息（期间一直阻塞直到服务器响应）
    QString getSyncData(const QString &strUrl);

private:
    QStackedWidget *stackedWidget;
    Ui::MainWindow *ui;
    QString m_styleString;
    //WebSocketTransfer *m_webSocketTransfer;
    PassiveHandler *m_socketHandler;
    DlgChangePasswd *m_changeDialog;
    QMenu *m_trayMenu;
    QSystemTrayIcon *m_trayIcon;
    bool m_isConnectedToServer;
    QString m_remoteHost ;
    int m_remotePort;
    bool m_ssl ;
    QString m_lastString;
    void switchPage();
    QString m_sourceType;     // 翻译的源类型
    QString m_transResultType;// 翻译结果的类型
    bool m_thesisMode;
    int m_transLevel;         // 翻译等级 0-初级  1-中级  2-高级
    QNetworkAccessManager* m_naManager;
    APIWidget apiWidget;         // 获取API数据类
    QString m_transResult;    // 翻译结果

signals:
    void closeSignal();

public slots:
      void aboutMe();
      void showPasswd(bool show);
      void showChangePassDialog();
      void setTempPassword(const QString &passwd);
private slots:
      void loadStyleSheet();
      QString getRandomString();
      void loadTrayMenu();
      //小图标事件
      void actionTriggered(QAction *action);
      void iconActivated(QSystemTrayIcon::ActivationReason ireason);
      //加载配置文件
      void loadUIConnect();
      void loadSettings();
      void startPassiveHandler(const QString &remoteHost, quint16 remotePort,bool ssl,const QString &tempPass);
//      void createConnectionToHandler( PassiveHandler *m_socketHandler);
      void finishedSockeHandler();
      void uiShowDeviceID(QString showID);
      void showConnectedStatus(bool showStatus);
      void on_bt_connectRemoteDevice_clicked();
      void on_ed_remoteID_textChanged(const QString &arg1);
      void On_PushButton1Result();
      void On_PushButton2Result();
      void On_PushButton3Result();
      void On_PushButton4Result();
      void On_PushButton5Result();
      void On_PushButton6Result();
      //在使用按键send后发生的一系列处理
      void on_pushButton_send_clicked();//翻译
      //当下拉框comboBox发生改变后，通过arg1传入改变后的字符串
      void on_comboBox_currentTextChanged(const QString &arg1);
      //当候选框发生改变后，通过checked传入变化的数值（true or false）
      void on_radioButton_clicked(bool checked);
      //更新输出框中的结果
      void updateResult();
      //通过下拉框 来选择使用的翻译的方式（初级/中级/高级）
      void on_comboBox_2_activated(int index);

      void on_pushButton_apiSetting_clicked();

};
//Json格式 从QString转为QJsonObject类型
QJsonObject QstringToJson(QString jsonString);
#endif // MAINWINDOW_H
