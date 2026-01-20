/*
 * Fooyin
 * Copyright 2024, Your Name
 *
 * Fooyin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Fooyin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Fooyin.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <QObject>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>

namespace Chromecast {

class HttpServer : public QObject
{
    Q_OBJECT

public:
    explicit HttpServer(QObject* parent = nullptr);
    ~HttpServer() override;

    bool start(quint16 port = 8010);
    void stop();
    bool isRunning() const;
    QHostAddress serverAddress() const;
    quint16 serverPort() const;
    QString serverUrl() const;

    QString createMediaUrl(const QString& mediaPath);

signals:
    void requestReceived(const QString& path);
    void error(const QString& message);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    void handleRequest(QTcpSocket* socket, const QString& request);
    void serveFile(QTcpSocket* socket, const QString& filePath, qint64 start = -1, qint64 end = -1);
    void send404(QTcpSocket* socket);
    QString getMimeType(const QString& filePath) const;

    QTcpServer* m_server{nullptr};
    QMap<QString, QString> m_mediaFiles; // URL path -> file path mapping
    bool m_isRunning{false};
    quint16 m_port{8010};
};

} // namespace Chromecast
