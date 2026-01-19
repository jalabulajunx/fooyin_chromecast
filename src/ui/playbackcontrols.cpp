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

#include "playbackcontrols.h"
#include "ui_playbackcontrols.h"

#include "../core/communicationmanager.h"

#include <QDebug>

namespace Chromecast {

PlaybackControls::PlaybackControls(CommunicationManager* communication, QWidget* parent)
    : Fooyin::FyWidget(parent)
    , ui(new Ui::PlaybackControls)
    , m_communication(communication)
{
    ui->setupUi(this);

    connect(ui->playButton, &QPushButton::clicked, this, &PlaybackControls::onPlayButtonClicked);
    connect(ui->pauseButton, &QPushButton::clicked, this, &PlaybackControls::onPauseButtonClicked);
    connect(ui->stopButton, &QPushButton::clicked, this, &PlaybackControls::onStopButtonClicked);
    connect(ui->volumeSlider, &QSlider::valueChanged, this, &PlaybackControls::onVolumeSliderChanged);
    connect(ui->seekSlider, &QSlider::valueChanged, this, &PlaybackControls::onSeekSliderChanged);

    // Initially disabled
    setConnected(false);
}

PlaybackControls::~PlaybackControls()
{
    delete ui;
}

void PlaybackControls::onPlaybackStatusChanged(Chromecast::PlaybackStatus status)
{
    qInfo() << "Playback status changed:" << static_cast<int>(status);
    updatePlaybackButtons(status);
}

void PlaybackControls::onVolumeChanged(int volume)
{
    ui->volumeSlider->setValue(volume);
    ui->volumeValueLabel->setText(QString("%1%").arg(volume));
}

void PlaybackControls::onPositionChanged(int position)
{
    ui->seekSlider->setValue(position);
}

void PlaybackControls::onTrackChanged(const QString& title, const QString& artist, const QString& album)
{
    QString trackInfo = QString("%1 - %2").arg(artist).arg(title);
    if (!album.isEmpty()) {
        trackInfo += QString(" (%1)").arg(album);
    }

    ui->trackLabel->setText(trackInfo);
}

void PlaybackControls::setConnected(bool connected)
{
    m_isConnected = connected;
    ui->playButton->setEnabled(connected);
    ui->pauseButton->setEnabled(connected);
    ui->stopButton->setEnabled(connected);
    ui->volumeSlider->setEnabled(connected);
    ui->seekSlider->setEnabled(connected);

    if (!connected) {
        ui->trackLabel->setText("Disconnected");
        updatePlaybackButtons(PlaybackStatus::Idle);
    }
}

void PlaybackControls::onPlayButtonClicked()
{
    emit playClicked();
}

void PlaybackControls::onPauseButtonClicked()
{
    emit pauseClicked();
}

void PlaybackControls::onStopButtonClicked()
{
    emit stopClicked();
}

void PlaybackControls::onVolumeSliderChanged(int value)
{
    emit volumeChanged(value);
    ui->volumeValueLabel->setText(QString("%1%").arg(value));
}

void PlaybackControls::onSeekSliderChanged(int value)
{
    emit seekRequested(value);
}

void PlaybackControls::updatePlaybackButtons(Chromecast::PlaybackStatus status)
{
    switch (status) {
        case PlaybackStatus::Playing:
            ui->playButton->setEnabled(false);
            ui->pauseButton->setEnabled(true);
            ui->stopButton->setEnabled(true);
            break;
        case PlaybackStatus::Paused:
            ui->playButton->setEnabled(true);
            ui->pauseButton->setEnabled(false);
            ui->stopButton->setEnabled(true);
            break;
        case PlaybackStatus::Loading:
        case PlaybackStatus::Buffering:
            ui->playButton->setEnabled(false);
            ui->pauseButton->setEnabled(false);
            ui->stopButton->setEnabled(false);
            break;
        default:
            ui->playButton->setEnabled(m_isConnected);
            ui->pauseButton->setEnabled(false);
            ui->stopButton->setEnabled(m_isConnected);
            break;
    }
}

QString PlaybackControls::name() const
{
    return "Chromecast Playback Controls";
}

QString PlaybackControls::layoutName() const
{
    return QStringLiteral("ChromecastPlaybackControls");
}

} // namespace Chromecast
