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

#include "devicewidget.h"
#include "ui_devicewidget.h"

#include "../core/discoverymanager.h"
#include "../core/communicationmanager.h"
#include "../core/device.h"

#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include <QTransform>

namespace Chromecast {

DeviceWidget::DeviceWidget(DiscoveryManager* discovery, CommunicationManager* communication, QWidget* parent)
    : Fooyin::FyWidget(parent)
    , ui(new Ui::DeviceWidget)
    , m_discovery(discovery)
    , m_communication(communication)
    , m_spinnerTimer(new QTimer(this))
{
    ui->setupUi(this);

    connect(ui->refreshButton, &QPushButton::clicked, this, &DeviceWidget::onRefreshButtonClicked);
    connect(ui->deviceComboBox, &QComboBox::currentIndexChanged, this, &DeviceWidget::onDeviceComboBoxChanged);
    connect(m_discovery, &DiscoveryManager::deviceDiscovered, this, &DeviceWidget::onDeviceDiscovered);
    connect(m_discovery, &DiscoveryManager::deviceLost, this, &DeviceWidget::onDeviceLost);
    connect(m_discovery, &DiscoveryManager::discoveryFinished, this, &DeviceWidget::onDiscoveryFinished);
    connect(m_communication, &CommunicationManager::connectionStatusChanged, this, &DeviceWidget::onConnectionStatusChanged);
    connect(m_spinnerTimer, &QTimer::timeout, this, &DeviceWidget::updateSpinner);

    // Show discovery in progress
    ui->discoveryLabel->setText("Searching for Chromecast devices...");
    ui->deviceComboBox->setEnabled(false);

    // Start spinner animation
    startSpinner();

    // Start initial discovery
    m_discovery->startDiscovery();
}

DeviceWidget::~DeviceWidget()
{
    delete ui;
}

void DeviceWidget::onRefreshButtonClicked()
{
    qInfo() << "Refreshing device list";
    ui->refreshButton->setEnabled(false);
    ui->refreshButton->setText("Searching...");
    ui->deviceComboBox->clear();
    ui->deviceComboBox->setEnabled(false);
    ui->discoveryLabel->setText("Searching for Chromecast devices...");
    startSpinner();
    m_discovery->startDiscovery();
}

void DeviceWidget::onDeviceComboBoxChanged(int index)
{
    if (index < 0) {
        return;
    }

    QString deviceId = ui->deviceComboBox->itemData(index).toString();

    if (!deviceId.isEmpty()) {
        qInfo() << "Device selected:" << deviceId;
        // Just emit the signal - the plugin will handle the connection
        emit deviceSelected(deviceId);
    }
    else {
        // "Select a device" item selected
        emit deviceSelected(QString());
    }
}

void DeviceWidget::onDeviceDiscovered(const Chromecast::DeviceInfo& device)
{
    qInfo() << "Device discovered:" << device.friendlyName;
    updateDeviceList();
}

void DeviceWidget::onDeviceLost(const Chromecast::DeviceInfo& device)
{
    qInfo() << "Device lost:" << device.friendlyName;
    updateDeviceList();
}

void DeviceWidget::onDiscoveryFinished()
{
    ui->refreshButton->setEnabled(true);
    ui->refreshButton->setText("Refresh");
    ui->deviceComboBox->setEnabled(true);
    stopSpinner();

    int deviceCount = ui->deviceComboBox->count() - 1; // Subtract "Select a device" item
    if (deviceCount <= 0) {
        ui->deviceComboBox->addItem("No devices found", QString());
        ui->discoveryLabel->setText("No Chromecast devices found");
    } else {
        QString pluralDevices = deviceCount == 1 ? "device" : "devices";
        ui->discoveryLabel->setText(QString("Found %1 Chromecast %2").arg(deviceCount).arg(pluralDevices));
    }
}

void DeviceWidget::onConnectionStatusChanged(Chromecast::ConnectionStatus status)
{
    qInfo() << "Connection status changed:" << static_cast<int>(status);
    updateConnectionStatus();
}

void DeviceWidget::updateDeviceList()
{
    // Block signals to prevent triggering device selection during list update
    ui->deviceComboBox->blockSignals(true);

    ui->deviceComboBox->clear();
    ui->deviceComboBox->addItem("Select a device", QString());

    QList<DeviceInfo> devices = m_discovery->devices();
    for (const DeviceInfo& device : devices) {
        if (device.isAvailable) {
            ui->deviceComboBox->addItem(device.friendlyName, device.id);
        }
    }

    if (ui->deviceComboBox->count() == 1) {
        ui->deviceComboBox->addItem("No devices found", QString());
    }

    // Re-enable signals
    ui->deviceComboBox->blockSignals(false);
}

void DeviceWidget::updateConnectionStatus()
{
    Chromecast::ConnectionStatus status = m_communication->connectionStatus();
    ui->statusLabel->setText(connectionStatusText(status));

    QColor statusColor;
    switch (status) {
        case Chromecast::ConnectionStatus::Connected:
            statusColor = QColor("#28a745"); // Green
            break;
        case Chromecast::ConnectionStatus::Connecting:
            statusColor = QColor("#ffc107"); // Yellow
            break;
        case Chromecast::ConnectionStatus::Error:
            statusColor = QColor("#dc3545"); // Red
            break;
        default:
            statusColor = QColor("#6c757d"); // Gray
            break;
    }

    QString style = QString("color: %1;").arg(statusColor.name());
    ui->statusLabel->setStyleSheet(style);
}

QString DeviceWidget::connectionStatusText(Chromecast::ConnectionStatus status) const
{
    switch (status) {
        case Chromecast::ConnectionStatus::Disconnected:
            return "Disconnected";
        case Chromecast::ConnectionStatus::Connecting:
            return "Connecting...";
        case Chromecast::ConnectionStatus::Connected:
            return "Connected";
        case Chromecast::ConnectionStatus::Disconnecting:
            return "Disconnecting...";
        case Chromecast::ConnectionStatus::Error:
            return "Connection Error";
        default:
            return "Unknown";
    }
}

QString DeviceWidget::name() const
{
    return "Chromecast Device Widget";
}

QString DeviceWidget::layoutName() const
{
    return QStringLiteral("ChromecastDeviceWidget");
}

void DeviceWidget::startSpinner()
{
    m_spinnerRotation = 0;
    ui->spinnerLabel->setVisible(true);
    m_spinnerTimer->start(100); // Update every 100ms
}

void DeviceWidget::stopSpinner()
{
    m_spinnerTimer->stop();
    ui->spinnerLabel->setVisible(false);
}

void DeviceWidget::updateSpinner()
{
    // Rotate through different spinner characters for animation effect
    const QStringList spinnerChars = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
    m_spinnerRotation = (m_spinnerRotation + 1) % spinnerChars.size();
    ui->spinnerLabel->setText(spinnerChars[m_spinnerRotation]);
}

} // namespace Chromecast
