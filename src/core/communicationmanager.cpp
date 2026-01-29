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

#include "communicationmanager.h"
#include "castsocket.h"
#include "castprotocol.h"
#include "cast_channel.pb.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>

namespace Chromecast {

CommunicationManager::CommunicationManager(QObject* parent)
    : QObject(parent)
{
    qInfo() << "CommunicationManager: Initialized with protobuf Cast protocol";
}

CommunicationManager::~CommunicationManager()
{
    disconnectFromDevice();
}

void CommunicationManager::ensureInitialized()
{
    if (m_socket) {
        return; // Already initialized
    }

    qInfo() << "CommunicationManager: Lazy-initializing network objects";

    // Create objects in the current thread
    m_socket = new CastSocket(this);
    m_heartbeatTimer = new QTimer(this);
    m_connectionTimer = new QTimer(this);
    m_mediaStatusPollTimer = new QTimer(this);

    // Connect CastSocket signals
    connect(m_socket, &CastSocket::connected, this, &CommunicationManager::onCastSocketConnected);
    connect(m_socket, &CastSocket::disconnected, this, &CommunicationManager::onCastSocketDisconnected);
    connect(m_socket, &CastSocket::error, this, &CommunicationManager::onCastSocketError);
    connect(m_socket, &CastSocket::messageReceived, this, &CommunicationManager::onCastMessageReceived);

    // Setup heartbeat timer (5 seconds)
    m_heartbeatTimer->setInterval(5000);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &CommunicationManager::onHeartbeatTimeout);

    // Setup connection timeout (10 seconds)
    m_connectionTimer->setSingleShot(true);
    m_connectionTimer->setInterval(10000);
    connect(m_connectionTimer, &QTimer::timeout, this, &CommunicationManager::onConnectionTimeout);

    // Setup media status polling timer (1 second) to get position updates
    m_mediaStatusPollTimer->setInterval(1000);
    connect(m_mediaStatusPollTimer, &QTimer::timeout, this, &CommunicationManager::onMediaStatusPollTimeout);
}

void CommunicationManager::connectToDevice(const DeviceInfo& device)
{
    qInfo() << "CommunicationManager: Connecting to" << device.friendlyName
            << "(" << device.ipAddress.toString() << ":" << device.port << ")";

    if (m_connectionStatus != ConnectionStatus::Disconnected) {
        qWarning() << "CommunicationManager: Already connected or connecting";
        return;
    }

    // Ensure network objects are created
    ensureInitialized();

    m_currentDevice = device;
    m_connectionStatus = ConnectionStatus::Connecting;
    emit connectionStatusChanged(m_connectionStatus);

    // Start connection timeout
    m_connectionTimer->start();

    // Connect to Chromecast
    m_socket->connectToDevice(device.ipAddress, device.port);
}

void CommunicationManager::disconnectFromDevice()
{
    if (m_connectionStatus == ConnectionStatus::Disconnected) {
        return;
    }

    qInfo() << "CommunicationManager: Disconnecting from Chromecast";

    if (m_heartbeatTimer) {
        stopHeartbeat();
    }
    if (m_connectionTimer) {
        m_connectionTimer->stop();
    }
    stopMediaStatusPolling();

    // Send CLOSE message before disconnecting
    if (m_socket && m_socket->isConnected()) {
        m_socket->sendMessage(CastProtocol::createCloseMessage(m_sourceId, CastProtocol::RECEIVER_ID));
        if (!m_sessionId.isEmpty()) {
            m_socket->sendMessage(CastProtocol::createCloseMessage(m_sourceId, m_sessionId));
        }
        m_socket->disconnect();
    }

    m_connectionStatus = ConnectionStatus::Disconnected;
    m_playbackStatus = PlaybackStatus::Idle;
    m_sessionId.clear();
    m_transportId.clear();
    m_mediaSessionId = 0;

    emit connectionStatusChanged(m_connectionStatus);
    emit playbackStatusChanged(m_playbackStatus);
}

bool CommunicationManager::isConnected() const
{
    return m_connectionStatus == ConnectionStatus::Connected;
}

ConnectionStatus CommunicationManager::connectionStatus() const
{
    return m_connectionStatus;
}

void CommunicationManager::onCastSocketConnected()
{
    qInfo() << "CommunicationManager: Socket connected, initiating Cast protocol handshake";

    if (m_connectionTimer) {
        m_connectionTimer->stop();
    }

    // Send CONNECT message
    sendConnect();

    // Start heartbeat
    startHeartbeat();

    // Request receiver status
    sendGetStatus();
}

