/*
 * NiMH Charger & Tester
 * Copyright (C) 2025 K.Takata
 *
 * Hardware: https://github.com/k-takata/PCB_NiMH_Charger_Tester
 */

//
// NOTE: Use the following settings when compiling:
//  - MVIO: "Disabled"
//  - printf(): "Full x.xk, prints floats"
//

#include <EEPROM.h>

#include <Adafruit_SSD1306.h>
#include "AnonymousPro8pt7b.h"

#define SPLASH_MESSAGE  "\nNiMH Charger & Tester\n\nVer. 1.02"


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

constexpr int baseline_Anonymous_Pro8pt = 11;


// Use the temperature sensor near the battery.
//#define USE_TEMP2


//////////////////////////////
// Pin definitions

// PORTA pins: 0~7
// PA0: Serial TX
// PA1: Serial RX
// PA2: I2C SDA
// PA3: I2C SCL
// PA4: NC
// PA5: NC
#define PIN_CHGCTL  PIN_PA6
#define PIN_DISCTL  PIN_PA7

// PORTC pins: 0~3
#define PIN_VBT1    PIN_PC0
#define PIN_VBT2    PIN_PC1
#define PIN_VTEMP1  PIN_PC2
#define PIN_VTEMP2  PIN_PC3

// PORTD pins: 0~7
#define PIN_PWMCHG  PIN_PD1
#define PIN_PWMDIS  PIN_PD2
#define PIN_SW1     PIN_PD3
#define PIN_SW2     PIN_PD4
#define PIN_SW3     PIN_PD5
// PD6: NC
#define PIN_VREFA   PIN_PD7

// PORTF pins: 0,1,6,7
#define PIN_VCHG    PIN_PF0
#define PIN_VDIS    PIN_PF1
// PF6: NC (~RESET)
// PF7: UPDI


//////////////////////////////
// Calibration settings

// Actual voltage of TP1 (VREFA, TL431). Ideally, 2.495 V.
const float vrefa_calib = 2.483;
//const float vrefa_calib = 2.474;
//const float vrefa_calib = 2.480;

// Load resistor for charging: 10 Ω
const float r_charge = 10.0;
// Load resistor for discharging: 1 Ω
const float r_discharge = 1.0;


// Charge ending voltage at 25 ℃ (when -ΔV detection doesn't work).
// Assuming -3 mV/K coef.
const float v_charge_end_default = 1.53;
// Discharge ending voltage.
const float v_discharge_end_default = 1.00;

#define V_CHARGE_END_MAX  1.60
#define V_CHARGE_END_MIN  1.30
#define V_DISCHARGE_END_MAX  1.10
#define V_DISCHARGE_END_MIN  0.90

// -ΔV detection start voltage
const float mdv_start = 1.44;

// -ΔV threshold voltage
const float mdv_threshold = 0.004;

#define MDV_COUNT 20

// 0 dV/dt detection count
#define ZDV_COUNT 180


enum LogEndReason { EndNone, EndMdv, EndZdv, EndVolt, EndTime };

//////////////////////////////


// Charge control
void chgctl(bool enable)
{
  digitalWrite(PIN_CHGCTL, enable ? HIGH : LOW);

  if (!enable) {
    // Set charging current to 0.0 mA.
    if (TCA0.SINGLE.CTRLA & TCA_SINGLE_ENABLE_bm) {
      setChgCurrent(0.0);
    }
    else {
      digitalWrite(PIN_PWMCHG, HIGH);
    }
  }
}

// Discharge control
void disctl(bool enable)
{
  digitalWrite(PIN_DISCTL, enable ? LOW : HIGH);

  if (!enable) {
    // Set discharging current to 0.0 mA.
    if (TCA0.SINGLE.CTRLA & TCA_SINGLE_ENABLE_bm) {
      setDisCurrent(0.0);
    }
    else {
      digitalWrite(PIN_PWMDIS, LOW);
    }
  }
}


// Voltage functions

// Calculate the voltage of Vdd by using the ADC value of TL431
float getVddVolt()
{
  int vrefa = analogReadEnh(PIN_VREFA, 15);
  float vdd = vrefa_calib * 32768.0 / vrefa;
  return vdd;
}

// Get BTn voltage
float getBtVolt(int bt, bool hires=true)
{
  if (hires) {
    int vbt = analogReadEnh(PIN_VBT1 + bt - 1, 15);
    return vbt * getVddVolt() / 32768.0;
  }
  else {
    int vbt = analogRead(PIN_VBT1 + bt - 1);
    return vbt * getVddVolt() / 4096.0;
  }
}


// PWM functions

#define PWM_MAX_VAL 1023  // 10 bits

// Initialize PWM (TCA0)
void initPwm()
{
  takeOverTCA0();
  TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP1EN_bm | TCA_SINGLE_CMP2EN_bm | TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
  TCA0.SINGLE.PER = PWM_MAX_VAL;
  //PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTD_gc;  // PWM on PORTD (default)

  setPwmChg(PWM_MAX_VAL);
  setPwmDis(0);

  TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm;
}

// Clip PWM value
int clipPwmVal(int val)
{
  if (val < 0) {
    val = 0;
  }
  else if (val > PWM_MAX_VAL) {
    val = PWM_MAX_VAL;
  }
  return val;
}

#ifndef PIN_VCHG
int last_pwm_chg = PWM_MAX_VAL;
#endif

// Set PWM value of PIN_PWMCHG
void setPwmChg(int val)
{
  val = clipPwmVal(val);
  TCA0.SINGLE.CMP1 = val;
#ifndef PIN_VCHG
  last_pwm_chg = val;
#endif
}

