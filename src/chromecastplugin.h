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

#include <core/plugins/coreplugin.h>
#include <core/plugins/plugin.h>
#include <gui/plugins/guiplugin.h>
#include <core/engine/outputplugin.h>
#include <chromecast/chromecast_common.h>

namespace Fooyin {
class SettingsManager;
class WidgetProvider;
class ActionManager;
class TrackSelectionController;
class PlayerController;
}

namespace Chromecast {

class DiscoveryManager;
class CommunicationManager;
class HttpServer;
class TranscodingManager;
class TrackMetadataExtractor;
class PlaybackIntegrator;
class DeviceWidget;
class ChromecastSettingsPage;

class ChromecastPlugin : public QObject,
                         public Fooyin::Plugin,
                         public Fooyin::OutputPlugin,
                         public Fooyin::CorePlugin,
                         public Fooyin::GuiPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.fooyin.fooyin.plugin/1.0" FILE "../metadata.json")
    Q_INTERFACES(Fooyin::Plugin Fooyin::OutputPlugin Fooyin::CorePlugin Fooyin::GuiPlugin)

public:
    // OutputPlugin interface
    [[nodiscard]] QString name() const override;
    [[nodiscard]] Fooyin::OutputCreator creator() const override;

    // CorePlugin interface
    void initialise(const Fooyin::CorePluginContext& context) override;

    // GuiPlugin interface
    void initialise(const Fooyin::GuiPluginContext& context) override;

private slots:
    void onDeviceSelected(const QString& deviceId);
    void onPlaybackStatusChanged(Chromecast::PlaybackStatus status);
    void onConnectionStatusChanged(Chromecast::ConnectionStatus status);

private:
    Fooyin::SettingsManager* m_settings{nullptr};
    Fooyin::WidgetProvider* m_widgetProvider{nullptr};
    Fooyin::ActionManager* m_actionManager{nullptr};
    Fooyin::PlayerController* m_playerController{nullptr};

    DiscoveryManager* m_discoveryManager{nullptr};
    CommunicationManager* m_communicationManager{nullptr};
    HttpServer* m_httpServer{nullptr};
    TranscodingManager* m_transcodingManager{nullptr};
    TrackMetadataExtractor* m_metadataExtractor{nullptr};
    PlaybackIntegrator* m_playbackIntegrator{nullptr};

    DeviceWidget* m_deviceWidget{nullptr};
    ChromecastSettingsPage* m_settingsPage{nullptr};

    QString m_selectedDeviceId; // Currently selected Chromecast device
};

} // namespace Chromecast
