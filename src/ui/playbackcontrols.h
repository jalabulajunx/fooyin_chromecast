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

#include <gui/fywidget.h>
#include <chromecast/chromecast_common.h>

namespace Chromecast {

class CommunicationManager;

namespace Ui {
class PlaybackControls;
}

class PlaybackControls : public Fooyin::FyWidget
{
    Q_OBJECT

public:
    explicit PlaybackControls(CommunicationManager* communication, QWidget* parent = nullptr);
    ~PlaybackControls() override;

    QString name() const override;
    QString layoutName() const override;

signals:
    void playClicked();
    void pauseClicked();
    void stopClicked();
    void volumeChanged(int volume);
    void seekRequested(int position);

public slots:
    void onPlaybackStatusChanged(Chromecast::PlaybackStatus status);
    void onVolumeChanged(int volume);
    void onPositionChanged(int position);
    void onTrackChanged(const QString& title, const QString& artist, const QString& album);
    void setConnected(bool connected);

private slots:
    void onPlayButtonClicked();
    void onPauseButtonClicked();
    void onStopButtonClicked();
    void onVolumeSliderChanged(int value);
    void onSeekSliderChanged(int value);

private:
    void updatePlaybackButtons(Chromecast::PlaybackStatus status);

    Ui::PlaybackControls* ui;
    CommunicationManager* m_communication{nullptr};
    bool m_isConnected{false};
};

} // namespace Chromecast
