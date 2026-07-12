## Cmod-A7-Master.xdc — pin constraints for the RISC-V MCU (top_wrapper)
##
## Only signals tied to custom top-level ports live here: the 48-pin DIP
## header, the USB UART, the XADC inputs, and the bitstream/boot settings.
## Pins wired through Digilent board-file interfaces are constrained
## automatically by Board-Files/ and are deliberately absent from this
## file: 12 MHz clock, on-board LEDs / RGB / button, QSPI flash, 512 KB SRAM.

## ── USB UART (FTDI — upload.py, stdout)
set_property -dict { PACKAGE_PIN J18   IOSTANDARD LVCMOS33 } [get_ports { uart_0_tx }]
set_property -dict { PACKAGE_PIN J17   IOSTANDARD LVCMOS33 } [get_ports { uart_0_rx }]

## ── XADC analog inputs — DIP 15/16 (0–3.3 V, differential pairs)
set_property -dict { PACKAGE_PIN G3    IOSTANDARD LVCMOS33 } [get_ports { vauxp4  }];  # DIP 15  ADC_0
set_property -dict { PACKAGE_PIN G2    IOSTANDARD LVCMOS33 } [get_ports { vauxn4  }];  # DIP 15  ADC_0 (n)
set_property -dict { PACKAGE_PIN H2    IOSTANDARD LVCMOS33 } [get_ports { vauxp12 }];  # DIP 16  ADC_1
set_property -dict { PACKAGE_PIN J2    IOSTANDARD LVCMOS33 } [get_ports { vauxn12 }];  # DIP 16  ADC_1 (n)

## ── DIP header, pins 1–14
set_property -dict { PACKAGE_PIN M3    IOSTANDARD LVCMOS33 } [get_ports { gpio_A_tri_io[0] }];  # DIP 01
set_property -dict { PACKAGE_PIN L3    IOSTANDARD LVCMOS33 } [get_ports { gpio_A_tri_io[1] }];  # DIP 02
set_property -dict { PACKAGE_PIN A16   IOSTANDARD LVCMOS33 } [get_ports { gpio_A_tri_io[2] }];  # DIP 03
set_property -dict { PACKAGE_PIN K3    IOSTANDARD LVCMOS33 } [get_ports { gpio_A_tri_io[3] }];  # DIP 04
set_property -dict { PACKAGE_PIN C15   IOSTANDARD LVCMOS33 } [get_ports { gpio_A_tri_io[4] }];  # DIP 05
set_property -dict { PACKAGE_PIN H1    IOSTANDARD LVCMOS33 } [get_ports { gpio_A_tri_io[5] }];  # DIP 06
set_property -dict { PACKAGE_PIN A15   IOSTANDARD LVCMOS33 } [get_ports { gpio_A_tri_io[6] }];  # DIP 07
set_property -dict { PACKAGE_PIN B15   IOSTANDARD LVCMOS33 } [get_ports { intr_tri_i[0]    }];  # DIP 08  INT_0
set_property -dict { PACKAGE_PIN A14   IOSTANDARD LVCMOS33 } [get_ports { intr_tri_i[1]    }];  # DIP 09  INT_1
set_property -dict { PACKAGE_PIN J3    IOSTANDARD LVCMOS33 } [get_ports { pwm_0            }];  # DIP 10  PWM_0
set_property -dict { PACKAGE_PIN J1    IOSTANDARD LVCMOS33 } [get_ports { uart_1_tx        }];  # DIP 11  UART TX
set_property -dict { PACKAGE_PIN K2    IOSTANDARD LVCMOS33 } [get_ports { uart_1_rx        }];  # DIP 12  UART RX
set_property -dict { PACKAGE_PIN L1    IOSTANDARD LVCMOS33 PULLUP true } [get_ports { i2c_ext_scl_io }];  # DIP 13  I2C SCL (add ext. 4.7k)
set_property -dict { PACKAGE_PIN L2    IOSTANDARD LVCMOS33 PULLUP true } [get_ports { i2c_ext_sda_io }];  # DIP 14  I2C SDA (add ext. 4.7k)
## DIP 15/16 = XADC (above) · DIP 24 = VU, 25 = GND (no FPGA I/O)

## ── DIP header, pins 17–23
set_property -dict { PACKAGE_PIN M1    IOSTANDARD LVCMOS33 } [get_ports { gpio_B_tri_io[0] }];  # DIP 17
set_property -dict { PACKAGE_PIN N3    IOSTANDARD LVCMOS33 } [get_ports { gpio_B_tri_io[1] }];  # DIP 18
set_property -dict { PACKAGE_PIN P3    IOSTANDARD LVCMOS33 } [get_ports { gpio_B_tri_io[2] }];  # DIP 19
set_property -dict { PACKAGE_PIN M2    IOSTANDARD LVCMOS33 } [get_ports { gpio_B_tri_io[3] }];  # DIP 20
set_property -dict { PACKAGE_PIN N1    IOSTANDARD LVCMOS33 } [get_ports { gpio_B_tri_io[4] }];  # DIP 21
set_property -dict { PACKAGE_PIN N2    IOSTANDARD LVCMOS33 } [get_ports { gpio_B_tri_io[5] }];  # DIP 22
set_property -dict { PACKAGE_PIN P1    IOSTANDARD LVCMOS33 } [get_ports { gpio_B_tri_io[6] }];  # DIP 23