#ifndef PIN_VDIS
int last_pwm_dis = 0;
#endif

// Set PWM value of PIN_PWMDIS
void setPwmDis(int val)
{
  val = clipPwmVal(val);
  TCA0.SINGLE.CMP2 = val;
#ifndef PIN_VDIS
  last_pwm_dis = val;
#endif
}


// Current functions

// Set charging current in mA
// minimum precision: Vdd / (PWM_MAX_VAL + 1) / r_charge = 5.0 / 1024 / 10 = 0.488 mA
void setChgCurrent(float ma)
{
  float vdd = getVddVolt();
  float v = vdd - ma * r_charge / 1000.0;
  int val = int(round(PWM_MAX_VAL * v / vdd));
  setPwmChg(val);
}

// Get charging current in mA
float getChgCurrent()
{
  float vdd = getVddVolt();
#ifdef PIN_VCHG
  return vdd * (32767 - analogReadEnh(PIN_VCHG, 15)) / 32768.0 / r_charge * 1000.0;
#else
  return (vdd - last_pwm_chg * vdd / PWM_MAX_VAL) / r_charge * 1000.0;
#endif
}

// Set discharging current in mA
// minimum precision: Vdd / (PWM_MAX_VAL + 1) / r_discharge = 5.0 / 1024 / 1.0 = 4.88 mA
void setDisCurrent(float ma)
{
  float vdd = getVddVolt();
  float v = ma * r_discharge / 1000.0;
  int val = int(round(PWM_MAX_VAL * v / vdd));
  setPwmDis(val);
}

// Get discharging current in mA
float getDisCurrent()
{
  float vdd = getVddVolt();
#ifdef PIN_VDIS
  return (vdd * analogReadEnh(PIN_VDIS, 15) / 32768.0) / r_discharge * 1000.0;
#else
  return last_pwm_dis * vdd / PWM_MAX_VAL / r_discharge * 1000.0;
#endif
}

// Get charging/discharging current in mA
float getCurrent(bool chg)
{
  if (chg) {
    return getChgCurrent();
  }
  else {
    return getDisCurrent();
  }
}


// Temperature functions

// Get the temperature of the board
float getTemp1()
{
  // MCP9700BT-E/TT
  // Offset: 500 mV at 0 ℃, Coef: 10.0 mV/℃
  // V = 0.5 + 0.01 T  ->  T = 100 V - 50
  int vtemp = analogReadEnh(PIN_VTEMP1, 14);
  return vtemp * getVddVolt() / 40.96 / 4 - 50.0;
}

// Get the temperature near the batteries
float getTemp2()
{
  int vtemp = analogReadEnh(PIN_VTEMP2, 14);
  return vtemp * getVddVolt() / 40.96 / 4 - 50.0;
}

// Get the temperature of AVR
float getTempAvr()
{
  int adc_temp = analogReadEnh(ADC_TEMPERATURE, 14);
  float sigrow_offset = SIGROW_TEMPSENSE1;
  float sigrow_slope = SIGROW_TEMPSENSE0;
  return (sigrow_offset - adc_temp * getVddVolt() / 2.048 / 4) * sigrow_slope / 4096 - 273;
}


#define BT_HIGH_THRESHOLD 1.8
#define BT_LOW_THRESHOLD  0.5

// Check battery connection
//  return:
//   -1: Error. BT1 or BT2 is over 1.8V.
//    0: Batteries are not connected.
//    1: BT1 is connected.
//    2: BT2 is connected.
//    3: Both BT1 and BT2 are connected.
int checkBtConnection()
{
  chgctl(false);
  disctl(false);

  // Discharge the capacitors connected to the VBT1,2 pins.
  pinMode(PIN_VBT1, OUTPUT);
  pinMode(PIN_VBT2, OUTPUT);
  digitalWrite(PIN_VBT1, LOW);
  digitalWrite(PIN_VBT2, LOW);
  delay(20);

  pinMode(PIN_VBT1, INPUT);
  pinMode(PIN_VBT2, INPUT);
  delay(1);

  float vbt1 = getBtVolt(1, false);
  float vbt2 = getBtVolt(2, false);

  int ret = 0;
  if (vbt1 > BT_HIGH_THRESHOLD || vbt2 > BT_HIGH_THRESHOLD) {
    ret = -1;
  }
  else {
    if (vbt1 > BT_LOW_THRESHOLD) {
      ret |= 1;
    }
    if (vbt2 > BT_LOW_THRESHOLD) {
      ret |= 2;
    }
  }
  Serial.printf(F("checkBtConnection: %.3f V, %.3f V, ret=%d\n"), vbt1, vbt2, ret);
  return ret;
}


void setup()
{
  pinMode(PIN_CHGCTL, OUTPUT);
  pinMode(PIN_DISCTL, OUTPUT);
  pinMode(PIN_PWMCHG, OUTPUT);
  pinMode(PIN_PWMDIS, OUTPUT);
  chgctl(false);
  disctl(false);
  pinMode(PIN_SW1, INPUT_PULLUP);
  pinMode(PIN_SW2, INPUT_PULLUP);
  pinMode(PIN_SW3, INPUT_PULLUP);

  analogReference(VDD);
  analogReadResolution(12);
  initPwm();

  Serial.begin(115200);

  // Display setup
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.clearDisplay();
  display.print(F(SPLASH_MESSAGE));
  display.display();

  loadSettings();

  attachInterrupt(digitalPinToInterrupt(PIN_SW1), buttonPushed1, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_SW2), buttonPushed2, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_SW3), buttonPushed3, FALLING);

  delay(2000);    // To show the splash screen
}


