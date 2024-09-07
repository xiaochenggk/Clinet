#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "activewindow.h"
#include <QSettings>
#include <QThread>
#include <QCommonStyle>
#include <QMessageBox>
#include <QUuid>
#include <QApplication>
#include <QListWidget>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QtDebug>//翻译
#include <QTime>
#include <QCryptographicHash>//计算hash值的类
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QList>
#include <cstdlib>
#include <ctime>

//3个List分别 与 初级、中级、高级 三种转换方式所对应
static QStringList LevelParam[3]={{"zh","en","de","zh"},
                                  {"zh","en","de","jp","spa","zh"},
                                  {"zh","en","de","jp","spa","it","pl","bul","zh"}};//翻译

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_socketHandler(Q_NULLPTR),
    m_changeDialog(new DlgChangePasswd(this)),
    m_trayMenu(new QMenu),
    m_trayIcon(new QSystemTrayIcon(this)),
    m_isConnectedToServer(false),
    m_remoteHost(""),
    m_remotePort(0),
    m_ssl(false),
    m_sourceType("auto"),//翻译
    m_transResultType("zh"),
    m_thesisMode(false),
    m_transLevel(0)
{
    ui->setupUi(this);


    stackedWidget = new QStackedWidget;//导航栏
    stackedWidget->setCurrentWidget(ui->page);//导航栏
    connect(ui->pushButton,SIGNAL(clicked(bool)),this,SLOT(On_PushButton1Result()));//导航栏
    connect(ui->pushButton_2,SIGNAL(clicked(bool)),this,SLOT(On_PushButton2Result()));//导航栏
    connect(ui->pushButton_3,SIGNAL(clicked(bool)),this,SLOT(On_PushButton3Result()));//导航栏
    connect(ui->pushButton_4,SIGNAL(clicked(bool)),this,SLOT(On_PushButton4Result()));//导航栏
    connect(ui->pushButton_5,SIGNAL(clicked(bool)),this,SLOT(On_PushButton5Result()));//导航栏
    connect(ui->pushButton_6,SIGNAL(clicked(bool)),this,SLOT(On_PushButton6Result()));//导航栏

    //loadStyleSheet();
    loadUIConnect();
    //加载配置文件
    loadTrayMenu();
    loadSettings();
    //因为m_naManager 为指针类型，所以需要给其分配空间
    //注：不需要在析构函数中delete指针，因为在new时继承了父类，会随父类析构时一起删掉
    m_naManager = new QNetworkAccessManager(this);
    // 当接收到数据完成信号后，更新结果 ，后面没有用到
//    connect(this,SIGNAL(recvDataFinishedSignal()),this,SLOT(updateResult()),Qt::AutoConnection);
    setWindowTitle("翻译小助手");

}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::getHashData(QString input){
    //返回哈希数据，第二个参数是采用何种算法
    QByteArray hashData = QCryptographicHash::hash(input.toLocal8Bit(),QCryptographicHash::Md5);
    //返回字节数组的十六进制编码，编码使用数字0-9和字母a-f
    return QString (hashData.toHex());
}

QString MainWindow::getSplicingData(QString inputStr){
    // 生成一个随机数
    QString strSalt = QString::number(qrand()% 1000);
    //qDebug() << apiWidget.getAppid();
    //qDebug() << apiWidget.getKey();
    //qDebug() << strSalt;
    //http://api.fanyi.baidu.com/api/trans/vip/translate?q=apple&from=en&to=zh&appid=2015063000000001&salt=1435660288&sign=f89f9594663708c1605f3d736d01d2d4
    // 返回拼接后的数据
    return QString("%1?q=%2&from=%3&to=%4&appid=%5&salt=%6&sign=%7")
            .arg(apiWidget.getUrl())
            .arg(inputStr)
            .arg(m_sourceType)
            .arg(m_transResultType)
            .arg(apiWidget.getAppid())
            .arg(strSalt)
            .arg(getHashData(apiWidget.getAppid()+inputStr+strSalt+apiWidget.getKey()));
}

