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

#include "chromecastoutput.h"
#include "discoverymanager.h"
#include "communicationmanager.h"
#include "httpserver.h"
#include "transcodingmanager.h"
#include "device.h"
#include "../integration/trackmetadata.h"

#include <core/player/playercontroller.h>
#include <core/track.h>

#include <QDebug>
#include <QFileInfo>
#include <QDir>

namespace Chromecast {

ChromecastOutput::ChromecastOutput(DiscoveryManager* discovery,
                                   CommunicationManager* communication,
                                   HttpServer* httpServer,
                                   TranscodingManager* transcoder,
                                   TrackMetadataExtractor* metadataExtractor,
                                   Fooyin::PlayerController* playerController)
    : m_discovery(discovery)
    , m_communication(communication)  // Use the shared instance from the plugin
    , m_httpServer(httpServer)
    , m_transcoder(transcoder)
    , m_metadataExtractor(metadataExtractor)
    , m_playerController(playerController)
{
    qInfo() << "ChromecastOutput created with shared CommunicationManager";

    // Connect to PlayerController signals to detect track changes
    if (m_playerController) {
        connect(m_playerController, &Fooyin::PlayerController::currentTrackChanged,
                this, &ChromecastOutput::onTrackChanged);
        connect(m_playerController, &Fooyin::PlayerController::playStateChanged,
                this, &ChromecastOutput::onPlayStateChanged);
    } else {
        qWarning() << "PlayerController not available in ChromecastOutput";
    }
}

ChromecastOutput::~ChromecastOutput()
{
    if (m_initialised) {
        uninit();
    }
    // m_communication is owned by this instance and will be deleted automatically
    qInfo() << "ChromecastOutput destroyed";
}

bool ChromecastOutput::init(const Fooyin::AudioFormat& format)
{
    qInfo() << "ChromecastOutput::init - Format:" << format.sampleRate() << "Hz,"
            << format.channelCount() << "channels";

    if (m_selectedDevice.isEmpty()) {
        m_errorString = "No Chromecast device selected";
        qWarning() << m_errorString;
        return false;
    }

    m_format = format;
    m_audioBuffer.clear();
    m_samplesWritten = 0;
    m_initialised = true;

    // If device was set but not connected (due to discovery not finished yet), try now
    if (!m_selectedDevice.isEmpty() && m_communication && !m_communication->isConnected()) {
        qInfo() << "ChromecastOutput::init - Retrying connection to selected device:" << m_selectedDevice;
        if (m_discovery) {
            QList<DeviceInfo> devices = m_discovery->devices();
            for (const DeviceInfo& deviceInfo : devices) {
                if (deviceInfo.id == m_selectedDevice) {
                    qInfo() << "ChromecastOutput::init - Connecting to device:" << deviceInfo.friendlyName;
                    m_communication->connectToDevice(deviceInfo);
                    break;
                }
            }
        }
    }

    qInfo() << "ChromecastOutput initialized successfully";

    // If there's already a track loaded in PlayerController, start streaming it now
    if (m_playerController) {
        Fooyin::Track currentTrack = m_playerController->currentTrack();
        if (currentTrack.isValid() && !currentTrack.filepath().isEmpty()) {
            qInfo() << "Starting streaming for current track:" << currentTrack.title();
            startStreaming(currentTrack);
        }
    }

    return true;
}

void ChromecastOutput::uninit()
{
    qInfo() << "ChromecastOutput::uninit";

    m_audioBuffer.clear();
    m_samplesWritten = 0;
    m_initialised = false;

    // Stop playback but keep connection alive for next track
    // This allows seamless track changes without reconnecting
    if (m_communication && m_communication->isConnected()) {
        m_communication->stop();
        // DO NOT disconnect - keep connection alive for next track
    }

    m_isStreaming = false;
    m_currentTrackPath.clear();
}

void ChromecastOutput::reset()
{
    qInfo() << "ChromecastOutput::reset";

    m_audioBuffer.clear();
    m_samplesWritten = 0;

    // Detect if this is a seek operation by checking if we're currently streaming
    // and the player position changed
    if (m_isStreaming && m_communication && m_communication->isConnected() && m_playerController) {
        uint64_t currentPos = m_playerController->currentPosition();

        // If position changed significantly (more than 1 second), this is a seek
        if (currentPos != m_lastPosition && std::abs(static_cast<int64_t>(currentPos - m_lastPosition)) > 1000) {
            qInfo() << "ChromecastOutput: Detected seek from" << m_lastPosition << "ms to" << currentPos << "ms";

            // Convert milliseconds to seconds for Chromecast
            int seekPositionSeconds = static_cast<int>(currentPos / 1000);
            m_communication->seek(seekPositionSeconds);
            m_lastPosition = currentPos;
        }
    }
}

void ChromecastOutput::drain()
{
    qInfo() << "ChromecastOutput::drain - flushing audio buffer";

    // For file-based streaming, we don't need to do much here
    // The HTTP server will handle serving the complete file
    m_audioBuffer.clear();
}

void ChromecastOutput::start()
{
    qInfo() << "ChromecastOutput::start - beginning playback";

    // Playback will be handled in the write() method when we receive audio buffers
    // and in the integration with PlayerController for track metadata
}

bool ChromecastOutput::initialised() const
{
    return m_initialised;
}

QString ChromecastOutput::device() const
{
    return m_selectedDevice;
}

Fooyin::OutputState ChromecastOutput::currentState()
{
    Fooyin::OutputState state;

    // If not streaming or not connected, return default state
    if (!m_isStreaming || !m_communication || !m_communication->isConnected()) {
        state.freeSamples = m_bufferSize;
        state.queuedSamples = 0;
        state.delay = 0.0;
        return state;
    }

    int sampleRate = m_format.sampleRate();
    int channels = m_format.channelCount();

    // Avoid division by zero
    if (sampleRate <= 0 || channels <= 0) {
        state.freeSamples = m_bufferSize;
        state.queuedSamples = 0;
        state.delay = 0.0;
        return state;
    }

    // Use real elapsed time to calculate how many samples "should" have been played
    // This is more accurate than relying on Chromecast position updates (which are slow)
    qint64 elapsedMs = 0;
    if (m_playbackTimer.isValid() && !m_isPaused) {
        elapsedMs = m_playbackTimer.elapsed() + m_pausedElapsed;
    } else if (m_isPaused) {
        elapsedMs = m_pausedElapsed;
    }

    // Convert elapsed milliseconds to samples played
    // samples = (ms / 1000) * sampleRate * channels
    uint64_t samplesPlayed = static_cast<uint64_t>(elapsedMs) * sampleRate * channels / 1000;

    // Queued samples = what we've written minus what should have been played
    int64_t queued = static_cast<int64_t>(m_samplesWritten) - static_cast<int64_t>(samplesPlayed);
    state.queuedSamples = static_cast<int>(std::max(static_cast<int64_t>(0), queued));

    // If buffer is "full" (queued > threshold), report no free space
    // This creates backpressure to slow down the decoder
    const int maxQueuedSamples = sampleRate * channels * 10; // 10 seconds buffer
    state.freeSamples = (state.queuedSamples < maxQueuedSamples) ? m_bufferSize : 0;

    // Delay in seconds
    state.delay = static_cast<double>(state.queuedSamples) / (sampleRate * channels);

    return state;
}

int ChromecastOutput::bufferSize() const
{
    return m_bufferSize;
}

Fooyin::OutputDevices ChromecastOutput::getAllDevices(bool /*isCurrentOutput*/)
{
    Fooyin::OutputDevices devices;

    if (!m_discovery) {
        qWarning() << "DiscoveryManager not available";
        return devices;
    }

    // Get all discovered Chromecast devices
    QList<DeviceInfo> chromecastDevices = m_discovery->devices();

    for (const DeviceInfo& device : chromecastDevices) {
        if (device.isAvailable) {
            Fooyin::OutputDevice outputDevice;
            outputDevice.name = device.id;
            outputDevice.desc = QString("%1 (%2)").arg(device.friendlyName, device.modelName);
            devices.push_back(outputDevice);
        }
    }

    if (devices.empty()) {
        qInfo() << "No Chromecast devices found";
        // Add a placeholder device
        Fooyin::OutputDevice placeholder;
        placeholder.name = "";
        placeholder.desc = "No Chromecast devices found";
        devices.push_back(placeholder);
    }

    return devices;
}

int ChromecastOutput::write(const Fooyin::AudioBuffer& buffer)
{
    if (!m_initialised) {
        return 0;
    }

    // Buffer the audio data for position tracking
    // In the hybrid approach, we receive audio buffers here but stream
    // the original file to Chromecast (see Phase 3 implementation)
    const int bytesCount = buffer.byteCount();
    m_audioBuffer.append(reinterpret_cast<const char*>(buffer.data()), bytesCount);

    // Track playback position
    m_samplesWritten += buffer.sampleCount();

    // Update last known position for seek detection
    if (m_playerController) {
        m_lastPosition = m_playerController->currentPosition();
    }

    // Keep buffer from growing too large (retain only last 1MB for position tracking)
    const int maxBufferSize = 1024 * 1024;
    if (m_audioBuffer.size() > maxBufferSize) {
        m_audioBuffer.remove(0, m_audioBuffer.size() - maxBufferSize);
    }

    // Return the number of samples written (consumed)
    return buffer.sampleCount();
}

void ChromecastOutput::setPaused(bool pause)
{
    qInfo() << "ChromecastOutput::setPaused -" << pause;

    if (pause && !m_isPaused) {
        // Pausing: save elapsed time
        if (m_playbackTimer.isValid()) {
            m_pausedElapsed += m_playbackTimer.elapsed();
        }
    } else if (!pause && m_isPaused) {
        // Resuming: restart the timer
        m_playbackTimer.restart();
    }

    m_isPaused = pause;

    if (m_communication && m_communication->isConnected()) {
        if (pause) {
            m_communication->pause();
        } else {
            // Resume playback
            // The actual resume is handled by sending a play command
            // This will be implemented when we integrate with the Cast protocol
        }
    }
}

void ChromecastOutput::setVolume(double volume)
{
    qInfo() << "ChromecastOutput::setVolume -" << volume;

    m_volume = volume;

    if (m_communication && m_communication->isConnected()) {
        // Convert from 0.0-1.0 to 0-100
        int volumePercent = static_cast<int>(volume * 100);
        m_communication->setVolume(volumePercent);
    }
}

void ChromecastOutput::setDevice(const QString& device)
{
    qInfo() << "ChromecastOutput::setDevice -" << device;

    if (m_selectedDevice == device) {
        return; // Already using this device
    }

    m_selectedDevice = device;

    if (device.isEmpty()) {
        // Disconnect from current device
        if (m_communication && m_communication->isConnected()) {
            m_communication->disconnectFromDevice();
        }
        return;
    }

    // Try to find the device and connect
    if (m_discovery) {
        QList<DeviceInfo> devices = m_discovery->devices();

        if (devices.isEmpty()) {
            qInfo() << "ChromecastOutput::setDevice - Device list empty, discovery may still be running";
            qInfo() << "ChromecastOutput::setDevice - Will retry connection when init() is called";
            return;
        }

        for (const DeviceInfo& deviceInfo : devices) {
            if (deviceInfo.id == device) {
                if (m_communication) {
                    qInfo() << "ChromecastOutput::setDevice - Connecting to device:" << deviceInfo.friendlyName;
                    m_communication->connectToDevice(deviceInfo);
                }
                return;
            }
        }

        qWarning() << "ChromecastOutput::setDevice - Device not found in discovery list:" << device;
    }
}

Fooyin::AudioFormat ChromecastOutput::format() const
{
    return m_format;
}

QString ChromecastOutput::error() const
{
    return m_errorString;
}

void ChromecastOutput::onTrackChanged(const Fooyin::Track& track)
{
    qInfo() << "ChromecastOutput::onTrackChanged -" << track.title();

    if (!m_initialised) {
        qWarning() << "ChromecastOutput not initialized, ignoring track change";
        return;
    }

    if (!m_communication || !m_communication->isConnected()) {
        qWarning() << "Not connected to Chromecast device, ignoring track change";
        return;
    }

    // Start streaming the new track
    startStreaming(track);
}

void ChromecastOutput::onPlayStateChanged(Fooyin::Player::PlayState state)
{
    qInfo() << "ChromecastOutput::onPlayStateChanged -" << static_cast<int>(state);

    if (!m_initialised || !m_communication || !m_communication->isConnected()) {
        return;
    }

    // Handle play state changes
    switch (state) {
        case Fooyin::Player::PlayState::Playing:
            if (m_isPaused) {
                // Resume playback
                setPaused(false);
            }
            break;
        case Fooyin::Player::PlayState::Paused:
            setPaused(true);
            break;
        case Fooyin::Player::PlayState::Stopped:
            m_communication->stop();
            m_isStreaming = false;
            m_currentTrackPath.clear();
            break;
    }
}

void ChromecastOutput::startStreaming(const Fooyin::Track& track)
{
    qInfo() << "ChromecastOutput::startStreaming - Track:" << track.title();

    QString filePath = track.filepath();
    if (filePath.isEmpty()) {
        qWarning() << "Track has empty file path";
        return;
    }

    // If we're already streaming a different track, stop it first
    if (m_isStreaming && m_currentTrackPath != filePath) {
        qInfo() << "Stopping previous stream before starting new one";
        if (m_communication && m_communication->isConnected()) {
            m_communication->stop();
        }
        m_isStreaming = false;
    }

    m_currentTrackPath = filePath;

    // Reset samples counter for new track (critical for correct position tracking)
    m_samplesWritten = 0;

    // Reset and start the playback timer for real-time position tracking
    m_pausedElapsed = 0;
    m_playbackTimer.start();

    // Check if file needs transcoding
    QString streamUrl;
    if (needsTranscoding(filePath)) {
        qInfo() << "Track requires transcoding:" << filePath;

        // Trigger transcoding
        if (m_transcoder) {
            // Create temporary file path for transcoded output
            QString tempDir = QDir::tempPath() + "/fooyin-chromecast";
            QDir().mkpath(tempDir);
            QFileInfo fileInfo(filePath);
            QString transcodedPath = QString("%1/%2.mp3").arg(tempDir, fileInfo.baseName());

            if (m_transcoder->transcodeFile(filePath, transcodedPath)) {
                streamUrl = m_httpServer->createMediaUrl(transcodedPath);
            } else {
                qWarning() << "Transcoding failed for:" << filePath;
                return;
            }
        } else {
            qWarning() << "Transcoder not available";
            return;
        }
    } else {
        // Stream original file via HTTP server
        qInfo() << "Streaming original file:" << filePath;
        if (m_httpServer) {
            streamUrl = m_httpServer->createMediaUrl(filePath);
        } else {
            qWarning() << "HTTP server not available";
            return;
        }
    }

    if (streamUrl.isEmpty()) {
        qWarning() << "Failed to create media URL";
        return;
    }

    // Extract metadata
    QString title = track.title();
    QString artist = track.artist();
    QString album = track.album();
    QString coverUrl; // TODO: Extract cover art URL

    qInfo() << "Sending LOAD command to Chromecast - URL:" << streamUrl;
    qInfo() << "  Title:" << title << "Artist:" << artist << "Album:" << album;

    // Send LOAD command to Chromecast
    m_communication->play(streamUrl, title, artist, album, coverUrl);
    m_isStreaming = true;
}

bool ChromecastOutput::needsTranscoding(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();

    // Chromecast natively supports these formats
    QStringList nativeFormats = {"mp3", "aac", "m4a", "opus", "flac", "ogg", "wav"};

    return !nativeFormats.contains(extension);
}

} // namespace Chromecast
