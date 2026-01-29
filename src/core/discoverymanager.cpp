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

#include "discoverymanager.h"

#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QHostAddress>
#include <QTimer>
#include <QDebug>
#include <QRegularExpression>

namespace Chromecast {

DiscoveryManager::DiscoveryManager(QObject* parent)
    : QObject(parent)
    , m_discoveryTimer(new QTimer(this))
{
    connect(m_discoveryTimer, &QTimer::timeout, this, &DiscoveryManager::onDiscoveryTimeout);
    m_discoveryTimer->setSingleShot(true);
}

DiscoveryManager::~DiscoveryManager()
{
    stopDiscovery();
}

void DiscoveryManager::startDiscovery(int timeout)
{
    if (m_isDiscovering) {
        return;
    }

    qInfo() << "Starting Chromecast device discovery";
    m_isDiscovering = true;
    m_devices.clear();

    initializeDiscovery();

    m_discoveryTimer->start(timeout);
}

void DiscoveryManager::stopDiscovery()
{
    if (!m_isDiscovering) {
        return;
    }

    qInfo() << "Stopping Chromecast device discovery";
    m_isDiscovering = false;
    m_discoveryTimer->stop();

    if (m_avahiProcess) {
        m_avahiProcess->kill();
        m_avahiProcess->waitForFinished(1000);
        m_avahiProcess->deleteLater();
        m_avahiProcess = nullptr;
    }
}

QList<DeviceInfo> DiscoveryManager::devices() const
{
    return m_devices;
}

bool DiscoveryManager::isDiscovering() const
{
    return m_isDiscovering;
}

void DiscoveryManager::onDiscoveryTimeout()
{
    m_isDiscovering = false;

    if (m_avahiProcess) {
        m_avahiProcess->kill();
    }

    qInfo() << "Discovery finished. Found" << m_devices.size() << "Chromecast devices";
    emit discoveryFinished();
}

void DiscoveryManager::onAvahiOutput()
{
    if (!m_avahiProcess) {
        return;
    }

    QString output = QString::fromUtf8(m_avahiProcess->readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        parseAvahiLine(line);
    }
}

void DiscoveryManager::onAvahiFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qInfo() << "avahi-browse finished with exit code:" << exitCode;

    if (exitStatus == QProcess::CrashExit) {
        qWarning() << "avahi-browse crashed";
        emit discoveryError("Device discovery tool crashed");
    }

    if (m_avahiProcess) {
        m_avahiProcess->deleteLater();
        m_avahiProcess = nullptr;
    }
}

void DiscoveryManager::initializeDiscovery()
{
    qInfo() << "Initializing mDNS discovery for Chromecast devices using avahi-browse";

    // Clean up any existing process
    if (m_avahiProcess) {
        m_avahiProcess->kill();
        m_avahiProcess->deleteLater();
    }

    m_avahiProcess = new QProcess(this);
    connect(m_avahiProcess, &QProcess::readyReadStandardOutput, this, &DiscoveryManager::onAvahiOutput);
    connect(m_avahiProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DiscoveryManager::onAvahiFinished);

    // Use avahi-browse to discover Chromecast devices
    // -r: resolve service info
    // -t: terminate after timeout
    // -p: parsable output format
    QStringList args = {"-r", "-t", "-p", "_googlecast._tcp"};

    qInfo() << "Running: avahi-browse" << args.join(" ");
    m_avahiProcess->start("avahi-browse", args);

    if (!m_avahiProcess->waitForStarted(1000)) {
        qWarning() << "Failed to start avahi-browse:" << m_avahiProcess->errorString();
        qWarning() << "Note: Install avahi-utils package for Chromecast discovery";
        emit discoveryError("Failed to start device discovery. Please install avahi-utils.");
        m_avahiProcess->deleteLater();
        m_avahiProcess = nullptr;
    }
}

void DiscoveryManager::parseAvahiLine(const QString& line)
{
    // Avahi parsable output format:
    // =;interface;protocol;name;type;domain;hostname;address;port;txt
    // Example:
    // =;eth0;IPv4;Living Room;_googlecast._tcp;local;chromecast.local;192.168.1.100;8009;"fn=Living Room" "md=Chromecast"

    if (!line.startsWith('=')) {
        return; // Only process "=" lines which indicate found services
    }

    QStringList parts = line.split(';');
    if (parts.size() < 9) {
        return;
    }

    QString serviceName = parts[3];  // Name of the device
    QString hostname = parts[6];     // Hostname
    QString addressStr = parts[7];   // IP address
    QString portStr = parts[8];      // Port

    // Parse IP address
    QHostAddress address(addressStr);
    if (address.isNull()) {
        qWarning() << "Invalid IP address in avahi output:" << addressStr;
        return;
    }

    // Parse port
    bool ok;
    quint16 port = portStr.toUShort(&ok);
    if (!ok) {
        qWarning() << "Invalid port in avahi output:" << portStr;
        port = 8009; // Default Chromecast port
    }

    // Create device info
    DeviceInfo device = parseDeviceInfo(serviceName, address, port);

    // Extract additional info from TXT records if available
    if (parts.size() > 9) {
        QString txtRecords = parts.mid(9).join(';');
        // Parse TXT records for friendly name and model
        QRegularExpression fnRegex(R"(fn=([^\"]+))");
        QRegularExpression mdRegex(R"(md=([^\"]+))");

        QRegularExpressionMatch fnMatch = fnRegex.match(txtRecords);
        if (fnMatch.hasMatch()) {
            device.friendlyName = fnMatch.captured(1);
        }

        QRegularExpressionMatch mdMatch = mdRegex.match(txtRecords);
        if (mdMatch.hasMatch()) {
            device.modelName = mdMatch.captured(1);
        }
    }

    // Check if device already exists
    auto it = std::find_if(m_devices.begin(), m_devices.end(),
                          [&device](const DeviceInfo& d) { return d.id == device.id; });

    if (it == m_devices.end()) {
        m_devices.append(device);
        qInfo() << "Chromecast device discovered:" << device.friendlyName
                << "(" << device.ipAddress.toString() << ":" << device.port << ")"
                << "Model:" << device.modelName;
        emit deviceDiscovered(device);
    } else {
        it->isAvailable = true;
    }
}

DeviceInfo DiscoveryManager::parseDeviceInfo(const QString& serviceName, const QHostAddress& address, quint16 port)
{
    DeviceInfo device;
    device.name = serviceName;
    device.ipAddress = address;
    device.port = port;
    device.friendlyName = serviceName; // Will be overridden by TXT record if available
    device.modelName = "Chromecast";   // Will be overridden by TXT record if available
    device.id = QString("%1:%2").arg(address.toString()).arg(port);
    device.isAvailable = true;

    return device;
}

} // namespace Chromecast
