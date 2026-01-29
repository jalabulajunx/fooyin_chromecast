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

#include <QObject>

namespace Fooyin {
class Track;
}

namespace Chromecast {

class CommunicationManager;
class HttpServer;
class TranscodingManager;
class TrackMetadataExtractor;

class PlaybackIntegrator : public QObject
{
    Q_OBJECT

public:
    explicit PlaybackIntegrator(CommunicationManager* communication, HttpServer* httpServer,
                                TranscodingManager* transcoder, TrackMetadataExtractor* metadataExtractor,
                                QObject* parent = nullptr);
    ~PlaybackIntegrator() override;

    void playTrack(const Fooyin::Track& track);
    void pause();
    void stop();
    void seek(int position);
    void setVolume(int volume);

signals:
    void playbackStarted();
    void playbackPaused();
    void playbackStopped();
    void trackChanged(const QString& title, const QString& artist, const QString& album);
    void error(const QString& message);

private slots:
    void onTranscodingFinished(const QString& sourcePath, const QString& destPath);
    void onTranscodingError(const QString& sourcePath, const QString& error);

private:
    void prepareTrackForPlayback(const Fooyin::Track& track);

    CommunicationManager* m_communication{nullptr};
    HttpServer* m_httpServer{nullptr};
    TranscodingManager* m_transcoder{nullptr};
    TrackMetadataExtractor* m_metadataExtractor{nullptr};

    QString m_currentTrackPath;
};

} // namespace Chromecast
