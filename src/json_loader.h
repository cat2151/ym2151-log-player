#include "types.h"

// Simple JSON parser for YM2151 log format
// Expected format: {"events": [{"time": 0, "addr": "0x08", "data": "0x00"}, ...]}
// Input is always treated as pass1 format (simple register writes) and converted to pass2 format
// Note: "is_data" field in input JSON is ignored

// Parse hex string (e.g., "0x08" -> 8)
static uint8_t parse_hex(const char *str)
{
    if (!str)
        return 0;

    uint32_t value = 0;
    if (str[0] == '0' && str[1] && (str[1] == 'x' || str[1] == 'X'))
    {
        str += 2;
    }
    while (*str)
    {
        char c = *str++;
        if (c >= '0' && c <= '9')
            value = value * 16 + (c - '0');
        else if (c >= 'a' && c <= 'f')
            value = value * 16 + (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F')
            value = value * 16 + (c - 'A' + 10);
        else
            break;
    }
    return (uint8_t)value;
}

// Parse integer from string
static uint32_t parse_uint(const char *str)
{
    if (!str)
        return 0;

    uint32_t value = 0;
    while (*str >= '0' && *str <= '9')
    {
        // Prevent overflow
        if (value > UINT32_MAX / 10)
            break;
        value = value * 10 + (*str - '0');
        str++;
    }
    return value;
}

// Find the next occurrence of a string in buffer
static char *find_str(char *buffer, const char *search)
{
    return strstr(buffer, search);
}

// Load events from JSON file
RegisterEventList *load_events_json(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        fprintf(stderr, "❌ Failed to open %s for reading\n", filename);
        return NULL;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Read entire file
    char *buffer = (char *)malloc(file_size + 1);
    if (!buffer)
    {
        fprintf(stderr, "❌ Failed to allocate memory for JSON buffer\n");
        fclose(fp);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, file_size, fp);
    buffer[read_size] = '\0';
    fclose(fp);

    // Create event list
    RegisterEventList *list = create_event_list();

    // Simple parser - find each event object
    char *pos = buffer;
    while ((pos = find_str(pos, "\"time\":")) != NULL)
    {
        pos += 7; // Skip "time":
        while (*pos == ' ')
            pos++;
        uint32_t time = parse_uint(pos);

        // Find addr
        char *addr_pos = find_str(pos, "\"addr\":");
        if (!addr_pos)
            break;
        addr_pos += 7; // Skip "addr":
        while (*addr_pos == ' ' || *addr_pos == '"')
            addr_pos++;
        uint8_t addr = parse_hex(addr_pos);

        // Find data
        char *data_pos = find_str(pos, "\"data\":");
        if (!data_pos)
            break;
        data_pos += 7; // Skip "data":
        while (*data_pos == ' ' || *data_pos == '"')
            data_pos++;
        uint8_t data = parse_hex(data_pos);

        // Input is always treated as pass1 format, so is_data is always 0
        // (any "is_data" field in the JSON is ignored)
        add_event_with_flag(list, time, addr, data, 0);

        pos = data_pos + 1;
    }

    free(buffer);
    printf("✅ Loaded %zu events from %s\n", list->count, filename);

    // Always convert pass1 format to pass2 format (split register writes with delays)
    if (list->count > 0)
    {
        printf("Converting pass1 format to pass2 format...\n");
        RegisterEventList *pass2 = convert_to_pass2_format(list);
        free_event_list(list);
        return pass2;
    }

    return list;
}

// Save events to JSON file in pass2 format
int save_events_json(const char *filename, RegisterEventList *events)
{
    FILE *fp = fopen(filename, "w");
    if (!fp)
    {
        fprintf(stderr, "❌ Failed to open %s for writing\n", filename);
        return 0;
    }

    // Write JSON header
    fprintf(fp, "{\n");
    fprintf(fp, "  \"event_count\": %zu,\n", events->count);
    fprintf(fp, "  \"events\": [\n");

    // Write each event
    for (size_t i = 0; i < events->count; i++)
    {
        RegisterEvent *event = &events->events[i];
        fprintf(fp, "    {\"time\": %u, \"addr\": \"0x%02X\", \"data\": \"0x%02X\", \"is_data\": %d}",
                event->sample_time, event->address, event->data, event->is_data_write);

        if (i < events->count - 1)
        {
            fprintf(fp, ",");
        }
        fprintf(fp, "\n");
    }

    // Write JSON footer
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");

    fclose(fp);
    printf("✅ Saved %zu events to %s (pass2 format)\n", events->count, filename);
    return 1;
}
