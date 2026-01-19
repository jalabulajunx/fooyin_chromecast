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

#include "chromecastsettingspage.h"
#include "devicewidget.h"

#include "../core/transcodingmanager.h"
#include "../core/discoverymanager.h"
#include "../core/communicationmanager.h"
#include <utils/settings/settingsmanager.h>

#include <QFormLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QDebug>
#include <QVBoxLayout>
#include <QComboBox>
#include <QSpinBox>

namespace Chromecast {

ChromecastSettingsPageWidget::ChromecastSettingsPageWidget(Fooyin::SettingsManager* settings, TranscodingManager* transcoder,
                                                           DiscoveryManager* discovery, CommunicationManager* communication)
    : m_settings(settings)
    , m_transcoder(transcoder)
    , m_discovery(discovery)
    , m_communication(communication)
    , m_deviceWidget(nullptr)
    , m_formatComboBox(nullptr)
    , m_qualityComboBox(nullptr)
    , m_portSpinBox(nullptr)
    , m_discoveryTimeoutSpinBox(nullptr)
{
    initializeSettings();
    setupUI();
}

void ChromecastSettingsPageWidget::load()
{
    int defaultFormat = m_settings->value("Chromecast/DefaultFormat").toInt();
    int defaultQuality = m_settings->value("Chromecast/DefaultQuality").toInt();
    int serverPort = m_settings->value("Chromecast/ServerPort").toInt();
    int discoveryTimeout = m_settings->value("Chromecast/DiscoveryTimeout").toInt();

    m_formatComboBox->setCurrentIndex(defaultFormat);
    m_qualityComboBox->setCurrentIndex(defaultQuality);
    m_portSpinBox->setValue(serverPort);
    m_discoveryTimeoutSpinBox->setValue(discoveryTimeout);
}

void ChromecastSettingsPageWidget::apply()
{
    // Save transcoding settings
    m_settings->set("Chromecast/DefaultFormat", m_formatComboBox->currentData().toInt());
    m_settings->set("Chromecast/DefaultQuality", m_qualityComboBox->currentData().toInt());

    // Save network settings
    int newPort = m_portSpinBox->value();
    int currentPort = m_settings->value("Chromecast/ServerPort").toInt();

    if (newPort != currentPort) {
        m_settings->set("Chromecast/ServerPort", newPort);
        qInfo() << "Chromecast: HTTP server port changed to" << newPort << "(restart required)";
    }

    m_settings->set("Chromecast/DiscoveryTimeout", m_discoveryTimeoutSpinBox->value());

    qInfo() << "Chromecast settings saved";
}

void ChromecastSettingsPageWidget::reset()
{
    // Note: We should use proper settings keys with the SettingsManager's template methods
    load();
}

void ChromecastSettingsPageWidget::onFormatChanged(int index)
{
    qDebug() << "Format changed to" << index;
}

void ChromecastSettingsPageWidget::onQualityChanged(int index)
{
    qDebug() << "Quality changed to" << index;
}

void ChromecastSettingsPageWidget::onPortChanged(int value)
{
    qDebug() << "Port changed to" << value;
}

void ChromecastSettingsPageWidget::onDiscoveryTimeoutChanged(int value)
{
    qDebug() << "Discovery timeout changed to" << value;
}

void ChromecastSettingsPageWidget::initializeSettings()
{
    if (!m_settings->contains("Chromecast/DefaultFormat")) {
        m_settings->createSetting("Chromecast/DefaultFormat", static_cast<int>(TranscodingFormat::AAC));
    }
    if (!m_settings->contains("Chromecast/DefaultQuality")) {
        m_settings->createSetting("Chromecast/DefaultQuality", static_cast<int>(TranscodingQuality::High));
    }
    if (!m_settings->contains("Chromecast/ServerPort")) {
        m_settings->createSetting("Chromecast/ServerPort", 8010);
    }
    if (!m_settings->contains("Chromecast/DiscoveryTimeout")) {
        m_settings->createSetting("Chromecast/DiscoveryTimeout", 10000);
    }
}

void ChromecastSettingsPageWidget::updateUi()
{
    load();
}

void ChromecastSettingsPageWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // Device selection
    auto* deviceGroup = new QGroupBox("Device Selection", this);
    auto* deviceLayout = new QVBoxLayout(deviceGroup);

    m_deviceWidget = new DeviceWidget(m_discovery, m_communication, this);
    deviceLayout->addWidget(m_deviceWidget);