## ── DIP header, pins 26–48
set_property -dict { PACKAGE_PIN R3    IOSTANDARD LVCMOS33 } [get_ports { gpio_D_tri_io[6] }];  # DIP 26
set_property -dict { PACKAGE_PIN T3    IOSTANDARD LVCMOS33 } [get_ports { gpio_D_tri_io[5] }];  # DIP 27
set_property -dict { PACKAGE_PIN R2    IOSTANDARD LVCMOS33 } [get_ports { gpio_D_tri_io[4] }];  # DIP 28
set_property -dict { PACKAGE_PIN T1    IOSTANDARD LVCMOS33 } [get_ports { gpio_D_tri_io[3] }];  # DIP 29
set_property -dict { PACKAGE_PIN T2    IOSTANDARD LVCMOS33 } [get_ports { gpio_D_tri_io[2] }];  # DIP 30
set_property -dict { PACKAGE_PIN U1    IOSTANDARD LVCMOS33 } [get_ports { gpio_D_tri_io[1] }];  # DIP 31
set_property -dict { PACKAGE_PIN W2    IOSTANDARD LVCMOS33 } [get_ports { gpio_D_tri_io[0] }];  # DIP 32
set_property -dict { PACKAGE_PIN V2    IOSTANDARD LVCMOS33 } [get_ports { intr_tri_i[3]    }];  # DIP 33  INT_3
set_property -dict { PACKAGE_PIN W3    IOSTANDARD LVCMOS33 } [get_ports { pwm_1            }];  # DIP 34  PWM_1
set_property -dict { PACKAGE_PIN V3    IOSTANDARD LVCMOS33 } [get_ports { spi_ext_sck_io   }];  # DIP 35  SPI SCLK
set_property -dict { PACKAGE_PIN W5    IOSTANDARD LVCMOS33 } [get_ports { spi_ext_io0_io   }];  # DIP 36  SPI MOSI
set_property -dict { PACKAGE_PIN V4    IOSTANDARD LVCMOS33 } [get_ports { spi_ext_io1_io   }];  # DIP 37  SPI MISO
set_property -dict { PACKAGE_PIN U4    IOSTANDARD LVCMOS33 } [get_ports { spi_ext_ss_io[0] }];  # DIP 38  SPI SS0
set_property -dict { PACKAGE_PIN V5    IOSTANDARD LVCMOS33 } [get_ports { spi_ext_ss_io[1] }];  # DIP 39  SPI SS1
set_property -dict { PACKAGE_PIN W4    IOSTANDARD LVCMOS33 } [get_ports { pwm_2            }];  # DIP 40  PWM_2
set_property -dict { PACKAGE_PIN U5    IOSTANDARD LVCMOS33 } [get_ports { intr_tri_i[2]    }];  # DIP 41  INT_2
set_property -dict { PACKAGE_PIN U2    IOSTANDARD LVCMOS33 } [get_ports { gpio_C_tri_io[6] }];  # DIP 42
set_property -dict { PACKAGE_PIN W6    IOSTANDARD LVCMOS33 } [get_ports { gpio_C_tri_io[5] }];  # DIP 43
set_property -dict { PACKAGE_PIN U3    IOSTANDARD LVCMOS33 } [get_ports { gpio_C_tri_io[4] }];  # DIP 44
set_property -dict { PACKAGE_PIN U7    IOSTANDARD LVCMOS33 } [get_ports { gpio_C_tri_io[3] }];  # DIP 45
set_property -dict { PACKAGE_PIN W7    IOSTANDARD LVCMOS33 } [get_ports { gpio_C_tri_io[2] }];  # DIP 46
set_property -dict { PACKAGE_PIN U8    IOSTANDARD LVCMOS33 } [get_ports { gpio_C_tri_io[1] }];  # DIP 47
set_property -dict { PACKAGE_PIN V8    IOSTANDARD LVCMOS33 } [get_ports { gpio_C_tri_io[0] }];  # DIP 48

## ── Unused connectors (available for expansion) ─────────────────────────
## Pmod header JA (8 data pins):
#set_property -dict { PACKAGE_PIN G17   IOSTANDARD LVCMOS33 } [get_ports { ja[0] }]
#set_property -dict { PACKAGE_PIN G19   IOSTANDARD LVCMOS33 } [get_ports { ja[1] }]
#set_property -dict { PACKAGE_PIN N18   IOSTANDARD LVCMOS33 } [get_ports { ja[2] }]
#set_property -dict { PACKAGE_PIN L18   IOSTANDARD LVCMOS33 } [get_ports { ja[3] }]
#set_property -dict { PACKAGE_PIN H17   IOSTANDARD LVCMOS33 } [get_ports { ja[4] }]
#set_property -dict { PACKAGE_PIN H19   IOSTANDARD LVCMOS33 } [get_ports { ja[5] }]
#set_property -dict { PACKAGE_PIN J19   IOSTANDARD LVCMOS33 } [get_ports { ja[6] }]
#set_property -dict { PACKAGE_PIN K18   IOSTANDARD LVCMOS33 } [get_ports { ja[7] }]

## ── Configuration / QSPI boot (verified on Macronix MX25L3273F) ─────────
## Single-width x1 read at 33 MHz + compression: power-on config still well
## under a second (~0.4 s for the ~1.6 MB compressed image), and — decisive,
## board-tested 2026-07-12 — x1 configuration works with the flash QE bit
## CLEARED. The flash-programming tools clear the non-volatile QE bit on
## every app-slot write; an x4 bitstream then cannot boot until QE is
## restored, an x1 bitstream does not care. x4 was used until 2026-07-12.
set_property BITSTREAM.CONFIG.SPI_BUSWIDTH 1 [current_design]
set_property BITSTREAM.CONFIG.CONFIGRATE 33 [current_design]
set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]