constexpr int debounceDelay = 50; // [ms]

volatile unsigned long pushed_time[3] = {0, 0, 0};
volatile bool button_pushed[3] = {false, false, false};

void buttonPushed(int n)
{
  const int i = n - 1;
  if (!button_pushed[i]) {
    pushed_time[i] = millis();
    button_pushed[i] = true;
  }
}

void buttonPushed1()
{
  buttonPushed(1);
}

void buttonPushed2()
{
  buttonPushed(2);
}

void buttonPushed3()
{
  buttonPushed(3);
}

bool checkButtonStatus(int n)
{
  const int i = n - 1;
  bool ret = false;
  if (button_pushed[i]) {
    if (millis() - pushed_time[i] >= debounceDelay) {
      if (digitalRead(PIN_SW1 + i) == LOW) {
        //Serial.printf("button %d pushed\n", n);
        ret = true;
      }
      button_pushed[i] = false;
    }
  }
  return ret;
}


enum State { Idle, Charging, Discharging };
State state = Idle;
State last_state = Idle;

enum DispMode { Detail = 0, Simple, Graph, NumModes };
DispMode disp_mode = Detail;

#define CURR_LEVEL_MAX  2
const float chg_current_table[2][CURR_LEVEL_MAX + 1] = {
  // For UM3 (AA)
  // 0.2C, 0.1C, 0.05C
  {400.0, 200.0, 100.0},
  // For UM4 (AAA)
  // 0.2C, 0.1C, 0.05C
  {140.0, 70.0, 35.0},
};
const float dis_current_table[2][CURR_LEVEL_MAX + 1] = {
  // For UM3 (AA)
  // 0.5C, 0.1C, 0.05C
  {1000.0, 200.0, 100.0},
  // For UM4 (AAA)
  // 0.5C, 0.1C, 0.05C
  {350.0, 70.0, 35.0},
};

int chg_curr_level = 1;
int dis_curr_level = 1;

float v_charge_end = v_charge_end_default;
float v_discharge_end = v_discharge_end_default;

const unsigned long chg_time_table[CURR_LEVEL_MAX + 1] = {
  7 * 3600UL * 1000UL,  // 0.2C
  14 * 3600UL * 1000UL, // 0.1C
  28 * 3600UL * 1000UL, // 0.05C
};


// SMA (Simple Moving Average) of (dis)charging voltage
#define SMA_NUM 10
static float sma_data[SMA_NUM];
static int sma_idx = 0;

void clearSma(float val=0.0)
{
  for (int i = 0; i < SMA_NUM; ++i) {
    sma_data[i] = val;
  }
  sma_idx = 0;
}

float updateSma(float val)
{
  sma_data[sma_idx] = val;
  sma_idx = (sma_idx + 1) % SMA_NUM;
  float total = 0.0;
  for (int i = 0; i < SMA_NUM; ++i) {
    total += sma_data[i];
  }
  return total / SMA_NUM;
}


// Data logging
struct LogData {
  uint32_t  vbt:11; // [1 mV] Max: 2.047 V
  uint32_t  res:12; // [5 mΩ] Max: 20.475 Ω, or [10 mΩ] Max: 40.95 Ω
  uint32_t  temp:9; // [0.1 ℃] Max: 51.1 ℃
};

#define MAX_LOG_DATA  1280
static LogData log_data[MAX_LOG_DATA];
static int log_capacity = 0;
static int log_idx = 0;
static bool log_chg = true;
static float log_res_scale = 0.0;
static LogEndReason log_reason = EndNone;

void clearLogData(bool chg)
{
  log_capacity = 0;
  log_idx = 0;
  log_chg = chg;
  log_res_scale = 0.0;
  log_reason = EndNone;
}

// Store log data every minute
void storeLogData(unsigned long time, float vbt, float res, float temp, float cap_mas, LogEndReason reason)
{
  if ((reason == EndNone) && (time < log_idx * 1000UL * 60UL)) {
    // Skip storing.
    return;
  }
  if (log_idx == MAX_LOG_DATA) {
    return;
  }
  if (log_idx == 0) {
    if (res < 5.0) {
      // Assuming max resistance < 20 Ω, use a higher resolution.
      log_res_scale = 200.0;
    }
    else {
      // Assuming max resistance > 20 Ω, use a lower resolution.
      log_res_scale = 100.0;
    }
  }
  log_data[log_idx].vbt = uint32_t(vbt * 1000);
  log_data[log_idx].res = uint32_t(res * log_res_scale);
  log_data[log_idx].temp = uint32_t(temp * 10);
  log_capacity = int(cap_mas / 3600);
  log_reason = reason;
  ++log_idx;
}

// Send the log data to PC
void sendLogData()
{
  if (log_chg) {
    Serial.print(F("Chg"));
  }
  else {
    Serial.print(F("Dis"));
  }
  Serial.println(F("/min,Volt,Ohm,Temp,Capacity,Reason"));
  for (int i = 0; i < log_idx; ++i) {
    Serial.printf(
      F("%d,%.3f,%.3f,%.1f,"),
      i,
      log_data[i].vbt / 1000.0,
      log_data[i].res / log_res_scale,
      log_data[i].temp / 10.0
    );
    if (i == log_idx - 1) {
      Serial.print(log_capacity);
      switch (log_reason) {
        case EndMdv:  Serial.print(F(",MinusDeltaV"));  break;
        case EndZdv:  Serial.print(F(",ZeroDeltaV"));   break;
        case EndVolt: Serial.print(F(",Voltage"));      break;
        case EndTime: Serial.print(F(",Time"));         break;
        default:  break;
      }
    }
    Serial.println();
  }
}

