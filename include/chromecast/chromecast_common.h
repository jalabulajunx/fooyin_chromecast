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

#include <QMetaType>
#include <QObject>

namespace Chromecast {

enum class TranscodingQuality
{
    High,
    Balanced,
    Efficient
};

enum class TranscodingFormat
{
    AAC,
    MP3,
    Opus,
    FLAC,
    Vorbis,
    WAV
};

enum class ConnectionStatus
{
    Disconnected,
    Connecting,
    Connected,
    Disconnecting,
    Error
};

enum class PlaybackStatus
{
    Idle,
    Loading,
    Buffering,
    Playing,
    Paused,
    Stopped,
    Error
};

}

Q_DECLARE_METATYPE(Chromecast::TranscodingQuality)
Q_DECLARE_METATYPE(Chromecast::TranscodingFormat)
Q_DECLARE_METATYPE(Chromecast::ConnectionStatus)
Q_DECLARE_METATYPE(Chromecast::PlaybackStatus)