void CommunicationManager::onCastSocketDisconnected()
{
    qInfo() << "CommunicationManager: Socket disconnected";

    if (m_heartbeatTimer) {
        stopHeartbeat();
    }
    if (m_connectionTimer) {
        m_connectionTimer->stop();
    }

    m_connectionStatus = ConnectionStatus::Disconnected;
    m_playbackStatus = PlaybackStatus::Idle;
    m_sessionId.clear();
    m_transportId.clear();
    m_mediaSessionId = 0;

    emit connectionStatusChanged(m_connectionStatus);
    emit playbackStatusChanged(m_playbackStatus);
}

void CommunicationManager::onCastSocketError(const QString& errorString)
{
    qWarning() << "CommunicationManager: Cast socket error:" << errorString;

    m_connectionStatus = ConnectionStatus::Error;
    emit connectionStatusChanged(m_connectionStatus);
    emit error(errorString);
}

void CommunicationManager::onCastMessageReceived(const extensions::api::cast_channel::CastMessage& message)
{
    QString ns = QString::fromStdString(message.namespace_());
    QJsonObject payload = CastProtocol::parsePayload(message);
    QString type = payload["type"].toString();

    qDebug() << "CommunicationManager: Received message - NS:" << ns << "Type:" << type;

    // Route message based on namespace
    if (ns == CastProtocol::NS_RECEIVER) {
        handleReceiverStatusMessage(message);
    }
    else if (ns == CastProtocol::NS_MEDIA) {
        handleMediaStatusMessage(message);
    }
    else if (ns == CastProtocol::NS_HEARTBEAT) {
        handleHeartbeatMessage(message);
    }
    else if (ns == CastProtocol::NS_CONNECTION) {
        // Connection namespace messages (CLOSE, etc.) - just log for now
        qDebug() << "CommunicationManager: Connection message:" << type;
    }
    else {
        qDebug() << "CommunicationManager: Unknown namespace:" << ns;
    }
}

void CommunicationManager::onHeartbeatTimeout()
{
    // Send PING
    m_socket->sendMessage(CastProtocol::createPingMessage());
}

void CommunicationManager::onConnectionTimeout()
{
    qWarning() << "CommunicationManager: Connection timeout";

    m_connectionStatus = ConnectionStatus::Error;
    emit connectionStatusChanged(m_connectionStatus);
    emit error("Connection timeout");

    if (m_socket) {
        m_socket->disconnect();
    }
}

void CommunicationManager::startHeartbeat()
{
    if (m_heartbeatTimer) {
        m_heartbeatTimer->start();
    }
}

void CommunicationManager::stopHeartbeat()
{
    if (m_heartbeatTimer) {
        m_heartbeatTimer->stop();
    }
}

void CommunicationManager::startMediaStatusPolling()
{
    if (m_mediaStatusPollTimer && !m_mediaStatusPollTimer->isActive()) {
        qInfo() << "CommunicationManager: Starting media status polling (1s interval)";
        m_mediaStatusPollTimer->start();
    }
}

void CommunicationManager::stopMediaStatusPolling()
{
    if (m_mediaStatusPollTimer) {
        m_mediaStatusPollTimer->stop();
    }
}

void CommunicationManager::onMediaStatusPollTimeout()
{
    // Request media status to get current position
    sendGetMediaStatus();
}

void CommunicationManager::sendConnect()
{
    if (m_socket) {
        m_socket->sendMessage(CastProtocol::createConnectMessage(m_sourceId, CastProtocol::RECEIVER_ID));
    }
}

void CommunicationManager::sendGetStatus()
{
    if (m_socket) {
        m_socket->sendMessage(CastProtocol::createGetStatusMessage(nextRequestId()));
    }
}

void CommunicationManager::sendGetMediaStatus()
{
    if (m_socket && !m_transportId.isEmpty()) {
        m_socket->sendMessage(CastProtocol::createGetMediaStatusMessage(
            nextRequestId(), m_sourceId, m_transportId));
    }
}

void CommunicationManager::launchDefaultMediaReceiver()
{
    qInfo() << "CommunicationManager: Launching Default Media Receiver app";

    if (m_socket) {
        // Launch the Default Media Receiver app (CC1AD845)
        m_socket->sendMessage(CastProtocol::createLaunchMessage(
            nextRequestId(),
            "CC1AD845"  // Default Media Receiver app ID
        ));
    }
}