QString MainWindow::getSyncData(const QString &strUrl)
{
    assert(!strUrl.isEmpty());
    //传入一个地址并转换为QUrl格式
    const QUrl url = QUrl::fromUserInput(strUrl);
    assert(url.isValid());

    QNetworkRequest request(url);
    QNetworkReply* reply = m_naManager->get(request); //m_naManager是QNetworkAccessManager对象

    //如下为 同步获取服务器响应 阻塞函数
    QEventLoop eventLoop;
    connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    eventLoop.exec(QEventLoop::ExcludeUserInputEvents);

    // 获取http状态码
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if(statusCode.isValid())
        qDebug() << "status code=" << statusCode.toInt();

    QVariant reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    if(reason.isValid())
        qDebug() << "reason=" << reason.toString();

    QNetworkReply::NetworkError err = reply->error();
    if(err != QNetworkReply::NoError) {
        qDebug() << "Failed: " << reply->errorString();
    }else{
        // 获取返回内容，并全部读取出存到retStr中
        QByteArray replyData = reply->readAll();
        // qDebug() <<replyData;

        //下面为针对百度api返回的json格式进行解析
        //e.g  {"from":"en","to":"zh","trans_result":[{"src":"apple","dst":"苹果"}]}
        // 第一步 将QString类型转为QJsonObject类型
        QJsonObject obj =  QstringToJson(replyData.toStdString().c_str());
        // 第二步 获取到trans_result对应字段后将其按数组取出（实际上只有一个）所以为.at(0)并返回Json对象
        QJsonObject obj2 = obj.value("trans_result").toArray().at(0).toObject();
        //针对错误API进行的处理: 判断返回数据是否异常并将异常输出到输出框
        if(obj2.isEmpty()){
            m_transResult=replyData;
        }else{
        // 第三步 将获取到的Json对象 取出dst 对应字段经转换后存入 m_transResult
            m_transResult = obj2.value("dst").toString();
        }
    }

    reply->deleteLater();
    reply = nullptr;
    return m_transResult;
}

void MainWindow::on_pushButton_send_clicked()
{
    //从文本框获取输入数据
    QString inputStr=ui->textEdit_input->toPlainText().toUtf8();
    //BUG： 解决输入字符跨行后 只翻译第一行的问题
    inputStr.replace(QString("\n"),QString(""));
    qDebug() << inputStr ;
    //判断为翻译模式还是论文模式
    // 翻译模式 为直接翻译成指定的语言，翻译次数一次
    // 论文模式 按照指定的翻译模式翻译，翻译次数为多次
    if(!m_thesisMode){
        qDebug()<<"当前为翻译模式";
        m_transResult = getSyncData(getSplicingData(inputStr));
    }else{
        //去重原理
        //初级 中->英->德->中
        //中级 中->英->德->日->西班牙->中
        //高级 中->英->德->日->西班牙->意大利->波兰->保加利亚->中
        // 中  英 德 日 西班牙 意大利 波兰 保加利亚
        // zh en de jp spa   it    pl  bul
        qDebug()<<"当前为论文模式";
        // 获取指定的翻译模式中的参数
        QStringList currLevelParam = LevelParam[m_transLevel];
        // 遍历指定的翻译模式中的参数并将数据按照指定参数翻译
        // -1的原因是 最后一次肯定是其他语言翻译成中文，后续i + 1 为最后一个参数，所以最后一次翻译为i - 1次才能保证数据访问不会越界
        for(int i = 0;i < currLevelParam.size() -1; i++){
            // 给源语言类型 赋值
            m_sourceType = currLevelParam.at(i);
            // 给目的语言类型 赋值
            m_transResultType = currLevelParam.at(i+1);

            // 判断是否为首次转换，首次转换时数据从文本框输入，后续翻译时，数据为上一次生成数据结果
            if(i == 0){
                m_transResult = getSyncData(getSplicingData(inputStr));
            }else{
                m_transResult = getSyncData(getSplicingData(m_transResult));
            }
            qDebug() << m_sourceType <<"->"<<m_transResultType<<" : "<<m_transResult;
        }
    }
    // 更新结果显示
    updateResult();
}
void MainWindow::updateResult(){
    // 更新指定数据到文本输出框
    ui->textEdit_output->setPlainText(m_transResult);
    qDebug() << m_transResult;
}

void MainWindow::on_comboBox_currentTextChanged(const QString &arg1)
{
    m_transResultType = arg1;
}

