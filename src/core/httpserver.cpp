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

#include "httpserver.h"

#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QDebug>
#include <QCryptographicHash>
#include <QNetworkInterface>

namespace Chromecast {

HttpServer::HttpServer(QObject* parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &HttpServer::onNewConnection);
}

HttpServer::~HttpServer()
{
    stop();
}

bool HttpServer::start(quint16 port)
{
    if (m_isRunning) {
        return true;
    }

    m_port = port;

    // IMPORTANT: Listen on IPv4 explicitly - Chromecast uses IPv4
    // QHostAddress::Any might bind to IPv6-only on some systems
    if (!m_server->listen(QHostAddress::AnyIPv4, port)) {
        qWarning() << "Failed to start HTTP server on port" << port << ":" << m_server->errorString();
        return false;
    }

    m_isRunning = true;
    m_port = m_server->serverPort(); // Get actual port (useful if port was 0)

    qInfo() << "HTTP server started on" << m_server->serverAddress().toString() << ":" << m_port;
    qInfo() << "HTTP server is listening:" << m_server->isListening();
    qInfo() << "HTTP server max pending connections:" << m_server->maxPendingConnections();

    return true;
}

void HttpServer::stop()
{
    if (!m_isRunning) {
        return;
    }

    m_server->close();
    m_mediaFiles.clear();
    m_isRunning = false;
    qInfo() << "HTTP server stopped";
}

bool HttpServer::isRunning() const
{
    return m_isRunning;
}

QHostAddress HttpServer::serverAddress() const
{
    return m_server->serverAddress();
}

quint16 HttpServer::serverPort() const
{
    return m_port;
}

QString HttpServer::serverUrl() const
{
    // Find the first non-localhost IPv4 address
    // This will be accessible to Chromecast devices on the local network
    QHostAddress lanAddress;

    const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& address : addresses) {
        // Skip localhost, IPv6, and invalid addresses
        if (address.protocol() == QAbstractSocket::IPv4Protocol
            && !address.isLoopback()
            && address != QHostAddress::LocalHost) {
            lanAddress = address;
            break;
        }
    }

    // Fallback to localhost if no LAN IP found (shouldn't happen in normal cases)
    if (lanAddress.isNull()) {
        qWarning() << "Could not detect LAN IP address, using localhost (Chromecast won't be able to connect)";
        lanAddress = QHostAddress::LocalHost;
    }

    QString url = QString("http://%1:%2").arg(lanAddress.toString()).arg(m_port);
    qDebug() << "HTTP server URL:" << url;

    return url;
}

QString HttpServer::createMediaUrl(const QString& mediaPath)
{
    if (!m_isRunning) {
        qWarning() << "HTTP server not running";
        return QString();
    }

    // Create a unique identifier for this file
    QByteArray hash = QCryptographicHash::hash(mediaPath.toUtf8(), QCryptographicHash::Md5);
    QString fileId = QString(hash.toHex().left(16));

    QFileInfo fileInfo(mediaPath);
    QString extension = fileInfo.suffix();
    QString urlPath = QString("/media/%1.%2").arg(fileId, extension);

    // Store the mapping
    m_mediaFiles[urlPath] = mediaPath;

    QString url = QString("%1%2").arg(serverUrl(), urlPath);
    qInfo() << "Created media URL:" << url << "for file:" << mediaPath;

    return url;
}

void HttpServer::onNewConnection()
{
    qInfo() << "HTTP Server: New connection received";
    while (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        qInfo() << "HTTP Server: Accepted connection from" << socket->peerAddress().toString();
        connect(socket, &QTcpSocket::readyRead, this, &HttpServer::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &HttpServer::onDisconnected);
    }
}

void HttpServer::onReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    QString request = QString::fromUtf8(socket->readAll());
    handleRequest(socket, request);
}

void HttpServer::onDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        socket->deleteLater();
    }
}

