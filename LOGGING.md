# Chromecast Plugin Logging

The fooyin Chromecast plugin uses Qt's logging categories for flexible, structured logging. This allows you to control verbosity for different components.

## Logging Categories

The plugin defines four logging categories:

| Category | Purpose | Default Level | When to Enable Debug |
|----------|---------|---------------|---------------------|
| `chromecast` | General plugin messages | Info | Always visible, shows important events |
| `chromecast.discovery` | Device discovery | Info | To debug mDNS/device detection issues |
| `chromecast.cast` | Cast protocol details | Warning | To debug communication with Chromecast |
| `chromecast.http` | HTTP server details | Warning | To debug file streaming issues |

## Default Behavior

By default, you'll see:
- **Info messages** from general plugin and discovery (device found, connected, disconnected)
- **Warning/Error messages** from all components (LOAD_FAILED, connection errors, etc.)

The verbose Cast protocol and HTTP server messages are **hidden by default** to reduce log spam.

## Enabling Debug Logging

To enable detailed logging for troubleshooting, set the `QT_LOGGING_RULES` environment variable:

### Enable all Chromecast debug logs:
```bash
export QT_LOGGING_RULES="chromecast*.debug=true"
fooyin
```

### Enable specific component debug logs:

**Cast protocol only** (shows all protobuf messages, session IDs, media status):
```bash
export QT_LOGGING_RULES="chromecast.cast.debug=true"
fooyin
```

**HTTP server only** (shows all HTTP requests, connections, file serving):
```bash
export QT_LOGGING_RULES="chromecast.http.debug=true"
fooyin
```

**Discovery only** (shows mDNS queries, device responses):
```bash
export QT_LOGGING_RULES="chromecast.discovery.debug=true"
fooyin
```

**Multiple categories** (separate with semicolons):
```bash
export QT_LOGGING_RULES="chromecast.cast.debug=true;chromecast.http.debug=true"
fooyin
```

### Disable info messages (errors/warnings only):
```bash
export QT_LOGGING_RULES="chromecast*.info=false"
fooyin
```

## Permanent Configuration

To make logging rules permanent, add them to your shell profile:

**Bash/Zsh** (`~/.bashrc` or `~/.zshrc`):
```bash
export QT_LOGGING_RULES="chromecast.cast.debug=true;chromecast.http.debug=true"
```

**Or create a Qt logging config file** (`~/.config/QtProject/qtlogging.ini`):
```ini
[Rules]
chromecast.cast.debug=true
chromecast.http.debug=true
```

## Example Output

### Default (Info level):
```
Initializing Chromecast plugin core
HTTP server started successfully on port 8010
Chromecast device discovered: "Living Room TV" (192.168.1.100:8009)
Found 2 Chromecast devices
Device selected: 192.168.1.100:8009
Connected to Chromecast
Streaming original file: /path/to/music.flac
```

### With chromecast.cast.debug=true:
```
CommunicationManager: Connecting to "Living Room TV" (192.168.1.100:8009)
CastSocket: Connected to Chromecast
CastSocket: Sending message - NS: "urn:x-cast:com.google.cast.tp.connection" From: "sender-0" To: "receiver-0" Size: 88
CommunicationManager: Received message - NS: "urn:x-cast:com.google.cast.receiver" Type: "RECEIVER_STATUS"
CommunicationManager: RECEIVER_STATUS payload: {"requestId":1,"status":{...}}
CommunicationManager: Got session ID: "abc-123-def-456"
CastProtocol: LOAD message payload: {"autoplay":true,"currentTime":0,"media":{...}}
CommunicationManager: Player state: "PLAYING" Media session ID: 1
```

### With chromecast.http.debug=true:
```
HTTP Server: New connection received
HTTP Server: Accepted connection from 192.168.1.100
HTTP Request: GET /media/25a14e22430891e7.flac
Serving file from 0-4567890 bytes
```

## Troubleshooting Guide

### Audio not playing
Enable: `chromecast.cast.debug=true` and `chromecast.http.debug=true`
Look for:
- `LOAD_FAILED` messages → Check file format, transcoding
- No HTTP connection from Chromecast → Check firewall (port 8010)
- `Media load error` → Check media URL accessibility

### Devices not found
Enable: `chromecast.discovery.debug=true`
Look for:
- mDNS responses
- Device availability flags
- Network interface issues

### Connection issues
Enable: `chromecast.cast.debug=true`
Look for:
- SSL handshake failures
- Session ID mismatches
- Heartbeat timeouts

### Seeking not working
Enable: `chromecast.cast.debug=true`
Look for:
- SEEK commands being sent
- Media session ID validity
- Player state transitions

## Performance Impact

Debug logging has minimal performance impact, but generates large log files. For production use, keep debug logs disabled and only enable them when troubleshooting specific issues.
