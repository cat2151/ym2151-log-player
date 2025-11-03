/*
 * Test program to measure YM2151 OPM chip actual output range
 * 
 * Purpose: Determine if dividing output by 2 is necessary
 * to prevent int16_t overflow during audio playback.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "../opm.h"
#include "types.h"

// Helper function to write register with proper delays
void write_register_with_delay(opm_t *chip, uint8_t addr, uint8_t data)
{
    // Write address
    OPM_Write(chip, 0, addr);
    for (int i = 0; i < DELAY_SAMPLES * CYCLES_PER_SAMPLE; i++)
    {
        OPM_Clock(chip, NULL, NULL, NULL, NULL);
    }
    
    // Write data
    OPM_Write(chip, 1, data);
    for (int i = 0; i < DELAY_SAMPLES * CYCLES_PER_SAMPLE; i++)
    {
        OPM_Clock(chip, NULL, NULL, NULL, NULL);
    }
}

// Test various note configurations to find maximum output
void test_max_output_range(void)
{
    opm_t chip;
    OPM_Reset(&chip);
    
    int32_t max_positive = 0;
    int32_t min_negative = 0;
    int32_t output[2];
    
    printf("Testing OPM chip output range...\n");
    printf("==========================================\n\n");
    
    // Test 1: Single loud note with full configuration (from sample_events.json)
    printf("Test 1: Single channel with maximum volume configuration\n");
    
    // Initialize all operators for channel 0 (based on sample_events.json)
    // Operator M1 (0x60)
    write_register_with_delay(&chip, 0x60, 0x00);  // TL=0 (max volume)
    write_register_with_delay(&chip, 0x80, 0x1F);  // AR (Attack Rate)
    write_register_with_delay(&chip, 0xA0, 0x05);  // D1R (Decay Rate)
    write_register_with_delay(&chip, 0xC0, 0x05);  // D2R (Sustain Rate)
    write_register_with_delay(&chip, 0xE0, 0xF7);  // RR (Release Rate)
    write_register_with_delay(&chip, 0x40, 0x01);  // MUL/DT1
    
    // Operator C1 (0x68)
    write_register_with_delay(&chip, 0x68, 0x7F);  // TL
    write_register_with_delay(&chip, 0x88, 0x1F);  // AR
    write_register_with_delay(&chip, 0xA8, 0x05);  // D1R
    write_register_with_delay(&chip, 0xC8, 0x05);  // D2R
    write_register_with_delay(&chip, 0xE8, 0xF7);  // RR
    write_register_with_delay(&chip, 0x48, 0x01);  // MUL/DT1
    
    // Operator M2 (0x70)
    write_register_with_delay(&chip, 0x70, 0x7F);  // TL
    write_register_with_delay(&chip, 0x90, 0x1F);  // AR
    write_register_with_delay(&chip, 0xB0, 0x05);  // D1R
    write_register_with_delay(&chip, 0xD0, 0x05);  // D2R
    write_register_with_delay(&chip, 0xF0, 0xF7);  // RR
    write_register_with_delay(&chip, 0x50, 0x01);  // MUL/DT1
    
    // Operator C2 (0x78)
    write_register_with_delay(&chip, 0x78, 0x7F);  // TL
    write_register_with_delay(&chip, 0x98, 0x1F);  // AR
    write_register_with_delay(&chip, 0xB8, 0x05);  // D1R
    write_register_with_delay(&chip, 0xD8, 0x05);  // D2R
    write_register_with_delay(&chip, 0xF8, 0xF7);  // RR
    write_register_with_delay(&chip, 0x58, 0x01);  // MUL/DT1
    
    // Channel settings
    write_register_with_delay(&chip, 0x20, 0xC7);  // RL/FL/CON for channel 0
    write_register_with_delay(&chip, 0x38, 0x00);  // PMS/AMS
    write_register_with_delay(&chip, 0x28, 0x3E);  // KC (Key Code)
    write_register_with_delay(&chip, 0x30, 0x00);  // KF (Key Fraction)
    
    // Key ON
    write_register_with_delay(&chip, 0x08, 0x78);  // Key ON all operators, channel 0
    
    // Generate samples and find range
    for (int i = 0; i < 50000; i++)
    {
        output[0] = 0;
        output[1] = 0;
        for (int j = 0; j < CYCLES_PER_SAMPLE; j++)
        {
            OPM_Clock(&chip, output, NULL, NULL, NULL);
        }
        
        if (output[0] > max_positive) max_positive = output[0];
        if (output[0] < min_negative) min_negative = output[0];
        if (output[1] > max_positive) max_positive = output[1];
        if (output[1] < min_negative) min_negative = output[1];
    }
    
    printf("  Range: %d to %d\n", min_negative, max_positive);
    printf("  int16_t range: -32768 to 32767\n");
    
    printf("  Range: %d to %d\n", min_negative, max_positive);
    printf("  int16_t range: -32768 to 32767\n");
    
    // Analysis
    printf("\n==========================================\n");
    printf("ANALYSIS RESULTS:\n");
    printf("==========================================\n");
    printf("OPM chip maximum output range: %d to %d\n", min_negative, max_positive);
    printf("int16_t range:                  -32768 to 32767\n\n");
    
    // Check if overflow would occur
    if (max_positive > 32767 || min_negative < -32768)
    {
        printf("⚠️  WARNING: Output EXCEEDS int16_t range!\n");
        printf("   Division by 2 is NECESSARY to prevent overflow.\n");
        printf("   Without /2, maximum clip would be: %d samples exceed range\n", 
               (max_positive > 32767 ? max_positive - 32767 : 0) + 
               (min_negative < -32768 ? -32768 - min_negative : 0));
    }
    else
    {
        printf("✅ Output is within int16_t range without division.\n");
        printf("   Headroom: +%d samples (positive), %d samples (negative)\n", 
               32767 - max_positive, min_negative - (-32768));
        printf("\n");
        printf("With /2 division:\n");
        printf("   Range becomes: %d to %d\n", min_negative / 2, max_positive / 2);
        printf("   Additional headroom: +%d samples (positive), %d samples (negative)\n",
               32767 - (max_positive / 2), (min_negative / 2) - (-32768));
    }
    
    printf("\n==========================================\n");
    printf("RECOMMENDATIONS:\n");
    printf("==========================================\n");
    if (max_positive > 32767 || min_negative < -32768)
    {
        printf("Division by 2 should be KEPT to prevent clipping.\n");
    }
    else
    {
        printf("Division by 2 provides extra headroom but is not strictly necessary\n");
        printf("to prevent overflow. Consider:\n");
        printf("  - Keep /2 for comfortable listening volume\n");
        printf("  - Keep /2 for safety margin against edge cases\n");
        printf("  - Remove /2 if maximum loudness is desired\n");
    }
    printf("==========================================\n");
}

int main(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  YM2151 OPM Chip Output Range Test                        ║\n");
    printf("║  音量を1/2にしている理由の可視化                           ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    test_max_output_range();
    
    printf("\n✅ Test completed.\n\n");
    return 0;
}
