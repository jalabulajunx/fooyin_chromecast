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

#include <chromecast/chromecast_common.h>

#include <QObject>
#include <QProcess>

namespace Chromecast {

class TranscodingManager : public QObject
{
    Q_OBJECT

public:
    explicit TranscodingManager(QObject* parent = nullptr);
    ~TranscodingManager() override;

    bool isFormatSupported(const QString& filePath) const;
    bool transcodeFile(const QString& sourcePath, const QString& destPath,
                       TranscodingFormat format = TranscodingFormat::AAC,
                       TranscodingQuality quality = TranscodingQuality::High);
    QString supportedFormats() const;
    QString formatName(TranscodingFormat format) const;
    QString qualityName(TranscodingQuality quality) const;

signals:
    void transcodingStarted(const QString& sourcePath);
    void transcodingProgress(const QString& sourcePath, int progress);
    void transcodingFinished(const QString& sourcePath, const QString& destPath);
    void transcodingError(const QString& sourcePath, const QString& error);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onProcessOutput();

private:
    bool isProcessRunning() const;

    QProcess* m_transcodeProcess{nullptr};
    QString m_currentSourcePath;
    QString m_currentDestPath;
};

} // namespace Chromecast
