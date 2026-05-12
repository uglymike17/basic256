/** Copyright (C) 2016, Florin Oprea <florinoprea.contact@gmail.com>
 **
 **  This program is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 2 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License along
 **  with this program; if not, write to the Free Software Foundation, Inc.,
 **  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **/


#include "BasicDownloader.h"

BasicDownloader::BasicDownloader(Error *e) :
    QObject()
{
    error = e;
    m_data = NULL;
    netreply = NULL;
    reply = true;
    cancel = false;
    inprogress = false;
    connect(&netmanager, SIGNAL (finished(QNetworkReply*)),this, SLOT (fileDownloaded(QNetworkReply*)));
}

BasicDownloader::~BasicDownloader() {
    stop();
}

void BasicDownloader::download(QUrl url){
    reply = false;
    inprogress = true;
    QNetworkRequest request(url);
//compile with older Ot (for Linux users)
#if QT_VERSION >= 0x050600
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif
    request.setHeader(QNetworkRequest::UserAgentHeader, "App/1.0");
    if(!cancel){
        netreply = netmanager.get(request);
        connect(this, SIGNAL(cancelDownload()), netreply, SLOT(abort()));
        if(cancel) emit(cancelDownload());
    }else{
        cancel=false;
        inprogress = false;
    }
}

void BasicDownloader::fileDownloaded(QNetworkReply* ) {
    //qDebug() << "BasicDownloader fileDownloaded() attr:" << netreply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString() << "err:" << netreply->error();
    if(netreply->error() != QNetworkReply::NoError) {
        error->q(ERROR_DOWNLOAD, netreply->errorString());
    }else if(netreply->attribute(QNetworkRequest::HttpStatusCodeAttribute) >= 300) {
        error->q(ERROR_DOWNLOAD, netreply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString());
    }else{
        m_data = netreply->readAll();
    }
    netreply->deleteLater();
    reply = true;
    emit(done());
    inprogress = false;
}

QByteArray BasicDownloader::data() const {
    if(!reply){
        QEventLoop loop;
        connect(this, SIGNAL(done()), &loop, SLOT(quit()));
        loop.exec();
    }
    return m_data;
}

void BasicDownloader::stop() {
    if(inprogress){
        cancel=true;
        emit(cancelDownload());
    }
}
