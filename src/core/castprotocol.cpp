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

#include "castprotocol.h"
#include "cast_channel.pb.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace Chromecast {

extensions::api::cast_channel::CastMessage CastProtocol::createMessage(
    const QString& sourceId,
    const QString& destinationId,
    const QString& namespace_,
    const QJsonObject& payload)
{
    extensions::api::cast_channel::CastMessage message;
    message.set_protocol_version(extensions::api::cast_channel::CastMessage_ProtocolVersion_CASTV2_1_0);
    message.set_source_id(sourceId.toStdString());
    message.set_destination_id(destinationId.toStdString());
    message.set_namespace_(namespace_.toStdString());
    message.set_payload_type(extensions::api::cast_channel::CastMessage_PayloadType_STRING);

    QJsonDocument doc(payload);
    message.set_payload_utf8(doc.toJson(QJsonDocument::Compact).toStdString());

    return message;
}

extensions::api::cast_channel::CastMessage CastProtocol::createConnectMessage(
    const QString& sourceId,
    const QString& destinationId)
{
    QJsonObject payload;
    payload["type"] = "CONNECT";

    return createMessage(sourceId, destinationId, NS_CONNECTION, payload);
}

extensions::api::cast_channel::CastMessage CastProtocol::createCloseMessage(
    const QString& sourceId,
    const QString& destinationId)
{
    QJsonObject payload;
    payload["type"] = "CLOSE";

    return createMessage(sourceId, destinationId, NS_CONNECTION, payload);
}

extensions::api::cast_channel::CastMessage CastProtocol::createPingMessage()
{
    QJsonObject payload;
    payload["type"] = "PING";

    return createMessage(SENDER_ID, RECEIVER_ID, NS_HEARTBEAT, payload);
}

extensions::api::cast_channel::CastMessage CastProtocol::createPongMessage()
{
    QJsonObject payload;
    payload["type"] = "PONG";

    return createMessage(SENDER_ID, RECEIVER_ID, NS_HEARTBEAT, payload);
}

extensions::api::cast_channel::CastMessage CastProtocol::createGetStatusMessage(int requestId)
{
    QJsonObject payload;
    payload["type"] = "GET_STATUS";
    payload["requestId"] = requestId;

    return createMessage(SENDER_ID, RECEIVER_ID, NS_RECEIVER, payload);
}

extensions::api::cast_channel::CastMessage CastProtocol::createLaunchMessage(
    int requestId,
    const QString& appId)
{
    QJsonObject payload;
    payload["type"] = "LAUNCH";
    payload["requestId"] = requestId;
    payload["appId"] = appId;

    return createMessage(SENDER_ID, RECEIVER_ID, NS_RECEIVER, payload);
}

extensions::api::cast_channel::CastMessage CastProtocol::createLoadMediaMessage(
    int requestId,
    const QString& sourceId,
    const QString& sessionId,
    const QString& mediaUrl,
    const QString& contentType,
    const QString& title,
    const QString& artist,
    const QString& album,
    const QString& coverUrl)
{
    QJsonObject media;
    media["contentId"] = mediaUrl;
    media["contentType"] = contentType;
    media["streamType"] = "BUFFERED";

    // Add MusicTrackMediaMetadata (metadataType = 3) for music with cover art
    QJsonObject metadata;
    metadata["metadataType"] = 3; // MusicTrackMediaMetadata

    if (!title.isEmpty()) {
        metadata["title"] = title;
    }
    if (!artist.isEmpty()) {
        metadata["artist"] = artist;
    }
    if (!album.isEmpty()) {
        metadata["albumName"] = album;
    }

    // Add cover art image if URL provided
    if (!coverUrl.isEmpty()) {
        QJsonArray images;
        QJsonObject image;
        image["url"] = coverUrl;
        images.append(image);
        metadata["images"] = images;
    }

    media["metadata"] = metadata;

    QJsonObject payload;
    payload["type"] = "LOAD";
    payload["requestId"] = requestId;
    payload["media"] = media;
    payload["autoplay"] = true;
    payload["currentTime"] = 0;

    // Log the LOAD message payload for debugging
    QJsonDocument doc(payload);
    qInfo() << "CastProtocol: LOAD message payload:" << doc.toJson(QJsonDocument::Compact);

    return createMessage(sourceId, sessionId, NS_MEDIA, payload);
}

extensions::api::cast_channel::CastMessage CastProtocol::createPlayMessage(
    int requestId,
    const QString& sourceId,
    const QString& sessionId,
    int mediaSessionId)
{
    QJsonObject payload;
    payload["type"] = "PLAY";
    payload["requestId"] = requestId;
    payload["mediaSessionId"] = mediaSessionId;

    return createMessage(sourceId, sessionId, NS_MEDIA, payload);
}

extensions::api::cast_channel::CastMessage CastProtocol::createPauseMessage(
    int requestId,
    const QString& sourceId,
    const QString& sessionId,
    int mediaSessionId)
{
    QJsonObject payload;
    payload["type"] = "PAUSE";
    payload["requestId"] = requestId;
    payload["mediaSessionId"] = mediaSessionId;

    return createMessage(sourceId, sessionId, NS_MEDIA, payload);
}

extensions::api::cast_channel::CastMessage CastProtocol::createStopMediaMessage(
    int requestId,
    const QString& sourceId,
    const QString& sessionId,
    int mediaSessionId)
{
    QJsonObject payload;
    payload["type"] = "STOP";
    payload["requestId"] = requestId;
    payload["mediaSessionId"] = mediaSessionId;

    return createMessage(sourceId, sessionId, NS_MEDIA, payload);
}

extensions::api::cast_channel::CastMessage CastProtocol::createSeekMessage(
    int requestId,
    const QString& sourceId,
    const QString& sessionId,
    int mediaSessionId,
    double currentTime)
{
    QJsonObject payload;
    payload["type"] = "SEEK";
    payload["requestId"] = requestId;
    payload["mediaSessionId"] = mediaSessionId;
    payload["currentTime"] = currentTime;

    return createMessage(sourceId, sessionId, NS_MEDIA, payload);
}

extensions::api::cast_channel::CastMessage CastProtocol::createSetVolumeMessage(
    int requestId,
    double level,
    bool muted)
{
    QJsonObject volume;
    volume["level"] = level;
    volume["muted"] = muted;

    QJsonObject payload;
    payload["type"] = "SET_VOLUME";
    payload["requestId"] = requestId;
    payload["volume"] = volume;

    return createMessage(SENDER_ID, RECEIVER_ID, NS_RECEIVER, payload);
}

extensions::api::cast_channel::CastMessage CastProtocol::createGetMediaStatusMessage(
    int requestId,
    const QString& sourceId,
    const QString& sessionId)
{
    QJsonObject payload;
    payload["type"] = "GET_STATUS";
    payload["requestId"] = requestId;

    return createMessage(sourceId, sessionId, NS_MEDIA, payload);
}

QJsonObject CastProtocol::parsePayload(const extensions::api::cast_channel::CastMessage& message)
{
    if (message.payload_type() != extensions::api::cast_channel::CastMessage_PayloadType_STRING) {
        qWarning() << "CastProtocol: Cannot parse non-string payload";
        return QJsonObject();
    }

    QJsonDocument doc = QJsonDocument::fromJson(
        QByteArray::fromStdString(message.payload_utf8())
    );

    if (!doc.isObject()) {
        qWarning() << "CastProtocol: Payload is not a JSON object";
        return QJsonObject();
    }

    return doc.object();
}

} // namespace Chromecast
