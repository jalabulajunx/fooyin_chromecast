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

#include "trackmetadata.h"

#include <QFileInfo>
#include <QPixmap>
#include <QDebug>

namespace Chromecast {

TrackMetadataExtractor::TrackMetadataExtractor(QObject* parent)
    : QObject(parent)
{
}

TrackMetadata TrackMetadataExtractor::extractMetadata(const QString& filePath)
{
    TrackMetadata metadata;
    QFileInfo fileInfo(filePath);

    metadata.filePath = filePath;
    metadata.title = fileInfo.completeBaseName();
    metadata.artist = "Unknown Artist";
    metadata.album = "Unknown Album";
    metadata.coverPath = QString();
    metadata.duration = 0;

    qDebug() << "Extracted metadata for" << filePath << ":" << metadata.title << "-" << metadata.artist;

    return metadata;
}

QPixmap TrackMetadataExtractor::extractCoverArt(const QString& filePath)
{
    Q_UNUSED(filePath);

    // In production, use taglib or similar to extract embedded cover art
    QPixmap defaultCover;
    return defaultCover;
}

} // namespace Chromecast
