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
#include "../core/device.h"

// Forward declaration for Qt Designer generated UI class
namespace Ui {
class DeviceWidget;
}

namespace Chromecast {

class DiscoveryManager;
class CommunicationManager;

class DeviceWidget : public Fooyin::FyWidget
{
    Q_OBJECT

public:
    explicit DeviceWidget(DiscoveryManager* discovery, CommunicationManager* communication, QWidget* parent = nullptr);
    ~DeviceWidget() override;

    QString name() const override;
    QString layoutName() const override;

signals:
    void deviceSelected(const QString& deviceId);

private slots:
    void onRefreshButtonClicked();
    void onDeviceComboBoxChanged(int index);
    void onDeviceDiscovered(const Chromecast::DeviceInfo& device);
    void onDeviceLost(const Chromecast::DeviceInfo& device);
    void onDiscoveryFinished();
    void onConnectionStatusChanged(Chromecast::ConnectionStatus status);
    void updateSpinner();

private:
    void updateDeviceList();
    void updateConnectionStatus();
    QString connectionStatusText(Chromecast::ConnectionStatus status) const;
    void startSpinner();
    void stopSpinner();

    Ui::DeviceWidget* ui;
    DiscoveryManager* m_discovery{nullptr};
    CommunicationManager* m_communication{nullptr};
    QTimer* m_spinnerTimer{nullptr};
    int m_spinnerRotation{0};
};

} // namespace Chromecast
