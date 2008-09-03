/* Cuberok
 * Copyright (C) 2008 Vasiliy Makarov <drmoriarty.0@gmail.com>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QtCore>
#include <QtNetwork>

class Downloader : public QObject
{
	Q_OBJECT
 public:
	Downloader();
	~Downloader();

	bool download(QUrl);
	bool done();
 
 signals:
	void complete(QString);
	void progress(int, int);
	void cancel(QString);

 private slots:
     void cancelDownload();
     void httpRequestFinished(int requestId, bool error);
     void readResponseHeader(const QHttpResponseHeader &responseHeader);
     void slotAuthenticationRequired(const QString &, quint16, QAuthenticator *);

 private:
     QHttp *http;
     QFile *file;
     int httpGetId, taskID;
     bool httpRequestAborted, finished;
};

class SyncDownloader : public QThread
{
    Q_OBJECT
 public:
	SyncDownloader ( QUrl, QString*, QObject * parent = 0 );
	~SyncDownloader();
    void run();

 protected:
	QUrl url;
	QString *file;
	QMutex mutex;

 private slots:
	void complete(QString);
	void cancel(QString);
};


#endif //DOWNLOADER_H