#define GRAPH_V_MAX 1600
#define GRAPH_V_MIN 1000

// Convert mV to Y coordinate
int mv_to_y(uint16_t mv)
{
  return (GRAPH_V_MAX - mv) * (SCREEN_HEIGHT - 1) / (GRAPH_V_MAX - GRAPH_V_MIN);
}

template<typename T, size_t N>
int isNearArray(const T (&arr)[N], int item)
{
  for (size_t i = 0; i < N; ++i) {
    if (arr[i] - 1 <= item && item <= arr[i] + 1) {
      return arr[i];
    }
  }
  return -1;
}

void displayHeader(bool chg)
{
  if (chg) {
    display.print(F("Chg: "));
  }
  else {
    display.print(F("Dis: "));
  }
}

// Show voltage graph
void showVoltGraph(unsigned long t)
{
  display.clearDisplay();
  display.setFont(nullptr);
  display.setCursor(25, 0);
  displayHeader(log_chg);
  display.print(tostrMillis(t));

  // Draw the scale marks: 1.0 - 1.5 V (0.1 V step)
  const int num_scale_marks = 6;
  int scale_marks[num_scale_marks];
  for (int i = 0; i < num_scale_marks; ++i) {
    int mv = GRAPH_V_MIN + 100 * i;
    int y = mv_to_y(mv);
    scale_marks[i] = y;
    //Serial.printf(F("mv=%d, y=%d\n"), mv, y);
    display.drawLine(0, y, SCREEN_WIDTH - 1, y, SSD1306_WHITE);
  }

  int old_ym = -1;
  int time_scale = (log_idx / SCREEN_WIDTH) + 1;
  int x;
  for (x = 0; x < SCREEN_WIDTH; ++x) {
    int i = x * time_scale;
    if (i >= log_idx) {
      break;
    }
    int vbt = log_data[i].vbt;
    // Clip the voltage value
    if (vbt > GRAPH_V_MAX) {
      vbt = GRAPH_V_MAX;
    }
    else if (vbt < GRAPH_V_MIN) {
      vbt = GRAPH_V_MIN;
    }
    int y = mv_to_y(vbt);
    //Serial.printf(F("i=%d, vbt=%d, (x,y)=(%d,%d)\n"), i, vbt, x, y);

    // Erase scale marks around the graph
    int ym = isNearArray(scale_marks, y);
    if (ym >= 0) {
      if (ym != y) {
        display.drawPixel(x, ym, SSD1306_BLACK);
      }
      if (x > 0 && old_ym < 0) {
        display.drawPixel(x - 1, ym, SSD1306_BLACK);
      }
    }
    else if (old_ym >= 0) {
      display.drawPixel(x, old_ym, SSD1306_BLACK);
    }
    old_ym = ym;

    // Draw the graph
    display.drawPixel(x, y, SSD1306_WHITE);
  }
  // Erase scale marks after the end of the graph
  if (x < SCREEN_WIDTH && old_ym >= 0) {
    display.drawPixel(x, old_ym, SSD1306_BLACK);
  }
}


#if 0
// Calculate battery's internal resistance on charging
// param:
//    vbt_idle [V]: Battery voltage on idle
//    vbt_chg [V]: Battery voltage on charging
//    curr [mA]: Charging current
float calcBtInternalResChg(float vbt_idle, float vbt_chg, float curr)
{
  return (vbt_chg - vbt_idle) / curr * 1000;
}

// Get battery's internal resistance on charging (Ohm)
float getBtInternalResChg(int bt, int curr_level=1)
{
  chgctl(false);
  disctl(false);
  delay(10);
  float vbt_idle = getBtVolt(bt);
  chgctl(true);

  setChgCurrent(chg_current_table[bt - 1][curr_level]);
  float chg_curr = getChgCurrent();
  delay(10);
  float vbt_chg = getBtVolt(bt);
  Serial.printf("chg_curr: %f mA, vbt_idle: %f V, vbt_chg: %f V\n", chg_curr, vbt_idle, vbt_chg);
  chgctl(false);
  return calcBtInternalResChg(vbt_idle, vbt_chg, chg_curr);
}

// Calculate battery's internal resistance on discharging
// param:
//    vbt_idle [V]: Battery voltage on idle
//    vbt_dis [V]: Battery voltage on discharging
//    curr [mA]: Discharging current
float calcBtInternalResDis(float vbt_idle, float vbt_dis, float curr)
{
  return (vbt_idle - vbt_dis) / curr * 1000;
}

// Get battery's internal resistance on discharging (Ohm)
float getBtInternalResDis(int bt, int curr_level=1)
{
  chgctl(false);
  disctl(false);
  delay(10);
  float vbt_idle = getBtVolt(bt);
  disctl(true);

  setDisCurrent(dis_current_table[bt - 1][curr_level]);
  float dis_curr = getDisCurrent();
  delay(100);
  float vbt_dis = getBtVolt(bt);
  Serial.printf("dis_curr: %f mA, vbt_idle: %f V, vbt_dis: %f V\n", dis_curr, vbt_idle, vbt_dis);
  chgctl(false);
  return calcBtInternalResDis(vbt_idle, vbt_dis, dis_curr);
}
#endif

