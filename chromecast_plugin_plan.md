# Fooyin Chromecast Plugin Implementation Plan

## Project Overview

Create a VLC-style Chromecast plugin for Fooyin music player with audio rendering and automatic transcoding support for unsupported formats.

## Architecture Summary

The plugin will follow a modular architecture with these key components:
1. **Device Discovery**: mDNS-based Chromecast discovery
2. **Communication**: Google Cast protocol implementation
3. **HTTP Server**: Local media server for streaming to Chromecast
4. **Transcoding Pipeline**: Reuse existing converter plugin infrastructure
5. **UI**: Device selector and playback controls
6. **Integration**: Fooyin playback system integration

## Phase 1: Foundation (Weeks 1-2)

### 1.1 Plugin Structure and Integration
- Create basic plugin structure following Fooyin plugin architecture
- Add metadata.json and CMakeLists.txt
- Implement core plugin class inheriting from Fooyin::CorePlugin and Fooyin::GuiPlugin

### 1.2 Device Discovery
- Implement mDNS discovery using Qt's Network module
- Listen for `_googlecast._tcp.local` service announcements
- Create DeviceInfo class to store device metadata
- Add device discovery manager with timeout and retry logic

### 1.3 Communication Layer
- Study Google Cast protocol and protobuf messages
- Implement communication manager for TCP connection (port 8009)
- Add session management and state tracking
- Implement basic message types (ping, connect, launch app)

### 1.4 Basic UI Components
- Create simple settings page
- Add device selector widget with discovery button
- Implement connection status indicator

## Phase 2: Media Streaming (Weeks 3-4)

### 2.1 Local HTTP Server
- Implement lightweight HTTP server using Qt's HttpServer
- Support byte-range requests for seeking
- Add CORS headers for Chromecast compatibility
- Serve media files from temporary storage or memory buffer

### 2.2 Media Control
- Implement playback commands (play, pause, stop, seek, volume)
- Add playlist management
- Implement metadata sending (title, artist, album, cover art)
- Handle Chromecast status responses

### 2.3 Playback Integration
- Connect to Fooyin's playback system
- Implement track change handling
- Synchronize playback state between Fooyin and Chromecast
- Add cover art extraction and display

## Phase 3: Transcoding (Weeks 5-6)

### 3.1 Format Detection
- Implement audio format detection using existing converter plugin infrastructure
- Check if audio format is natively supported by Chromecast
- Create format compatibility matrix

### 3.2 Transcoding Pipeline
- Integrate existing CodecWrapper classes (MP3, FLAC, Opus, Ogg)
- Implement on-the-fly transcoding using QProcess
- Add buffer management for smooth playback
- Optimize transcoding performance

### 3.3 Quality Profiles
- Implement quality profiles (High, Balanced, Efficient)
- Add transcoding format and quality settings
- Implement fallback mechanism for unsupported formats

## Phase 4: Enhancement (Weeks 7-8)

### 4.1 Advanced Features
- Add queue management
- Implement repeat and shuffle functionality
- Add volume normalization
- Support for gapless playback

### 4.2 Performance Optimization
- Optimize transcoding speed
- Implement cache management
- Reduce memory usage
- Improve buffer handling

### 4.3 Error Handling and Recovery
- Add error recovery for network drops
- Implement device disconnection handling
- Add transcoding failure fallback
- Improve error reporting to user

### 4.4 UI Enhancements
- Add playback progress bar
- Implement volume control slider
- Add metadata display
- Improve device selector UX

## Phase 5: Testing and Polish (Weeks 9-10)

### 5.1 Testing
- Test with various audio formats (supported and unsupported)
- Test with different Chromecast models (1st gen, 2nd gen, 3rd gen, Audio)
- Test network conditions (WiFi, wired, slow connections)
- Test transcoding performance on different hardware

### 5.2 Polish
- Refine UI/UX
- Add documentation
- Fix bugs and edge cases
- Optimize performance

## Technology Stack

### Core Technologies
- C++20
- Qt 6.2+ (Core, Network, Widgets, QML)
- Protocol Buffers (for Google Cast protocol)

### External Libraries
- Existing Fooyin converter plugin codecs:
  - flac (FLAC encoding)
  - lame (MP3 encoding)
  - opus-tools (Opus encoding)
  - vorbis-tools (Ogg Vorbis encoding)

### Build System
- CMake 3.18+
- Qt's build system integration

## Project Structure

