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
#include <QSslSocket>
#include <QHostAddress>
#include <QString>
#include <QByteArray>

// Forward declare protobuf classes
namespace extensions { namespace api { namespace cast_channel {
    class CastMessage;
}}}

namespace Chromecast {

class CastSocket : public QObject
{
    Q_OBJECT

public:
    explicit CastSocket(QObject* parent = nullptr);
    ~CastSocket() override;

    void connectToDevice(const QHostAddress& address, quint16 port = 8009);
    void disconnect();
    bool isConnected() const;

    void sendMessage(const extensions::api::cast_channel::CastMessage& message);

signals:
    void connected();
    void disconnected();
    void messageReceived(const extensions::api::cast_channel::CastMessage& message);
    void error(const QString& errorString);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSslErrors(const QList<QSslError>& errors);
    void onError(QAbstractSocket::SocketError socketError);

private:
    void readMessages();
    bool readMessageLength(quint32& length);
    bool readMessageData(quint32 length, QByteArray& data);

    QSslSocket* m_socket{nullptr};
    QByteArray m_readBuffer;
};

} // namespace Chromecast
