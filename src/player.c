/* YM2151 Log Player
 * Loads YM2151 register events from a JSON log file and plays them in real-time
 * Features:
 * - Load events from JSON log file
 * - Real-time playback with WAV file output
 */

#include "types.h"
#include "events.h"
#include "core.h"
#include "wav_writer.h"
#include "json_loader.h"

int main(int argc, char **argv)
{
    printf("YM2151 Log Player\n");
    printf("=====================================\n\n");

    // Check command line arguments
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <json_log_file>\n", argv[0]);
        fprintf(stderr, "Example: %s events.json\n", argv[0]);
        return 1;
    }

    const char *json_filename = argv[1];

    // Load events from JSON file
    RegisterEventList *events = load_events_json(json_filename);
    if (!events)
    {
        fprintf(stderr, "❌ Failed to load events from %s\n", json_filename);
        return 1;
    }

    // Calculate playback duration
    double duration = calculate_playback_duration(events);
    uint32_t total_samples = duration_to_samples(duration);

    // Initialize audio context
    AudioContext context;
    memset(&context, 0, sizeof(AudioContext));

    // Allocate WAV buffer
    context.wav_buffer = (int32_t *)malloc(total_samples * 2 * sizeof(int32_t));
    if (!context.wav_buffer)
    {
        fprintf(stderr, "❌ Failed to allocate WAV buffer\n");
        return 1;
    }
    context.wav_buffer_pos = 0;

    // Initialize OPM chip
    OPM_Reset(&context.chip);

    // Set playback parameters
    context.events = events;
    context.next_event_index = 0;
    context.samples_played = 0;
    context.total_samples = total_samples;
    context.is_playing = 1;

    printf("Initializing audio...\n");

    // Initialize resampler
    ma_resampler_config resamplerConfig = ma_resampler_config_init(
        ma_format_s16, 2, INTERNAL_SAMPLE_RATE, OUTPUT_SAMPLE_RATE,
        ma_resample_algorithm_linear);

    if (ma_resampler_init(&resamplerConfig, NULL, &context.resampler) != MA_SUCCESS)
    {
        fprintf(stderr, "❌ Failed to initialize resampler\n");
        free(context.wav_buffer);
        return 1;
    }

    // Configure MiniAudio device
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = ma_format_s16;
    deviceConfig.playback.channels = 2;
    deviceConfig.sampleRate = OUTPUT_SAMPLE_RATE;
    deviceConfig.dataCallback = data_callback;
    deviceConfig.pUserData = &context;

    ma_device device;
    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS)
    {
        fprintf(stderr, "❌ Failed to initialize audio device\n");
        ma_resampler_uninit(&context.resampler, NULL);
        free(context.wav_buffer);
        return 1;
    }

    printf("✅ Audio initialized\n\n");

    // Start playback
    if (ma_device_start(&device) != MA_SUCCESS)
    {
        fprintf(stderr, "❌ Failed to start audio device\n");
        ma_device_uninit(&device);
        ma_resampler_uninit(&context.resampler, NULL);
        free(context.wav_buffer);
        return 1;
    }

    printf("▶  Playing sequence...\n");

    // Wait for playback to finish
    while (context.is_playing)
    {
        ma_sleep(100);
    }

    printf("■  Playback complete\n\n");

    // Stop and cleanup audio
    ma_device_uninit(&device);
    ma_resampler_uninit(&context.resampler, NULL);

    // Save WAV file (hardcoded filename)
    const char *wav_filename = "output.wav";
    save_wav_file(wav_filename, context.wav_buffer, context.wav_buffer_pos);

    // Cleanup
    free(context.wav_buffer);
    free_event_list(events);

    printf("\n✅ Playback complete!\n");
    return 0;
}
