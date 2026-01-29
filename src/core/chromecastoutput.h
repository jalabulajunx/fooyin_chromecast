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

#include <core/engine/audiooutput.h>
#include <core/engine/audiobuffer.h>
#include <core/engine/audioformat.h>
#include <core/player/playerdefs.h>
#include <chromecast/chromecast_common.h>

#include <QObject>
#include <QString>
#include <QElapsedTimer>

namespace Fooyin {
class PlayerController;
class Track;
}

namespace Chromecast {

class DiscoveryManager;
class CommunicationManager;
class HttpServer;
class TranscodingManager;
class TrackMetadataExtractor;

/*!
 * ChromecastOutput implements the Fooyin AudioOutput interface to stream
 * audio to Chromecast devices. It acts as an audio output renderer,
 * similar to VLC's cast-to functionality.
 */
class ChromecastOutput : public Fooyin::AudioOutput
{
    Q_OBJECT

public:
    explicit ChromecastOutput(DiscoveryManager* discovery,
                             CommunicationManager* communication,
                             HttpServer* httpServer,
                             TranscodingManager* transcoder,
                             TrackMetadataExtractor* metadataExtractor,
                             Fooyin::PlayerController* playerController);
    ~ChromecastOutput() override;

    // AudioOutput interface implementation
    bool init(const Fooyin::AudioFormat& format) override;
    void uninit() override;
    void reset() override;
    void drain() override;
    void start() override;

    [[nodiscard]] bool initialised() const override;
    [[nodiscard]] QString device() const override;
    [[nodiscard]] Fooyin::OutputState currentState() override;
    [[nodiscard]] int bufferSize() const override;
    [[nodiscard]] Fooyin::OutputDevices getAllDevices(bool isCurrentOutput) override;

    int write(const Fooyin::AudioBuffer& buffer) override;

    void setPaused(bool pause) override;
    void setVolume(double volume) override;
    void setDevice(const QString& device) override;

    [[nodiscard]] Fooyin::AudioFormat format() const override;
    [[nodiscard]] QString error() const override;

private slots:
    void onTrackChanged(const Fooyin::Track& track);
    void onPlayStateChanged(Fooyin::Player::PlayState state);
    void onChromecastPlaybackStatusChanged(PlaybackStatus status);

private:
    void startStreaming(const Fooyin::Track& track);
    bool needsTranscoding(const QString& filePath) const;
    // Component pointers (not owned, except m_communication)
    DiscoveryManager* m_discovery{nullptr};
    CommunicationManager* m_communication{nullptr};  // Owned by this instance
    HttpServer* m_httpServer{nullptr};
    TranscodingManager* m_transcoder{nullptr};
    TrackMetadataExtractor* m_metadataExtractor{nullptr};
    Fooyin::PlayerController* m_playerController{nullptr};

    // State management
    bool m_initialised{false};
    QString m_selectedDevice;
    QString m_errorString;
    Fooyin::AudioFormat m_format;

    // Audio buffering and position tracking
    QByteArray m_audioBuffer;
    int m_bufferSize{8192}; // Buffer size in samples
    uint64_t m_samplesWritten{0};

    // Playback state
    bool m_isPaused{false};
    double m_volume{1.0};
    QString m_currentTrackPath;
    bool m_isStreaming{false};
    uint64_t m_lastPosition{0}; // Track last known position for seek detection

    // Real-time playback tracking
    QElapsedTimer m_playbackTimer;  // Tracks real elapsed time since playback started
    qint64 m_pausedElapsed{0};      // Accumulated time when paused
    bool m_waitingForPlayback{false}; // True when we've sent LOAD but waiting for PLAYING state
    bool m_playbackTimerStarted{false}; // True once timer has been started (after PLAYING state received)
};

} // namespace Chromecast
