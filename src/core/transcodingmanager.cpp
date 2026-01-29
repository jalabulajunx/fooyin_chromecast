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

#include "transcodingmanager.h"

#include <QFileInfo>
#include <QProcess>
#include <QDebug>

namespace Chromecast {

TranscodingManager::TranscodingManager(QObject* parent)
    : QObject(parent)
    , m_transcodeProcess(new QProcess(this))
{
    connect(m_transcodeProcess, &QProcess::finished, this, &TranscodingManager::onProcessFinished);
    connect(m_transcodeProcess, &QProcess::errorOccurred, this, &TranscodingManager::onProcessError);
    connect(m_transcodeProcess, &QProcess::readyReadStandardOutput, this, &TranscodingManager::onProcessOutput);
    connect(m_transcodeProcess, &QProcess::readyReadStandardError, this, &TranscodingManager::onProcessOutput);
}

TranscodingManager::~TranscodingManager()
{
    if (m_transcodeProcess->state() == QProcess::Running) {
        m_transcodeProcess->kill();
        m_transcodeProcess->waitForFinished(3000);
    }
}

bool TranscodingManager::isFormatSupported(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();

    // Chromecast natively supports:
    // AAC, MP3, Opus, FLAC, Vorbis, WAV
    QStringList supportedExtensions = {"aac", "mp3", "opus", "flac", "ogg", "wav"};

    return supportedExtensions.contains(extension);
}

bool TranscodingManager::transcodeFile(const QString& sourcePath, const QString& destPath,
                                      TranscodingFormat format, TranscodingQuality quality)
{
    if (!QFile::exists(sourcePath)) {
        qWarning() << "Source file does not exist:" << sourcePath;
        emit transcodingError(sourcePath, "Source file does not exist");
        return false;
    }

    if (isProcessRunning()) {
        qWarning() << "Transcoding process already running";
        emit transcodingError(sourcePath, "Transcoding process already running");
        return false;
    }

    qInfo() << "Starting transcoding:" << sourcePath << "to" << destPath;

    m_currentSourcePath = sourcePath;
    m_currentDestPath = destPath;

    // Build ffmpeg arguments
    QStringList args;
    args << "-y";  // Overwrite output file
    args << "-i" << sourcePath;

    // Set codec based on format
    switch (format) {
        case TranscodingFormat::MP3:
            args << "-codec:a" << "libmp3lame";
            break;
        case TranscodingFormat::AAC:
            args << "-codec:a" << "aac";
            break;
        case TranscodingFormat::Opus:
            args << "-codec:a" << "libopus";
            break;
        case TranscodingFormat::FLAC:
            args << "-codec:a" << "flac";
            break;
        case TranscodingFormat::Vorbis:
            args << "-codec:a" << "libvorbis";
            break;
        case TranscodingFormat::WAV:
            args << "-codec:a" << "pcm_s16le";
            break;
    }

    // Set bitrate/quality based on quality setting
    switch (quality) {
        case TranscodingQuality::High:
            if (format == TranscodingFormat::MP3) {
                args << "-b:a" << "320k";
            } else if (format == TranscodingFormat::AAC) {
                args << "-b:a" << "256k";
            } else if (format == TranscodingFormat::Opus) {
                args << "-b:a" << "192k";
            } else if (format == TranscodingFormat::Vorbis) {
                args << "-q:a" << "8";
            }
            break;
        case TranscodingQuality::Balanced:
            if (format == TranscodingFormat::MP3) {
                args << "-b:a" << "192k";
            } else if (format == TranscodingFormat::AAC) {
                args << "-b:a" << "160k";
            } else if (format == TranscodingFormat::Opus) {
                args << "-b:a" << "128k";
            } else if (format == TranscodingFormat::Vorbis) {
                args << "-q:a" << "5";
            }
            break;
        case TranscodingQuality::Efficient:
            if (format == TranscodingFormat::MP3) {
                args << "-b:a" << "128k";
            } else if (format == TranscodingFormat::AAC) {
                args << "-b:a" << "96k";
            } else if (format == TranscodingFormat::Opus) {
                args << "-b:a" << "96k";
            } else if (format == TranscodingFormat::Vorbis) {
                args << "-q:a" << "3";
            }
            break;
    }

    // Add output file
    args << destPath;

    qInfo() << "Running: ffmpeg" << args.join(" ");

    emit transcodingStarted(sourcePath);
    m_transcodeProcess->start("ffmpeg", args);

    // Wait briefly to check if process started
    if (!m_transcodeProcess->waitForStarted(1000)) {
        qWarning() << "Failed to start ffmpeg:" << m_transcodeProcess->errorString();
        qWarning() << "Note: Install ffmpeg for audio transcoding support";
        emit transcodingError(sourcePath, "Failed to start ffmpeg. Please install ffmpeg.");
        m_currentSourcePath.clear();
        m_currentDestPath.clear();
        return false;
    }

    return true;
}

QString TranscodingManager::supportedFormats() const
{
    return "MP3, AAC, FLAC, Opus, Vorbis, WAV";
}

QString TranscodingManager::formatName(TranscodingFormat format) const
{
    switch (format) {
        case TranscodingFormat::AAC:
            return "AAC";
        case TranscodingFormat::MP3:
            return "MP3";
        case TranscodingFormat::Opus:
            return "Opus";
        case TranscodingFormat::FLAC:
            return "FLAC";
        case TranscodingFormat::Vorbis:
            return "Vorbis";
        case TranscodingFormat::WAV:
            return "WAV";
        default:
            return "Unknown";
    }
}

QString TranscodingManager::qualityName(TranscodingQuality quality) const
{
    switch (quality) {
        case TranscodingQuality::High:
            return "High Quality";
        case TranscodingQuality::Balanced:
            return "Balanced";
        case TranscodingQuality::Efficient:
            return "Efficient";
        default:
            return "Unknown";
    }
}

void TranscodingManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        qInfo() << "Transcoding finished successfully:" << m_currentSourcePath;
        emit transcodingFinished(m_currentSourcePath, m_currentDestPath);
    }
    else {
        QString errorMsg = QString("Transcoding failed with code %1").arg(exitCode);
        qWarning() << errorMsg;
        emit transcodingError(m_currentSourcePath, errorMsg);
    }

    m_currentSourcePath.clear();
    m_currentDestPath.clear();
}

void TranscodingManager::onProcessError(QProcess::ProcessError error)
{
    QString errorMsg;
    switch (error) {
        case QProcess::FailedToStart:
            errorMsg = "Failed to start transcoding process";
            break;
        case QProcess::Crashed:
            errorMsg = "Transcoding process crashed";
            break;
        case QProcess::Timedout:
            errorMsg = "Transcoding timed out";
            break;
        default:
            errorMsg = "Unknown transcoding error";
            break;
    }

    qWarning() << errorMsg << ":" << m_transcodeProcess->errorString();
    emit transcodingError(m_currentSourcePath, errorMsg);

    m_currentSourcePath.clear();
    m_currentDestPath.clear();
}

void TranscodingManager::onProcessOutput()
{
    QByteArray output = m_transcodeProcess->readAllStandardOutput();
    QByteArray error = m_transcodeProcess->readAllStandardError();

    if (!output.isEmpty()) {
        qDebug() << "Transcoding output:" << QString::fromUtf8(output).trimmed();
    }

    if (!error.isEmpty()) {
        qWarning() << "Transcoding error:" << QString::fromUtf8(error).trimmed();
    }
}


bool TranscodingManager::isProcessRunning() const
{
    return m_transcodeProcess->state() == QProcess::Running;
}

} // namespace Chromecast