void MainWindow::on_radioButton_clicked(bool checked)
{
    m_thesisMode = checked;
}

void MainWindow::on_comboBox_2_activated(int index)
{
    m_transLevel = index;
}

QJsonObject QstringToJson(QString jsonString)
{
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonString.toLocal8Bit().data());
    if(jsonDocument.isNull())
    {
        qDebug()<< "===> please check the string "<< jsonString.toLocal8Bit().data();
    }
    QJsonObject jsonObject = jsonDocument.object();
    return jsonObject;
}

QString JsonToQstring(QJsonObject jsonObject)
{
    return QString(QJsonDocument(jsonObject).toJson());
}

void MainWindow::on_pushButton_apiSetting_clicked()
{
    qDebug()<<"切换到API设置页面";
    apiWidget.show();
}






void MainWindow::On_PushButton1Result()
{
    //按钮1槽函数
    //m_pageOne->show();//show hide 也可实现
    ui->stackedWidget->setCurrentWidget(ui->page);//切换到页面1
    // widget置于上层
//    ui->widget->raise();
}

void MainWindow::On_PushButton2Result()
{
    //按钮2槽函数
    ui->stackedWidget->setCurrentWidget(ui->APIWidget);//切换到页面2
}

void MainWindow::On_PushButton3Result()
{
    //按钮3槽函数
    ui->stackedWidget->setCurrentWidget(ui->page_3);//切换到页面3
}

void MainWindow::On_PushButton4Result()
{
    //按钮3槽函数
    ui->stackedWidget->setCurrentWidget(ui->page_4);//切换到页面3
}

void MainWindow::On_PushButton5Result()
{
    //按钮3槽函数
    ui->stackedWidget->setCurrentWidget(ui->page_5);//切换到页面3
}

void MainWindow::On_PushButton6Result()
{
    //按钮3槽函数
    ui->stackedWidget->setCurrentWidget(ui->page_6);//切换到页面3
}

void MainWindow::loadStyleSheet()
{
    m_styleString.clear();
    QFile file("style.css");
    if(file.open(QIODevice::ReadOnly))
    {
        m_styleString.append(file.readAll());
        file.close();
    }

    if(m_styleString.isEmpty())
        return;

    setStyleSheet(m_styleString);
}


void MainWindow::switchPage(){//侧边栏分页
    QPushButton *button = qobject_cast<QPushButton*>(sender());//得到按下的按钮的指针
    if(button==ui->pushButton)
        ui->stackedWidget->setCurrentIndex(1);//根据按下的button按索引显示相应的页面
    else if(button==ui->pushButton_2)
        ui->stackedWidget->setCurrentIndex(2);
    else if(button==ui->pushButton_3)
        ui->stackedWidget->setCurrentIndex(3);
    else if(button==ui->pushButton_4)
        ui->stackedWidget->setCurrentIndex(4);
    else if(button==ui->pushButton_5)
        ui->stackedWidget->setCurrentIndex(5);
    else if(button==ui->pushButton_6)
        ui->stackedWidget->setCurrentIndex(6);
}