void CommunicationManager::handleReceiverStatusMessage(const extensions::api::cast_channel::CastMessage& message)
{
    QJsonObject payload = CastProtocol::parsePayload(message);
    QString type = payload["type"].toString();

    qDebug() << "CommunicationManager: Receiver message type:" << type;

    if (type == "RECEIVER_STATUS") {
        // Log full status for debugging
        QJsonDocument statusDoc(payload);
        qInfo() << "CommunicationManager: RECEIVER_STATUS payload:" << statusDoc.toJson(QJsonDocument::Compact);

        QJsonObject status = payload["status"].toObject();
        QJsonArray applications = status["applications"].toArray();

        if (applications.isEmpty()) {
            // No app running, need to launch Default Media Receiver
            if (m_connectionStatus == ConnectionStatus::Connecting) {
                launchDefaultMediaReceiver();
            }
        } else {
            // Check if a media-capable app is running
            QJsonObject app = applications[0].toObject();
            QString appId = app["appId"].toString();

            qInfo() << "CommunicationManager: App running:" << appId << app["displayName"].toString();

            // Only connect to Default Media Receiver (CC1AD845) or media-capable apps
            // Backdrop (E8C28D3C) and other idle screen apps don't support media playback
            if (appId == "CC1AD845") {
                // Good - Default Media Receiver is running
                m_sessionId = app["sessionId"].toString();
                m_transportId = app["transportId"].toString();
                qInfo() << "CommunicationManager: Got session ID:" << m_sessionId;
            } else {
                // Wrong app running - launch Default Media Receiver
                qInfo() << "CommunicationManager: Non-media app running, launching Default Media Receiver";
                if (m_connectionStatus == ConnectionStatus::Connecting) {
                    launchDefaultMediaReceiver();
                }
                return;
            }

            qInfo() << "CommunicationManager: Got session ID:" << m_sessionId;

            // Send CONNECT to the app
            if (m_socket) {
                m_socket->sendMessage(CastProtocol::createConnectMessage(m_sourceId, m_sessionId));

                // Request media status to initialize media namespace
                m_socket->sendMessage(CastProtocol::createGetMediaStatusMessage(
                    nextRequestId(),
                    m_sourceId,
                    m_sessionId
                ));
            }

            // Mark as connected
            if (m_connectionStatus != ConnectionStatus::Connected) {
                m_connectionStatus = ConnectionStatus::Connected;
                emit connectionStatusChanged(m_connectionStatus);

                // If we have pending media, load it now
                if (!m_pendingMedia.url.isEmpty()) {
                    qInfo() << "CommunicationManager: Loading pending media";
                    play(m_pendingMedia.url, m_pendingMedia.title, m_pendingMedia.artist,
                         m_pendingMedia.album, m_pendingMedia.coverUrl);
                    m_pendingMedia = PendingMedia();
                }
            }
        }
    }
}

void CommunicationManager::handleMediaStatusMessage(const extensions::api::cast_channel::CastMessage& message)
{
    QJsonObject payload = CastProtocol::parsePayload(message);
    QString type = payload["type"].toString();

    qDebug() << "CommunicationManager: Media message type:" << type;

    if (type == "LOAD_FAILED" || type == "LOAD_CANCELLED" || type == "INVALID_REQUEST") {
        // Log the full error payload
        QJsonDocument errorDoc(payload);
        qWarning() << "CommunicationManager: Media load error:" << errorDoc.toJson(QJsonDocument::Compact);
        return;
    }

    if (type == "MEDIA_STATUS") {
        QJsonArray statusArray = payload["status"].toArray();
        if (!statusArray.isEmpty()) {
            QJsonObject status = statusArray[0].toObject();

            m_mediaSessionId = status["mediaSessionId"].toInt();
            QString playerState = status["playerState"].toString();

            qDebug() << "CommunicationManager: Player state:" << playerState
                     << "Media session ID:" << m_mediaSessionId;

            // Update playback status
            if (playerState == "PLAYING") {
                m_playbackStatus = PlaybackStatus::Playing;
                emit playbackStatusChanged(m_playbackStatus);
            }
            else if (playerState == "PAUSED") {
                m_playbackStatus = PlaybackStatus::Paused;
                emit playbackStatusChanged(m_playbackStatus);
            }
            else if (playerState == "BUFFERING") {
                m_playbackStatus = PlaybackStatus::Buffering;
                emit playbackStatusChanged(m_playbackStatus);
            }
            else if (playerState == "IDLE") {
                m_playbackStatus = PlaybackStatus::Idle;
                emit playbackStatusChanged(m_playbackStatus);
            }

            // Update position
            if (status.contains("currentTime")) {
                m_currentPosition = static_cast<int>(status["currentTime"].toDouble());
                qDebug() << "CommunicationManager: Position update:" << m_currentPosition << "seconds";
                emit positionChanged(m_currentPosition);
            } else {
                qDebug() << "CommunicationManager: No currentTime in MEDIA_STATUS";
            }
        }
    }
}

void CommunicationManager::handleHeartbeatMessage(const extensions::api::cast_channel::CastMessage& message)
{
    QJsonObject payload = CastProtocol::parsePayload(message);
    QString type = payload["type"].toString();

    if (type == "PING") {
        // Respond with PONG
        if (m_socket) {
            m_socket->sendMessage(CastProtocol::createPongMessage());
        }
    }
}

