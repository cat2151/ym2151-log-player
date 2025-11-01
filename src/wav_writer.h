#include "types.h"

// Save WAV file
int save_wav_file(const char *filename, int32_t *buffer, uint32_t num_samples)
{
    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        fprintf(stderr, "❌ Failed to open %s for writing\n", filename);
        return 0;
    }

    uint32_t data_size = num_samples * 2 * sizeof(int16_t);

    // Write WAV header
    WAVHeader header;
    memcpy(header.riff, "RIFF", 4);
    header.file_size = data_size + 36;
    memcpy(header.wave, "WAVE", 4);
    fwrite(&header, sizeof(WAVHeader), 1, fp);

    // Write FMT chunk
    FMTChunk fmt;
    memcpy(fmt.fmt, "fmt ", 4);
    fmt.chunk_size = 16;
    fmt.audio_format = 1; // PCM
    fmt.num_channels = 2; // Stereo
    fmt.sample_rate = INTERNAL_SAMPLE_RATE;
    fmt.byte_rate = INTERNAL_SAMPLE_RATE * 2 * sizeof(int16_t);
    fmt.block_align = 2 * sizeof(int16_t);
    fmt.bits_per_sample = 16;
    fwrite(&fmt, sizeof(FMTChunk), 1, fp);

    // Write DATA chunk header
    DATAChunk data;
    memcpy(data.data, "data", 4);
    data.data_size = data_size;
    fwrite(&data, sizeof(DATAChunk), 1, fp);

    // Write audio data (convert 32-bit to 16-bit)
    for (uint32_t i = 0; i < num_samples * 2; i++)
    {
        // Divide by 2 to convert from 32-bit to 16-bit range
        int16_t sample = (int16_t)(buffer[i] / 2);
        fwrite(&sample, sizeof(int16_t), 1, fp);
    }

    fclose(fp);
    printf("✅ Saved WAV file: %s\n", filename);
    return 1;
}
