# Enhancement: Add cover art display for Chromecast TV

## Summary

Added support for displaying album cover art on Chromecast TV devices during audio playback.

## Problem

When streaming audio to a Chromecast TV, only the track title, artist, and album name were displayed. The cover art was missing, resulting in a plain/generic display on the TV screen.

## Solution

Implemented cover art extraction and serving via the plugin's HTTP server, then included the cover URL in the Cast protocol LOAD message using proper `MusicTrackMediaMetadata`.

### Changes

#### 1. Plugin initialization (`chromecastplugin.h/cpp`)
- Added `std::shared_ptr<Fooyin::AudioLoader> m_audioLoader` member
- Store `context.audioLoader` from `CorePluginContext` during initialization
- Pass `AudioLoader` to `HttpServer` for cover extraction

#### 2. HTTP Server (`httpserver.h/cpp`)
- Updated constructor to accept `std::shared_ptr<Fooyin::AudioLoader>`
- Added `createCoverUrl(mediaPath)` method to generate cover art URLs
- Added `m_coverFiles` map to track cover URL to media file mappings
- Added `serveCover()` method that:
  - Creates a `Fooyin::Track` from the media file path
  - Uses `AudioLoader::readTrackCover()` to extract embedded cover art
  - Auto-detects image MIME type (JPEG/PNG/GIF) from file header
  - Serves the image with appropriate HTTP headers and caching

#### 3. Cast Protocol (`castprotocol.h/cpp`)
- Updated `createLoadMediaMessage()` signature:
  - Changed parameters from `(title, subtitle)` to `(title, artist, album, coverUrl)`
- Changed `metadataType` from `0` (GenericMediaMetadata) to `3` (MusicTrackMediaMetadata)
- Added proper metadata fields: `title`, `artist`, `albumName`
- Added `images` array containing the cover art URL

#### 4. Communication Manager (`communicationmanager.cpp`)
- Removed `Q_UNUSED(coverUrl)` placeholder
- Updated `play()` to pass all metadata including `coverUrl` to the LOAD message

#### 5. Chromecast Output (`chromecastoutput.cpp`)
- Added call to `m_httpServer->createCoverUrl(filePath)` when starting playback
- Pass cover URL to `m_communication->play()` along with other metadata

### Cast Protocol Payload

**Before:**
```json
{
  "media": {
    "contentId": "http://192.168.x.x:8010/media/abc123.mp3",
    "contentType": "audio/mpeg",
    "streamType": "BUFFERED",
    "metadata": {
      "metadataType": 0,
      "title": "Macarena",
      "subtitle": "A.R. Rahman - Moon Walk"
    }
  }
}
```

**After:**
```json
{
  "media": {
    "contentId": "http://192.168.x.x:8010/media/abc123.mp3",
    "contentType": "audio/mpeg",
    "streamType": "BUFFERED",
    "metadata": {
      "metadataType": 3,
      "title": "Macarena",
      "artist": "A.R. Rahman",
      "albumName": "Moon Walk",
      "images": [
        {"url": "http://192.168.x.x:8010/cover/def456.jpg"}
      ]
    }
  }
}
```

## Compatibility

- **Chromecast TV / Chromecast with Google TV**: Displays cover art on screen
- **Chromecast Audio**: Works correctly (ignores the images field since no display)
- **Google Home / Nest speakers**: Compatible (uses metadata for voice queries)

The `MusicTrackMediaMetadata` (metadataType: 3) is the correct metadata type for music playback across all Chromecast device types. The `images` array is optional and ignored by devices without displays.

## Testing

1. Build and install the plugin
2. Connect to a Chromecast TV device
3. Play a track with embedded cover art (MP3, FLAC, etc.)
4. Verify cover art displays on the TV screen
5. Test with Chromecast Audio to ensure audio playback still works

## Files Changed

- `src/chromecastplugin.h`
- `src/chromecastplugin.cpp`
- `src/core/httpserver.h`
- `src/core/httpserver.cpp`
- `src/core/castprotocol.h`
- `src/core/castprotocol.cpp`
- `src/core/communicationmanager.cpp`
- `src/core/chromecastoutput.cpp`

## References

- [Google Cast Media Metadata](https://developers.google.com/cast/docs/reference/web_receiver/cast.framework.messages.MediaMetadata)
- [MusicTrackMediaMetadata](https://developers.google.com/cast/docs/reference/web_receiver/cast.framework.messages.MusicTrackMediaMetadata)
- [Secondary Image for Audio](https://developers.google.com/cast/docs/web_receiver/secondary_image)