void MainWindow::aboutMe()
{
    QString text = tr("<h3>DxcDesk远程桌面</h3>\n\n"
                       "<p>基于C语言和Windows系统的远程控制系统的设计与实现</p>");
    QString contacts = tr("<p>联系:</p><p>邮箱:  xiaochenggk@163.com</p>"
                       "<p>个人博客: <a href=\"https://%1/\">%1</a></p>"
                       "<p>当前版本: <a href=\"http://%2/\">3.6 beta</a></p>").
            arg(QStringLiteral("https://space.bilibili.com/12924569/"),
                QStringLiteral("https://space.bilibili.com/12924569/"));

    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setWindowTitle(tr("关于QtyDesk远程桌面"));
    msgBox->setText(text);
    msgBox->setInformativeText(contacts);

    msgBox->setIconPixmap(QPixmap(":/img/images/logo.ico"));
    msgBox->exec();
    delete msgBox;
}
void MainWindow::loadUIConnect(){

    connect(ui->bt_settings,&BtnSettings::aboutQtyDesk,this,&MainWindow::aboutMe) ;

    connect(ui->bt_eye,&BtnShowPasswd::showPasswd,this,&MainWindow::showPasswd) ;

    connect(ui->bt_changePasswd,&BtnPassSetting::reflashPasswd,this,&MainWindow::getRandomString) ;
    connect(ui->bt_changePasswd,&BtnPassSetting::setNewPasswd,this,&MainWindow::showChangePassDialog) ;
    //     connect(ui->bt_changePasswd,&BtnPassSetting::copyPasswd,this,&MainWindow::copyPasswd) ;

    //对话框信号 到 主界面处理
    connect(m_changeDialog,&DlgChangePasswd::setPasswdOk,this,&MainWindow::setTempPassword) ;
}
void MainWindow::loadTrayMenu()
{
    QCommonStyle style;
    m_trayMenu->addAction(QIcon(style.standardPixmap(QStyle::SP_ComputerIcon)),"打开主界面");
    //m_trayMenu->addAction(QIcon(style.standardPixmap(QStyle::SP_MessageBoxInformation)),"基本设置");
    m_trayMenu->addAction(QIcon(style.standardPixmap(QStyle::SP_DialogCancelButton)),"退出");
    //托盘加入菜单
    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->setIcon(QIcon(":/img/images/favicon.ico"));
    m_trayIcon->setToolTip("DxcDesk");

    //菜单子项触发
    connect(m_trayMenu,SIGNAL(triggered(QAction*)),this,SLOT(actionTriggered(QAction*)));
    //图标触发
    connect(m_trayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
    //让托盘图标显示在系统托盘上
    m_trayIcon->show();
}
//点击图标触发
void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason ireason)
{
    switch (ireason)
    {
    case QSystemTrayIcon::Trigger:
        //单击
        //this->showNormal();
        break;
    case QSystemTrayIcon::DoubleClick:
        this->showNormal();
        break;
    case QSystemTrayIcon::MiddleClick:
        break;
    default:
        break;
    }
}
//菜单子项触发
void MainWindow::actionTriggered(QAction *action)
{
    if(action->text() == "打开主界面")
    {
        this->showNormal();
    }
    else if(action->text() == "退出"){
        emit closeSignal();
    }
}
//加载配置文件
void MainWindow::loadSettings()
{
    QSettings settings("config.ini",QSettings::IniFormat);
    settings.beginGroup("REMOTE_DESKTOP_SERVER");

    QString remoteHost = settings.value("remoteHost").toString();
    if(remoteHost.isEmpty())
    {
        remoteHost = "121.4.64.98";
        settings.setValue("remoteHost",remoteHost);
    }
    m_remoteHost = remoteHost ;

    int remotePort = settings.value("remotePort",0).toInt();
    if(remotePort == 0)
    {
        remotePort = 8080;
        settings.setValue("remotePort",remotePort);
    }
    m_remotePort = remotePort ;

    QString tempPass = settings.value("tempPass").toString();

    if(tempPass.isEmpty())
    {
        //得到随机数
        tempPass = getRandomString();
        settings.setValue("tempPass",tempPass);
    }

    int ssl = settings.value("ssl",0).toInt();

    if(1 == ssl){
        m_ssl = true ;
        settings.setValue("ssl",ssl);
        if (!QSslSocket::supportsSsl()) {
            QMessageBox::information(0, "Secure Socket Client",
                                     "This system does not support OpenSSL."
                                     " The program will proceed with an insecure connection");
        }
    }else{
        m_ssl = false;
//        QMessageBox::information(0, "Secure Socket Client",
//                                 "The program will proceed with an insecure connection.");
    }
    settings.endGroup();
    settings.sync();

    //被控连接
    startPassiveHandler(remoteHost, remotePort,ssl,tempPass);
}

