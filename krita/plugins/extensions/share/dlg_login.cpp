#include "dlg_login.h"
#include "ui_dlg_login.h"

#include <QDebug>
#include <QStringList>

#include "o2deviantart.h"

DlgLogin::DlgLogin(O2DeviantART *deviant, QWidget *parent)
    : QDialog(parent)
    , m_deviant(deviant)
    , ui(new Ui::DlgLogin)
{
    ui->setupUi(this);
    connect(ui->webView, SIGNAL(urlChanged(QUrl)), this, SLOT(urlChanged(QUrl)));

}

DlgLogin::~DlgLogin()
{
    delete ui;
}

void DlgLogin::urlChanged(const QUrl &url)
{
    qDebug() << "URL =" << url << m_deviant->token() << "," << m_accessToken;
    QString str = url.toString();

    if (str.contains("access_token")) {
        QStringList query = str.split("#");
        QStringList lst = query[1].split("&");
        for (int i=0; i<lst.count(); i++ ) {
            QStringList pair = lst[i].split("=");
            if (pair[0] == "access_token") {
                qDebug() << "Access token" << m_accessToken;
                m_accessToken = pair[1];
                emit accessTokenObtained();
                QDialog::accept();
            }
        }
    }
    else if (str.contains("blank")) {
        QDialog::close();
    }
}

void DlgLogin::setLoginUrl(const QUrl &url)
{
    qDebug() << "setLoginUrl" << url;
    ui->webView->setUrl(url);
}
