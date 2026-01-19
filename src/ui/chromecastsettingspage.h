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

#include <utils/settings/settingspage.h>

#include <QComboBox>
#include <QSpinBox>

namespace Fooyin {
class SettingsManager;
}

namespace Chromecast {

class TranscodingManager;
class DeviceWidget;
class DiscoveryManager;
class CommunicationManager;

class ChromecastSettingsPageWidget : public Fooyin::SettingsPageWidget
{
    Q_OBJECT

public:
    explicit ChromecastSettingsPageWidget(Fooyin::SettingsManager* settings, TranscodingManager* transcoder,
                                          DiscoveryManager* discovery, CommunicationManager* communication);

    void load() override;
    void apply() override;
    void reset() override;

private slots:
    void onFormatChanged(int index);
    void onQualityChanged(int index);
    void onPortChanged(int value);
    void onDiscoveryTimeoutChanged(int value);

private:
    void initializeSettings();
    void updateUi();
    void setupUI();

    Fooyin::SettingsManager* m_settings;
    TranscodingManager* m_transcoder;
    DiscoveryManager* m_discovery;
    CommunicationManager* m_communication;
    DeviceWidget* m_deviceWidget;
    QComboBox* m_formatComboBox;
    QComboBox* m_qualityComboBox;
    QSpinBox* m_portSpinBox;
    QSpinBox* m_discoveryTimeoutSpinBox;
};

class ChromecastSettingsPage : public Fooyin::SettingsPage
{
    Q_OBJECT

public:
    explicit ChromecastSettingsPage(Fooyin::SettingsManager* settings, TranscodingManager* transcoder,
                                     DiscoveryManager* discovery, CommunicationManager* communication);
};

} // namespace Chromecast
