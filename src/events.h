#include "types.h"

// Initialize register event list
RegisterEventList *create_event_list()
{
    RegisterEventList *list = (RegisterEventList *)malloc(sizeof(RegisterEventList));
    if (!list)
    {
        fprintf(stderr, "❌ Failed to allocate memory for event list\n");
        exit(1);
    }
    list->capacity = 256;
    list->count = 0;
    list->events = (RegisterEvent *)malloc(sizeof(RegisterEvent) * list->capacity);
    if (!list->events)
    {
        fprintf(stderr, "❌ Failed to allocate memory for events array\n");
        free(list);
        exit(1);
    }
    return list;
}

// Add event to list with is_data_write flag
void add_event_with_flag(RegisterEventList *list, uint32_t sample_time, uint8_t address, uint8_t data, uint8_t is_data_write)
{
    if (list->count >= list->capacity)
    {
        list->capacity *= 2;
        RegisterEvent *new_events = (RegisterEvent *)realloc(list->events, sizeof(RegisterEvent) * list->capacity);
        if (!new_events)
        {
            fprintf(stderr, "❌ Failed to reallocate memory for events\n");
            exit(1);
        }
        list->events = new_events;
    }
    list->events[list->count].sample_time = sample_time;
    list->events[list->count].address = address;
    list->events[list->count].data = data;
    list->events[list->count].is_data_write = is_data_write;
    list->count++;
}

// Free event list
void free_event_list(RegisterEventList *list)
{
    free(list->events);
    free(list);
}

// Calculate samples for a duration at internal sample rate
uint32_t duration_to_samples(double duration_seconds)
{
    return (uint32_t)(duration_seconds * INTERNAL_SAMPLE_RATE);
}

// Convert pass1 format events to pass2 format (add register write delays and split addr/data writes)
// This function takes simple register write events and converts them into the format needed by the YM2151,
// which requires separate address and data register writes with timing delays between them.
RegisterEventList *convert_to_pass2_format(RegisterEventList *pass1)
{
    RegisterEventList *list = create_event_list();

    printf("Converting to pass2 format: Splitting register writes and adding delays\n");
    printf("  Delay per register write: %d samples\n", DELAY_SAMPLES);

    uint32_t accumulated_delay = 0;
    uint32_t last_time = 0;

    for (size_t i = 0; i < pass1->count; i++)
    {
        RegisterEvent *event = &pass1->events[i];

        // If this event is at a different time, reset accumulated delay
        if (event->sample_time != last_time)
        {
            accumulated_delay = 0;
            last_time = event->sample_time;
        }

        // Split each pass1 event into two pass2 events with timing:
        // Note: Both address and data are stored in each event for clarity in JSON output
        // and to track the complete register write operation.

        // 1. Address register write at time T
        uint32_t addr_time = event->sample_time + accumulated_delay;
        add_event_with_flag(list, addr_time, event->address, event->data, 0); // is_data_write = 0
        accumulated_delay += DELAY_SAMPLES;

        // 2. Data register write at time T + DELAY_SAMPLES
        uint32_t data_time = event->sample_time + accumulated_delay;
        add_event_with_flag(list, data_time, event->address, event->data, 1); // is_data_write = 1
        accumulated_delay += DELAY_SAMPLES;
    }

    printf("  Conversion complete: %zu events (split from %zu pass1 events)\n\n", list->count, pass1->count);
    return list;
}

// Calculate total playback duration from events
double calculate_playback_duration(RegisterEventList *events)
{
    if (events->count == 0)
        return 1.0;

    // Find last event time
    uint32_t last_event_time = 0;
    for (size_t i = 0; i < events->count; i++)
    {
        if (events->events[i].sample_time > last_event_time)
        {
            last_event_time = events->events[i].sample_time;
        }
    }

    // Add 1 second after last event
    uint32_t total_samples = last_event_time + INTERNAL_SAMPLE_RATE;
    double duration = (double)total_samples / INTERNAL_SAMPLE_RATE;

    printf("Playback duration calculation:\n");
    printf("  Last event at: %u samples (%.3f seconds)\n",
           last_event_time, (double)last_event_time / INTERNAL_SAMPLE_RATE);
    printf("  Total duration: %.3f seconds (%u samples)\n\n", duration, total_samples);

    return duration;
}