    // Connect device selection to save to settings and connect
    connect(m_deviceWidget, &DeviceWidget::deviceSelected, this, [this](const QString& deviceId) {
        qInfo() << "Settings: Device selected:" << deviceId;
        m_settings->set("Chromecast/SelectedDevice", deviceId);

        // Also connect immediately if device is valid
        if (!deviceId.isEmpty() && m_discovery && m_communication) {
            const auto& devices = m_discovery->devices();
            for (const auto& device : devices) {
                if (device.id == deviceId) {
                    qInfo() << "Settings: Connecting to device:" << device.friendlyName;
                    m_communication->connectToDevice(device);
                    break;
                }
            }
        }
    });

    mainLayout->addWidget(deviceGroup);

    // Transcoding settings
    auto* transcodingGroup = new QGroupBox("Transcoding", this);
    auto* transcodingLayout = new QFormLayout(transcodingGroup);

    m_formatComboBox = new QComboBox(transcodingGroup);
    m_formatComboBox->addItem("AAC", static_cast<int>(TranscodingFormat::AAC));
    m_formatComboBox->addItem("MP3", static_cast<int>(TranscodingFormat::MP3));
    m_formatComboBox->addItem("Opus", static_cast<int>(TranscodingFormat::Opus));
    m_formatComboBox->addItem("FLAC", static_cast<int>(TranscodingFormat::FLAC));
    m_formatComboBox->addItem("Vorbis", static_cast<int>(TranscodingFormat::Vorbis));
    m_formatComboBox->addItem("WAV", static_cast<int>(TranscodingFormat::WAV));
    transcodingLayout->addRow("Default format:", m_formatComboBox);

    m_qualityComboBox = new QComboBox(transcodingGroup);
    m_qualityComboBox->addItem("High Quality", static_cast<int>(TranscodingQuality::High));
    m_qualityComboBox->addItem("Balanced", static_cast<int>(TranscodingQuality::Balanced));
    m_qualityComboBox->addItem("Efficient", static_cast<int>(TranscodingQuality::Efficient));
    transcodingLayout->addRow("Default quality:", m_qualityComboBox);

    mainLayout->addWidget(transcodingGroup);

    // Network settings
    auto* networkGroup = new QGroupBox("Network", this);
    auto* networkLayout = new QFormLayout(networkGroup);

    m_portSpinBox = new QSpinBox(networkGroup);
    m_portSpinBox->setRange(1024, 65535);
    m_portSpinBox->setValue(8010);
    networkLayout->addRow("HTTP server port:", m_portSpinBox);

    m_discoveryTimeoutSpinBox = new QSpinBox(networkGroup);
    m_discoveryTimeoutSpinBox->setRange(1000, 30000);
    m_discoveryTimeoutSpinBox->setSingleStep(1000);
    m_discoveryTimeoutSpinBox->setValue(10000);
    m_discoveryTimeoutSpinBox->setSuffix(" ms");
    networkLayout->addRow("Discovery timeout:", m_discoveryTimeoutSpinBox);

    mainLayout->addWidget(networkGroup);

    mainLayout->addStretch();

    // Connect signals
    connect(m_formatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChromecastSettingsPageWidget::onFormatChanged);
    connect(m_qualityComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChromecastSettingsPageWidget::onQualityChanged);
    connect(m_portSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ChromecastSettingsPageWidget::onPortChanged);
    connect(m_discoveryTimeoutSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ChromecastSettingsPageWidget::onDiscoveryTimeoutChanged);
}

ChromecastSettingsPage::ChromecastSettingsPage(Fooyin::SettingsManager* settings, TranscodingManager* transcoder,
                                               DiscoveryManager* discovery, CommunicationManager* communication)
    : SettingsPage{settings->settingsDialog()}
{
    setId("Chromecast.Settings");
    setName("Chromecast");
    setCategory({"Plugins"});
    qInfo() << "ChromecastSettingsPage: Registering settings page with ID:" << "Chromecast.Settings" << "Category: Plugins";
    qInfo() << "ChromecastSettingsPage: SettingsDialog pointer:" << settings->settingsDialog();
    setWidgetCreator([settings, transcoder, discovery, communication] {
        qInfo() << "ChromecastSettingsPage: Widget creator called - creating ChromecastSettingsPageWidget";
        return new ChromecastSettingsPageWidget(settings, transcoder, discovery, communication);
    });
}

} // namespace Chromecast