// Calculate battery's internal resistance on (dis)charging
// param:
//    chg: true: charging, false: discharging
//    vbt_idle [V]: Battery voltage on idle
//    vbt_now [V]: Battery voltage on (dis)charging
//    curr [mA]: (Dis)charging current
float calcBtInternalRes(bool chg, float vbt_idle, float vbt_now, float curr)
{
  float val = (vbt_now - vbt_idle) / curr * 1000;
  return (chg) ? val : -val;
}


String tostrMillis(unsigned long ms)
{
  char buf[20];
  unsigned long t = ms / 1000;
  unsigned s = t % 60;
  t /= 60;
  unsigned m = t % 60;
  unsigned h = t / 60;
  snprintf_P(buf, sizeof(buf), PSTR("%02u:%02u:%02u"), h, m, s);
  return String(buf);
}


// Settings functions
struct Settings {
  int8_t chg_curr_level;
  int8_t dis_curr_level;
  float v_charge_end;
  float v_discharge_end;
  uint8_t key;
};
#define EEPROM_KEY  123
#define EEPROM_ADDR 0x00

// Load settings from EEPROM
void loadSettings()
{
  Settings settings;

  EEPROM.get(EEPROM_ADDR, settings);
  if (settings.key != EEPROM_KEY) {
    // Not saved to EEPROM. Load the defaults.
    settings.chg_curr_level = 1;
    settings.dis_curr_level = 1;
    settings.v_charge_end = v_charge_end_default;
    settings.v_discharge_end = v_discharge_end_default;
  }
  else {
    // Validate
    if (settings.chg_curr_level < 0 || settings.chg_curr_level > CURR_LEVEL_MAX) {
      settings.chg_curr_level = 1;
    }
    if (settings.dis_curr_level < 0 || settings.dis_curr_level > CURR_LEVEL_MAX) {
      settings.dis_curr_level = 1;
    }
    if (settings.v_charge_end < V_CHARGE_END_MIN || settings.v_charge_end > V_CHARGE_END_MAX) {
      v_charge_end = v_charge_end_default;
    }
    if (settings.v_discharge_end < V_DISCHARGE_END_MIN || settings.v_discharge_end > V_DISCHARGE_END_MAX) {
      v_discharge_end = v_discharge_end_default;
    }
  }
  chg_curr_level = settings.chg_curr_level;
  dis_curr_level = settings.dis_curr_level;
  v_charge_end = settings.v_charge_end;
  v_discharge_end = settings.v_discharge_end;
}

// Save settings to EEPROM
void saveSettings()
{
  Settings settings;

  settings.chg_curr_level = chg_curr_level;
  settings.dis_curr_level = dis_curr_level;
  settings.v_charge_end = v_charge_end;
  settings.v_discharge_end = v_discharge_end;
  settings.key = EEPROM_KEY;
  EEPROM.put(EEPROM_ADDR, settings);
}

// Main settings menu
void showMainMenu()
{
  const int num_item = 5;
  int cursor = 4;

  while (true) {
    display.clearDisplay();
    display.setFont(nullptr);
    display.setCursor(0, 0);
    display.println(F("Main settings:"));
    display.printf(F("%c Chg current\n"),  (cursor == 0) ? '>' : ' ');
    display.printf(F("%c Dis current\n"),  (cursor == 1) ? '>' : ' ');
    display.printf(F("%c Chg end volt\n"), (cursor == 2) ? '>' : ' ');
    display.printf(F("%c Dis end volt\n"), (cursor == 3) ? '>' : ' ');
    display.println();
    display.printf(F("%c Close\n"),        (cursor == 4) ? '>' : ' ');
    display.display();

    if (checkButtonStatus(1)) { // UP button
      cursor = (cursor - 1 + num_item) % num_item;
    }
    else if (checkButtonStatus(2)) {  // Down button
      cursor = (cursor + 1) % num_item;
    }
    else if (checkButtonStatus(3)) {  // Mode/OK button
      switch (cursor) {
        case 0: showCurrMenu(true);   break;
        case 1: showCurrMenu(false);  break;
        case 2: showVoltMenu(true);   break;
        case 3: showVoltMenu(false);  break;
        default:
          saveSettings();
          return;
      }
    }
    delay(10);
  }
}

// Show charging/discharging current settings menu
void showCurrMenu(bool chg)
{
  const int num_item = 5;
  int cursor = 0;
  int &curr_level = (chg) ? chg_curr_level : dis_curr_level;
  int select = curr_level;

  while (true) {
    display.clearDisplay();
    display.setFont(nullptr);
    display.setCursor(0, 0);
    if (chg) {
      display.println(F("Chg current:"));
      display.printf(F("%c%c 0.2C\n"), (cursor == 0) ? '>' : ' ', (select == 0) ? '*' : ' ');
    }
    else {
      display.println(F("Dis current:"));
      display.printf(F("%c%c 0.5C\n"), (cursor == 0) ? '>' : ' ', (select == 0) ? '*' : ' ');
    }
    display.printf(F("%c%c 0.1C\n"),   (cursor == 1) ? '>' : ' ', (select == 1) ? '*' : ' ');
    display.printf(F("%c%c 0.05C\n"),  (cursor == 2) ? '>' : ' ', (select == 2) ? '*' : ' ');
    display.println();
    display.printf(F("%c OK  %c Cancel\n"), (cursor == 3) ? '>' : ' ', (cursor == 4) ? '>' : ' ');
    display.display();

    if (checkButtonStatus(1)) { // Up button
      cursor = (cursor - 1 + num_item) % num_item;
    }
    else if (checkButtonStatus(2)) {  // Down button
      cursor = (cursor + 1) % num_item;
    }
    else if (checkButtonStatus(3)) {  // Mode/OK button
      if (cursor < 3) {
        select = cursor;
      }
      else {  // OK or Cancel
        if (cursor == 3) {  // OK
          curr_level = select;
        }
        return;
      }
    }
    delay(10);
  }
}

