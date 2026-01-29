/*
 * Fooyin
 * Copyright 2026, Sundararajan Mohan
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

#include "device.h"
#include <chromecast/chromecast_common.h>

#include <QObject>
#include <QTimer>
#include <QString>
#include <QMap>

// Forward declarations
namespace extensions { namespace api { namespace cast_channel {
    class CastMessage;
}}}

namespace Chromecast {

class CastSocket;

class CommunicationManager : public QObject
{
    Q_OBJECT

public:
    explicit CommunicationManager(QObject* parent = nullptr);
    ~CommunicationManager() override;

    void connectToDevice(const DeviceInfo& device);
    void disconnectFromDevice();
    bool isConnected() const;
    ConnectionStatus connectionStatus() const;

    void play(const QString& mediaUrl, const QString& title, const QString& artist, const QString& album,
              const QString& coverUrl);
    void pause();
    void stop();
    void seek(int position);
    void setVolume(int volume);

    // Get current playback position in seconds (from Chromecast MEDIA_STATUS)
    int currentPosition() const { return m_currentPosition; }

signals:
    void connectionStatusChanged(ConnectionStatus status);
    void playbackStatusChanged(PlaybackStatus status);
    void volumeChanged(int volume);
    void positionChanged(int position);
    void error(const QString& message);

private slots:
    void onCastSocketConnected();
    void onCastSocketDisconnected();
    void onCastSocketError(const QString& errorString);
    void onCastMessageReceived(const extensions::api::cast_channel::CastMessage& message);
    void onHeartbeatTimeout();
    void onConnectionTimeout();
    void onMediaStatusPollTimeout();

private:
    void ensureInitialized();
    void startHeartbeat();
    void stopHeartbeat();
    void startMediaStatusPolling();
    void stopMediaStatusPolling();
    void sendConnect();
    void sendGetStatus();
    void sendGetMediaStatus();
    void launchDefaultMediaReceiver();

    void handleReceiverStatusMessage(const extensions::api::cast_channel::CastMessage& message);
    void handleMediaStatusMessage(const extensions::api::cast_channel::CastMessage& message);
    void handleHeartbeatMessage(const extensions::api::cast_channel::CastMessage& message);

    int nextRequestId();

    CastSocket* m_socket{nullptr};
    DeviceInfo m_currentDevice;
    ConnectionStatus m_connectionStatus{ConnectionStatus::Disconnected};
    PlaybackStatus m_playbackStatus{PlaybackStatus::Idle};

    QTimer* m_heartbeatTimer{nullptr};
    QTimer* m_connectionTimer{nullptr};
    QTimer* m_mediaStatusPollTimer{nullptr};

    int m_requestIdCounter{1};
    int m_currentVolume{100};
    int m_currentPosition{0};

    // Cast session info
    QString m_sourceId{"sender-0"};  // Use standard sender ID
    QString m_sessionId;
    QString m_transportId;
    int m_mediaSessionId{0};

    // Pending media load info (stored until session is ready)
    struct PendingMedia {
        QString url;
        QString title;
        QString artist;
        QString album;
        QString coverUrl;
    };
    PendingMedia m_pendingMedia;
};

} // namespace Chromecast
