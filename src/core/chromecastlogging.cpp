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

#include "chromecastlogging.h"

// Define logging categories
// By default, debug messages are disabled in release builds
Q_LOGGING_CATEGORY(lcChromecast, "chromecast", QtInfoMsg)
Q_LOGGING_CATEGORY(lcChromecastCast, "chromecast.cast", QtWarningMsg)      // Only warnings/errors by default
Q_LOGGING_CATEGORY(lcChromecastHttp, "chromecast.http", QtWarningMsg)      // Only warnings/errors by default
Q_LOGGING_CATEGORY(lcChromecastDiscovery, "chromecast.discovery", QtInfoMsg)

// To enable debug logging, users can set QT_LOGGING_RULES environment variable:
// export QT_LOGGING_RULES="chromecast.cast.debug=true;chromecast.http.debug=true"
