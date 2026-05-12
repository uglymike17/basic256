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


#ifndef BASICDOWNLOADER_H
#define BASICDOWNLOADER_H

#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include "Error.h"

class BasicDownloader : public QObject
{
    Q_OBJECT
public:
    explicit BasicDownloader(Error* e);
    virtual ~BasicDownloader();
    QByteArray data() const;
    bool reply;
    bool inprogress;
    bool cancel;
    void download(QUrl url);

signals:
    void done();
    void cancelDownload();

public slots:
    void stop();

private slots:
    void fileDownloaded(QNetworkReply* );

private:
    QNetworkAccessManager netmanager;
    QNetworkReply* netreply;
    QByteArray m_data;
    Error *error;
};

#endif // BASICDOWNLOADER_H
