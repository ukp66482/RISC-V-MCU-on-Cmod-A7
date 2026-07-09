/* ===========================================================================
 * esp32_bridge - turns an ESP32 DevKit into an I2C + SPI "receiver station"
 * for the RISC-V MCU showcase, so the serial buses can be demonstrated and
 * verified without any display modules.
 *
 * Open the Arduino serial monitor at 115200: every telemetry line the MCU
 * pushes over I2C scrolls by, and every SPI frame is hex/ASCII dumped.
 *
 * Wiring (all 3.3 V, common ground, ESP32 powered from its own USB):
 *   MCU DIP 13 (SCL)  -> GPIO 22        MCU DIP 35 (SCK)  -> GPIO 18
 *   MCU DIP 14 (SDA)  -> GPIO 21        MCU DIP 36 (MOSI) -> GPIO 23
 *   MCU DIP 25 (GND)  -> GND            MCU DIP 37 (MISO) <- GPIO 19
 *                                       MCU DIP 38 (SS0)  -> GPIO 5
 *
 * Protocol:
 *   I2C slave at 0x28. Writes are treated as text (telemetry lines).
 *   A read returns one byte: XOR checksum of the last message received -
 *   the MCU uses it for the round-trip PASS check ('n' command).
 *   SPI slave: 32-byte frames; the reply queued for every frame starts
 *   with "ESP32-OK".
 * =========================================================================*/
#include <Wire.h>
#include <WiFi.h>
#include "driver/spi_slave.h"

#ifndef VSPI_HOST
#define VSPI_HOST SPI3_HOST          /* arduino-esp32 core 3.x naming */
#endif

#define I2C_ADDR   0x28
#define PIN_SCK    18
#define PIN_MOSI   23
#define PIN_MISO   19
#define PIN_CS     5

/* ---- I2C slave ---- */
static volatile uint8_t  i2c_sum;          /* XOR of last message  */
static volatile bool     i2c_flag;
static char              i2c_buf[128];
static volatile int      i2c_len;

void onI2CReceive(int n)
{
    uint8_t sum = 0;
    int len = 0;
    while (Wire.available()) {
        char c = Wire.read();
        sum ^= (uint8_t)c;
        if (len < (int)sizeof(i2c_buf) - 1)
            i2c_buf[len++] = c;
    }
    i2c_buf[len] = 0;
    i2c_len = len;
    i2c_sum = sum;
    i2c_flag = true;
    /* pre-stage the reply NOW: the master's read-back races the request
     * callback (the byte is clocked out before onRequest can fill the TX
     * FIFO), so load the checksum into the slave TX buffer here, while we
     * already know it - the next read returns it immediately */
    uint8_t s = sum;
    Wire.slaveWrite(&s, 1);
}

void onI2CRequest(void)
{
    Wire.write(i2c_sum);                 /* fallback if slaveWrite lapsed */
}

/* ---- SPI slave (hardware, VSPI) ---- */
/* DMA wants 32-bit aligned buffers in internal RAM */
static uint8_t spi_rx[32] __attribute__((aligned(4)));
static uint8_t spi_tx[32] __attribute__((aligned(4)));

void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_OFF);

    Wire.begin((uint8_t)I2C_ADDR, 21, 22, 100000);
    Wire.onReceive(onI2CReceive);
    Wire.onRequest(onI2CRequest);

    spi_slave_interface_config_t scfg = {};
    scfg.spics_io_num = PIN_CS;
    scfg.flags = 0;
    scfg.queue_size = 2;
    scfg.mode = 0;
    spi_bus_config_t bcfg = {};
    bcfg.mosi_io_num = PIN_MOSI;
    bcfg.miso_io_num = PIN_MISO;
    bcfg.sclk_io_num = PIN_SCK;
    bcfg.quadwp_io_num = -1;
    bcfg.quadhd_io_num = -1;
    esp_err_t e = spi_slave_initialize(VSPI_HOST, &bcfg, &scfg, SPI_DMA_CH_AUTO);

    Serial.println();
    Serial.println("=========================================================");
    Serial.println("        RISC-V MCU  -  ESP32 receiver station");
    Serial.println("=========================================================");
    Serial.printf("  I2C slave 0x%02X (SDA=21 SCL=22)   SPI slave %s\r\n",
                  I2C_ADDR, e == ESP_OK ? "(SCK18 MOSI23 MISO19 CS5)"
                                        : "INIT FAILED");
    Serial.println("  live telemetry decoded below; [ok] = that bus delivered");
    Serial.println("---------------------------------------------------------");
}

/* parse "$MCU,up,mv,angle,mode,btn"; returns true on a valid telemetry line */
static bool parse_mcu(const char *s, long *up, long *mv, long *ang,
                      char *mode, long *btn)
{
    if (strncmp(s, "$MCU,", 5) != 0)
        return false;
    return sscanf(s + 5, "%ld,%ld,%ld,%c,%ld", up, mv, ang, mode, btn) == 5;
}

/* copy the printable prefix of a raw SPI frame (drops the zero padding) */
static void trim_frame(const uint8_t *src, int n, char *dst, int cap)
{
    int j = 0;
    for (int i = 0; i < n && j < cap - 1; i++) {
        char c = (char)src[i];
        if (c == 0)
            break;
        dst[j++] = (c >= 32 && c < 127) ? c : '.';
    }
    dst[j] = 0;
}

/* print one decoded line, tagged with the bus it arrived on */
static void show(const char *bus, const char *csv)
{
    long up, mv, ang, btn;
    char mode;
    if (parse_mcu(csv, &up, &mv, &ang, &mode, &btn))
        Serial.printf("  [%s] up=%4lds | pot=%4ld mV | servo=%3ld deg | "
                      "mode %c | btn %ld\r\n", bus, up, mv, ang, mode, btn);
    else
        Serial.printf("  [%s] control msg: \"%s\"\r\n", bus, csv);
}

void loop()
{
    /* --- I2C arrival --- */
    if (i2c_flag) {
        i2c_flag = false;
        show("I2C", i2c_buf);
    }

    /* --- keep one SPI transaction armed (100 ms timeout keeps loop live) --- */
    memset(spi_tx, 0, sizeof(spi_tx));
    strcpy((char *)spi_tx, "ESP32-OK bridge alive");
    spi_slave_transaction_t t = {};
    t.length = 8 * sizeof(spi_rx);
    t.tx_buffer = spi_tx;
    t.rx_buffer = spi_rx;
    if (spi_slave_transmit(VSPI_HOST, &t, pdMS_TO_TICKS(100)) == ESP_OK) {
        int n = t.trans_len / 8;
        char frame[40];
        trim_frame(spi_rx, n, frame, sizeof frame);
        if (frame[0])                    /* skip empty transactions */
            show("SPI", frame);
    }
}
