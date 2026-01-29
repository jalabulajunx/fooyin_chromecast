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

#pragma once

#include "device.h"

#include <QObject>
#include <QTimer>
#include <QList>
#include <QProcess>

namespace Chromecast {

class DiscoveryManager : public QObject
{
    Q_OBJECT

public:
    explicit DiscoveryManager(QObject* parent = nullptr);
    ~DiscoveryManager() override;

    void startDiscovery(int timeout = 10000);
    void stopDiscovery();
    QList<DeviceInfo> devices() const;
    bool isDiscovering() const;

signals:
    void deviceDiscovered(const Chromecast::DeviceInfo& device);
    void deviceLost(const Chromecast::DeviceInfo& device);
    void discoveryFinished();
    void discoveryError(const QString& error);

private slots:
    void onDiscoveryTimeout();
    void onAvahiOutput();
    void onAvahiFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void initializeDiscovery();
    void parseAvahiLine(const QString& line);
    DeviceInfo parseDeviceInfo(const QString& serviceName, const QHostAddress& address, quint16 port);

    QTimer* m_discoveryTimer{nullptr};
    QProcess* m_avahiProcess{nullptr};
    QList<DeviceInfo> m_devices;
    bool m_isDiscovering{false};
};

} // namespace Chromecast