void MainWindow::startPassiveHandler(const QString &remoteHost, quint16 port,bool ssl,const QString &tempPass)
{
    QThread *thread = new QThread;
    m_socketHandler = new PassiveHandler;
    m_socketHandler->setSSL(ssl);

    m_socketHandler->setType(SocketHandler::SESSION1);
    m_socketHandler->setRemoteHost(remoteHost);
    m_socketHandler->setRemotePort(port);
    m_socketHandler->setTempPass(tempPass);

    //    m_socketHandler->setName(name);
    //    m_socketHandler->setLoginPass(login, pass);
    //    m_socketHandler->setProxyLoginPass(proxyLogin, proxyPass);
    //线程开始,网络处理类创建socket
    connect(thread, &QThread::started, m_socketHandler, &SocketHandler::createSocket);
    connect(this, &MainWindow::closeSignal, m_socketHandler, &SocketHandler::removeSocket);
    connect(m_socketHandler, &SocketHandler::finished, this, &MainWindow::finishedSockeHandler);
    connect(m_socketHandler, &SocketHandler::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, m_socketHandler, &SocketHandler::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    connect(m_socketHandler, &PassiveHandler::showDeviceID, this, &MainWindow::uiShowDeviceID);
    connect(m_socketHandler, &SocketHandler::connectedStatus,this, &MainWindow::showConnectedStatus);

    //处理图片 和 模拟输入
    //createConnectionToHandler(m_socketHandler);

    m_socketHandler->moveToThread(thread);
    thread->start();
}


void MainWindow::showConnectedStatus(bool showStatus){
    m_isConnectedToServer = showStatus;
    QString message("");

    if(m_isConnectedToServer){
        ui->bt_lamp->setStyleSheet("background-color:rgb(50,190,166);border-radius:10px;");
        message.append("就绪");
        //ui->bt_connectRemoteDevice->setEnabled(true);
    }else{
        ui->bt_lamp->setStyleSheet("background-color:rgb(185,54,54);border-radius:10px;");
        message.append("连接失败，启动重连。。。 ");
        //ui->bt_connectRemoteDevice->setEnabled(false);
    }
    ui->lb_connect_state->setText(message);
}
void MainWindow::uiShowDeviceID(QString showID){
    showID = showID.mid(0,3)+" "+ showID.mid(3,3) +" "+ showID.mid(6,3) ;
    ui->lb_showDeviceId->setText(showID);
    //    ui->bt_connectRemoteDevice->setEnabled(true);
    //    ui->lb_showDeviceId->setText(showID);
}

void MainWindow::finishedSockeHandler()
{
    m_socketHandler = Q_NULLPTR;

    //if(!m_webSocketTransfer)
    //这里还不能退出，暂时这样处理。
    QApplication::quit();
}

void MainWindow::showPasswd(bool show)
{
    if(show){
        if(m_socketHandler)
            ui->lb_passwd->setText(m_socketHandler->getTempPass());
    }else{
        ui->lb_passwd->setText("******");
    }
}
QString MainWindow::getRandomString()
{
    QString strUUID = QUuid::createUuid().toString().remove("{").remove("}").remove("-");
    QString randomPass = strUUID.right(6) ;

    setTempPassword(randomPass);

    return randomPass;
}



//开始按钮连接
void MainWindow::on_bt_connectRemoteDevice_clicked()
{
    QString remoteID = ui->ed_remoteID->text().remove(QRegExp("\\s"));
    if(remoteID.isEmpty()){
        ui->ed_remoteID->setFocus();
        return ;
    }
    QString remotePass = ui->ed_remotePass->text();
    //注意内存泄漏，不用时候记得删除
    ActiveWindow *activeWindow = new ActiveWindow;
    activeWindow->setWindowTitle("远程主机ID: "+remoteID);
    activeWindow->show();
    activeWindow->startActiveHandler(m_remoteHost,m_remotePort,remoteID,remotePass,m_ssl);
}

void MainWindow::on_ed_remoteID_textChanged(const QString &arg1)
{
    QString willRemoteId =ui->ed_remoteID->text();
    if(arg1.size() >= m_lastString.size()){
        if( arg1.length()%4 == 3 ){
            willRemoteId = willRemoteId+" ";
            ui->ed_remoteID->setText(willRemoteId);
        }
    }
    m_lastString = willRemoteId ;
}
//--------------------显示改变密码对话框------------------------------//
void MainWindow::showChangePassDialog(){
    m_changeDialog->exec();
}
//--------------------显示改变密码对话框------------------------------//

void MainWindow::setTempPassword(const QString &passwd)
{ 
    if(m_socketHandler){
        m_socketHandler->setTempPass(passwd);
    }
    QSettings settings("config.ini",QSettings::IniFormat);
    settings.beginGroup("REMOTE_DESKTOP_SERVER");

    settings.setValue("tempPass",passwd);

    settings.endGroup();
    settings.sync();
}



