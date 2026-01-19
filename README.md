# Fooyin Chromecast Plugin

A Chromecast audio output plugin for Fooyin music player. Stream your music library to Chromecast devices as an **audio output renderer**, similar to VLC's "cast to" functionality.

## Status

**✅ Implementation Complete** - Core functionality is fully implemented and builds successfully. Awaiting testing with real Chromecast hardware.

**Build Status:** ✅ Compiles successfully
**Last Updated:** 2026-01-19

### What's Working
- ✅ Device discovery with mDNS (avahi-browse)
- ✅ HTTP server for audio streaming (port 8010)
- ✅ Cast protocol communication (native C++ with Protocol Buffers)
- ✅ Plugin settings page with device selection
- ✅ Loading spinner during device discovery
- ✅ Audio output registration with fooyin
- ✅ Transcoding manager (quality presets)
- ✅ Metadata extraction and display

## Features

- **Audio Output Renderer**: Appears as "Chromecast" in fooyin's output device menu
- **Native Integration**: Uses fooyin's playback controls (no duplicate UI)
- **Device Discovery**: Automatically detects Chromecast devices using mDNS (avahi-browse)
- **HTTP Streaming**: Built-in HTTP server streams audio files to Chromecast
- **Automatic Transcoding**: Converts unsupported formats using ffmpeg
- **Cast Protocol**: Communicates with Chromecast using native C++ Protocol Buffers implementation
- **Playback Controls**: Play, pause, stop, seek, and volume control
- **Metadata Support**: Sends track title, artist, and album to Chromecast
- **Format Detection**: Automatically determines which files need transcoding

## Installation

### Prerequisites

**Required:**
```bash
# Arch Linux
sudo pacman -S fooyin qt6-base avahi ffmpeg protobuf

# Ubuntu/Debian
sudo apt install fooyin qt6-base avahi-daemon avahi-utils ffmpeg libprotobuf-dev

# Start avahi service
sudo systemctl start avahi-daemon
sudo systemctl enable avahi-daemon
```

### Build and Install Plugin

```bash
# Clone and build
git clone <repository-url>
cd fooyin_chromecast
cmake -B build
cmake --build build

# Install to user directory
mkdir -p ~/.local/share/fooyin/plugins
cp build/fyplugin_chromecastplugin.so ~/.local/share/fooyin/plugins/

# Or system-wide (requires sudo)
sudo cp build/fyplugin_chromecastplugin.so /usr/lib/fooyin/plugins/
```

## Usage Guide

### Quick Start

1. **Configure Your Chromecast Device**
   - In fooyin, open **Plugins → Chromecast** from the menu
   - Wait for device discovery to complete (spinner will appear)
   - Select your Chromecast device from the dropdown
   - Click **Apply** to save settings

2. **Select Chromecast as Output**
   - Go to **Settings → Playback → Output**
   - Select **"Chromecast"** from the Device dropdown
   - Click **Apply**

3. **Start Playing**
   - Use fooyin's normal playback controls
   - Music will stream to your selected Chromecast device

### Detailed Configuration

#### Plugin Settings (Plugins → Chromecast)

**Device Selection:**
- **Chromecast Device Dropdown**: Lists all discovered devices on your network
- **Refresh Button**: Manually trigger device discovery
- **Loading Spinner**: Indicates discovery is in progress
- **Status Label**: Shows connection status and device count
- Devices are identified by IP:port (e.g., `192.168.1.100:8009`)

**Transcoding Settings:**
- **Default Format**: Choose AAC, MP3, Opus, FLAC, Vorbis, or WAV
  - AAC: Best compatibility (recommended)
  - MP3: Universal compatibility, larger files
  - Opus: Best quality/size ratio
  - FLAC: Lossless, large files
- **Default Quality**:
  - High Quality: 320kbps MP3 / 256kbps AAC / 192kbps Opus
  - Balanced: 192kbps MP3 / 160kbps AAC / 128kbps Opus (recommended)
  - Efficient: 128kbps MP3 / 96kbps AAC / 96kbps Opus

**Network Settings:**
- **HTTP Server Port**: Default 8010 (requires restart if changed)
  - Must be accessible from your Chromecast device
  - Ensure firewall allows incoming connections
- **Discovery Timeout**: How long to search for devices (default 10000ms)
  - Increase if devices aren't found on first try
  - Range: 1000-30000ms

