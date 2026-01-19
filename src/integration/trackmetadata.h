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

#include <QObject>
#include <QString>
#include <QPixmap>

namespace Chromecast {

struct TrackMetadata
{
    QString title;
    QString artist;
    QString album;
    QString coverPath;
    QString filePath;
    int duration{0}; // in milliseconds
};

class TrackMetadataExtractor : public QObject
{
    Q_OBJECT

public:
    explicit TrackMetadataExtractor(QObject* parent = nullptr);

    TrackMetadata extractMetadata(const QString& filePath);
    QPixmap extractCoverArt(const QString& filePath);
};

} // namespace Chromecast
