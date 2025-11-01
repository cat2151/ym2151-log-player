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

// Add event to list (sets is_data_write=0, used by pass1 generation)
void add_event(RegisterEventList *list, uint32_t sample_time, uint8_t address, uint8_t data)
{
    add_event_with_flag(list, sample_time, address, data, 0);
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

// MIDI note to YM2151 KC/KF conversion
void midi_to_kc_kf(uint8_t midi_note, uint8_t *kc, uint8_t *kf)
{
    // YM2151のノートテーブル
    // Based on empirical testing with 440Hz (A4) = KC 0x4A
    const uint8_t note_table[12] = {
        0,  // C# (YM2151 note 0)
        1,  // D
        2,  // D#
        4,  // E
        5,  // F
        6,  // F#
        8,  // G
        9,  // G#
        10, // A
        12, // A#
        13, // B
        14  // C  (YM2151 note 14)
    };

    // MIDI noteを1つ下げることで、C音が前オクターブのB位置を参照。ただし、MIDI note 0の場合はアンダーフローを防ぐ
    uint8_t adjusted_midi = (midi_note > 0) ? midi_note - 1 : 0;
    uint8_t midi_octave = (adjusted_midi / 12) - 1;
    uint8_t note_in_octave = adjusted_midi % 12;
    uint8_t ym_note = note_table[note_in_octave];

    *kc = (midi_octave << 4) | ym_note;
    *kf = 0; // No fine tuning for now (could be used to distinguish F from F#)
}

// Pass 1: Generate musical events (no delays)
RegisterEventList *generate_pass1_events()
{
    RegisterEventList *list = create_event_list();

    // Calculate timing
    double quarter_note_duration = 60.0 / BPM; // 0.5 seconds for BPM 120
    uint32_t quarter_note_samples = duration_to_samples(quarter_note_duration);

    printf("Pass 1: Generating musical events\n");
    printf("  Quarter note duration: %.4f seconds (%u samples)\n",
           quarter_note_duration, quarter_note_samples);

    // Initialize: Reset all channels
    for (int ch = 0; ch < 8; ch++)
    {
        add_event(list, 0, 0x08, ch);
    }

    // Configure channel 0 (only once, before playing any notes)
    int channel = 0;

    // RL_FB_CONNECT
    add_event(list, 0, 0x20 + channel, 0xC7);

    // PMS/AMS
    add_event(list, 0, 0x38 + channel, 0x00);

    // Configure operators (once for the channel)
    for (int op = 0; op < 4; op++)
    {
        int slot = channel + (op * 8);

        // DT1/MUL
        add_event(list, 0, 0x40 + slot, 0x01);

        // TL (Total Level)
        if (op == 0)
        {
            add_event(list, 0, 0x60 + slot, 0x00); // Max volume for carrier
        }
        else
        {
            add_event(list, 0, 0x60 + slot, 0x7F); // Silent for others
        }

        // KS/AR
        add_event(list, 0, 0x80 + slot, 0x1F);

        // AMS/D1R
        add_event(list, 0, 0xA0 + slot, 0x05);

        // DT2/D2R
        add_event(list, 0, 0xC0 + slot, 0x05);

        // D1L/RR
        add_event(list, 0, 0xE0 + slot, 0xF7);
    }

    // Play sequence: MIDI notes 60, 64, 67, 71 (C4, E4, G4, B4)
    uint8_t notes[] = {60, 64, 67, 71};
    const char *note_names[] = {"C4", "E4", "G4", "B4"};

    for (int i = 0; i < 4; i++)
    {
        uint32_t note_start_time = i * quarter_note_samples;
        uint8_t kc, kf;
        midi_to_kc_kf(notes[i], &kc, &kf);

        printf("  Note %d (%s, MIDI %d): start=%u samples, KC=0x%02X, KF=0x%02X\n",
               i, note_names[i], notes[i], note_start_time, kc, kf);

        // Set KC (Key Code)
        add_event(list, note_start_time, 0x28 + channel, kc);

        // Set KF (Key Fraction)
        add_event(list, note_start_time, 0x30 + channel, kf);

        // Key ON
        add_event(list, note_start_time, 0x08, 0x78 | channel);

        // Key OFF (at end of quarter note)
        uint32_t note_end_time = (i + 1) * quarter_note_samples;
        add_event(list, note_end_time, 0x08, channel);
    }

    printf("  Pass 1 complete: %zu events\n\n", list->count);
    return list;
}

// Pass 2: Add register write delays and split addr/data writes
RegisterEventList *generate_pass2_events(RegisterEventList *pass1)
{
    RegisterEventList *list = create_event_list();

    printf("Pass 2: Splitting register writes and adding delays\n");
    printf("  Delay per register write: %d samples\n", DELAY_SAMPLES);

    uint32_t accumulated_delay = 0;
    uint32_t last_time = 0;

    for (size_t i = 0; i < pass1->count; i++)
    {
        RegisterEvent *event = &pass1->events[i];

        // If this event is at a different musical time, reset accumulated delay
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

    printf("  Pass 2 complete: %zu events (split from %zu pass1 events)\n\n", list->count, pass1->count);
    return list;
}

// Calculate total playback duration from pass2 events
double calculate_playback_duration(RegisterEventList *pass2)
{
    if (pass2->count == 0)
        return 1.0;

    // Find last event time
    uint32_t last_event_time = 0;
    for (size_t i = 0; i < pass2->count; i++)
    {
        if (pass2->events[i].sample_time > last_event_time)
        {
            last_event_time = pass2->events[i].sample_time;
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

// Save events to JSON file
void save_events_json(const char *filename, RegisterEventList *events)
{
    FILE *fp = fopen(filename, "w");
    if (!fp)
    {
        fprintf(stderr, "❌ Failed to open %s for writing\n", filename);
        return;
    }

    fprintf(fp, "{\n");
    fprintf(fp, "  \"event_count\": %zu,\n", events->count);
    fprintf(fp, "  \"events\": [\n");

    for (size_t i = 0; i < events->count; i++)
    {
        RegisterEvent *e = &events->events[i];
        fprintf(fp, "    {\"time\": %u, \"addr\": \"0x%02X\", \"data\": \"0x%02X\", \"is_data\": %u}",
                e->sample_time, e->address, e->data, e->is_data_write);
        if (i < events->count - 1)
        {
            fprintf(fp, ",");
        }
        fprintf(fp, "\n");
    }

    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");

    fclose(fp);
    printf("✅ Saved events to %s\n", filename);
}
