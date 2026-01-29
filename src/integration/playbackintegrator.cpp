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

#include "playbackintegrator.h"

#include "trackmetadata.h"
#include "../core/communicationmanager.h"
#include "../core/httpserver.h"
#include "../core/transcodingmanager.h"
#include <core/track.h>

#include <QFileInfo>
#include <QTemporaryFile>
#include <QDebug>

namespace Chromecast {

PlaybackIntegrator::PlaybackIntegrator(CommunicationManager* communication, HttpServer* httpServer,
                                       TranscodingManager* transcoder, TrackMetadataExtractor* metadataExtractor,
                                       QObject* parent)
    : QObject(parent)
    , m_communication(communication)
    , m_httpServer(httpServer)
    , m_transcoder(transcoder)
    , m_metadataExtractor(metadataExtractor)
{
    connect(m_transcoder, &TranscodingManager::transcodingFinished, this, &PlaybackIntegrator::onTranscodingFinished);
    connect(m_transcoder, &TranscodingManager::transcodingError, this, &PlaybackIntegrator::onTranscodingError);
}

PlaybackIntegrator::~PlaybackIntegrator()
{
    stop();
}

void PlaybackIntegrator::playTrack(const Fooyin::Track& track)
{
    qInfo() << "Preparing to play track:" << track.title();

    // Stop any ongoing playback
    stop();

    // Save current track path
    m_currentTrackPath = track.filepath();

    // Extract metadata
    TrackMetadata metadata = m_metadataExtractor->extractMetadata(m_currentTrackPath);

    // Check if format is supported
    if (m_transcoder->isFormatSupported(m_currentTrackPath)) {
        qInfo() << "Track format is supported natively";
        QString mediaUrl = m_httpServer->createMediaUrl(m_currentTrackPath);
        m_communication->play(mediaUrl, metadata.title, metadata.artist, metadata.album, metadata.coverPath);
        emit playbackStarted();
        emit trackChanged(metadata.title, metadata.artist, metadata.album);
    }
    else {
        qInfo() << "Track format requires transcoding";
        prepareTrackForPlayback(track);
    }
}

void PlaybackIntegrator::pause()
{
    m_communication->pause();
    emit playbackPaused();
}

void PlaybackIntegrator::stop()
{
    m_communication->stop();
    emit playbackStopped();
    m_currentTrackPath.clear();
}

void PlaybackIntegrator::seek(int position)
{
    m_communication->seek(position);
}

void PlaybackIntegrator::setVolume(int volume)
{
    m_communication->setVolume(volume);
}

void PlaybackIntegrator::onTranscodingFinished(const QString& sourcePath, const QString& destPath)
{
    if (sourcePath == m_currentTrackPath) {
        qInfo() << "Transcoding finished, playing track";

        TrackMetadata metadata = m_metadataExtractor->extractMetadata(sourcePath);
        QString mediaUrl = m_httpServer->createMediaUrl(destPath);

        m_communication->play(mediaUrl, metadata.title, metadata.artist, metadata.album, metadata.coverPath);
        emit playbackStarted();
        emit trackChanged(metadata.title, metadata.artist, metadata.album);
    }
}

void PlaybackIntegrator::onTranscodingError(const QString& sourcePath, const QString& errorMsg)
{
    if (sourcePath == m_currentTrackPath) {
        qWarning() << "Transcoding error:" << errorMsg;
        emit error(errorMsg);
        m_currentTrackPath.clear();
    }
}

void PlaybackIntegrator::prepareTrackForPlayback(const Fooyin::Track& track)
{
    QFileInfo fileInfo(track.filepath());
    QString tempFileName = QString("%1_%2").arg(fileInfo.completeBaseName()).arg("transcoded");
    QTemporaryFile* tempFile = new QTemporaryFile(tempFileName + "XXXXXX.mp3", this);

    if (tempFile->open()) {
        QString destPath = tempFile->fileName();
        tempFile->close();

        qInfo() << "Creating transcoded file:" << destPath;

        m_transcoder->transcodeFile(track.filepath(), destPath);
    }
    else {
        QString errorMsg = "Failed to create temporary file for transcoding";
        qWarning() << errorMsg;
        emit error(errorMsg);
    }
}

} // namespace Chromecast
