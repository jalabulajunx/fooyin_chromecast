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

#include "chromecastplugin.h"

#include "core/discoverymanager.h"
#include "core/communicationmanager.h"
#include "core/httpserver.h"
#include "core/transcodingmanager.h"
#include "core/chromecastoutput.h"
#include "integration/trackmetadata.h"
#include "integration/playbackintegrator.h"
#include "ui/devicewidget.h"
#include "ui/chromecastsettingspage.h"

#include <gui/widgetprovider.h>
#include <utils/settings/settingsmanager.h>
#include <utils/actions/actionmanager.h>
#include <core/track.h>
#include <core/engine/enginecontroller.h>

#include <QDebug>
#include <memory>

namespace Chromecast {

QString ChromecastPlugin::name() const
{
    return QStringLiteral("Chromecast");
}

Fooyin::OutputCreator ChromecastPlugin::creator() const
{
    // Return a factory function that creates ChromecastOutput instances
    return [this]() -> std::unique_ptr<Fooyin::AudioOutput> {
        auto output = std::make_unique<ChromecastOutput>(
            m_discoveryManager,
            m_communicationManager,
            m_httpServer,
            m_transcodingManager,
            m_metadataExtractor,
            m_playerController
        );

        // Set the selected device if one has been chosen
        if (!m_selectedDeviceId.isEmpty()) {
            output->setDevice(m_selectedDeviceId);
        }

        return output;
    };
}

void ChromecastPlugin::initialise(const Fooyin::CorePluginContext& context)
{
    qInfo() << "Initializing Chromecast plugin core";

    m_settings = context.settingsManager;
    m_playerController = context.playerController;
    m_audioLoader = context.audioLoader;

    // Initialize core managers
    m_discoveryManager = new DiscoveryManager(this);
    m_communicationManager = new CommunicationManager(this);
    m_httpServer = new HttpServer(m_audioLoader, this);
    m_transcodingManager = new TranscodingManager(this);
    m_metadataExtractor = new TrackMetadataExtractor(this);
    m_playbackIntegrator = new PlaybackIntegrator(m_communicationManager, m_httpServer,
                                                   m_transcodingManager, m_metadataExtractor, this);

    // Start HTTP server - use default port 8010 if setting doesn't exist
    quint16 serverPort = 8010;
    if (m_settings->contains("Chromecast/ServerPort")) {
        serverPort = m_settings->value("Chromecast/ServerPort").toInt();
    }
    qInfo() << "Starting HTTP server on port" << serverPort;
    if (!m_httpServer->start(serverPort)) {
        qWarning() << "Failed to start HTTP server on port" << serverPort;
    } else {
        qInfo() << "HTTP server started successfully on port" << m_httpServer->serverPort();
    }

    // Connect signals
    connect(m_communicationManager, &CommunicationManager::connectionStatusChanged,
            this, &ChromecastPlugin::onConnectionStatusChanged);
    connect(m_communicationManager, &CommunicationManager::playbackStatusChanged,
            this, &ChromecastPlugin::onPlaybackStatusChanged);

    // Register Chromecast as an audio output
    if (auto* engineController = context.engine) {
        qInfo() << "Registering Chromecast output with EngineController";
        engineController->addOutput(name(), creator());
    } else {
        qWarning() << "EngineController not available, cannot register Chromecast output";
    }
}

void ChromecastPlugin::initialise(const Fooyin::GuiPluginContext& context)
{
    qInfo() << "Initializing Chromecast plugin GUI";

    m_widgetProvider = context.widgetProvider;
    m_actionManager = context.actionManager;

    // Create UI components
    m_deviceWidget = new DeviceWidget(m_discoveryManager, m_communicationManager);
    m_settingsPage = new ChromecastSettingsPage(m_settings, m_transcodingManager, m_discoveryManager, m_communicationManager);

    // Register widgets
    m_widgetProvider->registerWidget(
        "ChromecastDeviceSelector",
        [this]() { return new DeviceWidget(m_discoveryManager, m_communicationManager); },
        "Chromecast Device Selector"
    );

    // Connect signals
    connect(m_deviceWidget, &DeviceWidget::deviceSelected, this, &ChromecastPlugin::onDeviceSelected);

    qInfo() << "Chromecast plugin initialized successfully";
}

void ChromecastPlugin::onDeviceSelected(const QString& deviceId)
{
    qInfo() << "Device selected:" << deviceId;
    m_selectedDeviceId = deviceId;

    // Find the device info and connect
    if (m_discoveryManager && m_communicationManager) {
        const auto& devices = m_discoveryManager->devices();
        for (const auto& device : devices) {
            QString deviceIdentifier = QString("%1:%2").arg(device.ipAddress.toString(), QString::number(device.port));
            if (deviceIdentifier == deviceId) {
                qInfo() << "Connecting to device:" << device.name;
                m_communicationManager->connectToDevice(device);
                break;
            }
        }
    }
}

void ChromecastPlugin::onPlaybackStatusChanged(Chromecast::PlaybackStatus status)
{
    qInfo() << "Playback status changed:" << static_cast<int>(status);
    // Playback status will be handled by ChromecastOutput when implemented
}

void ChromecastPlugin::onConnectionStatusChanged(Chromecast::ConnectionStatus status)
{
    qInfo() << "Connection status changed:" << static_cast<int>(status);
    // Connection status will be handled by ChromecastOutput when implemented
}

} // namespace Chromecast
