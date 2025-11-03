#include "types.h"

// Process register events up to current sample time
void process_events_until(AudioContext *ctx, uint32_t current_sample)
{
    // Pass2 events already include timing for addr and data register writes
    // Each event specifies exactly when to write, no additional delays needed

    while (ctx->next_event_index < ctx->events->count)
    {
        RegisterEvent *event = &ctx->events->events[ctx->next_event_index];

        if (event->sample_time > current_sample)
        {
            break; // Haven't reached this event yet
        }

        // Write to the appropriate register (address or data)
        // The timing is already calculated in pass2, so no cycle consumption here
        if (event->is_data_write)
        {
            // Data register write
            OPM_Write(&ctx->chip, OPM_DATA_REGISTER, event->data);
        }
        else
        {
            // Address register write
            OPM_Write(&ctx->chip, OPM_ADDRESS_REGISTER, event->address);
        }

        ctx->next_event_index++;
    }
}

// MiniAudio data callback
void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    AudioContext *pContext = (AudioContext *)pDevice->pUserData;
    int16_t *pOutputS16 = (int16_t *)pOutput;

    (void)pInput;

    // Start timing measurement
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    if (!pContext->is_playing)
    {
        memset(pOutput, 0, frameCount * 2 * sizeof(int16_t));
        return;
    }

    // Calculate required input frames
    ma_uint64 requiredInputFrames = 0;
    ma_result result = ma_resampler_get_required_input_frame_count(&pContext->resampler, frameCount, &requiredInputFrames);
    if (result != MA_SUCCESS)
    {
        memset(pOutput, 0, frameCount * 2 * sizeof(int16_t));
        return;
    }

    if (requiredInputFrames > INTERNAL_BUFFER_SIZE)
    {
        requiredInputFrames = INTERNAL_BUFFER_SIZE;
    }

    // Generate internal samples
    ma_uint64 actualInputFrames = 0;
    for (ma_uint64 i = 0; i < requiredInputFrames; i++)
    {
        if (pContext->samples_played >= pContext->total_samples)
        {
            // Fill rest with silence
            for (ma_uint64 j = i; j < requiredInputFrames; j++)
            {
                pContext->internal_buffer[j * 2] = 0;
                pContext->internal_buffer[j * 2 + 1] = 0;
            }
            actualInputFrames = requiredInputFrames;
            pContext->is_playing = 0;
            break;
        }

        // Process any register events that should happen now
        process_events_until(pContext, pContext->samples_played);

        // Generate one stereo sample
        int32_t output[2] = {0, 0};
        for (int j = 0; j < CYCLES_PER_SAMPLE; j++)
        {
            OPM_Clock(&pContext->chip, output, NULL, NULL, NULL);
        }

        // Store to internal buffer (convert to 16-bit)
        pContext->internal_buffer[i * 2] = (int16_t)(output[0] / 2);
        pContext->internal_buffer[i * 2 + 1] = (int16_t)(output[1] / 2);

        // Also store to WAV buffer (keep as 32-bit)
        if (pContext->wav_buffer)
        {
            pContext->wav_buffer[pContext->wav_buffer_pos * 2] = output[0];
            pContext->wav_buffer[pContext->wav_buffer_pos * 2 + 1] = output[1];
            pContext->wav_buffer_pos++;
        }

        pContext->samples_played++;
    }

    if (actualInputFrames == 0)
    {
        actualInputFrames = requiredInputFrames;
    }

    // Resample
    ma_uint64 inputFramesProcessed = actualInputFrames;
    ma_uint64 outputFramesProcessed = frameCount;
    result = ma_resampler_process_pcm_frames(&pContext->resampler,
                                             pContext->internal_buffer, &inputFramesProcessed,
                                             pOutputS16, &outputFramesProcessed);

    if (result != MA_SUCCESS)
    {
        memset(pOutput, 0, frameCount * 2 * sizeof(int16_t));
        return;
    }

    // Fill remaining with silence
    if (outputFramesProcessed < frameCount)
    {
        for (ma_uint64 i = outputFramesProcessed; i < frameCount; i++)
        {
            pOutputS16[i * 2] = 0;
            pOutputS16[i * 2 + 1] = 0;
        }
    }

    // End timing measurement
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_ms = (end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                        (end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;

    // Update timing statistics
    pContext->total_callback_time_ms += elapsed_ms;
    pContext->callback_count++;
    
    if (elapsed_ms > pContext->max_callback_time_ms)
    {
        pContext->max_callback_time_ms = elapsed_ms;
    }
    
    if (elapsed_ms < pContext->min_callback_time_ms)
    {
        pContext->min_callback_time_ms = elapsed_ms;
    }
}