// Show charging/discharging voltage settings menu
void showVoltMenu(bool chg)
{
  const int num_item = 3;
  int cursor = 0;
  bool focus = false;
  float &v_end = (chg) ? v_charge_end : v_discharge_end;
  float volt = v_end;
  const float v_scale = 100.0;
  int ivolt = int(volt * v_scale);
  const int ivolt_max = (chg) ? int(V_CHARGE_END_MAX * v_scale) : int(V_DISCHARGE_END_MAX * v_scale);
  const int ivolt_min = (chg) ? int(V_CHARGE_END_MIN * v_scale) : int(V_DISCHARGE_END_MIN * v_scale);

  while (true) {
    display.clearDisplay();
    display.setFont(nullptr);
    display.setCursor(0, 0);
    if (chg) {
      display.println(F("Chg end volt:"));
    }
    else {
      display.println(F("Dis end volt:"));
    }
    display.println();
    display.printf(F("%c %c %4.2f V %c\n"), (cursor == 0) ? '>' : ' ', (focus) ? '+' : ' ', volt, (focus) ? '-' : ' ');
    display.println();
    display.printf(F("%c OK  %c Cancel\n"), (cursor == 1) ? '>' : ' ', (cursor == 2) ? '>' : ' ');
    display.display();

    if (checkButtonStatus(1)) { // Up button
      if (focus) {
        if (ivolt < ivolt_max) {
          ++ivolt;
          volt = ivolt / v_scale;
        }
      }
      else {
        cursor = (cursor - 1 + num_item) % num_item;
      }
    }
    else if (checkButtonStatus(2)) {  // Down button
      if (focus) {
        if (ivolt > ivolt_min) {
          --ivolt;
          volt = ivolt / v_scale;
        }
      }
      else {
        cursor = (cursor + 1) % num_item;
      }
    }
    else if (checkButtonStatus(3)) {  // Mode/OK button
      if (cursor == 0) {
        focus = !focus;
      }
      else {  // OK or Cancel
        if (cursor == 1) {  // OK
          v_end = volt;
        }
        return;
      }
    }
    delay(10);
  }
}

#if 0
// Show display settings menu
void showDispMenu()
{
  const int num_item = 5;
  int cursor = 0;
  int select = int(disp_mode);

  while (true) {
    display.clearDisplay();
    display.setFont(nullptr);
    display.setCursor(0, 0);
    display.println(F("Disp settings:"));
    display.printf(F("%c%c Detail\n"), (cursor == 0) ? '>' : ' ', (select == 0) ? '*' : ' ');
    display.printf(F("%c%c Simple\n"), (cursor == 1) ? '>' : ' ', (select == 1) ? '*' : ' ');
    display.printf(F("%c%c Graph\n"),  (cursor == 2) ? '>' : ' ', (select == 2) ? '*' : ' ');
    display.println();
    display.printf(F("%c OK  %c Cancel\n"), (cursor == 3) ? '>' : ' ', (cursor == 4) ? '>' : ' ');
    display.display();

    if (checkButtonStatus(1)) { // Up
      cursor = (cursor - 1 + num_item) % num_item;
    }
    else if (checkButtonStatus(2)) {  // Down
      cursor = (cursor + 1) % num_item;
    }
    else if (checkButtonStatus(3)) {
      if (cursor < 3) {
        select = cursor;
      }
      else {  // OK or Cancel
        if (cursor == 3) {  // OK
          disp_mode = static_cast<DispMode>(select);
        }
        return;
      }
    }
    delay(10);
  }
}
#endif


// Get a line from serial if available
String serialReadline()
{
  static char buf[64];
  static char lastc = '\0';
  static int idx = 0;

  while (Serial.available() > 0) {
    char c = Serial.read();
    if (lastc == '\r' && c == '\n') {
      idx = 0;
      lastc = c;
      continue;
    }
    buf[idx++] = lastc = c;
    if (c == '\n' || c == '\r' || idx == sizeof(buf) - 1) {
      buf[idx] = '\0';
      idx = 0;
      return String(buf);
    }
  }
  return String();
}


static int last_bt = 0;
static unsigned long last_bt_millis = 0;
static unsigned long start_millis = 0;
static unsigned long prev_millis = 0;
static unsigned long skip_total = 0;
static float cap_mas = 0.0;  // Capacity in mA * sec
static float prev_curr = 0.0;
static float mdv_max = 0.0;
static bool mdv_enable = false;
static int mdv_count = 0;
static int zdv_max = 0;
static int zdv_count = 0;

// Set charging/discharging current based on the current settings
void setCurrent(bool chg, int bt)
{
  if (chg) {
    setChgCurrent(chg_current_table[bt - 1][chg_curr_level]);
  }
  else {
    setDisCurrent(dis_current_table[bt - 1][dis_curr_level]);
  }
}

