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

#include <QString>
#include <QJsonObject>

// Forward declare protobuf classes
namespace extensions { namespace api { namespace cast_channel {
    class CastMessage;
}}}

namespace Chromecast {

/**
 * Helper class for creating Cast protocol messages
 */
class CastProtocol
{
public:
    // Cast namespaces
    static constexpr const char* NS_CONNECTION = "urn:x-cast:com.google.cast.tp.connection";
    static constexpr const char* NS_HEARTBEAT = "urn:x-cast:com.google.cast.tp.heartbeat";
    static constexpr const char* NS_RECEIVER = "urn:x-cast:com.google.cast.receiver";
    static constexpr const char* NS_MEDIA = "urn:x-cast:com.google.cast.media";

    // Standard IDs
    static constexpr const char* SENDER_ID = "sender-0";
    static constexpr const char* RECEIVER_ID = "receiver-0";

    // Create a basic Cast message
    static extensions::api::cast_channel::CastMessage createMessage(
        const QString& sourceId,
        const QString& destinationId,
        const QString& namespace_,
        const QJsonObject& payload
    );

    // Connection messages
    static extensions::api::cast_channel::CastMessage createConnectMessage(
        const QString& sourceId,
        const QString& destinationId
    );

    static extensions::api::cast_channel::CastMessage createCloseMessage(
        const QString& sourceId,
        const QString& destinationId
    );

    // Heartbeat messages
    static extensions::api::cast_channel::CastMessage createPingMessage();
    static extensions::api::cast_channel::CastMessage createPongMessage();

    // Receiver messages
    static extensions::api::cast_channel::CastMessage createGetStatusMessage(int requestId);
    static extensions::api::cast_channel::CastMessage createLaunchMessage(
        int requestId,
        const QString& appId
    );

    // Media messages
    static extensions::api::cast_channel::CastMessage createLoadMediaMessage(
        int requestId,
        const QString& sourceId,
        const QString& sessionId,
        const QString& mediaUrl,
        const QString& contentType,
        const QString& title = QString(),
        const QString& subtitle = QString()
    );

    static extensions::api::cast_channel::CastMessage createPlayMessage(
        int requestId,
        const QString& sourceId,
        const QString& sessionId,
        int mediaSessionId
    );

    static extensions::api::cast_channel::CastMessage createPauseMessage(
        int requestId,
        const QString& sourceId,
        const QString& sessionId,
        int mediaSessionId
    );

    static extensions::api::cast_channel::CastMessage createStopMediaMessage(
        int requestId,
        const QString& sourceId,
        const QString& sessionId,
        int mediaSessionId
    );

    static extensions::api::cast_channel::CastMessage createSeekMessage(
        int requestId,
        const QString& sourceId,
        const QString& sessionId,
        int mediaSessionId,
        double currentTime
    );

    static extensions::api::cast_channel::CastMessage createSetVolumeMessage(
        int requestId,
        double level,
        bool muted = false
    );

    static extensions::api::cast_channel::CastMessage createGetMediaStatusMessage(
        int requestId,
        const QString& sourceId,
        const QString& sessionId
    );

    // Helper to parse JSON from Cast message
    static QJsonObject parsePayload(const extensions::api::cast_channel::CastMessage& message);
};

} // namespace Chromecast
