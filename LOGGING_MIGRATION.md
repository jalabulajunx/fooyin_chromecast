# Logging Migration Plan

This document outlines the plan to migrate from generic qInfo()/qDebug() calls to categorized logging.

## What's Implemented

1. **Logging infrastructure** (`src/core/chromecastlogging.h/cpp`)
   - Four categories: general, discovery, cast, http
   - Proper default levels (info for user-facing, warning for verbose components)

2. **UI feedback** (`src/ui/devicewidget.ui/cpp`)
   - Discovery status label shows "Searching for Chromecast devices..."
   - Shows device count after discovery: "Found X Chromecast device(s)"
   - Users know when discovery is in progress

## Next Steps (Optional)

To complete the logging migration, update these files to use the new categories:

### Priority 1: High-verbosity components

**communicationmanager.cpp** - Replace all qInfo()/qDebug() with:
```cpp
#include "../core/chromecastlogging.h"

// Connection events (keep as info):
qCInfo(lcChromecast) << "Connected to Chromecast";

// Protocol details (change to debug):
qCDebug(lcChromecastCast) << "Received message - NS:" << ...;
qCDebug(lcChromecastCast) << "RECEIVER_STATUS payload:" << ...;
qCDebug(lcChromecastCast) << "Got session ID:" << ...;
qCDebug(lcChromecastCast) << "Player state:" << ...;

// Errors (keep as warning):
qCWarning(lcChromecastCast) << "Cannot seek - not ready";
```

**castsocket.cpp** - Replace with:
```cpp
// All socket details to debug level:
qCDebug(lcChromecastCast) << "Sending message - NS:" << ...;
qCDebug(lcChromecastCast) << "Connected to Chromecast";
```

**httpserver.cpp** - Replace with:
```cpp
// Server lifecycle (keep info):
qCInfo(lcChromecast) << "HTTP server started on" << ...;

// Request details (change to debug):
qCDebug(lcChromecastHttp) << "New connection received";
qCDebug(lcChromecastHttp) << "Accepted connection from" << ...;
qCDebug(lcChromecastHttp) << "HTTP Request:" << method << path;
```

### Priority 2: User-facing components

**chromecastoutput.cpp** - Replace with:
```cpp
// Important events (info):
qCInfo(lcChromecast) << "ChromecastOutput initialized successfully";
qCInfo(lcChromecast) << "Starting streaming for current track:" << title;

// Debug details (debug):
qCDebug(lcChromecast) << "ChromecastOutput::reset";
qCDebug(lcChromecast) << "Detected seek from" << ...;
```

**discoverymanager.cpp** - Replace with:
```cpp
#include "../core/chromecastlogging.h"

qCInfo(lcChromecastDiscovery) << "Chromecast device discovered:" << ...;
qCInfo(lcChromecastDiscovery) << "Discovery finished. Found" << count << "devices";
```

**chromecastplugin.cpp** - Replace with:
```cpp
qCInfo(lcChromecast) << "Chromecast plugin initialized successfully";
qCInfo(lcChromecast) << "HTTP server started successfully on port" << port;
```

## Recommended Log Levels

| Message Type | Category | Level | Example |
|--------------|----------|-------|---------|
| Plugin lifecycle | `lcChromecast` | Info | "Plugin initialized", "HTTP server started" |
| Device events | `lcChromecastDiscovery` | Info | "Device discovered", "Found X devices" |
| User actions | `lcChromecast` | Info | "Device selected", "Starting streaming" |
| Connection state | `lcChromecast` | Info | "Connected", "Disconnected" |
| Cast protocol details | `lcChromecastCast` | **Debug** | Message payloads, session IDs |
| HTTP requests | `lcChromecastHttp` | **Debug** | Request details, connections |
| Errors/Warnings | Any category | Warning/Critical | LOAD_FAILED, connection errors |

## Benefits

After migration:

1. **Cleaner default logs**: Users see only important events, not protocol spam
2. **Easy troubleshooting**: Enable debug logs for specific components when needed
3. **Better performance**: Less I/O in production (debug logs disabled by default)
4. **Professional appearance**: Structured, categorized logs like other Qt apps

## Current State

The logging infrastructure is in place and compiled. The current qInfo()/qDebug() calls still work but are not categorized. Migration can be done incrementally, starting with the most verbose components (communicationmanager, castsocket, httpserver).

## How to Test

1. Run fooyin normally - should see reduced log output (info level only)
2. Enable debug for a component: `QT_LOGGING_RULES="chromecast.cast.debug=true" fooyin`
3. Verify debug messages appear for that component only
4. Check that errors/warnings always appear regardless of settings