// Start charging/discharging
void startOperation(bool chg)
{
  int bt = checkBtConnection();
  if (bt == 1 || bt == 2) {
    if (chg) {
      disctl(false);
      chgctl(true);
    }
    else {
      chgctl(false);
      disctl(true);
    }
    setCurrent(chg, bt);
    if (start_millis == 0 || last_state != ((chg) ? Charging : Discharging)) {
      // Start (dis)charging, reset time and capacity
      prev_millis = start_millis = millis();
      skip_total = 0;
      cap_mas = 0.0;
      // Clear log
      clearLogData(chg);
    }
    else {
      // Resume (dis)charging, adjust time
      unsigned long t = millis();
      start_millis += t - prev_millis;
      prev_millis = t;
    }
    // Reset -ΔV settings (for charging)
    mdv_max = 0.0;
    mdv_enable = false;
    mdv_count = 0;
    zdv_max = 0;
    zdv_count = 0;
    // Clear SMA
    clearSma((chg) ? 0.0 : 1.5);

    prev_curr = getCurrent(chg);
    last_state = state = (chg) ? Charging : Discharging;
  }
  last_bt_millis = millis();
  last_bt = bt;
}

// Main operation for charging/discharging
void operate(bool chg, unsigned long now, float vbt_idle)
{
  if (chg) {
    chgctl(true);
  }
  else {
    disctl(true);
  }
  setCurrent(chg, last_bt);
  unsigned long t = millis();
  unsigned long skip = t - now;
  cap_mas += (t - prev_millis - skip) / 1000.0 * prev_curr;
  int cap = int(cap_mas / 3600.0);
  skip_total += skip;
  prev_millis = t;

  delay((chg) ? 20 : 100);
  float vbt_now = getBtVolt(last_bt);
  float curr = getCurrent(chg);
  float res = calcBtInternalRes(chg, vbt_idle, vbt_now, curr);
  float temp1 = getTemp1();
  float temp2 = getTemp2();
#ifdef USE_TEMP2
  float temp_now = temp2;
#else
  float temp_now = temp1;
#endif
  unsigned long op_time = t - start_millis - skip_total;
  if (disp_mode == Graph) {
    showVoltGraph(op_time);
  }
  else {
    if (disp_mode == Detail) {
      displayHeader(chg);
      display.printf(F("%5.3f V\n"), vbt_now);
      displayHeader(chg);
      display.printf(F("%5.1f mA\n"), curr);
      if (res >= 10.0) {
        display.printf(F("Res: %5.2f \xEA\n" /* Ω */), res);
      }
      else {
        display.printf(F("Res: %5.3f \xEA\n" /* Ω */), res);
      }
    }
    display.printf(F("Cap:  %4d mAh\n"), cap);
    display.println(String(F("Time: ")) + tostrMillis(op_time));
    if (disp_mode == Detail) {
      display.printf(F("Temp: %4.1f \xF8""C\n" /* U+00B0 */), temp_now);
    }
  }
  Serial.printf(F("time: %s, vbt_idle: %5.3f, vbt_now: %5.3f, curr: %5.1f, res: %5.3f, cap: %d, temp1: %4.1f, temp2: %4.1f\n"), tostrMillis(op_time).c_str(), vbt_idle, vbt_now, curr, res, cap, temp1, temp2);
  prev_curr = curr;

  float v_sma = updateSma(vbt_idle);

  LogEndReason reason = EndNone;

  // Check finish of operation
  if (chg) {
    // Detect -ΔV and 0 dV/dt
    if (v_sma >= mdv_start) {
      mdv_enable = true;
    }
    if (mdv_enable) {
      // 0 dV/dt
      int iv_sma = int(v_sma * 1000);   // Convert to mV
      if (iv_sma > zdv_max) {
        zdv_max = iv_sma;
        zdv_count = 0;
      }
      else {
        ++zdv_count;
        if (zdv_count > ZDV_COUNT) {  // TODO: Change the threshold based on chg_curr_level?
          // Finish charging.
          Serial.println(F("Chg end (0 dV/dt)"));
          chgctl(false);
          state = Idle;
          reason = EndZdv;
        }
      }

      // -ΔV
      if (v_sma > mdv_max) {
        mdv_max = v_sma;
        mdv_count = 0;
      }
      else if (mdv_max - mdv_threshold > v_sma) {
        ++mdv_count;
        if (mdv_count > MDV_COUNT) {  // TODO: Change the threshold based on chg_curr_level?
          // Finish charging.
          Serial.println(F("Chg end (-ΔV)"));
          chgctl(false);
          state = Idle;
          reason = EndMdv;
        }
      }
      Serial.printf(F("mdv_max: %.3f, v_sma: %.3f, mdv_count: %d, zdv_count: %d\n"), mdv_max, v_sma, mdv_count, zdv_count);
    }

    if (v_sma > v_charge_end + (-0.003 * (temp_now - 25.0))) {
      // Finish charging.
      Serial.printf(F("Chg end (voltage): v_sma=%.3f\n"), v_sma);
      chgctl(false);
      state = Idle;
      reason = EndVolt;
    }
    else if (op_time > chg_time_table[chg_curr_level]) {
      // Finish charging.
      Serial.printf(F("Chg end (time): time=%lu sec\n"), op_time / 1000);
      chgctl(false);
      state = Idle;
      reason = EndTime;
    }
  }
  else {
    if (v_sma < v_discharge_end) {
      // Finish discharging.
      Serial.printf(F("Dis end: v_sma=%.3f\n"), v_sma);
      disctl(false);
      state = Idle;
      reason = EndVolt;
    }
  }

  if (op_time > SMA_NUM * 1000) {
    // Store log data
    // Skip the first SMA_NUM seconds because the value is not reliable.
    storeLogData(op_time, v_sma, res, temp_now, cap_mas, reason);
  }
}

