# Copilot Instructions for ym2151-log-player

## Project Overview

This is a YM2151 (OPM) sound chip log player that reads register events from JSON files and plays them back using the Nuked-OPM emulator. The project implements both real-time audio playback via MiniAudio and WAV file output.

## Core Architecture

### Data Flow
```
JSON Event Log → Event List → OPM Emulator → Audio Resampler → (Live Audio + WAV File)
```

**Key Components:**
- **Event Loading** (`json_loader.h`): Simple JSON parser for YM2151 register events
- **Audio Engine** (`player.c`): Main playback loop with MiniAudio integration
- **OPM Emulation** (`opm.c/h`): Nuked-OPM library (LGPL 2.1)
- **Audio Processing**: Internal resampling from ~55.9kHz to 48kHz output

### Critical Files Structure
- `src/player.c` - Main application entry point
- `src/types.h` - Core data structures and constants
- `src/events.h` - Event list management and MIDI-to-YM2151 conversion
- `src/json_loader.h` - JSON parsing implementation
- `build.py` - Cross-platform build script (Zig CC preferred, GCC fallback)

## Build System

**Primary Build Commands:**
```bash
python build.py build-phase4          # Current platform (recommended)
python build.py build-phase4-gcc      # Linux with GCC
python build.py build-phase4-windows  # Windows (cross-compile if on Linux)
```

**Key Dependencies:**
- Zig CC (preferred) or GCC for compilation
- No external audio libraries required (MiniAudio is header-only)
- Nuked-OPM is bundled (`opm.c/h`)

## JSON Event Format

Events use absolute timing (not deltas) with dual-phase register writes:
```json
{
  "events": [
    {"time": 0, "addr": "0x08", "data": "0x00", "is_data": 0},  // Address write
    {"time": 2, "addr": "0x08", "data": "0x00", "is_data": 1}   // Data write
  ]
}
```

**Important:** YM2151 requires separate address/data register writes with timing delays (`DELAY_SAMPLES = 2`).

## Audio Configuration

**Sample Rates:**
- Internal: `OPM_CLOCK / CYCLES_PER_SAMPLE` (~55,930 Hz)
- Output: 48,000 Hz (resampled via MiniAudio)
- Buffer: 4096 samples internal stereo buffer

**Audio Context** (`AudioContext` struct):
- Manages OPM chip state, event playback, and WAV recording
- Real-time callback processes events by sample timing
- Automatic WAV output to hardcoded `output.wav`

## YM2151-Specific Patterns

**Register Write Protocol:**
1. Write address to register 0 (`is_data=0`)
2. Wait `DELAY_SAMPLES` (2 samples)
3. Write data to register 1 (`is_data=1`)
4. Wait `DELAY_SAMPLES` before next operation

**MIDI Note Conversion:**
- Use `midi_to_kc_kf()` for note-to-register mapping
- Channel configuration includes operators, feedback, and envelope settings
- Key ON: `0x08 register = 0x78 | channel`
- Key OFF: `0x08 register = channel`

## Development Workflows

**Testing Audio Output:**
```bash
./player sample_events.json  # Plays sample file + generates output.wav
```

**Adding New Events:**
- Modify event generation in `events.h` functions
- Use `add_event_with_flag()` for proper is_data flagging
- Maintain absolute timing in sample units

**Debugging:**
- Check WAV output file for audio verification
- Monitor console output for event loading confirmation
- Use printf debugging in audio callback (careful with real-time constraints)

## Code Conventions

**Memory Management:**
- Manual allocation/deallocation (C-style)
- Event lists use dynamic arrays with capacity doubling
- Always pair `create_event_list()` with `free_event_list()`

**Error Handling:**
- Use descriptive emoji prefixes: ✅ success, ❌ error
- Exit on critical failures (memory allocation, audio init)
- Return NULL/false for recoverable errors

**Timing Constants:**
- All timing in samples, not milliseconds
- Use `duration_to_samples()` for conversions
- Internal sample rate drives emulation accuracy

## Integration Points

**External Dependencies:**
- MiniAudio (header-only, public domain)
- Nuked-OPM (LGPL 2.1, bundled)
- Standard C library only

**Platform Specifics:**
- Windows: Uses Zig CC for cross-compilation
- Linux: Prefers Zig CC, fallback to GCC with pthread/dl linking
- No platform-specific audio code (MiniAudio abstracts this)

## Testing Strategy

Currently minimal automated testing. Manual verification via:
1. Load `sample_events.json` successfully
2. Generate audio output without distortion
3. Verify `output.wav` file creation and content
4. Cross-platform build verification
