# Bug: Tracks skip prematurely (~50-55 seconds) when streaming to Chromecast

## Summary

When playing audio through the Chromecast output plugin, tracks would skip to the next song after approximately 50-55 seconds of playback, regardless of actual track duration. Local playback worked correctly.

## Environment

- fooyin version: master
- Chromecast plugin: custom implementation
- Chromecast device: Chromecast Ultra
- Audio format: FLAC, 44.1kHz stereo

## Symptoms

- Track starts playing normally on Chromecast
- After ~50-55 seconds, fooyin abruptly skips to the next track
- The Chromecast was still playing the HTTP stream correctly
- Issue occurred consistently with different tracks (tested with 224-second and 217-second tracks)

## Root Cause

The `ChromecastOutput::currentState()` method was returning incorrect buffer state information:

```cpp
// BROKEN CODE
Fooyin::OutputState ChromecastOutput::currentState()
{
    Fooyin::OutputState state;
    state.freeSamples = m_bufferSize;   // Always 8192
    state.queuedSamples = 0;            // PROBLEM: Always 0!
    state.delay = 0.0;
    return state;
}
```

### Why this caused premature skipping

1. Fooyin's audio engine uses `OutputState.queuedSamples` to track how much audio is buffered and waiting to be played
2. The engine calculates playback position based on samples written minus samples still queued
3. With `queuedSamples = 0`, the engine believed all written audio was immediately "played"
4. The decoder finished reading the file in ~50 seconds (decoding is fast)
5. Engine's internal position clock rapidly advanced to end-of-track
6. `trackAboutToFinish()` was emitted, triggering skip to next track

The Chromecast itself was playing correctly via HTTP streaming, but fooyin's engine didn't know this and thought the track had ended.

## Solution

Track real elapsed time since playback started and calculate proper `queuedSamples`:

```cpp
// FIXED CODE
Fooyin::OutputState ChromecastOutput::currentState()
{
    Fooyin::OutputState state;

    if (!m_isStreaming || !m_communication || !m_communication->isConnected()) {
        state.freeSamples = m_bufferSize;
        state.queuedSamples = 0;
        state.delay = 0.0;
        return state;
    }

    int sampleRate = m_format.sampleRate();
    int channels = m_format.channelCount();

    if (sampleRate <= 0 || channels <= 0) {
        state.freeSamples = m_bufferSize;
        state.queuedSamples = 0;
        state.delay = 0.0;
        return state;
    }

    // Use real elapsed time to calculate samples that "should" have been played
    qint64 elapsedMs = 0;
    if (m_playbackTimer.isValid() && !m_isPaused) {
        elapsedMs = m_playbackTimer.elapsed() + m_pausedElapsed;
    } else if (m_isPaused) {
        elapsedMs = m_pausedElapsed;
    }

    // Convert elapsed milliseconds to samples played
    uint64_t samplesPlayed = static_cast<uint64_t>(elapsedMs) * sampleRate * channels / 1000;

    // Queued samples = written minus played
    int64_t queued = static_cast<int64_t>(m_samplesWritten) - static_cast<int64_t>(samplesPlayed);
    state.queuedSamples = static_cast<int>(std::max(static_cast<int64_t>(0), queued));

    // Create backpressure when buffer is "full"
    const int maxQueuedSamples = sampleRate * channels * 10; // 10 seconds
    state.freeSamples = (state.queuedSamples < maxQueuedSamples) ? m_bufferSize : 0;

    state.delay = static_cast<double>(state.queuedSamples) / (sampleRate * channels);

    return state;
}
```

### Additional changes required

1. **Add `QElapsedTimer` member** to `ChromecastOutput`:
   ```cpp
   QElapsedTimer m_playbackTimer;
   qint64 m_pausedElapsed{0};
   ```

2. **Start timer when streaming starts** in `startStreaming()`:
   ```cpp
   m_samplesWritten = 0;
   m_pausedElapsed = 0;
   m_playbackTimer.start();
   ```

3. **Handle pause/resume** in `setPaused()`:
   ```cpp
   if (pause && !m_isPaused) {
       if (m_playbackTimer.isValid()) {
           m_pausedElapsed += m_playbackTimer.elapsed();
       }
   } else if (!pause && m_isPaused) {
       m_playbackTimer.restart();
   }
   ```

4. **Optional: Add media status polling** to CommunicationManager for position sync (poll every 1 second)

## Files Modified

- `src/core/chromecastoutput.h` - Added QElapsedTimer members
- `src/core/chromecastoutput.cpp` - Fixed currentState(), updated setPaused(), startStreaming()
- `src/core/communicationmanager.h` - Added media status polling timer and methods
- `src/core/communicationmanager.cpp` - Implemented media status polling

## Testing

1. Build and install the plugin
2. Connect to Chromecast device
3. Play a track longer than 60 seconds
4. Verify track plays past the 55-second mark
5. Test pause/resume functionality
6. Test manual track skip
7. Test seek functionality

## Lessons Learned

When implementing custom `AudioOutput` for streaming outputs (Chromecast, Airplay, etc.):

1. **Never return `queuedSamples = 0`** unless playback has truly consumed all audio
2. **Track real-time playback** using `QElapsedTimer` or similar
3. **Create proper backpressure** by returning `freeSamples = 0` when buffer is full
4. The engine relies heavily on `OutputState` for position tracking and end-of-track detection
