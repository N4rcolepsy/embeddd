/*
 * Educational ADXL355-format decoding exercise, C17.
 * Original teaching example, not the video's source and not a hardware driver.
 * Compile/run on a host with a C compiler:
 *   cc -std=c17 -Wall -Wextra -Wconversion -pedantic measurement_decode.c -o decode
 *   ./decode
 * Host C17 build/run passed on 2026-09-01; see memory_io/verification.json.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

/* Caller supplies at least two readable bytes. */
static uint16_t decode_temperature_raw(const uint8_t bytes[2])
{
    return (uint16_t)((((uint16_t)bytes[0] & 0x0Fu) << 8)
                      | (uint16_t)bytes[1]);
}

/* Caller supplies at least three readable bytes; low nibble is ignored. */
static int32_t decode_acceleration_raw(const uint8_t bytes[3])
{
    uint32_t u = ((uint32_t)bytes[0] << 12)
               | ((uint32_t)bytes[1] << 4)
               | ((uint32_t)bytes[2] >> 4);
    return (u & UINT32_C(0x80000)) != 0u
         ? (int32_t)u - INT32_C(0x100000)
         : (int32_t)u;
}

/* Nominal modern datasheet conversion; not a calibrated thermometer. */
static float temperature_c(uint16_t raw)
{
    return 25.0f + ((float)raw - 1885.0f) / -9.05f;
}

/* Only for the nominal +/-2 g range. A different range needs another scale. */
static float acceleration_mps2_2g(int32_t raw)
{
    return (float)raw * (9.80665f / 256000.0f);
}

int main(void)
{
    static const uint8_t cases[][3] = {
        {0x00u, 0x00u, 0x00u},
        {0x00u, 0x00u, 0x10u},
        {0x7Fu, 0xFFu, 0xF0u},
        {0x80u, 0x00u, 0x00u},
        {0xFFu, 0xFFu, 0xF0u},
        {0x3Eu, 0x80u, 0x00u},
        {0xC1u, 0x80u, 0x00u}
    };
    static const int32_t expected[] = {
        0, 1, 524287, -524288, -1, 256000, -256000
    };
    const uint8_t temp[] = {0x07u, 0x5Du}; /* 1885 */
    const uint8_t masked_temp[] = {0xF7u, 0x5Du};

    for (unsigned int i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        assert(decode_acceleration_raw(cases[i]) == expected[i]);
    }
    assert(decode_temperature_raw(temp) == 1885u);
    assert(decode_temperature_raw(masked_temp) == 1885u);
    assert(temperature_c(1885u) == 25.0f);

    printf("Example: +1 g = %.5f m/s^2\n",
           (double)acceleration_mps2_2g(256000));
    puts("Representative decoder checks passed.");
    return 0;
}
