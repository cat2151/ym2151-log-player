/* Simple test program to verify JSON loading functionality */

#include "types.h"
#include "events.h"
#include "json_loader.h"

int main(int argc, char **argv)
{
    printf("JSON Loader Test\n");
    printf("================\n\n");

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <json_log_file>\n", argv[0]);
        return 1;
    }

    const char *json_filename = argv[1];

    // Load events from JSON file
    RegisterEventList *events = load_events_json(json_filename);
    if (!events)
    {
        fprintf(stderr, "❌ Failed to load events\n");
        return 1;
    }

    printf("Event details:\n");
    printf("  Total events: %zu\n", events->count);

    // Show first 10 events
    size_t show_count = events->count < 10 ? events->count : 10;
    printf("  First %zu events:\n", show_count);
    for (size_t i = 0; i < show_count; i++)
    {
        RegisterEvent *e = &events->events[i];
        printf("    [%zu] time=%u addr=0x%02X data=0x%02X is_data=%u\n",
               i, e->sample_time, e->address, e->data, e->is_data_write);
    }

    // Calculate duration
    double duration = calculate_playback_duration(events);
    uint32_t total_samples = duration_to_samples(duration);
    printf("\nPlayback info:\n");
    printf("  Duration: %.3f seconds\n", duration);
    printf("  Total samples: %u\n", total_samples);

    free_event_list(events);
    printf("\n✅ Test passed!\n");
    return 0;
}