void loop()
{
  static int error_count = 0;

  bool disp = false;
  unsigned long now = millis();
  // Check battery status every second. Also, update display at the same time.
  if (now - last_bt_millis > 1000) {
    static State save_state = Idle;

    //Serial.printf("Vdd: %.3f V, Vbt1: %.3f V, Vbt2: %.3f V, Temp1: %.2f C, Temp2: %.2f C\n", getVddVolt(), getBtVolt(1), getBtVolt(2), getTemp1(), getTemp2());
    int bt = checkBtConnection();
    if (bt == 1 || bt == 2) {
      if ((error_count > 0) && (save_state != Idle)) {
        // Recover from battery connection error
        state = save_state;
        save_state = Idle;
        start_millis += now - prev_millis;
        prev_millis = now;
      }
      error_count = 0;
      //Serial.printf("Bt%d internal resistance: %f Ohm, %f Ohm\n", bt, getBtInternalResChg(bt), getBtInternalResDis(bt));
    }
    else {
      // Battery connection error
      ++error_count;
      if (error_count == 1) {
        // Save the state for recovery
        save_state = state;
        state = Idle;
        // Pause (dis)charging
        disctl(false);
        chgctl(false);
        prev_millis = now;
      }
      if (error_count > 5) {
        // Cannot recover
        prev_millis = start_millis = 0;
        cap_mas = 0.0;
        save_state = Idle;
        error_count = 0;
      }
    }
    last_bt_millis = now;
    last_bt = bt;
    disp = true;
    //Serial.printf(F("Temp1: %.2f, TempAVR: %.2f\n"), getTemp1(), getTempAvr());
  }
  float vbt_idle = 0.0;
  if (disp) {
    display.clearDisplay();
    if (disp_mode == Detail || disp_mode == Graph) {
      display.setFont(nullptr);
      display.setCursor(0, 0);
    }
    else {
      display.setFont(&Anonymous_Pro8pt7b);
      display.setCursor(0, baseline_Anonymous_Pro8pt);   // Set baseline
    }
    if (last_bt == 1 || last_bt == 2) {
      delay(10);
      vbt_idle = getBtVolt(last_bt);
    }
    if (disp_mode != Graph) {
      // Show Vdd and battery status
      display.printf(F("Vdd: %5.3f V\n"), getVddVolt());
      if (last_bt == 1 || last_bt == 2) {
        display.printf(F("BT%d: %5.3f V\n"), last_bt, vbt_idle);
      }
      else if (last_bt == 3) {
        display.println(F("BT: both"));
      }
      else if (last_bt == 0) {
        display.println(F("BT: no"));
      }
      else {
        display.println(F("BT: error"));
      }
    }
  }

  switch (state) {
    default:
    case Idle:
      if (checkButtonStatus(1)) { // Chg button
        startOperation(true);
        button_pushed[0] = false;
      }
      else if (checkButtonStatus(2)) {  // Dis button
        startOperation(false);
        button_pushed[1] = false;
      }
      else if (checkButtonStatus(3)) {  // Mode button
        if (disp_mode == Graph) {
          showMainMenu();
        }
        disp_mode = static_cast<DispMode>((int(disp_mode) + 1) % int(NumModes));
        last_bt_millis = 0; // To update display
      }
      else {  // No buttons pushed
        if (disp) {
          unsigned long t = 0;
          if (start_millis != 0) {
            t = prev_millis - start_millis - skip_total;
          }
          if (disp_mode == Graph) {
            showVoltGraph(t);
          }
          else {
            if (start_millis != 0) {
              display.printf(F("Cap:  %4d mAh\n"), int(cap_mas / 3600.0));
              display.println(String(F("Time: ")) + tostrMillis(t));
            }
            if (disp_mode == Detail) {
#ifdef USE_TEMP2
              float temp_now = getTemp2();
#else
              float temp_now = getTemp1();
#endif
              display.printf(F("Temp: %4.1f \xF8""C\n" /* U+00B0 */), temp_now);
              if ((last_state != Idle) && (log_reason != EndNone)) {
                const char *reason_str;
                switch (log_reason) {
                  case EndMdv:  reason_str = "-dv";   break;
                  case EndZdv:  reason_str = "0dv";   break;
                  case EndVolt: reason_str = "Volt";  break;
                  case EndTime: reason_str = "Time";  break;
                  default:      reason_str = nullptr; break;
                }
                display.printf(F("%s end: %s\n"),
                    (last_state == Charging) ? "Chg" : "Dis",
                    reason_str);
              }
            }
          }
        }
      }
      break;

    case Charging:
      if (disp) {
        operate(true, now, vbt_idle);
      }
      if (checkButtonStatus(1) || checkButtonStatus(2)) { // Chg or Dis button
        // Pause charging
        chgctl(false);
        state = Idle;
      }
      else if (checkButtonStatus(3)) {  // Mode button
        disp_mode = static_cast<DispMode>((int(disp_mode) + 1) % int(NumModes));
        last_bt_millis = 0; // To update display
      }
      break;

    case Discharging:
      if (disp) {
        operate(false, now, vbt_idle);
      }
      if (checkButtonStatus(1) || checkButtonStatus(2)) { // Chg or Dis button
        // Pause discharging
        disctl(false);
        state = Idle;
      }
      else if (checkButtonStatus(3)) {  // Mode button
        disp_mode = static_cast<DispMode>((int(disp_mode) + 1) % int(NumModes));
        last_bt_millis = 0; // To update display
      }
      break;
  }
  if (disp) {
    display.display();
  }

  String s = serialReadline();
  if (s.length() > 0) {
    s.trim();
    if (s.equals("send")) {
      sendLogData();
    }
  }

  delay(10);
}
