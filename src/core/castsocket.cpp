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

#include "castsocket.h"
#include "cast_channel.pb.h"

#include <QDebug>
#include <QDataStream>

namespace Chromecast {

CastSocket::CastSocket(QObject* parent)
    : QObject(parent)
    , m_socket(new QSslSocket(this))
{
    // Wait for SSL encryption to complete, not just TCP connection
    connect(m_socket, &QSslSocket::encrypted, this, &CastSocket::onConnected);
    connect(m_socket, &QSslSocket::disconnected, this, &CastSocket::onDisconnected);
    connect(m_socket, &QSslSocket::readyRead, this, &CastSocket::onReadyRead);
    connect(m_socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
            this, &CastSocket::onSslErrors);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QSslSocket::errorOccurred),
            this, &CastSocket::onError);

    // Configure SSL to ignore certificate errors (Chromecast uses self-signed certs)
    m_socket->setPeerVerifyMode(QSslSocket::VerifyNone);
}

CastSocket::~CastSocket()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
}

void CastSocket::connectToDevice(const QHostAddress& address, quint16 port)
{
    qInfo() << "CastSocket: Connecting to" << address.toString() << ":" << port;

    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        qWarning() << "CastSocket: Already connected or connecting";
        return;
    }

    m_readBuffer.clear();
    m_socket->connectToHostEncrypted(address.toString(), port);
}

void CastSocket::disconnect()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
}

bool CastSocket::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState
           && m_socket->isEncrypted();
}

void CastSocket::sendMessage(const extensions::api::cast_channel::CastMessage& message)
{
    if (!isConnected()) {
        qWarning() << "CastSocket: Cannot send message - not connected";
        return;
    }

    // Serialize the protobuf message
    std::string serialized = message.SerializeAsString();
    quint32 messageSize = static_cast<quint32>(serialized.size());

    // Log what we're sending
    QString ns = QString::fromStdString(message.namespace_());
    qDebug() << "CastSocket: Sending message - NS:" << ns
             << "From:" << QString::fromStdString(message.source_id())
             << "To:" << QString::fromStdString(message.destination_id())
             << "Size:" << messageSize;

    // Cast protocol: send 4-byte big-endian length, then message
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << messageSize;
    packet.append(serialized.data(), serialized.size());

    qint64 written = m_socket->write(packet);
    if (written != packet.size()) {
        qWarning() << "CastSocket: Failed to write complete message";
    }

    m_socket->flush();
}

void CastSocket::onConnected()
{
    qInfo() << "CastSocket: Connected to Chromecast";
    emit connected();
}

void CastSocket::onDisconnected()
{
    qInfo() << "CastSocket: Disconnected from Chromecast";
    m_readBuffer.clear();
    emit disconnected();
}

void CastSocket::onReadyRead()
{
    // Append new data to buffer
    m_readBuffer.append(m_socket->readAll());
    readMessages();
}

void CastSocket::onSslErrors(const QList<QSslError>& errors)
{
    // Chromecast uses self-signed certificates, so we ignore SSL errors
    qDebug() << "CastSocket: Ignoring SSL errors (expected for Chromecast):";
    for (const QSslError& error : errors) {
        qDebug() << "  -" << error.errorString();
    }
    m_socket->ignoreSslErrors();
}

void CastSocket::onError(QAbstractSocket::SocketError socketError)
{
    QString errorStr = m_socket->errorString();
    qWarning() << "CastSocket error:" << socketError << "-" << errorStr;
    emit error(errorStr);
}

void CastSocket::readMessages()
{
    while (true) {
        // Need at least 4 bytes for message length
        if (m_readBuffer.size() < 4) {
            return;
        }

        // Read message length (4 bytes, big-endian)
        quint32 messageLength;
        QDataStream stream(m_readBuffer);
        stream.setByteOrder(QDataStream::BigEndian);
        stream >> messageLength;

        // Check if we have the full message
        if (m_readBuffer.size() < static_cast<int>(4 + messageLength)) {
            return; // Wait for more data
        }

        // Extract message data
        QByteArray messageData = m_readBuffer.mid(4, messageLength);
        m_readBuffer.remove(0, 4 + messageLength);

        // Parse protobuf message
        extensions::api::cast_channel::CastMessage message;
        if (message.ParseFromArray(messageData.data(), messageData.size())) {
            emit messageReceived(message);
        } else {
            qWarning() << "CastSocket: Failed to parse Cast message";
        }
    }
}

} // namespace Chromecast