#### Output Device Selection (Settings → Playback → Output)

Simply select **"Chromecast"** from the Device dropdown. This tells fooyin to send all audio to the Chromecast device you configured in the plugin settings.

### Playback Behavior

Once configured, the plugin:
- **Automatically streams** files in native formats (AAC, MP3, Opus, FLAC, Vorbis, WAV)
- **Automatically transcodes** unsupported formats (WMA, APE, ALAC, etc.) using your quality settings
- **Sends metadata** (title, artist, album) to display on Chromecast
- **Synchronizes playback** state (play/pause/stop/seek)
- **Caches transcoded files** in `/tmp/fooyin-chromecast/` for the session

### Tips

- **First-time setup**: Discovery takes ~5 seconds. Wait for spinner to stop before selecting device
- **Reconnecting**: Device reconnects automatically when you start playback
- **Multiple Chromecasts**: Each device appears separately in the dropdown with its IP address
- **Network changes**: Click Refresh if you move to a different network
- **Transcoding quality**: Use "Balanced" for good quality and reasonable file sizes

## Architecture

This plugin implements the **Fooyin::AudioOutput** interface, making it a true audio output renderer:

```
User → Fooyin Controls → Audio Engine → ChromecastOutput → HTTP Server → Chromecast
                                              ↑
                          PlayerController (metadata, state)
```

**Key Components:**
- **ChromecastOutput**: AudioOutput implementation that receives audio buffers
- **DiscoveryManager**: mDNS device discovery using avahi-browse
- **HttpServer**: Serves audio files to Chromecast over HTTP (with byte-range support)
- **CommunicationManager**: Sends commands to Chromecast using native Cast protocol (Protocol Buffers)
- **TranscodingManager**: Converts unsupported formats using ffmpeg

### Supported Formats

**Natively supported by Chromecast:**
- AAC (LC-AAC, HE-AAC)
- MP3
- Opus
- FLAC (up to 96kHz/24-bit)
- Vorbis
- WAV (LPCM)

