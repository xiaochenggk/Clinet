#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QPushButton>
#include "apiwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE


class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    //����QString���� ��ȡMD5����
    QString getHashData(QString);
    //���������ַ�����ƴ����������ʽ�����ݲ�����
    QString getSplicingData(QString);
    //ʹ��get������� ���ݵĻ�ȡ
    //�������� ����get��Ϣ����������Ӧ�󷵻أ��õ�������Ϣ���ڼ�һֱ����ֱ����������Ӧ��
    QString getSyncData(const QString &strUrl);

signals:

private slots:
    //��ʹ�ð���send������һϵ�д���
    void on_pushButton_send_clicked();
    //��������comboBox�����ı��ͨ��arg1����ı����ַ���
    void on_comboBox_currentTextChanged(const QString &arg1);
    //����ѡ�����ı��ͨ��checked����仯����ֵ��true or false��
    void on_radioButton_clicked(bool checked);
    //����������еĽ��
    void updateResult();
    //ͨ�������� ��ѡ��ʹ�õķ���ķ�ʽ������/�м�/�߼���
    void on_comboBox_2_activated(int index);

    void on_pushButton_apiSetting_clicked();

private:
    Ui::Widget *ui;
    QString m_sourceType;     // �����Դ����
    QString m_transResultType;// ������������
    bool m_thesisMode;        // �Ƿ�������ģʽ
    QString m_transResult;    // ������
    QNetworkAccessManager* m_naManager;
    int m_transLevel;         // ����ȼ� 0-����  1-�м�  2-�߼�
    QPushButton apiSettingButton;// ���API���ð���
    APIWidget apiWidget;         // ��ȡAPI������

};
//Json��ʽ ��QStringתΪQJsonObject����
QJsonObject QstringToJson(QString jsonString);
//Json��ʽ ��QJsonObjectתΪQString���ͣ�����û���õ�
QString JsonToQstring(QJsonObject jsonObject);
#endif // WIDGET_H
