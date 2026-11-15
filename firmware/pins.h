// Pins des reTerminal Sticky – aus reterminal-sticky FW 0.31.8 (Schaltplan reTerminal_Sticky_Schematic_diagram_260609)
#pragma once

static const int PIN_BTN_KI   = 4;    // Tasten aktiv low
static const int PIN_BTN_UP   = 5;
static const int PIN_BTN_DOWN = 6;
static const int PIN_PWR_HOLD = 45;   // Selbsthaltung, im Akkubetrieb sonst aus
static const int PIN_BUZZER   = 48;
static const int PIN_TP_SCL = 2, PIN_TP_SDA = 3, PIN_TP_INT = 21, PIN_TP_RST = 41, PIN_TP_EN = 42;
static const int PIN_MISC_SCL = 0, PIN_MISC_SDA = 1;   // Akku BQ27220, SHT40, LSM6DS3, PCF8563
static const int PIN_CHG_STATE = 40;  // low = lädt
static const int PIN_CHG_EN_N  = 39;  // aktiv LOW treiben, sonst lädt der Akku nie (interner Pull-up an MTCK)
static const int PIN_USB = 9;         // VIN_5V über Teiler, per ADC lesen
static const int PIN_SD_CS = 8, PIN_SD_PWR = 10, PIN_SD_DET = 11;

static const int W = 800, H = 480;
