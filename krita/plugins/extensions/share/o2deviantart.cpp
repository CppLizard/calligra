/*
Copyright (c) 2012, Akos Polster
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation i
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" i
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <QDebug>
#include <QDateTime>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QDesktopServices>

#include "o2deviantart.h"

static const char *FbEndpoint = "https://www.deviantart.com/oauth2/draft15/authorize";
static const char *FbTokenUrl = "https://www.deviantart.com/oauth2/draft15/token";
static const quint16 FbLocalPort = 1965;

O2DeviantART::O2DeviantART(QObject *parent): O2(parent) {
    setRequestUrl(FbEndpoint);
    setTokenUrl(FbTokenUrl);
    setLocalPort(FbLocalPort);
}

void O2DeviantART::onVerificationReceived(const QMap<QString, QString> response) {
    emit closeBrowser();
    if (response.contains("error")) {
        qWarning() << "O2DeviantART::onVerificationReceived: Verification failed";
        foreach (QString key, response.keys()) {
            qWarning() << "O2DeviantART::onVerificationReceived:" << key << response.value(key);
        }
        emit linkingFailed();
        return;
    }

    // Save access code
    setCode(response.value(QString("code")));

    // Exchange access code for access/refresh tokens
    QUrl url(tokenUrl_);
    url.addQueryItem("client_id", clientId_);
    url.addQueryItem("client_secret", clientSecret_);
    url.addQueryItem("scope", scope_);
    url.addQueryItem("code", code());
    url.addQueryItem("redirect_uri", redirectUri_);

    QNetworkRequest tokenRequest(url);
    QNetworkReply *tokenReply = manager_->get(tokenRequest);
    timedReplies_.add(tokenReply);
    connect(tokenReply, SIGNAL(finished()), this, SLOT(onTokenReplyFinished()), Qt::QueuedConnection);
    connect(tokenReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onTokenReplyError(QNetworkReply::NetworkError)), Qt::QueuedConnection);
}

void O2DeviantART::onTokenReplyFinished() {
    QNetworkReply *tokenReply = qobject_cast<QNetworkReply *>(sender());
    if (tokenReply->error() == QNetworkReply::NoError) {

        // Process reply
        QByteArray replyData = tokenReply->readAll();
        QMap<QString, QString> reply;
        foreach (QString pair, QString(replyData).split("&")) {
            QStringList kv = pair.split("=");
            if (kv.length() == 2) {
                reply.insert(kv[0], kv[1]);
            }
        }

        // Interpret reply
        setToken(reply.contains("access_token")? reply.value("access_token"): "");
        // FIXME: I have no idea how to interpret DeviantART's "expires" value. So let's use a default of 2 hours
        setExpires(QDateTime::currentMSecsSinceEpoch() / 1000 + 2 * 60 * 60);
        setRefreshToken(reply.contains("refresh_token")? reply.value("refresh_token"): "");

        timedReplies_.remove(tokenReply);
        emit linkedChanged();
        emit tokenChanged();
        emit linkingSucceeded();
    }
}

void O2DeviantART::unlink() {
    O2::unlink();
    // FIXME: Delete relevant cookies, too
}
