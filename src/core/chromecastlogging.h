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

#include <QLoggingCategory>

// Logging categories for different components
Q_DECLARE_LOGGING_CATEGORY(lcChromecast)         // General plugin messages
Q_DECLARE_LOGGING_CATEGORY(lcChromecastCast)     // Cast protocol (verbose)
Q_DECLARE_LOGGING_CATEGORY(lcChromecastHttp)     // HTTP server (verbose)
Q_DECLARE_LOGGING_CATEGORY(lcChromecastDiscovery) // Device discovery

// Usage:
// qCInfo(lcChromecast) << "Important user-facing message";
// qCWarning(lcChromecast) << "Warning message";
// qCDebug(lcChromecastCast) << "Verbose Cast protocol details";
// qCDebug(lcChromecastHttp) << "Verbose HTTP details";