int CommunicationManager::nextRequestId()
{
    return m_requestIdCounter++;
}

void CommunicationManager::play(const QString& mediaUrl, const QString& title, const QString& artist,
                                const QString& album, const QString& coverUrl)
{
    if (!m_socket || !m_socket->isConnected()) {
        qWarning() << "CommunicationManager: Not connected to Chromecast";
        return;
    }

    // If we don't have a session yet, store as pending
    if (m_sessionId.isEmpty()) {
        qInfo() << "CommunicationManager: Session not ready, storing media as pending";
        m_pendingMedia.url = mediaUrl;
        m_pendingMedia.title = title;
        m_pendingMedia.artist = artist;
        m_pendingMedia.album = album;
        m_pendingMedia.coverUrl = coverUrl;
        return;
    }

    qInfo() << "CommunicationManager: Loading media:" << title;

    // Determine content type from file extension
    QString contentType = "audio/mpeg";
    if (mediaUrl.endsWith(".flac", Qt::CaseInsensitive)) {
        contentType = "audio/flac";
    } else if (mediaUrl.endsWith(".mp3", Qt::CaseInsensitive)) {
        contentType = "audio/mpeg";
    } else if (mediaUrl.endsWith(".m4a", Qt::CaseInsensitive) || mediaUrl.endsWith(".aac", Qt::CaseInsensitive)) {
        contentType = "audio/aac";
    } else if (mediaUrl.endsWith(".ogg", Qt::CaseInsensitive)) {
        contentType = "audio/ogg";
    } else if (mediaUrl.endsWith(".opus", Qt::CaseInsensitive)) {
        contentType = "audio/opus";
    } else if (mediaUrl.endsWith(".wav", Qt::CaseInsensitive)) {
        contentType = "audio/wav";
    }

    // Send LOAD message with full metadata including cover art
    if (m_socket) {
        m_socket->sendMessage(CastProtocol::createLoadMediaMessage(
            nextRequestId(),
            m_sourceId,
            m_sessionId,
            mediaUrl,
            contentType,
            title,
            artist,
            album,
            coverUrl
        ));
    }

    // Reset position when starting new media
    m_currentPosition = 0;

    // Start polling for media status to get position updates
    startMediaStatusPolling();

    m_playbackStatus = PlaybackStatus::Loading;
    emit playbackStatusChanged(m_playbackStatus);
}

void CommunicationManager::pause()
{
    if (!m_socket || !m_socket->isConnected() || m_sessionId.isEmpty() || m_mediaSessionId == 0) {
        qWarning() << "CommunicationManager: Cannot pause - not ready";
        return;
    }

    qInfo() << "CommunicationManager: Pausing playback";

    m_socket->sendMessage(CastProtocol::createPauseMessage(
        nextRequestId(),
        m_sourceId,
        m_sessionId,
        m_mediaSessionId
    ));
}

void CommunicationManager::stop()
{
    // Stop media status polling
    stopMediaStatusPolling();

    if (!m_socket || !m_socket->isConnected() || m_sessionId.isEmpty() || m_mediaSessionId == 0) {
        qWarning() << "CommunicationManager: Cannot stop - not ready";
        return;
    }

    qInfo() << "CommunicationManager: Stopping playback";

    m_socket->sendMessage(CastProtocol::createStopMediaMessage(
        nextRequestId(),
        m_sourceId,
        m_sessionId,
        m_mediaSessionId
    ));

    m_playbackStatus = PlaybackStatus::Stopped;
    emit playbackStatusChanged(m_playbackStatus);
}

void CommunicationManager::seek(int position)
{
    if (!m_socket || !m_socket->isConnected() || m_sessionId.isEmpty() || m_mediaSessionId == 0) {
        qWarning() << "CommunicationManager: Cannot seek - not ready";
        return;
    }

    qInfo() << "CommunicationManager: Seeking to" << position << "seconds";

    m_socket->sendMessage(CastProtocol::createSeekMessage(
        nextRequestId(),
        m_sourceId,
        m_sessionId,
        m_mediaSessionId,
        static_cast<double>(position)
    ));
}

void CommunicationManager::setVolume(int volume)
{
    if (!m_socket || !m_socket->isConnected()) {
        qWarning() << "CommunicationManager: Cannot set volume - not connected";
        return;
    }

    qInfo() << "CommunicationManager: Setting volume to" << volume << "%";

    // Convert 0-100 to 0.0-1.0
    double level = volume / 100.0;

    m_socket->sendMessage(CastProtocol::createSetVolumeMessage(
        nextRequestId(),
        level,
        false  // not muted
    ));

    m_currentVolume = volume;
    emit volumeChanged(volume);
}

} // namespace Chromecast