```
fooyin-chromecast/
├── src/
│   ├── core/
│   │   ├── device.h
│   │   ├── device.cpp
│   │   ├── discoverymanager.h
│   │   ├── discoverymanager.cpp
│   │   ├── communicationmanager.h
│   │   ├── communicationmanager.cpp
│   │   ├── httpserver.h
│   │   ├── httpserver.cpp
│   │   ├── transcodingmanager.h
│   │   └── transcodingmanager.cpp
│   ├── ui/
│   │   ├── devicewidget.h
│   │   ├── devicewidget.cpp
│   │   ├── playbackcontrols.h
│   │   ├── playbackcontrols.cpp
│   │   ├── chromecastsettingspage.h
│   │   └── chromecastsettingspage.cpp
│   ├── integration/
│   │   ├── playbackintegrator.h
│   │   ├── playbackintegrator.cpp
│   │   ├── trackmetadata.h
│   │   └── trackmetadata.cpp
│   ├── chromecastplugin.h
│   └── chromecastplugin.cpp
├── include/
│   └── chromecast/
│       ├── cast_channel.pb.h
│       └── chromecast_common.h
├── proto/
│   └── cast_channel.proto
├── metadata.json
├── CMakeLists.txt
└── README.md
```

## Key Files and Classes

### Core Classes
1. **ChromecastPlugin**: Main plugin entry point
2. **DiscoveryManager**: Device discovery and management
3. **CommunicationManager**: Google Cast protocol implementation
4. **HttpServer**: Local HTTP server for media streaming
5. **TranscodingManager**: Format detection and transcoding
6. **PlaybackIntegrator**: Fooyin playback system integration

### UI Classes
1. **DeviceWidget**: Device selection and connection UI
2. **PlaybackControls**: Playback control buttons
3. **ChromecastSettingsPage**: Plugin configuration page

## Configuration Options

### Plugin Settings
- **Default transcoding format**: AAC, MP3, Opus, FLAC, Vorbis, WAV
- **Transcoding quality**: High, Balanced, Efficient
- **HTTP server port**: Default 8010
- **Discovery timeout**: Default 10 seconds
- **Buffer size**: Default 5 seconds
- **Metadata extraction**: Enable/disable cover art, track info

## Supported Audio Formats

### Natively Supported
- AAC (LC-AAC, HE-AAC)
- MP3
- Opus
- FLAC (up to 96kHz/24-bit)
- Vorbis
- WAV (LPCM)

### Transcoded Formats
- WMA
- APE (Monkey's Audio)
- WavPack
- ALAC
- DSD
- AC3
- DTS

## Performance Considerations

### Transcoding Performance
- Use dedicated codec executables (reuse converter plugin infrastructure)
- Optimize buffer sizes for minimal latency
- Implement progressive buffering
- Support format detection to avoid unnecessary transcoding

### Network Performance
- Use local HTTP server with chunked transfer encoding
- Support range requests for seeking
- Implement adaptive buffer management
- Optimize metadata sending

## Security Considerations

### Network Security
- Local network communication only
- TLS support for communication
- CORS configuration
- Input validation for metadata

### Resource Security
- Secure temporary file management
- Memory safety with Qt containers
- Proper error handling and recovery

## Testing Strategy

### Unit Tests
- Test discovery functionality
- Test communication protocol
- Test transcoding pipeline
- Test HTTP server

### Integration Tests
- Test playback integration with Fooyin
- Test device connection and disconnection
- Test transcoding of various formats
- Test playback control commands

### Performance Tests
- Transcoding speed tests
- Memory usage tests
- Playback latency tests
- Network performance tests

## Deployment

### Installation
- Plugin installation via Fooyin plugin manager
- Codec dependencies installation (flac, lame, opus-tools, vorbis-tools)
- Platform-specific installation instructions

### Requirements
- Fooyin 0.8+
- Qt 6.2+
- C++20 compatible compiler
- Codec executables installed

## Future Enhancements

### Short-Term
- Add support for more audio formats
- Improve transcoding performance
- Add gapless playback support

### Long-Term
- Add video support
- Implement multi-room audio
- Add Chromecast Audio group support
- Implement casting from other sources

## Risk Assessment

### Technical Risks
1. **Protocol complexity**: Google Cast protocol implementation
2. **Transcoding performance**: Ensuring smooth playback on lower-end hardware
3. **Network stability**: Handling network variations and disconnections

### Mitigation Strategies
1. Use existing open-source protocol implementations as reference
2. Optimize transcoding pipeline and buffer management
3. Implement robust error handling and recovery mechanisms

### Resource Risks
1. **Development time**: Complex protocol and transcoding integration
2. **Testing resources**: Need for various Chromecast models

### Mitigation Strategies
1. Modular architecture for incremental development
2. Community testing and feedback

## Success Criteria

### Functional Criteria
- Device discovery works reliably
- Connection to Chromecast is stable
- Audio playback is smooth
- Unsupported formats are transcoded automatically
- Playback controls work correctly

### Performance Criteria
- Transcoding starts within 2 seconds
- Playback latency < 5 seconds
- Memory usage < 100MB
- CPU usage < 50% on average

### User Experience Criteria
- UI is intuitive and responsive
- Connection status is clearly indicated
- Error messages are helpful and actionable
- Settings are easy to configure