void HttpServer::handleRequest(QTcpSocket* socket, const QString& request)
{
    // Parse HTTP request
    QStringList lines = request.split("\r\n");
    if (lines.isEmpty()) {
        send404(socket);
        return;
    }

    // Parse first line: GET /path HTTP/1.1
    QStringList requestLine = lines[0].split(" ");
    if (requestLine.size() < 2) {
        send404(socket);
        return;
    }

    QString method = requestLine[0];
    QString path = requestLine[1];

    qInfo() << "HTTP Request:" << method << path;

    // Check for Range header (for seeking support)
    qint64 rangeStart = -1;
    qint64 rangeEnd = -1;
    for (const QString& line : lines) {
        if (line.startsWith("Range:", Qt::CaseInsensitive)) {
            // Parse Range: bytes=start-end
            QString rangeStr = line.mid(6).trimmed();
            if (rangeStr.startsWith("bytes=")) {
                QString byteRange = rangeStr.mid(6);
                QStringList rangeParts = byteRange.split("-");
                if (rangeParts.size() >= 1 && !rangeParts[0].isEmpty()) {
                    rangeStart = rangeParts[0].toLongLong();
                }
                if (rangeParts.size() >= 2 && !rangeParts[1].isEmpty()) {
                    rangeEnd = rangeParts[1].toLongLong();
                }
            }
        }
    }

    // Find the file for this path
    if (!m_mediaFiles.contains(path)) {
        qWarning() << "File not found for path:" << path;
        send404(socket);
        return;
    }

    QString filePath = m_mediaFiles[path];
    serveFile(socket, filePath, rangeStart, rangeEnd);
}

void HttpServer::serveFile(QTcpSocket* socket, const QString& filePath, qint64 start, qint64 end)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file:" << filePath;
        send404(socket);
        return;
    }

    qint64 fileSize = file.size();
    QString mimeType = getMimeType(filePath);

    // Handle range request
    if (start >= 0) {
        if (end < 0 || end >= fileSize) {
            end = fileSize - 1;
        }

        qint64 contentLength = end - start + 1;

        // Send 206 Partial Content response
        QString response = QString(
            "HTTP/1.1 206 Partial Content\r\n"
            "Content-Type: %1\r\n"
            "Content-Length: %2\r\n"
            "Content-Range: bytes %3-%4/%5\r\n"
            "Accept-Ranges: bytes\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n"
            "\r\n"
        ).arg(mimeType).arg(contentLength).arg(start).arg(end).arg(fileSize);

        socket->write(response.toUtf8());

        // Seek to start position and send data
        file.seek(start);
        qint64 bytesToSend = contentLength;
        const qint64 chunkSize = 64 * 1024; // 64KB chunks

        while (bytesToSend > 0 && socket->isOpen()) {
            qint64 toRead = qMin(chunkSize, bytesToSend);
            QByteArray data = file.read(toRead);
            if (data.isEmpty()) {
                break;
            }
            socket->write(data);
            bytesToSend -= data.size();
        }
    } else {
        // Send full file with 200 OK
        QString response = QString(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %1\r\n"
            "Content-Length: %2\r\n"
            "Accept-Ranges: bytes\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n"
            "\r\n"
        ).arg(mimeType).arg(fileSize);

        socket->write(response.toUtf8());

        // Send file data in chunks
        const qint64 chunkSize = 64 * 1024; // 64KB chunks
        while (!file.atEnd() && socket->isOpen()) {
            QByteArray data = file.read(chunkSize);
            socket->write(data);
        }
    }

    socket->flush();
    socket->waitForBytesWritten();
    socket->disconnectFromHost();
}

void HttpServer::send404(QTcpSocket* socket)
{
    QString response =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 13\r\n"
        "Connection: close\r\n"
        "\r\n"
        "404 Not Found";

    socket->write(response.toUtf8());
    socket->flush();
    socket->disconnectFromHost();
}

QString HttpServer::getMimeType(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();

    // Audio MIME types
    QMap<QString, QString> mimeTypes = {
        {"mp3", "audio/mpeg"},
        {"flac", "audio/flac"},
        {"ogg", "audio/ogg"},
        {"opus", "audio/opus"},
        {"m4a", "audio/mp4"},
        {"aac", "audio/aac"},
        {"wav", "audio/wav"},
        {"wma", "audio/x-ms-wma"},
        {"ape", "audio/x-ape"},
        {"wv", "audio/x-wavpack"}
    };

    return mimeTypes.value(extension, "application/octet-stream");
}

} // namespace Chromecast