**Formats requiring transcoding:**
- WMA
- APE (Monkey's Audio)
- WavPack
- ALAC
- DSD
- AC3
- DTS

## Transcoding

The plugin automatically transcodes unsupported formats using ffmpeg:

**Quality Presets:**
- **High**: 320kbps MP3, 256kbps AAC, 192kbps Opus
- **Balanced**: 192kbps MP3, 160kbps AAC, 128kbps Opus (default)
- **Efficient**: 128kbps MP3, 96kbps AAC, 96kbps Opus

Transcoded files are cached in `/tmp/fooyin-chromecast/` during the session.

## Troubleshooting

### Plugin doesn't load

**Symptoms:**
- "Chromecast" doesn't appear in Settings → Playback → Output
- "Chromecast" menu item missing from Plugins menu

**Solutions:**
```bash
# Check fooyin logs for errors
journalctl -f | grep -i chromecast

# Verify plugin is installed
ls -lh ~/.local/share/fooyin/plugins/fyplugin_chromecastplugin.so
# OR
ls -lh /usr/lib/fooyin/plugins/fyplugin_chromecastplugin.so

# Verify Qt 6 is installed
pacman -Q qt6-base  # Arch
dpkg -l | grep qt6-base  # Ubuntu/Debian

# Rebuild if necessary
cd ~/Projects/fooyin_chromecast
rm -rf build
cmake -B build && cmake --build build
cp build/fyplugin_chromecastplugin.so ~/.local/share/fooyin/plugins/
```

### No devices found

**Symptoms:**
- Device dropdown shows "No Chromecast devices found"
- Spinner stops but dropdown is empty

**Solutions:**
```bash
# 1. Check avahi-daemon is running
systemctl status avahi-daemon
sudo systemctl start avahi-daemon
sudo systemctl enable avahi-daemon

# 2. Verify Chromecast is on same network
# - Check your router's DHCP client list
# - Ensure computer and Chromecast are on same subnet
# - Disable network isolation/AP isolation on router

# 3. Test manually
avahi-browse -r -t -p _googlecast._tcp
# Should show output like:
# =;eth0;IPv4;Living Room TV;_googlecast._tcp;local
# ...;192.168.1.100;8009;...

# 4. Check firewall
sudo ufw status  # Ubuntu
sudo firewall-cmd --list-all  # Fedora
# Ensure UDP 5353 (mDNS) is allowed

# 5. Increase discovery timeout in plugin settings
# Plugins → Chromecast → Discovery Timeout → 15000ms
```

### Can't connect to device

**Symptoms:**
- Device appears in dropdown but doesn't connect
- Status shows "Disconnected" or "Connection failed"

**Solutions:**
```bash
# 1. Check firewall allows HTTP port 8010
sudo ufw allow 8010/tcp  # Ubuntu
sudo firewall-cmd --add-port=8010/tcp --permanent  # Fedora

# 2. Check HTTP server started
# Look in fooyin logs for:
# "HTTP server started successfully on port 8010"
```

### Playback doesn't start

**Symptoms:**
- Connected to device but hitting play does nothing
- No audio from Chromecast

**Solutions:**
```bash
# 1. Check fooyin logs
journalctl -f | grep -i chromecast

# 2. Verify audio file format
file /path/to/audio/file.mp3
# Ensure format is supported or transcoding is configured

# 3. Check HTTP server is accessible from Chromecast
# From another device on network:
curl http://<your-computer-ip>:8010/
# Should return HTTP 200 or 404 (not connection refused)

# 4. Restart fooyin and try again
```

### Transcoding fails

**Symptoms:**
- Native formats work but some files don't play
- Logs show ffmpeg errors

**Solutions:**
```bash
# 1. Verify ffmpeg is installed
ffmpeg -version
# Should show version info (4.0+)

# 2. Install if missing
sudo pacman -S ffmpeg  # Arch
sudo apt install ffmpeg  # Ubuntu/Debian

# 3. Check temp directory is writable
ls -ld /tmp/fooyin-chromecast/
# Should show drwx------ (755 or 700)

# 4. Test transcoding manually
ffmpeg -i input.wma -b:a 192k output.mp3
# Should complete without errors

# 5. Check available formats in plugin settings
# Plugins → Chromecast → Default Format
# Try different formats (AAC, MP3, Opus)
```

### Audio stutters or cuts out

**Symptoms:**
- Playback starts but stutters
- Audio cuts out frequently

**Solutions:**
- **Reduce transcoding quality**: Use "Efficient" instead of "High Quality"
- **Check network**: Ensure strong WiFi signal to both computer and Chromecast
- **Close bandwidth-heavy apps**: Stop other network-intensive applications
- **Use wired connection**: Connect computer via ethernet if possible
- **Lower output bitrate**: Plugins → Chromecast → Default Quality → Efficient

### Settings don't save

**Symptoms:**
- Device selection resets after restart
- Transcoding settings not applied

**Solutions:**
```bash
# 1. Check fooyin config directory
ls -lh ~/.config/fooyin/

# 2. Check settings file permissions
ls -l ~/.config/fooyin/fooyin.conf
# Should be writable by user

# 3. Look for apply confirmation in logs
# After clicking Apply, check logs for:
# "Chromecast settings saved"

# 4. Restart fooyin to reload settings
```

### Getting More Help

**Enable debug logging:**
```bash
# Run fooyin with verbose output
QT_LOGGING_RULES="chromecast.*=true" fooyin 2>&1 | tee chromecast-debug.log

# Check specific log categories:
# chromecast.general - Plugin lifecycle
# chromecast.discovery - Device discovery
# chromecast.cast - Cast protocol
# chromecast.http - HTTP server
```

**Report issues:**
- Include debug logs
- Note fooyin version: `fooyin --version`
- Note OS and network setup
- List Chromecast model and firmware version

## Development

See [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md) for detailed implementation status and architecture documentation.

## License

This plugin is released under the GNU General Public License v3.0.

## Technology Stack

- **C++20** - Core language
- **Qt 6.2+** - UI and networking framework
- **Protocol Buffers** - Cast protocol communication
- **ffmpeg** - Audio transcoding
- **avahi** - mDNS device discovery
- **Fooyin API** - OutputPlugin interface

## Contributing

The core implementation is complete! Help needed for:
- Testing with real Chromecast devices
- UI enhancements (settings page, status indicators)
- Documentation improvements
- Bug reports and fixes

## Credits

- **Fooyin**: https://github.com/ludouzi/fooyin
- **Qt**: https://www.qt.io/
