/*
 Reprap firmware based on Sprinter
 Optimized for Sanguinololu 1.2 and above / RAMPS 

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>. */

/*
 This firmware is a mashup between Sprinter, grbl and parts from marlin.
 (https://github.com/kliment/Sprinter)

 Changes by Doppler Michael (midopple)

 Planner is from Simen Svale Skogsrud
 https://github.com/simen/grbl

 Parts of Marlin Firmware from ErikZalm
 https://github.com/ErikZalm/Marlin-non-gen6

 Sprinter Changelog
 -  Look forward function --> calculate 16 Steps forward, get from Firmaware Marlin and Grbl
 -  Stepper control with Timer 1 (Interrupt)
 -  Extruder heating with PID use a Softpwm (Timer 2) with 500 hz to free Timer1 for Steppercontrol
 -  command M220 Sxxx --> tune Printing speed online (+/- 50 %)
 -  G2 / G3 command --> circle function
 -  Baudrate set to 250 kbaud
 -  Testet on Sanguinololu Board
 -  M30 Command can delete files on SD Card
 -  move string to flash to free RAM vor forward planner
 -  M203 Temperature monitor for Repetier

 Version 1.3.04T
 - Implement Plannercode from Marlin V1 big thanks to Erik
 - Stepper interrupt with Step loops
 - Stepperfrequency 30 Khz
 - New Command
 * M202 - Set maximum feedrate that your machine can sustain (M203 X200 Y200 Z300 E10000) in mm/sec
 * M204 - Set default acceleration: S normal moves T filament only moves (M204 S3000 T7000) im mm/sec^2
 * M205 - advanced settings:  minimum travel speed S=while printing T=travel only,  X= maximum xy jerk, Z=maximum Z jerk, E = max E jerk
 - Remove unused Variables
 - Check Uart Puffer while circle processing (CMD: G2 / G3)
 - Fast Xfer Function --> move Text to Flash
 - Option to deactivate ARC (G2/G3) function (save flash)
 - Removed modulo (%) operator, which uses an expensive divide

 Version 1.3.05T
 - changed homing function to not conflict with min_software_endstops/max_software_endstops (thanks rGlory)
 - Changed check in arc_func
 - Corrected distance calculation. (thanks jv4779)
 - MAX Feed Rate for Z-Axis reduced to 2 mm/s some Printers had problems with 4 mm/s

 Version 1.3.06T
 - the microcontroller can store settings in the EEPROM
 - M500 - stores paramters in EEPROM
 - M501 - reads parameters from EEPROM (if you need reset them after you changed them temporarily).
 - M502 - reverts to the default "factory settings". You still need to store them in EEPROM afterwards if you want to.
 - M503 - Print settings

 Version 1.3.07T
 - Optimize Variable Size (faster Code)
 - Remove unused Code from Interrupt --> faster ~ 22 us per step
 - Replace abs with fabs --> Faster and smaler
 - Add "store_eeprom.cpp" to makefile

 Version 1.3.08T
 - If a line starts with ';', it is ignored but comment_mode is reset.
 A ';' inside a line ignores just the portion following the ';' character.
 The beginning of the line is still interpreted.

 - Same fix for SD Card, tested and work

 Version 1.3.09T
 - Move SLOWDOWN Function up

 Version 1.3.10T
 - Add info to GEN7 Pins
 - Update pins.h for gen7, working setup for 20MHz
 - calculate feedrate without extrude before planner block is set
 - New Board --> GEN7 @ 20 Mhz �
 - ENDSTOPS_ONLY_FOR_HOMING Option ignore Endstop always --> fault is cleared

 Version 1.3.11T
 - fix for broken include in store_eeprom.cpp  --> Thanks to kmeehl (issue #145)
 - Make fastio & Arduino pin numbering consistent for AT90USB128x. --> Thanks to lincomatic
 - Select Speedtable with F_CPU
 - Use same Values for Speedtables as Marlin

 Version 1.3.12T
 - Fixed arc offset.

 Version 1.3.13T
 - Extrudemultiply with code M221 Sxxx (S100 original Extrude value)
 - use Feedratefactor only when Extrude > 0
 - M106 / M107 can drive the FAN with PWM + Port check for not using Timer 1
 - Added M93 command. Sends current steps for all axis.
 - New Option --> FAN_SOFT_PWM, with this option the FAN PWM can use every digital I/O

 Version 1.3.14T
 - When endstop is hit count the virtual steps, so the print lose no position when endstop is hit

 Version 1.3.15T
 - M206 - set additional homing offset
 - Option for minimum FAN start speed --> #define MINIMUM_FAN_START_SPEED  50  (set it to zero to deactivate)

 Version 1.3.16T
 - Extra Max Feedrate for Retract (MAX_RETRACT_FEEDRATE)

 Version 1.3.17T
 - M303 - PID relay autotune possible
 - G4 Wait until last move is done

 Version 1.3.18T
 - Problem with Thermistor 3 table when sensor is broken and temp is -20 ｰC

 Version 1.3.19T
 - Set maximum acceleration. If "steps per unit" is Change the acc were not recalculated
 - Extra Parameter for Max Extruder Jerk
 - New Parameter (max_e_jerk) in EEPROM --> Default settings after update !

 Version 1.3.20T
 - fix a few typos and correct english usage
 - reimplement homing routine as an inline function
 - refactor eeprom routines to make it possible to modify the value of a single parameter
 - calculate eeprom parameter addresses based on previous param address plus sizeof(type)
 - add 0 C point in Thermistortable 7

 Version 1.3.21T
 - M301 set PID Parameter, and Store to EEPROM
 - If no PID is used, deaktivate Variables for PID settings

 Version 1.3.22T
 - Error in JERK calculation after G92 command is send, make problems
 with Z-Lift function in Slic3r
 - Add homing values can shown with M206 D

 */

//#include <avr/pgmspace.h>
#include <math.h>

#include "fastio.h"
#include "Configuration.h"
#include "pins.h"
#include "Sprinter.h"
#include "speed_lookuptable.h"
#include "heater.h"

#include <stdio.h>
#include <stdint.h>
#include "xil_types.h"
#include "xtmrctr.h"
#include "xparameters.h"
#include "xil_io.h"
#include "xil_exception.h"
#include "xgpio.h"
#include "xsysmon.h"
#include "xintc.h"

//#include "xparameters.h"
//#include "xil_io.h"
//#include "SeeedOLED.h"
#include "circular_buffer.h"
#include "timer.h"
#include "gpio.h"
#include "i2c.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
extern "C" {
  #include "uart.h"
}
#include "xuartlite.h"
#include "xuartlite_l.h"
#define UARTLITE_DEVICE2_ID	XPAR_UARTLITE_0_DEVICE_ID

uart uart_dev0;
char *ok_msg = "ok\r\n";
char temp_msg[32];
char dispenser_cmd_msg[32];
XUartLite UartLite1;
char dispenser_command_bytes[1];
bool cold_extrusion = false;

// settings for Musashi dispenser
#define STX 0x02
#define ETX 0x03
#define SPACE 0x20
#define ENQ 0x05
#define ACK 0x06
#define EOT 0x04
#define CAN 0x18
char discharge_dispenser_string[] = "04DI  CF";
char can_string[] = {'0','4',CAN,CAN,'6','C'};
char a0_string[] = {ACK, STX,'0','2','A','0','2','D',ETX};
char a2_string[] = {ACK, STX,'0','2','A','2','2','B',ETX};
int dispenser_delay_time = 100;

#define CLEAR_DISPLAY       0x1
#define PRINT_STRING        0x3

//XGpio LEDInst;
//XGpio BTNInst;
XGpio ShieldInst;
XGpio CK_ShieldInst;
//XGpio HeaterInst;
static int btn_value;

#define STEP_X_PIN 2
#define STEP_Y_PIN 3
#define STEP_Z_PIN 4
#define STEP_E_PIN 5 
#define DIR_X_PIN 6
#define DIR_Y_PIN 7
#define DIR_Z_PIN 8
#define DIR_E_PIN 9 
#define X_MIN_PIN 10 
#define Y_MIN_PIN 11 
#define Z_MIN_PIN 12

//#define HEATER_PIN 18 //A4
#define THERMISTOR_PIN 19 //A5

// ChipKit shield pins
#define X_EN_PIN 0
#define Y_EN_PIN 1
#define Z_EN_PIN 2
#define E_EN_PIN 3
#define HTR0_PIN 4
#define HTR1_PIN 5
#define FAN0_PIN 6
#define FAN1_PIN 7
#define DISP_PIN 8
#define UV_PIN 9 

//#define BTNS_DEVICE_ID    XPAR_BUTTONS_DEVICE_ID
//#define BTN_INT       XGPIO_IR_CH1_MASK
//#define LEDS_DEVICE_ID    XPAR_LEDS_DEVICE_ID
#define SHIELDS_DEVICE_ID   XPAR_IOP_ARDUINO_GPIO_SUBSYSTEM_ARDUINO_GPIO_BASEADDR
//#define CK_SHIELDS_DEVICE_ID  XPAR_IOP_ARDUINO_GPIO_SUBSYSTEM_CK_GPIO_BASEADDR
#define CK_SHIELDS_DEVICE_ID XPAR_GPIO_1_DEVICE_ID

//#define HEATER_DEVICE_ID  XPAR_IOP_ARDUINO_GPIO_SUBSYSTEM_CK_GPIO_BASEADDR

#define _SET(TARGET,BIT) (TARGET |= 1 << BIT)
#define _CLR(TARGET,BIT) (TARGET &= ~(1 << BIT))
#define _TGL(TARGET,BIT) (TARGET ^= 1 << BIT)
#define _CHK(TARGET,BIT) ((TARGET >> BIT) & 1)

static int shields_data;
static int ck_shields_data;
static int intr_cntr;
static int pulse_status = 0;
static XTmrCtr TimerInstancePtr;
static XTmrCtr TimerInstancePtr2;
static XTmrCtr TimerInstancePtr3;
static XTmrCtr TimerInstancePtr4;
static XTmrCtr TimerInstancePtr5;

#define SYSMON_DEVICE_ID XPAR_SYSMON_0_DEVICE_ID //ID of xadc_wiz_0
#define XSysMon_RawToExtVoltage(AdcData) ((((float)(AdcData))*(1.0f))/65536.0f) //(ADC 16bit result)/16/4096 = (ADC 16bit result)/65536
static XSysMon SysMonInst; //a sysmon instance
static int SysMonFractionToInt(float FloatNum);
u8 SeqMode;
u32 ExtVolRawData;
float ExtVolData;
XSysMon_Config *SysMonConfigPtr;
XSysMon *SysMonInstPtr = &SysMonInst;
#define PULSES_500HZ 0x30d40

// #define F_CPU 650000000
#define F_CPU 16000000

#ifdef USE_ARC_FUNCTION
#include "arc_func.h"
#endif

#ifdef SDSUPPORT
#include "SdFat.h"
#endif

#ifdef USE_EEPROM_SETTINGS
#include "store_eeprom.h"
#endif

#ifndef CRITICAL_SECTION_START
//#define CRITICAL_SECTION_START  unsigned char _sreg = SREG; cli()
//#define CRITICAL_SECTION_END    SREG = _sreg
#endif //CRITICAL_SECTION_START

void __cxa_pure_virtual() {
}
;

bool all_axis;
uint8_t current_tool_number = 0;
uint8_t last_tool_number;


#define DISPENSER_BASE_PRESSURE 4000 // 400.0[kPa]
float extruder_base_speed = 0.14; // When F=1200 (20[mm/s]), delta_E/delta_r = 0.14 (calculated from reference G-code from KJ)
int current_dispenser_pressure = DISPENSER_BASE_PRESSURE;
int last_dispenser_pressure = DISPENSER_BASE_PRESSURE;
bool is_first_block_element = true;
bool is_dispenser_running = false;
unsigned long previous_discarded_millis;
unsigned long previous_millis_dispenser = 0;
int success_count;
int total_count;
bool use_dispenser = true;

// look here for descriptions of gcodes: http://linuxcnc.org/handbook/gcode/g-code.html
// http://objects.reprap.org/wiki/Mendel_User_Manual:_RepRapGCodes

//Implemented Codes
//-------------------
// G0  -> G1
// G1  - Coordinated Movement X Y Z E
// G2  - CW ARC
// G3  - CCW ARC
// G4  - Dwell S<seconds> or P<milliseconds>
// G28 - Home all Axis
// G90 - Use Absolute Coordinates
// G91 - Use Relative Coordinates
// G92 - Set current position to cordinates given

//RepRap M Codes
// M104 - Set extruder target temp
// M105 - Read current temp
// M106 - Fan on
// M107 - Fan off
// M109 - Wait for extruder current temp to reach target temp.
// M112 - Emergency stop
// M114 - Display current position

//Custom M Codes
// M20  - List SD card
// M21  - Init SD card
// M22  - Release SD card
// M23  - Select SD file (M23 filename.g)
// M24  - Start/resume SD print
// M25  - Pause SD print
// M26  - Set SD position in bytes (M26 S12345)
// M27  - Report SD print status
// M28  - Start SD write (M28 filename.g)
// M29  - Stop SD write
//   -  <filename> - Delete file on sd card
// M42  - Set output on free pins, on a non pwm pin (over pin 13 on an arduino mega) use S255 to turn it on and S0 to turn it off. Use P to decide the pin (M42 P23 S255) would turn pin 23 on
// M80  - Turn on Power Supply
// M81  - Turn off Power Supply
// M82  - Set E codes absolute (default)
// M83  - Set E codes relative while in Absolute Coordinates (G90) mode
// M84  - Disable steppers until next move, 
//        or use S<seconds> to specify an inactivity timeout, after which the steppers will be disabled.  S0 to disable the timeout.
// M85  - Set inactivity shutdown timer with parameter S<seconds>. To disable set zero (default)
// M92  - Set axis_steps_per_unit - same syntax as G92
// M93  - Send axis_steps_per_unit
// M115 - Capabilities string
// M119 - Show Endstopper State 
// M140 - Set bed target temp
// M190 - Wait for bed current temp to reach target temp.
// M201 - Set maximum acceleration in units/s^2 for print moves (M201 X1000 Y1000)
// M202 - Set maximum feedrate that your machine can sustain (M203 X200 Y200 Z300 E10000) in mm/sec
// M203 - Set temperture monitor to Sx
// M204 - Set default acceleration: S normal moves T filament only moves (M204 S3000 T7000) in mm/sec^2
// M205 - advanced settings:  minimum travel speed S=while printing T=travel only,  X=maximum xy jerk, Z=maximum Z jerk
// M206 - set additional homing offset

// M220 - set speed factor override percentage S=factor in percent 
// M221 - set extruder multiply factor S100 --> original Extrude Speed 

// M301 - Set PID parameters P I and D
// M303 - PID relay autotune S<temperature> sets the target temperature. (default target temperature = 150C)

// M400 - Finish all moves

// M500 - stores paramters in EEPROM
// M501 - reads parameters from EEPROM (if you need to reset them after you changed them temporarily).
// M502 - reverts to the default "factory settings". You still need to store them in EEPROM afterwards if you want to.
// M503 - Print settings

// Debug feature / Testing the PID for Hotend
// M601 - Show Temp jitter from Extruder (min / max value from Hotend Temperature while printing)
// M602 - Reset Temp jitter from Extruder (min / max val) --> Don't use it while Printing
// M603 - Show Free Ram

// M700 - Versatile command for debug

#define _VERSION_TEXT "1.3.22T / 20.08.2012"

//Stepper Movement Variables
char axis_codes[NUM_AXIS] = { 'X', 'Y', 'Z', 'E' };
float axis_steps_per_unit[4] = _AXIS_STEP_PER_UNIT;

const float homing_feedrate_mm_m[] = {HOMING_FEEDRATE_Z, HOMING_FEEDRATE_Z,HOMING_FEEDRATE_Z, 0};
static float feedrate_mm_m = 1500.0, saved_feedrate_mm_m;
float max_feedrate[4] = _MAX_FEEDRATE;
float homing_feedrate[] = _HOMING_FEEDRATE;
bool axis_relative_modes[] = _AXIS_RELATIVE_MODES;

float move_acceleration = _ACCELERATION;         // Normal acceleration mm/s^2
float retract_acceleration = _RETRACT_ACCELERATION;// Normal acceleration mm/s^2
float max_xy_jerk = _MAX_XY_JERK;
float max_z_jerk = _MAX_Z_JERK;
float max_e_jerk = _MAX_E_JERK;
unsigned long min_seg_time = _MIN_SEG_TIME;
#ifdef PIDTEMP
unsigned int PID_Kp = PID_PGAIN,
    PID_Ki = PID_IGAIN, PID_Kd = PID_DGAIN;
#endif

long max_acceleration_units_per_sq_second[4] =
    _MAX_ACCELERATION_UNITS_PER_SQ_SECOND; // X, Y, Z and E max acceleration in mm/s^2 for printing moves or retracts

//float max_start_speed_units_per_second[] = _MAX_START_SPEED_UNITS_PER_SECOND;
//long  max_travel_acceleration_units_per_sq_second[] = _MAX_TRAVEL_ACCELERATION_UNITS_PER_SQ_SECOND; // X, Y, Z max acceleration in mm/s^2 for travel moves

float mintravelfeedrate = DEFAULT_MINTRAVELFEEDRATE;
float minimumfeedrate = DEFAULT_MINIMUMFEEDRATE;

unsigned long axis_steps_per_sqr_second[NUM_AXIS];
unsigned long plateau_steps;

//unsigned long axis_max_interval[NUM_AXIS];
//unsigned long axis_travel_steps_per_sqr_second[NUM_AXIS];
//unsigned long max_interval;
//unsigned long steps_per_sqr_second;

//adjustable feed factor for online tuning printer speed
volatile int feedmultiply=100;//100->original / 200 -> Factor 2 / 50 -> Factor 0.5
int saved_feedmultiply;
volatile bool feedmultiplychanged=false;
volatile int extrudemultiply=100;//100->1 200->2

//boolean acceleration_enabled = false, accelerating = false;
//unsigned long interval;
float destination[NUM_AXIS] = {0.0, 0.0, 0.0, 0.0};
float current_position[NUM_AXIS] = {0.0, 0.0, 0.0, 0.0};
float add_homing[3]= {0,0,0};

static unsigned short virtual_steps_x = 0;
static unsigned short virtual_steps_y = 0;
static unsigned short virtual_steps_z = 0;

bool home_all_axis = true;
bool home_x_axis = false;
bool home_y_axis = false;
bool home_z_axis = false;
//unsigned ?? ToDo: Check
int feedrate = 1500,
    next_feedrate, saved_feedrate;

long gcode_N, gcode_LastN;
bool relative_mode = false;  //Determines Absolute or Relative Coordinates

static volatile bool endstop_x_hit = false;
static volatile bool endstop_y_hit = false;
static volatile bool endstop_z_hit = false;


#define SIN_60 0.8660254037844386
#define COS_60 0.5
#define TOWER_1 X_AXIS
#define TOWER_2 Y_AXIS
#define TOWER_3 Z_AXIS

float delta_diagonal_rod = DELTA_DIAGONAL_ROD;
float delta_radius = DELTA_RADIUS;
float delta_radius_trim_tower_1 = DELTA_RADIUS_TRIM_TOWER_1;
float delta_radius_trim_tower_2 = DELTA_RADIUS_TRIM_TOWER_2;
float delta_radius_trim_tower_3 = DELTA_RADIUS_TRIM_TOWER_3;

float delta_diagonal_rod_trim_tower_1 = DELTA_DIAGONAL_ROD_TRIM_TOWER_1;
float delta_diagonal_rod_trim_tower_2 = DELTA_DIAGONAL_ROD_TRIM_TOWER_2;
float delta_diagonal_rod_trim_tower_3 = DELTA_DIAGONAL_ROD_TRIM_TOWER_3;
float delta_diagonal_rod_2_tower_1 = sq(delta_diagonal_rod + delta_diagonal_rod_trim_tower_1);
float delta_diagonal_rod_2_tower_2 = sq(delta_diagonal_rod + delta_diagonal_rod_trim_tower_2);
float delta_diagonal_rod_2_tower_3 = sq(delta_diagonal_rod + delta_diagonal_rod_trim_tower_3);
float delta_tower1_x = -SIN_60 * (delta_radius + DELTA_RADIUS_TRIM_TOWER_1); // front left tower
float delta_tower1_y = -COS_60 * (delta_radius + delta_radius_trim_tower_1);
float delta_tower2_x =  SIN_60 * (delta_radius + DELTA_RADIUS_TRIM_TOWER_2); // front right tower
float delta_tower2_y = -COS_60 * (delta_radius + delta_radius_trim_tower_2);
float delta_tower3_x = 0;                                                    // back middle tower
float delta_tower3_y = (delta_radius + delta_radius_trim_tower_3);
float delta[3];

//unsigned long steps_taken[NUM_AXIS];
//long axis_interval[NUM_AXIS]; // for speed delay
//float time_for_move;
//bool relative_mode_e = false;  //Determines Absolute or Relative E Codes while in Absolute Coordinates mode. E is always relative in Relative Coordinates mode.
//long timediff = 0;

bool is_homing = false;
int homing_status = 0;
bool clear_to_send = true;

//experimental feedrate calc
//float d = 0;
//float axis_diff[NUM_AXIS] = {0, 0, 0, 0};

#ifdef USE_ARC_FUNCTION
//For arc center point coordinates, sent by commands G2/G3
float offset[3] = {0.0, 0.0, 0.0};
#endif

#ifdef STEP_DELAY_RATIO
long long_step_delay_ratio = STEP_DELAY_RATIO * 100;
#endif

///oscillation reduction
#ifdef RAPID_OSCILLATION_REDUCTION
float cumm_wait_time_in_dir[NUM_AXIS]= {0.0,0.0,0.0,0.0};
bool prev_move_direction[NUM_AXIS]= {1,1,1,1};
float osc_wait_remainder = 0.0;
#endif

#if (MINIMUM_FAN_START_SPEED > 0)
unsigned char fan_last_speed = 0;
unsigned char fan_org_start_speed = 0;
unsigned long previous_millis_fan_start = 0;
#endif

// comm variables and Commandbuffer
// BUFSIZE is reduced from 8 to 6 to free more RAM for the PLANNER
#define MAX_CMD_SIZE 96
#define BUFSIZE 6 //8
char cmdbuffer[BUFSIZE][MAX_CMD_SIZE] = {};
char cmdbuffer_dummy[BUFSIZE][MAX_CMD_SIZE] = {};
bool fromsd[BUFSIZE];

//Need 1kb Ram --> only work with Atmega1284
#ifdef SD_FAST_XFER_AKTIV
char fastxferbuffer[SD_FAST_XFER_CHUNK_SIZE + 1];
int lastxferchar;
long xferbytes;
#endif

unsigned char bufindr = 0;
unsigned char bufindw = 0;
unsigned char buflen = 0;
char serial_char;
int serial_count = 0;
boolean comment_mode = false;
static char *strchr_pointer; // just a pointer to find chars in the cmd string like X, Y, Z, E, etc

//Send Temperature in ｰC to Host
int hotendtC = 0, bedtempC = 0;

//Inactivity shutdown variables
static unsigned long previous_millis_cmd = 0;
unsigned long max_inactive_time = 0;
unsigned long stepper_inactive_time = 0;

//Temp Monitor for repetier
unsigned char manage_monitor = 255;
//heater pwm value
extern volatile unsigned char g_heater_pwm_val;

static block_t block_buffer[BLOCK_BUFFER_SIZE]; // A ring buffer for motion instructions
static volatile unsigned char block_buffer_head; // Index of the next block to be pushed
static volatile unsigned char block_buffer_tail; // Index of the block to process now
// The current position of the tool in absolute steps
static long position[4] = {0,0,0,0};
static float previous_speed[4]; // Speed of previous path line segment
static float previous_nominal_speed; // Nominal speed of previous path line segment
static unsigned char G92_reset_previous_speed = 0;

static float junction_deviation = 0.1;
static float max_E_feedrate_calc = MAX_RETRACT_FEEDRATE;
static bool retract_feedrate_aktiv = false;

  static block_t *current_block;  // A pointer to the block currently being traced

  // Variables used by The Stepper Driver Interrupt
  static unsigned char out_bits;        // The next stepping-bits to be output
  static long counter_x,       // Counter variables for the bresenham line tracer
  counter_y, counter_z, counter_e;
  static unsigned long step_events_completed; // The number of step events executed in the current block
#ifdef ADVANCE
  static long advance_rate, advance, final_advance = 0;
  static short old_advance = 0;
#endif
  static short e_steps;
  static unsigned char busy = false; // TRUE when SIG_OUTPUT_COMPARE1A is being serviced. Used to avoid retriggering that handler.
  static long acceleration_time, deceleration_time;
  static unsigned short acc_step_rate; // needed for deceleration start point
  static char step_loops;
  static unsigned short OCR1A_nominal;


  static bool old_x_min_endstop = false;
  static bool old_x_max_endstop = false;
  static bool old_y_min_endstop = false;
  static bool old_y_max_endstop = false;
  static bool old_z_min_endstop = false;
  static bool old_z_max_endstop = false;

  unsigned long codenum_heater; 
  bool waiting_until_setpoint = false;
  bool target_direction_heating;
  static short last_e_steps;

  // DATA_ADDR definitions for BRAM
  #define BUFLEN_DATA_ADDR 100
  #define X_DATA_ADDR 101
  #define Y_DATA_ADDR 102
  #define Z_DATA_ADDR 103
  #define E_DATA_ADDR 104
  #define HOTEND_TEMP_ADDR 105
  #define ENDSTOP_X_ADDR 106
  #define ENDSTOP_Y_ADDR 107
  #define ENDSTOP_Z_ADDR 108
  #define BUFLEN_ACCUM_DATA_ADDR 109
  #define BUFLEN_ACCUM_FINISHED_DATA_ADDR 110
  #define NEXT_BUFFER_HEAD_ADDR 111
  #define HOTEND_TEMP_RAW_ADDR 112


  int e_pulse_counter;

//------------------------------------------------
//Init the SD card 
//------------------------------------------------
#ifdef SDSUPPORT
Sd2Card card;
SdVolume volume;
SdFile root;
SdFile file;
uint32_t filesize = 0;
uint32_t sdpos = 0;
bool sdmode = false;
bool sdactive = false;
bool savetosd = false;
int16_t read_char_int;

void initsd()
{
  sdactive = false;
#if SDSS >- 1
  if(root.isOpen())
    root.close();

  if (!card.init(SPI_FULL_SPEED,SDSS)) {
    //if (!card.init(SPI_HALF_SPEED,SDSS))
    //          //showString(PSTR("SD init fail\r\n"));
    // printf("SD init fail\r\n");
  }
  else if (!volume.init(&card))
    //        //showString(PSTR("volume.init failed\r\n"));
    // printf("volume.init failed\r\n");
  else if (!root.openRoot(&volume))
    //      //showString(PSTR("openRoot failed\r\n"));
    // printf("openRoot failed\r\n");
  else {
    sdactive = true;
    print_disk_info();

#ifdef SDINITFILE
    file.close();
    if(file.open(&root, "init.g", O_READ)) {
      sdpos = 0;
      filesize = file.fileSize();
      sdmode = true;
    }
#endif
  }

#endif
}

#ifdef SD_FAST_XFER_AKTIV

#ifdef PIDTEMP
extern volatile unsigned char g_heater_pwm_val;
#endif

void fast_xfer()
{
  char *pstr;
  boolean done = false;

  //force heater pins low
  if(HEATER_0_PIN > -1) WRITE(HEATER_0_PIN,LOW);
  //if(HEATER_1_PIN > -1) WRITE(HEATER_1_PIN,LOW);

#ifdef PIDTEMP
  g_heater_pwm_val = 0;
#endif

  lastxferchar = 1;
  xferbytes = 0;

  pstr = strstr(strchr_pointer+4, " ");

  if(pstr == NULL)
  {
    //      //showString(PSTR("invalid command\r\n"));
    showString("invalid command\r\n");
    return;
  }

  *pstr = '\0';

  //check mode (currently only RAW is supported
  if(strcmp(strchr_pointer+4, "RAW") != 0)
  {
    //      //showString(PSTR("Invalid transfer codec\r\n"));
    // printf("Invalid transfer codec\r\n");
    return;
  } else {
    //      //showString(PSTR("Selected codec: "));
    // printf("Selected codec: ");
    // printf(strchr_pointer+4);
    //Serial.println(strchr_pointer+4);
  }

  if (!file.open(&root, pstr+1, O_CREAT | O_APPEND | O_WRITE | O_TRUNC))
  {
    //      //showString(PSTR("open failed, File: "));
    //      Serial.print(pstr+1);
    //      //showString(PSTR("."));
    // printf("open failed, File: ");
    // printf("%d",pstr+1);
    // printf(".");
  } else {
    // printf("Writing to file: ");
    // printf(pstr+1);
    //      //showString(PSTR("Writing to file: "));
    //      //Serial.println(pstr+1);
  }

  //    //showString(PSTR("ok\r\n"));
  // printf("ok\r\n");

  //RAW transfer codec
  //Host sends \0 then up to SD_FAST_XFER_CHUNK_SIZE then \0
  //when host is done, it sends \0\0.
  //if a non \0 character is recieved at the beginning, host has failed somehow, kill the transfer.

  //read SD_FAST_XFER_CHUNK_SIZE bytes (or until \0 is recieved)
  while(!done)
  {
    while(!Serial.available())
    {
    }
    if(Serial.read() != 0)
    {
      //host has failed, this isn't a RAW chunk, it's an actual command
      file.sync();
      file.close();
      return;
    }

    for(int i=0;i<SD_FAST_XFER_CHUNK_SIZE+1;i++)
    {
      while(!Serial.available())
      {
      }
      lastxferchar = Serial.read();
      //buffer the data...
      fastxferbuffer[i] = lastxferchar;

      xferbytes++;

      if(lastxferchar == 0)
        break;
    }

    if(fastxferbuffer[0] != 0)
    {
      fastxferbuffer[SD_FAST_XFER_CHUNK_SIZE] = 0;
      file.write(fastxferbuffer);
      // printf("ok\r\n");
      //        //showString(PSTR("ok\r\n"));
    } else {
      // printf("Wrote ");
      // printf(xferbytes);
      // printf(" bytes.\r\n");
      //        //showString(PSTR("Wrote "));
      //        Serial.print(xferbytes);
      //        //showString(PSTR(" bytes.\r\n"));
      done = true;
    }
  }

  file.sync();
  file.close();
}
#endif

void print_disk_info(void)
{

  // print the type of card
  //    //showString(PSTR("\nCard type: "));
  // printf("\nCard type: ");
  switch(card.type())
  {
  case SD_CARD_TYPE_SD1:
    // printf("SD1\r\n");
    //        //showString(PSTR("SD1\r\n"));
    break;
  case SD_CARD_TYPE_SD2:
    // printf("SD2\r\n");
    //        //showString(PSTR("SD2\r\n"));
    break;
  case SD_CARD_TYPE_SDHC:
    // printf("SDHC\r\n");
    //        //showString(PSTR("SDHC\r\n"));
    break;
  default:
    // printf("Unknown\r\n");
    //        //showString(PSTR("Unknown\r\n"));
  }

  //uint64_t freeSpace = volume.clusterCount()*volume.blocksPerCluster()*512;
  //uint64_t occupiedSpace = (card.cardSize()*512) - freeSpace;
  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  //    //showString(PSTR("\nVolume type is FAT"));
  //    //Serial.println(volume.fatType(), DEC);

  volumesize = volume.blocksPerCluster();// clusters are collections of blocks
  volumesize *= volume.clusterCount();// we'll have a lot of clusters
  volumesize *= 512;// SD card blocks are always 512 bytes
  volumesize /= 1024;//kbytes
  volumesize /= 1024;//Mbytes
  //    //showString(PSTR("Volume size (Mbytes): "));
  //    //Serial.println(volumesize);

  // list all files in the card with date and size
  //root.ls(LS_R | LS_DATE | LS_SIZE);
}

FORCE_INLINE void write_command(char *buf)
{
  char* begin = buf;
  char* npos = 0;
  char* end = buf + strlen(buf) - 1;

  file.writeError = false;

  if((npos = strchr(buf, 'N')) != NULL)
  {
    begin = strchr(npos, ' ') + 1;
    end = strchr(npos, '*') - 1;
  }

  end[1] = '\r';
  end[2] = '\n';
  end[3] = '\0';

  ////Serial.println(begin);
  // print("%d\r\n",begin);
  file.write(begin);

  if (file.writeError)
  {
    //          //showString(PSTR("error writing to file\r\n"));
    // printf("error writing to file\r\n");
  }
}

#endif

//int FreeRam1(void) {
//  extern int __bss_end;
//  extern int* __brkval;
//  int free_memory;
//
//  if (reinterpret_cast<int>(__brkval) == 0) {
//    // if no heap use from end of bss section
//    free_memory = reinterpret_cast<int>(&free_memory)
//        - reinterpret_cast<int>(&__bss_end);
//  } else {
//    // use from top of stack to heap
//    free_memory = reinterpret_cast<int>(&free_memory)
//        - reinterpret_cast<int>(__brkval);
//  }
//
//  return free_memory;
//}

//------------------------------------------------
//Function the check the Analog OUT pin for not using the Timer1
//------------------------------------------------
void analogWrite_check(uint8_t check_pin, int val) {
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)
  //Atmega168/328 can't use OCR1A and OCR1B
  //These are pins PB1/PB2 or on Arduino D9/D10
  if ((check_pin != 9) && (check_pin != 10)) {
    //analogWrite(check_pin, val);
  }
#endif

#if defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__)
  //Atmega664P/1284P can't use OCR1A and OCR1B
  //These are pins PD4/PD5 or on Arduino D12/D13
  if((check_pin != 12) && (check_pin != 13))
  {
    analogWrite(check_pin, val);
  }
#endif

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  //Atmega1280/2560 can't use OCR1A, OCR1B and OCR1C
  //These are pins PB5,PB6,PB7 or on Arduino D11,D12 and D13
  if((check_pin != 11) && (check_pin != 12) && (check_pin != 13))
  {
    analogWrite(check_pin, val);
  }
#endif
}

//------------------------------------------------
//Print a String from Flash to Serial (save RAM)
//------------------------------------------------
void showString (char *s)
{
  //  char c;
  //
  //  while ((c = pgm_read_byte(s++)) != 0)
  // printf("%s",s);
}

// void initializeOLED(){
 // SeeedOled.init();  //initialze SEEED OLED display
 // SeeedOled.clearDisplay();          //clear the screen and set start position to top left corn    er
 // SeeedOled.setNormalDisplay();      //Set display to normal mode (i.e non-inverse mode)
 // SeeedOled.setPageMode();           //Set addressing mode to Page Mode
 // SeeedOled.setTextXY(0,0);          //Set the cursor to Xth Page, Yth Column
 // SeeedOled.putString("ZSprinter2"); //Print the String
//}

//------------------------------------------------
// Init 
//------------------------------------------------

void enable_x(){
  #if X_EN_PIN > -1
    _CLR(ck_shields_data, X_EN_PIN);
    XGpio_DiscreteWrite(&CK_ShieldInst, 1, ck_shields_data);
  #endif
}

void disable_x(){
  #if X_EN_PIN > -1
    _SET(ck_shields_data, X_EN_PIN);;
    XGpio_DiscreteWrite(&CK_ShieldInst, 1, ck_shields_data);
  #endif
}
void enable_y(){
  #if X_EN_PIN > -1
    _CLR(ck_shields_data, Y_EN_PIN);
    XGpio_DiscreteWrite(&CK_ShieldInst, 1, ck_shields_data);
  #endif
}

void disable_y(){
  #if X_EN_PIN > -1
    _SET(ck_shields_data, Y_EN_PIN);
    XGpio_DiscreteWrite(&CK_ShieldInst, 1, ck_shields_data);
  #endif
}

void enable_z(){
  #if X_EN_PIN > -1
    _CLR(ck_shields_data, Z_EN_PIN);
    XGpio_DiscreteWrite(&CK_ShieldInst, 1, ck_shields_data);
  #endif
}

void disable_z(){
  #if X_EN_PIN > -1
    _SET(ck_shields_data, Z_EN_PIN);
    XGpio_DiscreteWrite(&CK_ShieldInst, 1, ck_shields_data);
  #endif
}

void enable_e(){
  #if X_EN_PIN > -1
    _CLR(ck_shields_data, E_EN_PIN);
    XGpio_DiscreteWrite(&CK_ShieldInst, 1, ck_shields_data);
  #endif
}

void disable_e(){
  #if X_EN_PIN > -1
    _SET(ck_shields_data, E_EN_PIN);
    XGpio_DiscreteWrite(&CK_ShieldInst, 1, ck_shields_data);
  #endif
}

void uart_print(char * msg){
  unsigned int length = strlen(msg);
   if(length<16){
    uart_write(uart_dev0,msg,length);
    usleep(1000);
   }  else if(length<32){
    uart_write(uart_dev0,msg,16);
    usleep(1000);
    uart_write(uart_dev0,msg+16,length-16);
    usleep(1000);
   } else if(length<48){
    uart_write(uart_dev0,msg,16);
    usleep(1000);
    uart_write(uart_dev0,msg+16,16);
    usleep(1000);
    uart_write(uart_dev0,msg+32,length-32);
    usleep(1000);
   } else {
    uart_write(uart_dev0,msg,16);
    usleep(1000);
    uart_write(uart_dev0,msg+16,16);
    usleep(1000);
    uart_write(uart_dev0,msg+32,16);
    usleep(1000);
    uart_write(uart_dev0,msg+48,length-48);
    usleep(1000);
   }
}

void rs232c_read(u8* read_data){
//    XUartLite_Recv(&xuart[dev_id], read_data, length);
    unsigned int ReceivedCount = 0;
    while (1) {
            ReceivedCount += XUartLite_Recv(&UartLite1,
                           read_data + ReceivedCount,
                           1);
            if (read_data[ReceivedCount - 1] == ETX ) {
                break;
            }
        }
}

void rs232c_write(char command_byte){
    dispenser_command_bytes[0] = command_byte; 
    XUartLite_Send(&UartLite1, (u8 *) dispenser_command_bytes, 1);
    usleep(1000);
}

void rs232c_print(char *msg){
   unsigned int length = strlen(msg);
   if(length<16){
    XUartLite_Send(&UartLite1, (u8 *) msg, length);
    usleep(1000);
   }  else if (length<32) {
    XUartLite_Send(&UartLite1, (u8 *) msg, 16);
    usleep(1000);
    char shifted_msg[16];
    for(int i=0;i<length-16;i++){
      shifted_msg[i] = msg[16+i];
    }
    XUartLite_Send(&UartLite1,(u8 *) shifted_msg, length-16);
    usleep(1000);
   } 
}

void setup() {
  MAILBOX_CMD_ADDR = 0x0;
  MAILBOX_DATA(BUFLEN_DATA_ADDR) = 0;
  MAILBOX_DATA(BUFLEN_ACCUM_DATA_ADDR) = 0;
  MAILBOX_DATA(BUFLEN_ACCUM_FINISHED_DATA_ADDR) = 0;
  MAILBOX_DATA(NEXT_BUFFER_HEAD_ADDR) = 0;
  MAILBOX_DATA_FLOAT(X_DATA_ADDR) = 0;
  MAILBOX_DATA_FLOAT(Y_DATA_ADDR) = 0;
  MAILBOX_DATA_FLOAT(Z_DATA_ADDR) = 0;
  MAILBOX_DATA_FLOAT(E_DATA_ADDR) = 0;
  MAILBOX_DATA_FLOAT(HOTEND_TEMP_ADDR) = 0;

  initializeUART0(); // for communicating with the host (serial console)
  initializeUART1(); // for setting dispenser (RS-232C)

  initializeGPIO();
  initializeGPIO_ChipKit();

  initializeAxiTimer();

  for (int i = 0; i < BUFSIZE; i++) {
    fromsd[i] = false;
  }

  initializeXADC();

  //Initialize Dir Pins
#if X_DIR_PIN > -1
  //  SET_OUTPUT(X_DIR_PIN);
#endif
#if Y_DIR_PIN > -1
  //  SET_OUTPUT(Y_DIR_PIN);
#endif
#if Z_DIR_PIN > -1
  //  SET_OUTPUT(Z_DIR_PIN);
#endif
#if E_DIR_PIN > -1
  //  SET_OUTPUT(E_DIR_PIN);
#endif

  //Initialize Enable Pins - steppers default to disabled.

#if (X_ENABLE_PIN > -1)
  //  SET_OUTPUT(X_ENABLE_PIN);
  //  if(!X_ENABLE_ON)
  //  WRITE(X_ENABLE_PIN,HIGH);
#endif
#if (Y_ENABLE_PIN > -1)
  //  SET_OUTPUT(Y_ENABLE_PIN);
  //  if(!Y_ENABLE_ON)
  //  WRITE(Y_ENABLE_PIN,HIGH);
#endif
#if (Z_ENABLE_PIN > -1)
  //  SET_OUTPUT(Z_ENABLE_PIN);
  //  if (!Z_ENABLE_ON)
  //    WRITE(Z_ENABLE_PIN, HIGH);
#endif
#if (E_ENABLE_PIN > -1)
  //  SET_OUTPUT(E_ENABLE_PIN);
  //  if(!E_ENABLE_ON)
  //    WRITE(E_ENABLE_PIN,HIGH);
#endif

#ifdef CONTROLLERFAN_PIN
  //  SET_OUTPUT(CONTROLLERFAN_PIN); //Set pin used for driver cooling fan
#endif

#ifdef EXTRUDERFAN_PIN
  //  SET_OUTPUT(EXTRUDERFAN_PIN); //Set pin used for extruder cooling fan
#endif

  //endstops and pullups
#ifdef ENDSTOPPULLUPS
#if X_MIN_PIN > -1
  //  SET_INPUT(X_MIN_PIN);
  //  WRITE(X_MIN_PIN, HIGH);
#endif
#if X_MAX_PIN > -1
  //  SET_INPUT(X_MAX_PIN);
  //  WRITE(X_MAX_PIN,HIGH);
#endif
#if Y_MIN_PIN > -1
  //  SET_INPUT(Y_MIN_PIN);
  //  WRITE(Y_MIN_PIN, HIGH);
#endif
#if Y_MAX_PIN > -1
  //  SET_INPUT(Y_MAX_PIN);
  //  WRITE(Y_MAX_PIN,HIGH);
#endif
#if Z_MIN_PIN > -1
  //  SET_INPUT(Z_MIN_PIN);
  //  WRITE(Z_MIN_PIN, HIGH);
#endif
#if Z_MAX_PIN > -1
  //  SET_INPUT(Z_MAX_PIN);
  //  WRITE(Z_MAX_PIN,HIGH);
#endif
#else
#if X_MIN_PIN > -1
  //  SET_INPUT(X_MIN_PIN);
#endif
#if X_MAX_PIN > -1
  //  SET_INPUT(X_MAX_PIN);
#endif
#if Y_MIN_PIN > -1
  //  SET_INPUT(Y_MIN_PIN);
#endif
#if Y_MAX_PIN > -1
  //  SET_INPUT(Y_MAX_PIN);
#endif
#if Z_MIN_PIN > -1
  //  SET_INPUT(Z_MIN_PIN);
#endif
#if Z_MAX_PIN > -1
  //  SET_INPUT(Z_MAX_PIN);
#endif
#endif

#if (HEATER_0_PIN > -1)
  //  SET_OUTPUT(HEATER_0_PIN);
  //  WRITE(HEATER_0_PIN, LOW);
#endif
#if (HEATER_1_PIN > -1)
  //    SET_OUTPUT(HEATER_1_PIN);
  //  WRITE(HEATER_1_PIN,LOW);
#endif

  //Initialize Fan Pin
#if (FAN_PIN > -1)
  //  SET_OUTPUT(FAN_PIN);
#endif

  //Initialize Alarm Pin
#if (ALARM_PIN > -1)
  //  SET_OUTPUT(ALARM_PIN);
  WRITE(ALARM_PIN,LOW);
#endif

  //Initialize LED Pin
#if (LED_PIN > -1)
  //  SET_OUTPUT(LED_PIN);
  //  WRITE(LED_PIN,LOW);
#endif

  //Initialize Step Pins
#if (X_STEP_PIN > -1)
  //  SET_OUTPUT(X_STEP_PIN);
#endif
#if (Y_STEP_PIN > -1)
  //  SET_OUTPUT(Y_STEP_PIN);
#endif
#if (Z_STEP_PIN > -1)
  //  SET_OUTPUT(Z_STEP_PIN);
#endif
#if (E_STEP_PIN > -1)
  //  SET_OUTPUT(E_STEP_PIN);
#endif

  //***BELOW COMMENTED OUT ALSO IN MASTER BRANCH***
  //    for(int i=0; i < NUM_AXIS; i++){
  //        axis_max_interval[i] = 100000000.0 / (max_start_speed_units_per_second[i] * axis_steps_per_unit[i]);
  //        axis_steps_per_sqr_second[i] = max_acceleration_units_per_sq_second[i] * axis_steps_per_unit[i];
  //        axis_travel_steps_per_sqr_second[i] = max_travel_acceleration_units_per_sq_second[i] * axis_steps_per_unit[i];
  //    }

#ifdef HEATER_USES_MAX6675
  //  SET_OUTPUT(SCK_PIN);
  //  WRITE(SCK_PIN,0);
  //
  //  SET_OUTPUT(MOSI_PIN);
  //  WRITE(MOSI_PIN,1);
  //
  //  SET_INPUT(MISO_PIN);
  //  WRITE(MISO_PIN,1);
  //
  //  SET_OUTPUT(MAX6675_SS);
  //  WRITE(MAX6675_SS,1);
#endif  

#ifdef SDSUPPORT

  //power to SD reader
#if SDPOWER > -1
  //  SET_OUTPUT(SDPOWER);
  //  WRITE(SDPOWER,HIGH);
#endif

  //  //showString(PSTR("SD Start\r\n"));
  //  initsd();

#endif

//#if defined(PID_SOFT_PWM) || (defined(FAN_SOFT_PWM) && (FAN_PIN > -1))
 // printf("Soft PWM Init\r\n");
//  init_Timer2_softpwm();
//#endif

  // printf("Planner Init\r\n");
  plan_init();  // Initialize planner; 
  // printf("Stepper Timer init\r\n");
  st_init();    // Initialize stepper

  // #ifdef USE_EEPROM_SETTINGS
  //  //first Value --> Init with default
  //  //second value --> Print settings to UART
  //  EEPROM_RetrieveSettings(false,false);
  // #endif

#ifdef PIDTEMP
  updatePID();
#endif

  //Free Ram
  // showString("Free Ram: ");
  //Serial.println(FreeRam1());

  //Planner Buffer Size
  // printf("Plan Buffer Size: ");
  //printf("%d",(int)sizeof(block_t)*BLOCK_BUFFER_SIZE);
  //printf("/");
  //printf("%d\r\n",BLOCK_BUFFER_SIZE);

  for (int8_t i = 0; i < NUM_AXIS; i++) {
      axis_steps_per_sqr_second[i] = max_acceleration_units_per_sq_second[i]
                       * axis_steps_per_unit[i];
  }


  
}

//PYNQ-MicroBlaze UART, 115200 bps
void initializeUART0(){
  uart_dev0 = uart_open(1,0);
  char dummy_msg[] = "*";
  uart_print(dummy_msg);
  char msg[] = "Zsprinter started !!!\r\n";
  uart_print(msg);
}

//Additional UART Lite, 38400 bps
void initializeUART1(){
   int Status;
   Status = XUartLite_Initialize(&UartLite1, UARTLITE_DEVICE2_ID);
   if (Status != XST_SUCCESS) {
      //return XST_FAILURE;
      char msg[] = "UART1 error\r\n";
      uart_print(msg);
   }
}

char current_command[64] = {};
int current_command_length = 0;
u8 returned_message[32] = {};

void get_command_mailbox(){
   int x_position, y_position, length;
   //char ch[64];
   int i;

   length=MAILBOX_DATA(0);
   x_position=MAILBOX_DATA(1);
   y_position=MAILBOX_DATA(2);
   current_command_length = length;
   for(i=0; i<length; i++){
       //ch[i] = MAILBOX_DATA(3+i);
       current_command[i] = MAILBOX_DATA(3+i);
   }
   //ch[i]='\0'; // make sure it is null terminated string
   current_command[i+1]='\0'; // make sure it is null terminated string
}

//------------------------------------------------
//MAIN LOOP
//------------------------------------------------

// void mydelay(int factor){
//   for(int j=0;j<factor;j++){
//     for(int i=0;i<1000;i++){
//       asm volatile("nop");
//     }
//   }
// }

char recv_buffer[64];

void loop() {
  //uart_readline(uart_dev0, recv_buffer);
  memset( current_command, '\0' , strlen(current_command));
  uart_readline(uart_dev0, current_command);
  current_command_length = strlen(current_command);
  for(int i=0;i<current_command_length;i++){
     parse_command(current_command[i],i);
  }
  //char msg[] = "\r\nYou received: ";
  //uart_print(msg);
  //uart_print(recv_buffer);

/*
  while( MAILBOX_CMD_ADDR==0 || clear_to_send==false); // Wait until any of the conditions satisfied

  if(MAILBOX_CMD_ADDR!=0){
      u32 cmd = MAILBOX_CMD_ADDR;
      if (cmd==PRINT_STRING) {
      	clear_to_send = false;
        get_command_mailbox();

        // SeeedOled.setTextXY(1,0);          //Set the cursor to Xth Page, Yth Column
        // SeeedOled.putString(current_command); //Print the String
        comment_mode = false; //this is required!!

        for(int i=0;i<current_command_length;i++){
          parse_command(current_command[i],i);
        }

        // SeeedOled.setTextXY(2,0);          //Set the cursor to Xth Page, Yth Column
        // SeeedOled.putString(cmdbuffer[bufindr]); //Print the String
      }
      // MAILBOX_CMD_ADDR = 0x0;
   }
*/
}

//------------------------------------------------
//Check Uart buffer while arc function ist calc a circle
//------------------------------------------------

void check_buffer_while_arc() {
  if (buflen < (BUFSIZE - 1)) {
    //get_command();
    get_command_mailbox();
  }
}

//------------------------------------------------
//READ COMMAND FROM UART
//------------------------------------------------
/*
void get_command() {
  serial_char = getchar();
  printf("%c",serial_char);

  if (serial_char == '\n' || serial_char == '\r'
      || (serial_char == ':' && comment_mode == false)
      || serial_count >= (MAX_CMD_SIZE - 1)) {
    if (!serial_count) { //if empty line
      comment_mode = false; // for new command
      return;
    }
    cmdbuffer[bufindw][serial_count] = 0; //terminate string

    fromsd[bufindw] = false;
    if (strstr(cmdbuffer[bufindw], "N") != NULL) {
      strchr_pointer = strchr(cmdbuffer[bufindw], 'N');
      gcode_N = (strtol(
          &cmdbuffer[bufindw][strchr_pointer - cmdbuffer[bufindw]
                                   + 1], NULL, 10));
      if (gcode_N != gcode_LastN + 1
          && (strstr(cmdbuffer[bufindw], "M110") == NULL)) {
        //showString("Serial Error: Line Number is not Last Line Number+1, Last Line:");
        printf("Serial Error: Line Number is not Last Line Number+1, Last Line:");
        printf("%d\r\n",gcode_LastN);
        printf("%d\r\n",gcode_N);
        //Serial.println(gcode_LastN);
        ////Serial.println(gcode_N);
        FlushSerialRequestResend();
        serial_count = 0;
        return;
      }

      if (strstr(cmdbuffer[bufindw], "*") != NULL) {
        byte checksum = 0;
        byte count = 0;
        while (cmdbuffer[bufindw][count] != '*')
          checksum = checksum ^ cmdbuffer[bufindw][count++];
        strchr_pointer = strchr(cmdbuffer[bufindw], '*');

        if ((int) (strtod(
            &cmdbuffer[bufindw][strchr_pointer
                      - cmdbuffer[bufindw] + 1], NULL))
                      != checksum) {
          //showString("Error: checksum mismatch, Last Line:");
          printf("Error: checksum mismatch, Last Line:");
          //Serial.println(gcode_LastN);
          printf("%d\r\n",gcode_LastN);
          FlushSerialRequestResend();
          serial_count = 0;
          return;
        }
        //if no errors, continue parsing
      } else {
        //showString("Error: No Checksum with line number, Last Line:");
        printf("Error: No Checksum with line number, Last Line:");
        // Serial.println(gcode_LastN);
        printf("%d\r\n",gcode_LastN);
        FlushSerialRequestResend();
        serial_count = 0;
        return;
      }

      gcode_LastN = gcode_N;
      //if no errors, continue parsing
    } else  // if we don't receive 'N' but still see '*'
    {
      if ((strstr(cmdbuffer[bufindw], "*") != NULL)) {
        //showString("Error: No Line Number with checksum, Last Line:");
        printf("Error: No Line Number with checksum, Last Line:");
        //Serial.println(gcode_LastN);
        printf("%d\r\n",gcode_LastN);
        serial_count = 0;
        return;
      }
    }

    if ((strstr(cmdbuffer[bufindw], "G") != NULL)) {
      strchr_pointer = strchr(cmdbuffer[bufindw], 'G');
      switch ((int) ((strtod(
          &cmdbuffer[bufindw][strchr_pointer - cmdbuffer[bufindw]
                                   + 1], NULL)))) {
      case 0:
      case 1:
        // showString("ok\r\n");
        ////Serial.println("ok");
        printf("ok\r\n");
        break;


      default:
        break;
      }
    }
    //Removed modulo (%) operator, which uses an expensive divide and multiplication
    //bufindw = (bufindw + 1)%BUFSIZE;
    bufindw++;
    if (bufindw == BUFSIZE)
      bufindw = 0;
    buflen += 1;

    comment_mode = false; //for new command
    serial_count = 0; //clear buffer
  } else {
    if (serial_char == ';')
      comment_mode = true;
    if (!comment_mode)
      cmdbuffer[bufindw][serial_count++] = serial_char;
  }
}
*/


void parse_command(char serial_char, int idx) {

  if (serial_char == '\n' || serial_char == '\r'
      || (serial_char == ':' && comment_mode == false)
      || serial_count >= (MAX_CMD_SIZE - 1)) {

    if (!serial_count) { //if empty line
      comment_mode = false; // for new command
      return;
    }
    cmdbuffer[bufindw][serial_count] = 0; //terminate string

    fromsd[bufindw] = false;
    if (strstr(cmdbuffer[bufindw], "N") != NULL) {
      strchr_pointer = strchr(cmdbuffer[bufindw], 'N');
      gcode_N = (strtol(
          &cmdbuffer[bufindw][strchr_pointer - cmdbuffer[bufindw]
                                   + 1], NULL, 10));
      if (gcode_N != gcode_LastN + 1
          && (strstr(cmdbuffer[bufindw], "M110") == NULL)) {
        //showString("Serial Error: Line Number is not Last Line Number+1, Last Line:");
        // printf("Serial Error: Line Number is not Last Line Number+1, Last Line:");
        // printf("%d\r\n",gcode_LastN);
        // printf("%d\r\n",gcode_N);
        //Serial.println(gcode_LastN);
        ////Serial.println(gcode_N);
        FlushSerialRequestResend();
        serial_count = 0;
        return;
      }

      if (strstr(cmdbuffer[bufindw], "*") != NULL) {
        byte checksum = 0;
        byte count = 0;
        while (cmdbuffer[bufindw][count] != '*')
          checksum = checksum ^ cmdbuffer[bufindw][count++];
        strchr_pointer = strchr(cmdbuffer[bufindw], '*');

        if ((int) (strtod(
            &cmdbuffer[bufindw][strchr_pointer
                      - cmdbuffer[bufindw] + 1], NULL))
                      != checksum) {
          //showString("Error: checksum mismatch, Last Line:");
          // printf("Error: checksum mismatch, Last Line:");
          //Serial.println(gcode_LastN);
          // printf("%d\r\n",gcode_LastN);
          FlushSerialRequestResend();
          serial_count = 0;
          return;
        }
        //if no errors, continue parsing
      } else {
        //showString("Error: No Checksum with line number, Last Line:");
        // printf("Error: No Checksum with line number, Last Line:");
        // Serial.println(gcode_LastN);
        // printf("%d\r\n",gcode_LastN);
        FlushSerialRequestResend();
        serial_count = 0;
        return;
      }

      gcode_LastN = gcode_N;
      //if no errors, continue parsing
    } else  // if we don't receive 'N' but still see '*'
    {
      if ((strstr(cmdbuffer[bufindw], "*") != NULL)) {
        //showString("Error: No Line Number with checksum, Last Line:");
        // printf("Error: No Line Number with checksum, Last Line:");
        //Serial.println(gcode_LastN);
        // printf("%d\r\n",gcode_LastN);
        serial_count = 0;
        return;
      }
    }

    if ((strstr(cmdbuffer[bufindw], "G") != NULL)) {
      strchr_pointer = strchr(cmdbuffer[bufindw], 'G');
      switch ((int) ((strtod(
          &cmdbuffer[bufindw][strchr_pointer - cmdbuffer[bufindw]
                                   + 1], NULL)))) {
      case 0:
      case 1:
      // clear_to_send = true;
      // MAILBOX_CMD_ADDR = 0x0;
        // showString("ok\r\n");
        ////Serial.println("ok");
        // printf("ok\r\n");
        //uart_write(uart_dev0,ok_msg,strlen(ok_msg));
        break;


      default:
        break;
      }
    }
    //Removed modulo (%) operator, which uses an expensive divide and multiplication
    //bufindw = (bufindw + 1)%BUFSIZE;

    bufindw++;
    if (bufindw == BUFSIZE)
      bufindw = 0;
    buflen += 1;
    MAILBOX_DATA(BUFLEN_DATA_ADDR) = buflen;
    int buflen_accum = MAILBOX_DATA(BUFLEN_ACCUM_DATA_ADDR);
    MAILBOX_DATA(BUFLEN_ACCUM_DATA_ADDR) = buflen_accum + 1;

    comment_mode = false; //for new command
    serial_count = 0; //clear buffer
  } else {
    if (serial_char == ';'){
      comment_mode = true;
    }
    if (!comment_mode){
      cmdbuffer[bufindw][serial_count++] = serial_char;
    } 
  }
}

static bool check_endstops = true;

void enable_endstops(bool check) {
  check_endstops = check;
}

FORCE_INLINE float code_value() {
  return (strtod(&cmdbuffer[bufindr][strchr_pointer - cmdbuffer[bufindr] + 1],
      NULL));
}
FORCE_INLINE long code_value_long() {
  return (strtol(&cmdbuffer[bufindr][strchr_pointer - cmdbuffer[bufindr] + 1],
      NULL, 10));
}
FORCE_INLINE bool code_seen(char code_string[]) {
  return (strstr(cmdbuffer[bufindr], code_string) != NULL);
}  //Return True if the string was found

FORCE_INLINE bool code_seen(char code) {
  strchr_pointer = strchr(cmdbuffer[bufindr], code);
  return (strchr_pointer != NULL);  //Return True if a character was found
}

FORCE_INLINE void homing_routine(char axis) {
  int min_pin, max_pin, home_dir, max_length, home_bounce;

  switch (axis) {
  case X_AXIS:
    min_pin = X_MIN_PIN;
    max_pin = X_MAX_PIN;
    home_dir = X_HOME_DIR;
    max_length = X_MAX_LENGTH;
    home_bounce = 10;
    break;
  case Y_AXIS:
    min_pin = Y_MIN_PIN;
    max_pin = Y_MAX_PIN;
    home_dir = Y_HOME_DIR;
    max_length = Y_MAX_LENGTH;
    home_bounce = 10;
    break;
  case Z_AXIS:
    min_pin = Z_MIN_PIN;
    max_pin = Z_MAX_PIN;
    home_dir = Z_HOME_DIR;
    max_length = Z_MAX_LENGTH;
    home_bounce = 4;
    break;
  default:
    //never reached
    break;
  }

  if ((min_pin > -1 && home_dir == -1) || (max_pin > -1 && home_dir == 1)) {
    current_position[axis] = -1.5 * max_length * home_dir;
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS],
        current_position[Z_AXIS], current_position[E_AXIS]);
    destination[axis] = 0;
    feedrate = homing_feedrate[axis];
    prepare_move();
    st_synchronize();

    current_position[axis] = home_bounce / 2 * home_dir;
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS],
        current_position[Z_AXIS], current_position[E_AXIS]);
    destination[axis] = 0;
    prepare_move();
    st_synchronize();

    current_position[axis] = -home_bounce * home_dir;
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS],
        current_position[Z_AXIS], current_position[E_AXIS]);
    destination[axis] = 0;
    feedrate = homing_feedrate[axis] / 2;
    prepare_move();
    st_synchronize();

    current_position[axis] = (home_dir == -1) ? 0 : max_length;
    current_position[axis] += add_homing[axis];
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS],
        current_position[Z_AXIS], current_position[E_AXIS]);
    destination[axis] = current_position[axis];
    feedrate = 0;
  }
}


inline void line_to_axis_pos(int axis, float where, float fr_mm_m = 0.0) {
  float old_feedrate_mm_m = feedrate_mm_m;
  current_position[axis] = where;
  feedrate_mm_m = (fr_mm_m != 0.0) ? fr_mm_m : homing_feedrate_mm_m[axis];
  plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], MMM_TO_MMS(feedrate_mm_m));
  // st_synchronize(); // comment out -> cannot wait inside the AXI timer
  feedrate_mm_m = old_feedrate_mm_m;
}

inline void sync_plan_position() {
  plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
}
//inline void sync_plan_position_e() { planner.set_e_position_mm(current_position[E_AXIS]); }

inline void line_to_current_position() {
  plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], MMM_TO_MMS(feedrate_mm_m) );
}
//
// line_to_destination
// Move the planner, not necessarily synced with current_position
//
inline void line_to_destination(float fr_mm_m) {
  plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], MMM_TO_MMS(fr_mm_m));
}
inline void line_to_destination() { line_to_destination(feedrate_mm_m); }

inline void sync_plan_position_delta() {
  inverse_kinematics(current_position);
  plan_set_position(delta[X_AXIS], delta[Y_AXIS], delta[Z_AXIS], current_position[E_AXIS]);
}

FORCE_INLINE void homeaxis(int axis){
  int min_pin, max_pin, home_dir, max_length, home_bump_mm;
  float axis_homing_feedrate_mm_m;

  switch (axis) {
  case X_AXIS:
    min_pin = X_MIN_PIN;
    max_pin = X_MAX_PIN;
    home_dir = X_HOME_DIR;
    max_length = Z_MAX_LENGTH;
    home_bump_mm = X_HOME_BUMP_MM;
    axis_homing_feedrate_mm_m = homing_feedrate_mm_m[Z_AXIS];
    break;
  case Y_AXIS:
    min_pin = Y_MIN_PIN;
    max_pin = Y_MAX_PIN;
    home_dir = Y_HOME_DIR;
    max_length = Z_MAX_LENGTH;
    home_bump_mm = Y_HOME_BUMP_MM;
    axis_homing_feedrate_mm_m = homing_feedrate_mm_m[Z_AXIS];
    break;
  case Z_AXIS:
    min_pin = Z_MIN_PIN;
    max_pin = Z_MAX_PIN;
    home_dir = Z_HOME_DIR;
    max_length = Z_MAX_LENGTH;
    home_bump_mm = Z_HOME_BUMP_MM;
    axis_homing_feedrate_mm_m = homing_feedrate_mm_m[Z_AXIS];
    break;
  default:
    //never reached
    break;
  }

  // Set the axis position as setup for the move
  current_position[axis] = 0;
  sync_plan_position();
  // Move towards the endstop until an endstop is triggered
  line_to_axis_pos(axis, 1.5 * max_length * home_dir);
  // Set the axis position as setup for the move
  current_position[axis] = 0;
  sync_plan_position();
  // Move away from the endstop by the axis HOME_BUMP_MM
  line_to_axis_pos(axis, -home_bump_mm * home_dir);
  // Move slowly towards the endstop until triggered
  line_to_axis_pos(axis, 2 * home_bump_mm * home_dir, axis_homing_feedrate_mm_m / 10);
  // reset current_position to 0 to reflect hitting endpoint
  current_position[axis] = 0;
  sync_plan_position();

  // not necessary
  //set_axis_is_at_home(axis);

  //sync_plan_position_delta();
  sync_plan_position();

  destination[axis] = current_position[axis];
  switch(axis){
  case X_AXIS:
    endstop_x_hit = false; // clear endstop hit flags
    break;
  case Y_AXIS:
    endstop_y_hit = false; // clear endstop hit flags
    break;
  case Z_AXIS:
    endstop_z_hit = false; // clear endstop hit flags
    break;
  default:
    break;
  }
  //axis_known_position[axis] = true;
  //axis_homed[axis] = true;
}

//no work, since we define pins in THIS file, not pins.h
void dumpAllPins(){
  // printf("MOTHERBOARD :%d\r\n"  , MOTHERBOARD );
  // printf("X_STEP_PIN  :%d\r\n"  , X_STEP_PIN  );
  // printf("X_DIR_PIN   :%d\r\n"  , X_DIR_PIN   );
  // printf("X_ENABLE_PIN:%d\r\n"  , X_ENABLE_PIN );
  // printf("X_MIN_PIN   :%d\r\n" ,  X_MIN_PIN  );
  // printf("X_MAX_PIN   :%d\r\n" ,  X_MAX_PIN  );
  // printf("Y_STEP_PIN  :%d\r\n" ,  Y_STEP_PIN );
  // printf("Y_DIR_PIN   :%d\r\n" ,  Y_DIR_PIN  );
  // printf("Y_ENABLE_PIN:%d\r\n"  , Y_ENABLE_PIN );
  // printf("Y_MIN_PIN   :%d\r\n"  , Y_MIN_PIN    );
  // printf("Y_MAX_PIN   :%d\r\n"  , Y_MAX_PIN    );
  // printf("Z_STEP_PIN  :%d\r\n"  , Z_STEP_PIN   );
  // printf("Z_DIR_PIN   :%d\r\n"  , Z_DIR_PIN    );
  // printf("Z_ENABLE_PIN:%d\r\n"  , Z_ENABLE_PIN );
  // printf("Z_MIN_PIN   :%d\r\n"  , Z_MIN_PIN    );
  // printf("Z_MAX_PIN   :%d\r\n"  , Z_MAX_PIN    );
  // printf("E_STEP_PIN  :%d\r\n"  , E_STEP_PIN   );
  // printf("E_DIR_PIN   :%d\r\n"  , E_DIR_PIN    );
  // printf("E_ENABLE_PIN:%d\r\n"  , E_ENABLE_PIN );
  // printf("SDPOWER     :%d\r\n"  , SDPOWER      );
  // printf("SDSS        :%d\r\n"  , SDSS         );
  // printf("LED_PIN     :%d\r\n"  , LED_PIN      );
  // printf("FAN_PIN     :%d\r\n"  , FAN_PIN      );
  // printf("PS_ON_PIN   :%d\r\n"  , PS_ON_PIN    );
  // printf("KILL_PIN    :%d\r\n"  , KILL_PIN     );
  // printf("ALARM_PIN   :%d\r\n"  , ALARM_PIN    );
  // printf("HEATER_0_PIN:%d\r\n"  , HEATER_0_PIN );
  // printf("TEMP_0_PIN  :%d\r\n"  , TEMP_0_PIN   );
}

/**
 * T0-T3: Switch tool, usually switching extruders
 *
 *   F[units/min] Set the movement feedrate
 *   S1           Don't move the tool in XY after change
 */
inline void gcode_T(uint8_t tmp_extruder) {
  last_tool_number = current_tool_number;
  current_tool_number = tmp_extruder;
  //sprintf(temp_msg, "current tool:%d\r\n",current_tool_number);
  //uart_print(temp_msg); 
}


//------------------------------------------------
// CHECK COMMAND AND CONVERT VALUES
//------------------------------------------------
// FORCE_INLINE void process_commands() {
void process_commands() {
  unsigned long codenum; //throw away variable
  char *starpos = NULL;

  //printf("recv: %s\r\n",cmdbuffer[bufindr]);

  if (code_seen('G')) {
    switch ((int) code_value()) {
    case 0: // G0 -> G1
    case 1: // G1
      //print("process commands fired.\r\n");
#if (defined DISABLE_CHECK_DURING_ACC) || (defined DISABLE_CHECK_DURING_MOVE) || (defined DISABLE_CHECK_DURING_TRAVEL)
      //manage_heater();
      manage_heater(SysMonInstPtr);
#endif
      get_coordinates(); // For X Y Z E F
      prepare_move();
      previous_millis_cmd = millis();
      ClearToSend();
      return;
      //break;
#ifdef USE_ARC_FUNCTION
    case 2: // G2  - CW ARC
      get_arc_coordinates();
      prepare_arc_move(true);
      previous_millis_cmd = millis();
      //break;
      return;
    case 3:// G3  - CCW ARC
      get_arc_coordinates();
      prepare_arc_move(false);
      previous_millis_cmd = millis();
      //break;
      return;
#endif
    case 4: // G4 dwell
      codenum = 0;
      if (code_seen('P'))
        codenum = code_value(); // milliseconds to wait
      if (code_seen('S'))
        codenum = code_value() * 1000; // seconds to wait
      codenum += millis();  // keep track of when we started waiting
      st_synchronize();  // wait for all movements to finish
      while (millis() < codenum) {
        //manage_heater();
        manage_heater(SysMonInstPtr);
      } 
      break;
    case 28: //G28 Home all Axis one at a time
      saved_feedrate = feedrate;
      saved_feedmultiply = feedmultiply;
      previous_millis_cmd = millis();

      feedmultiply = 100;

      enable_endstops(true);

      for (int i = 0; i < NUM_AXIS; i++) {
        destination[i] = current_position[i];
      }
      feedrate = 0;
      is_homing = true;

      #ifdef DELTA
        /**
         * A delta can only safely home all axes at the same time
         */
        // Pretend the current position is 0,0,0
        // This is like quick_home_xy() but for 3 towers.
        current_position[X_AXIS] = current_position[Y_AXIS] = current_position[Z_AXIS] = 0.0;
        sync_plan_position();
        // Move all carriages up together until the first endstop is hit.
        current_position[X_AXIS] = current_position[Y_AXIS] = current_position[Z_AXIS] = 3.0 * (Z_MAX_LENGTH);
        feedrate_mm_m = 1.732 * homing_feedrate_mm_m[X_AXIS];
        line_to_current_position();
      #else 
        home_all_axis = !((code_seen(axis_codes[X_AXIS])) || (code_seen(axis_codes[Y_AXIS])) || (code_seen(axis_codes[Z_AXIS])));
        if(home_all_axis){
          home_x_axis = home_y_axis = home_z_axis = true;
        } else {
          home_x_axis = code_seen(axis_codes[X_AXIS]);
          home_y_axis = code_seen(axis_codes[Y_AXIS]);
          home_z_axis = code_seen(axis_codes[Z_AXIS]);
        }
      #endif

      return;
//       st_synchronize();

//       //endstops.hit_on_purpose(); // clear endstop hit flags
//       endstop_x_hit = false;
//       endstop_y_hit = false;
//       endstop_z_hit = false;
//       current_position[X_AXIS] = current_position[Y_AXIS] = current_position[Z_AXIS] = 0.0;

//       homeaxis(X_AXIS);
//       homeaxis(Y_AXIS);
//       homeaxis(Z_AXIS);

//       // sync_plan_position_delta();
//       sync_plan_position();

//       // set the current position as top
//       // trigger sync_plan_position_delta() to exclude orthogonal coordinates
//       current_position[X_AXIS] = current_position[Y_AXIS] = 0.0;
//       current_position[Z_AXIS] = Z_MAX_POS;
//       destination[X_AXIS] = current_position[X_AXIS];
//       destination[Y_AXIS] = current_position[Y_AXIS];
//       destination[Z_AXIS] = current_position[Z_AXIS];
//       destination[E_AXIS] = current_position[E_AXIS];
//       inverse_kinematics(destination);
//       sync_plan_position_delta();

//       // step back to avoid the switch
//       destination[X_AXIS] = current_position[X_AXIS];
//       destination[Y_AXIS] = current_position[Y_AXIS];
//       destination[Z_AXIS] = current_position[Z_AXIS] - Z_HOME_BUMP_MM;
//       destination[E_AXIS] = current_position[E_AXIS];
//       inverse_kinematics(destination);
//       plan_buffer_line(delta[X_AXIS], delta[Y_AXIS],delta[Z_AXIS], destination[E_AXIS], feedrate_mm_m);
//       for (int i = 0; i < NUM_AXIS; i++) {
//         current_position[i] = destination[i];
//       }
//       sync_plan_position_delta();


// #ifdef ENDSTOPS_ONLY_FOR_HOMING
//       enable_endstops(false);
// #endif

//       is_homing = false;
//       feedrate = saved_feedrate;
//       feedmultiply = saved_feedmultiply;

//       previous_millis_cmd = millis();
//       break;
    case 90: // G90
      relative_mode = false;
      break;
    case 91: // G91
      relative_mode = true;
      break;
    case 92: // G92
//      if (!code_seen(axis_codes[E_AXIS])){
//        st_synchronize();
//      }
    
      //temporary hack for delta
      //#ifdef DELTA
      //G92 E0
      //  codenum = code_value();
      //  current_position[E_AXIS] = codenum;
      //  position[E_AXIS] = -1*last_e_steps;
      //#endif

      for (int i = 0; i < NUM_AXIS; i++) {
         if (code_seen(axis_codes[i]))
           current_position[i] = code_value();
       }

      plan_set_position(current_position[X_AXIS],
          current_position[Y_AXIS], current_position[Z_AXIS],
          current_position[E_AXIS]);

      if (!code_seen(axis_codes[E_AXIS])){
        current_position[E_AXIS] = code_value();
        position[E_AXIS] = -1*last_e_steps;
      }

      break;
    default:
#ifdef SEND_WRONG_CMD_INFO
      //showString(PSTR("Unknown G-COM:"));
      //Serial.println(cmdbuffer[bufindr]);
      // printf("Unknown G-COM:");
      // printf("%s\r\n",cmdbuffer[bufindr]);
#endif
      break;
    }
  }

  else if (code_seen('M')) {
    switch ((int) code_value()) {

    case 17: //enable all steppers
     enable_x();
     enable_y();
     enable_z();
     enable_e();
     break;

    case 18: //disable motors (similar to M84)
     all_axis = !((code_seen('X')) || (code_seen('Y')) || (code_seen('Z')) || (code_seen('E')));
     if (all_axis) {
       disable_x();
       disable_y();
       disable_z();
       disable_e();
     } else {
       //stepper.synchronize();
       if (code_seen('X')) disable_x();
       if (code_seen('Y')) disable_y();
       if (code_seen('Z')) disable_z();
       if (code_seen('E')) disable_e();
     }
      break;

  #ifdef SDSUPPORT
    case 20: // M20 - list SD card
      //showString(PSTR("Begin file list\r\n"));
      root.ls();
      //showString(PSTR("End file list\r\n"));
      break;
    case 21:// M21 - init SD card
      sdmode = false;
      initsd();
      break;
    case 22://M22 - release SD card
      sdmode = false;
      sdactive = false;
      break;
    case 23://M23 - Select file
      if(sdactive)
      {
        sdmode = false;
        file.close();
        starpos = (strchr(strchr_pointer + 4,'*'));

        if(starpos!=NULL)
          *(starpos-1)='\0';

        if (file.open(&root, strchr_pointer + 4, O_READ))
        {
          //showString(PSTR("File opened:"));
          //Serial.print(strchr_pointer + 4);
          // printf("File opened:");
          // printf("%d",strchr_pointer+4);
          // printf(" Size:");
          //showString(PSTR(" Size:"));
          // printf("%d\r\n",file.fileSize());
          //Serial.println(file.fileSize());
          sdpos = 0;
          filesize = file.fileSize();
          // printf("File selected\r\n");
          //showString(PSTR("File selected\r\n"));
        }
        else
        {
          // printf("file.open failed\r\n");
          //showString(PSTR("file.open failed\r\n"));
        }
      }
      break;
    case 24: //M24 - Start SD print
      if(sdactive)
      {
        sdmode = true;
      }
      break;
    case 25: //M25 - Pause SD print
      if(sdmode)
      {
        sdmode = false;
      }
      break;
    case 26: //M26 - Set SD index
      if(sdactive && code_seen('S'))
      {
        sdpos = code_value_long();
        file.seekSet(sdpos);
      }
      break;
    case 27: //M27 - Get SD status
      if(sdactive)
      {
        //showString(PSTR("SD printing byte "));
        //Serial.print(sdpos);
        //showString(PSTR("/"));
        //Serial.println(filesize);
        // printf("SD printing byte ");
        // printf("%d",sdpos);
        // printf("/");
        // printf("%d\r\n",filesize);
      }
      else
      {
        // printf("Not SD printing\r\n");
        //showString(PSTR("Not SD printing\r\n"));
      }
      break;
    case 28: //M28 - Start SD write
      if(sdactive)
      {
        char* npos = 0;
        file.close();
        sdmode = false;
        starpos = (strchr(strchr_pointer + 4,'*'));
        if(starpos != NULL)
        {
          npos = strchr(cmdbuffer[bufindr], 'N');
          strchr_pointer = strchr(npos,' ') + 1;
          *(starpos-1) = '\0';
        }

        if (!file.open(&root, strchr_pointer+4, O_CREAT | O_APPEND | O_WRITE | O_TRUNC))
        {
          //showString("open failed, File: ");
          //Serial.print(strchr_pointer + 4);
          //showString(".");
          // printf("open failed, File: ");
          // printf("%d",strchr_pointer + 4);
          // printf(".");
        }
        else
        {
          savetosd = true;
          //showString("Writing to file: ");
          // printf("Writing to file: ");
          // print("%d\r\n",strchr_pointer + 4);
        }
      }
      break;
    case 29: //M29 - Stop SD write
      //processed in write to file routine above
      //savetosd = false;
      break;
#ifndef SD_FAST_XFER_AKTIV
    case 30: // M30 filename - Delete file
      if(sdactive)
      {
        sdmode = false;
        file.close();

        starpos = (strchr(strchr_pointer + 4,'*'));

        if(starpos!=NULL)
          *(starpos-1)='\0';

        if(file.remove(&root, strchr_pointer + 4))
        {
          // printf("File deleted\r\n");
          //showString(PSTR("File deleted\r\n"));
        }
        else
        {
          // printf("Deletion failed\r\n");
          //showString(PSTR("Deletion failed\r\n"));
        }
      }
      break;
#else
    case 30: //M30 - fast SD transfer
      fast_xfer();
      break;
    case 31://M31 - high speed xfer capabilities
      // printf("RAW:");
      // printf("%d\r\n",SD_FAST_XFER_CHUNK_SIZE);
      //showString(PSTR("RAW:"));
      //Serial.println(SD_FAST_XFER_CHUNK_SIZE);
      break;
#endif

#endif
    case 42: //M42 -Change pin status via gcode
      if (code_seen('S')) {
#ifdef CHAIN_OF_COMMAND
        st_synchronize(); // wait for all movements to finish
#endif
        int pin_status = code_value();
        if (code_seen('P') && pin_status >= 0 && pin_status <= 255) {
          int pin_number = code_value();
          for (int i = 0; i < sizeof(sensitive_pins) / sizeof(int);
              i++) {
            if (sensitive_pins[i] == pin_number) {
              pin_number = -1;
              break;
            }
          }

          if (pin_number > -1) {
            pinMode(pin_number, OUTPUT);
            digitalWrite(pin_number, pin_status);
            //analogWrite(pin_number, pin_status);
          }
        }
      }
      break;
    case 104: // M104
#ifdef CHAIN_OF_COMMAND
      st_synchronize(); // wait for all movements to finish
#endif
      if(code_seen('T')){
        if(code_value()==1){
          //ignore temperature of dispenser head
          break;
        }
      }

      if (code_seen('S'))
        target_raw = temp2analogh(target_temp = code_value());
#ifdef WATCHPERIOD
      if(target_raw > current_raw)
      {
        watchmillis = max(1,millis());
        watch_raw = current_raw;
      }
      else
      {
        watchmillis = 0;
      }
#endif
      break;
    case 140: // M140 set bed temp
#ifdef CHAIN_OF_COMMAND
      st_synchronize(); // wait for all movements to finish
#endif
#if TEMP_1_PIN > -1 || defined BED_USES_AD595
      if (code_seen('S'))
        target_bed_raw = temp2analogBed(code_value());
#endif
      break;
    case 105: // M105
      //sprintf(temp_msg, "h:%d t:%d\r\n",block_buffer_head,block_buffer_tail);
      //uart_print(temp_msg);
      if(is_homing) return;
      //if(block_buffer_tail!=block_buffer_head) {
      // uart_print("busy\r\n");
      // return;
      //} 
      // return if state is busy 
#if (TEMP_0_PIN > -1) || defined (HEATER_USES_MAX6675)|| defined HEATER_USES_AD595
      hotendtC = analog2temp(current_raw);
#endif
#if TEMP_1_PIN > -1 || defined BED_USES_AD595
      bedtempC = analog2tempBed(current_bed_raw);
#endif
#if (TEMP_0_PIN > -1) || defined (HEATER_USES_MAX6675) || defined HEATER_USES_AD595
      //showString(PSTR("ok T:"));
      //Serial.print(hotendtC);
      //printf("current_raw:%d\r\n",current_raw);
      // printf("ok T:%d",hotendtC);

      sprintf(temp_msg, "ok T:%d\r\n",hotendtC);
      uart_print(temp_msg);
      
      MAILBOX_DATA_FLOAT(HOTEND_TEMP_ADDR) = hotendtC;
      clear_to_send = true;
      MAILBOX_CMD_ADDR = 0x0;
      return;
#ifdef PIDTEMP
      //showString(PSTR(" @:"));
      //Serial.print(heater_duty);
      /*
       //showString(PSTR(",P:"));
       Serial.print(pTerm);
       //showString(PSTR(",I:"));
       Serial.print(iTerm);
       //showString(PSTR(",D:"));
       Serial.print(dTerm);
       */
      // printf(" @:%d,P:%d,I:%d,D:%d",heater_duty,pTerm,iTerm,dTerm);
#ifdef AUTOTEMP
      //showString(PSTR(",AU:"));
      //Serial.print(autotemp_setpoint);
      // printf(",AU:%d",autotemp_setpoint);
#endif
#endif
#if TEMP_1_PIN > -1 || defined BED_USES_AD595
      // printf(" B:%d\r\n",bedtempC);
      //showString(PSTR(" B:"));
      //Serial.println(bedtempC);
#else
      // printf("\r\n");
      //Serial.println();
#endif
#else
#error No temperature source available
#endif
      return;
      //break;
    case 109: 
    //{ // M109 - Wait for extruder heater to reach target.
// #ifdef CHAIN_OF_COMMAND
//       st_synchronize(); // wait for all movements to finish
// #endif
      if(code_seen('T')){
        if(code_value()==1){
          //ignore temperature of dispenser head
          break;
        }
      }

      if (code_seen('S'))
        target_raw = temp2analogh(target_temp = code_value());
// #ifdef WATCHPERIOD
//       if(target_raw>current_raw)
//       {
//         watchmillis = max(1,millis());
//         watch_raw = current_raw;
//       }
//       else
//       {
//         watchmillis = 0;
//       }
// #endif
      // codenum = millis();
      codenum_heater = millis();
      waiting_until_setpoint = true;
      target_direction_heating = (current_raw < target_raw); // true if heating, false if cooling
      return;

      /*
      // See if we are heating up or cooling down 
      bool target_direction = (current_raw < target_raw); // true if heating, false if cooling

#ifdef TEMP_RESIDENCY_TIME
      long residencyStart;
      residencyStart = -1;
      // continue to loop until we have reached the target temp
      // _and_ until TEMP_RESIDENCY_TIME hasn't passed since we reached it 
      while( (target_direction ? (current_raw < target_raw) : (current_raw > target_raw))
          || (residencyStart > -1 && (millis() - residencyStart) < TEMP_RESIDENCY_TIME*1000) ) {
#else
        while (target_direction ?
            (current_raw < target_raw) : (current_raw > target_raw)) {
#endif
          if ((millis() - codenum) > 1000) //Print Temp Reading every 1 second while heating up/cooling down
          {
            //showString(PSTR("T:"));
            //Serial.println( analog2temp(current_raw) );
            // printf("T:%d\r\n",analog2temp(current_raw));
            codenum = millis();
          }
          //manage_heater();
          manage_heater(SysMonInstPtr);
#if (MINIMUM_FAN_START_SPEED > 0)
          manage_fan_start_speed();
#endif
// #ifdef TEMP_RESIDENCY_TIME
//          //start/restart the TEMP_RESIDENCY_TIME timer whenever we reach target temp for the first time
//          //or when current temp falls outside the hysteresis after target temp was reached 
//           if ( (residencyStart == -1 && target_direction && current_raw >= target_raw)
//               || (residencyStart == -1 && !target_direction && current_raw <= target_raw)
//               || (residencyStart > -1 && labs(analog2temp(current_raw) - analog2temp(target_raw)) > TEMP_HYSTERESIS) ) {
//             residencyStart = millis();
//           }
// #endif
        }
      }
      break;
    */ 
    case 190: // M190 - Wait for bed heater to reach target temperature.
#ifdef CHAIN_OF_COMMAND
      st_synchronize(); // wait for all movements to finish
#endif
#if TEMP_1_PIN > -1
      if (code_seen('S'))
        target_bed_raw = temp2analogBed(code_value());
      codenum = millis();
      while (current_bed_raw < target_bed_raw) {
        if ((millis() - codenum) > 1000) //Print Temp Reading every 1 second while heating up.
        {
          hotendtC = analog2temp(current_raw);
          //showString(PSTR("T:"));
          //Serial.print(hotendtC);
          //showString(PSTR(" B:"));
          // printf("T:%d B:%d\r\n",hotendtC,analog2tempBed(current_bed_raw));
          //Serial.println( analog2tempBed(current_bed_raw) );
          codenum = millis();
        }
        //manage_heater();
        manage_heater(SysMonInstPtr);
#if (MINIMUM_FAN_START_SPEED > 0)
        manage_fan_start_speed();
#endif
      }
#endif
      break;
#if FAN_PIN > -1
    case 106: //M106 Fan On
#ifdef CHAIN_OF_COMMAND
      st_synchronize(); // wait for all movements to finish
#endif
      if (code_seen('S')) {
        unsigned char l_fan_code_val = constrain(code_value(), 0, 255);

#if (MINIMUM_FAN_START_SPEED > 0)
        if(l_fan_code_val > 0 && fan_last_speed == 0)
        {
          if(l_fan_code_val < MINIMUM_FAN_START_SPEED)
          {
            fan_org_start_speed = l_fan_code_val;
            l_fan_code_val = MINIMUM_FAN_START_SPEED;
            previous_millis_fan_start = millis();
          }
          fan_last_speed = l_fan_code_val;
        }
        else
        {
          fan_last_speed = l_fan_code_val;
          fan_org_start_speed = 0;
        }
#endif

#if defined(FAN_SOFT_PWM) && (FAN_PIN > -1)
        g_fan_pwm_val = l_fan_code_val;
#else
        WRITE(FAN_PIN, HIGH);
        analogWrite_check(FAN_PIN, l_fan_code_val;
#endif

      } else {
#if defined(FAN_SOFT_PWM) && (FAN_PIN > -1)
        g_fan_pwm_val = 255;
#else
        WRITE(FAN_PIN, HIGH);
        analogWrite_check(FAN_PIN, 255 );
#endif
      }
      break;
    case 107: //M107 Fan Off
#if defined(FAN_SOFT_PWM) && (FAN_PIN > -1)
      g_fan_pwm_val = 0;
#else
      analogWrite_check(FAN_PIN, 0);
      WRITE(FAN_PIN, LOW);
#endif
      break;
#endif
#if (PS_ON_PIN > -1)
    case 80: // M81 - ATX Power On
      SET_OUTPUT(PS_ON_PIN);//GND
      break;
    case 81:// M81 - ATX Power Off
#ifdef CHAIN_OF_COMMAND
      st_synchronize(); // wait for all movements to finish
#endif
      SET_INPUT(PS_ON_PIN); //Floating
      break;
#endif
    case 82:
      axis_relative_modes[3] = false;
      break;
    case 83:
      axis_relative_modes[3] = true;
      break;
    case 84:
      //st_synchronize(); // wait for all movements to finish
      if (code_seen('S')) {
        stepper_inactive_time = code_value() * 1000;
      } else if (code_seen('T')) {
        enable_x();
        enable_y();
        enable_z();
        enable_e();
      } else {
        disable_x();
        disable_y();
        disable_z();
        disable_e();
      }
      break;
    case 85: // M85
      code_seen('S');
      max_inactive_time = code_value() * 1000;
      break;
    case 92: // M92
      for (int i = 0; i < NUM_AXIS; i++) {
        if (code_seen(axis_codes[i])) {
          axis_steps_per_unit[i] = code_value();
          axis_steps_per_sqr_second[i] =
              max_acceleration_units_per_sq_second[i]
                                 * axis_steps_per_unit[i];
        }
      }

      // Update start speed intervals and axis order. TODO: refactor axis_max_interval[] calculation into a function, as it
      // should also be used in setup() as well
      //        long temp_max_intervals[NUM_AXIS];
      //        for(int i=0; i < NUM_AXIS; i++)
      //        {
      //          axis_max_interval[i] = 100000000.0 / (max_start_speed_units_per_second[i] * axis_steps_per_unit[i]);//TODO: do this for
      //          all steps_per_unit related variables
      //        }
      break;
    case 93: // M93 show current axis steps.
    clear_to_send = true;
    MAILBOX_CMD_ADDR = 0x0;
      uart_print(ok_msg);
      // printf("ok \r\n");
      // printf("X:%dY:%dZ:%dE:%d\r\n",axis_steps_per_unit[0],axis_steps_per_unit[1],axis_steps_per_unit[2],axis_steps_per_unit[3]);
      //showString(PSTR("ok "));
      //showString(PSTR("X:"));
      //Serial.print(axis_steps_per_unit[0]);
      //showString(PSTR("Y:"));
      //Serial.print(axis_steps_per_unit[1]);
      //showString(PSTR("Z:"));
      //Serial.print(axis_steps_per_unit[2]);
      //showString(PSTR("E:"));
      //Serial.println(axis_steps_per_unit[3]);
      break;

    case 112: //emergency stop
      uart_print("mb reset\r\n");
      microblaze_disable_interrupts();
      (*((void (*)())(0x00)))(); // restart

    case 115: // M115
      //showString(PSTR("FIRMWARE_NAME: Sprinter Experimental PROTOCOL_VERSION:1.0 MACHINE_TYPE:Mendel EXTRUDER_COUNT:1\r\n"));
      // printf("FIRMWARE_NAME: Sprinter Experimental PROTOCOL_VERSION:1.0 MACHINE_TYPE:Mendel EXTRUDER_COUNT:1\r\n");
      // printf("%s\r\n",uuid);
      ////Serial.println(uuid);
      // printf("%s\r\n",_DEF_CHAR_UUID);
      //showString(PSTR(_DEF_CHAR_UUID));
      //showString(PSTR("\r\n"));
      break;
    case 114: // M114
      // printf("X: %f, Y: %f, Z: %f, E: %f\r\n",current_position[0],current_position[1],current_position[2],current_position[3]);
      //showString(PSTR("X:"));
      //  Serial.print(current_position[0]);
      //showString(PSTR("Y:"));
      //Serial.print(current_position[1]);
      //showString(PSTR("Z:"));
      //Serial.print(current_position[2]);
      //showString(PSTR("E:"));
      //Serial.println(current_position[3]);
      uart_print("X:");
      sprintf(temp_msg, "%.2f ",current_position[0]);
      uart_print(temp_msg);
      uart_print("Y:");
      sprintf(temp_msg,"%.2f ",current_position[1]);
      uart_print(temp_msg);
      uart_print("Z:");
      sprintf(temp_msg,"%.2f ",current_position[2]);
      uart_print(temp_msg);
      uart_print("E:");
      sprintf(temp_msg,"%.2f",current_position[3]);
      uart_print(temp_msg);

      MAILBOX_DATA_FLOAT(X_DATA_ADDR) = current_position[0];
      MAILBOX_DATA_FLOAT(Y_DATA_ADDR) = current_position[1];
      MAILBOX_DATA_FLOAT(Z_DATA_ADDR) = current_position[2];
      MAILBOX_DATA_FLOAT(E_DATA_ADDR) = current_position[3];
      break;

    case 119: // M119

#if (X_MIN_PIN > -1)
      //showString(PSTR("x_min:"));
      //Serial.print((READ(X_MIN_PIN) ^ X_ENDSTOP_INVERT) ? "H " : "L ");
      sprintf(temp_msg, "x_min: %s ", _CHK(XGpio_DiscreteRead(&ShieldInst, 1), X_MIN_PIN) != X_ENDSTOP_INVERT ? "H " : "L ");
      uart_print(temp_msg);
#endif
#if (X_MAX_PIN > -1)
      //showString(PSTR("x_max:"));
      //Serial.print((READ(X_MAX_PIN)^X_ENDSTOP_INVERT)?"H ":"L ");
      sprintf(temp_msg, "x_max: %s ", _CHK(XGpio_DiscreteRead(&ShieldInst, 1), X_MAX_PIN) != X_ENDSTOP_INVERT ? "H " : "L ");
      uart_print(temp_msg);
      #endif
#if (Y_MIN_PIN > -1)
      //showString(PSTR("y_min:"));
      //Serial.print((READ(Y_MIN_PIN) ^ Y_ENDSTOP_INVERT) ? "H " : "L ");
      sprintf(temp_msg, "y_min: %s ", _CHK(XGpio_DiscreteRead(&ShieldInst, 1), Y_MIN_PIN) != Y_ENDSTOP_INVERT ? "H " : "L ");
      uart_print(temp_msg);
#endif
#if (Y_MAX_PIN > -1)
      //showString(PSTR("y_max:"));
      //Serial.print((READ(Y_MAX_PIN)^Y_ENDSTOP_INVERT)?"H ":"L ");
      sprintf(temp_msg, "y_max: %s ", _CHK(XGpio_DiscreteRead(&ShieldInst, 1), Y_MAX_PIN) != Y_ENDSTOP_INVERT ? "H " : "L ");
      uart_print(temp_msg));
#endif
#if (Z_MIN_PIN > -1)
      //showString(PSTR("z_min:"));
      //Serial.print((READ(Z_MIN_PIN) ^ Z_ENDSTOP_INVERT) ? "H " : "L ");
      sprintf(temp_msg, "z_min: %s ", _CHK(XGpio_DiscreteRead(&ShieldInst, 1), Z_MIN_PIN) != Z_ENDSTOP_INVERT ? "H " : "L ");
      uart_print(temp_msg);
#endif
#if (Z_MAX_PIN > -1)
      //showString(PSTR("z_max:"));
      //Serial.print((READ(Z_MAX_PIN)^Z_ENDSTOP_INVERT)?"H ":"L ");
      sprintf(temp_msg, "z_max: %s ", _CHK(XGpio_DiscreteRead(&ShieldInst, 1), Z_MAX_PIN) != Z_ENDSTOP_INVERT ? "H " : "L ");
      uart_print(temp_msg);
#endif

      //showString(PSTR("\r\n"));
      break;
    case 201: // M201  Set maximum acceleration in units/s^2 for print moves (M201 X1000 Y1000)

      for (int8_t i = 0; i < NUM_AXIS; i++) {
        if (code_seen(axis_codes[i])) {
          max_acceleration_units_per_sq_second[i] = code_value();
          axis_steps_per_sqr_second[i] = code_value()
                    * axis_steps_per_unit[i];
        }
      }
      break;
#if 0 // Not used for Sprinter/grbl gen6
    case 202: // M202
      for(int i=0; i < NUM_AXIS; i++)
      {
        if(code_seen(axis_codes[i])) axis_travel_steps_per_sqr_second[i] = code_value() * axis_steps_per_unit[i];
      }
      break;
#else
    case 202: // M202 max feedrate mm/sec
      for (int8_t i = 0; i < NUM_AXIS; i++) {
        if (code_seen(axis_codes[i]))
          max_feedrate[i] = code_value();
      }
      break;
#endif
    case 203: // M203 Temperature monitor
      if (code_seen('S'))
        manage_monitor = code_value();
      if (manage_monitor == 100)
        manage_monitor = 1; // Set 100 to heated bed
      break;
    case 204: // M204 acceleration S normal moves T filmanent only moves
      if (code_seen('S'))
        move_acceleration = code_value();
      if (code_seen('T'))
        retract_acceleration = code_value();
      break;
    case 205: //M205 advanced settings:  minimum travel speed S=while printing T=travel only,  B=minimum segment time X= maximum xy jerk, Z=maximum Z jerk, E= max E jerk
      if (code_seen('S'))
        minimumfeedrate = code_value();
      if (code_seen('T'))
        mintravelfeedrate = code_value();
      //if(code_seen('B')) minsegmenttime = code_value() ;
      if (code_seen('X'))
        max_xy_jerk = code_value();
      if (code_seen('Z'))
        max_z_jerk = code_value();
      if (code_seen('E'))
        max_e_jerk = code_value();
      break;
    case 206: // M206 additional homing offset
      if (code_seen('D')) {
        // printf("Addhome X:%d Y:%d Z%d\r\n",add_homing[0],add_homing[1],add_homing[2]);
        //showString(PSTR("Addhome X:")); Serial.print(add_homing[0]);
        //showString(PSTR(" Y:")); Serial.print(add_homing[1]);
        //showString(PSTR(" Z:")); //Serial.println(add_homing[2]);
      }

      for (int8_t cnt_i = 0; cnt_i < 3; cnt_i++) {
        if (code_seen(axis_codes[cnt_i]))
          add_homing[cnt_i] = code_value();
      }
      break;
    case 220: // M220 S<factor in percent>- set speed factor override percentage
    {
      if (code_seen('S')) {
        feedmultiply = code_value();
        feedmultiply = constrain(feedmultiply, 20, 200);
        feedmultiplychanged = true;
      }
    }
    break;
    case 221: // M221 S<factor in percent>- set extrude factor override percentage
    {
      if (code_seen('S')) {
        extrudemultiply = code_value();
        extrudemultiply = constrain(extrudemultiply, 40, 200);
      }
    }
    break;
#ifdef PIDTEMP
    case 301: // M301
    {
      if (code_seen('P'))
        PID_Kp = code_value();
      if (code_seen('I'))
        PID_Ki = code_value();
      if (code_seen('D'))
        PID_Kd = code_value();
      updatePID();
    }
    break;
#endif //PIDTEMP      
#ifdef PID_AUTOTUNE
    case 303: // M303 PID autotune
    {
      float help_temp = 150.0;
      if (code_seen('S'))
        help_temp = code_value();
      PID_autotune(SysMonInstPtr, help_temp);
    }
    break;
#endif
    case 400: // M400 finish all moves
    {
      st_synchronize();
    }
    break;
#ifdef USE_EEPROM_SETTINGS
    case 500: // Store settings in EEPROM
    {
      EEPROM_StoreSettings();
    }
    break;
    case 501: // Read settings from EEPROM
    {
      EEPROM_RetrieveSettings(false,true);
      for(int8_t i=0; i < NUM_AXIS; i++)
      {
        axis_steps_per_sqr_second[i] = max_acceleration_units_per_sq_second[i] * axis_steps_per_unit[i];
      }
    }
    break;
    case 502: // Revert to default settings
    {
      EEPROM_RetrieveSettings(true,true);
      for(int8_t i=0; i < NUM_AXIS; i++)
      {
        axis_steps_per_sqr_second[i] = max_acceleration_units_per_sq_second[i] * axis_steps_per_unit[i];
      }
    }
    break;
    case 503: // print settings currently in memory
    {
      EEPROM_printSettings();
    }
    break;
#endif      
#ifdef DEBUG_HEATER_TEMP
    case 601: // M601  show Extruder Temp jitter
#if (TEMP_0_PIN > -1) || defined (HEATER_USES_MAX6675)|| defined HEATER_USES_AD595
      if(current_raw_maxval > 0)
        tt_maxval = analog2temp(current_raw_maxval);
      if(current_raw_minval < 10000)
        tt_minval = analog2temp(current_raw_minval);
#endif

      // printf("Tmin:%d / Tmax:%d \r\n",tt_minval,tt_maxval);
      //showString(PSTR("Tmin:"));
      //Serial.print(tt_minval);
      //showString(PSTR(" / Tmax:"));
      //Serial.print(tt_maxval);
      //showString(PSTR(" "));
      break;
    case 602:// M602  reset Extruder Temp jitter
      current_raw_minval = 32000;
      current_raw_maxval = -32000;

      // printf("T Minmax Reset \r\n");
      //showString(PSTR("T Minmax Reset "));
      break;
#endif
    case 603: // M603  Free RAM
      // printf("Free Ram: %d\r\n",FreeRam1());
      //showString(PSTR("Free Ram: "));
      //Serial.println(FreeRam1());
      break;

    case 700: //UV LED ON
      uv_led_on();   
      break;

    case 701: //UV LED OFF
      uv_led_off();   
      break;

    case 702: // dispenser on
      dispenser_on();
      break;

    case 703: // dispenser off
      dispenser_off();
      break;

    case 704: // Change dispenser discharge pressure
      int target_pressure;
      if (code_seen('S')){
        target_pressure = code_value();
      }
      update_dispenser_pressure(1, target_pressure); 
      break;

    case 705:
      use_dispenser = !use_dispenser;  
      if(use_dispenser){
      	uart_print("dispenser pressure control enabled.\r\n");
      } else {
      	uart_print("dispenser pressure control disabled.\r\n");
      }
      break;

    case 709:
      //homeaxis(X_AXIS);
      break;
    case 710:
      //homeaxis(Y_AXIS);
      break;
    case 711:
      //homeaxis(Z_AXIS);
      break;
    case 712:
      sprintf(temp_msg, "P:%d I:%d D:%d\r\n",PID_Kp, PID_Ki, PID_Kd);
      uart_print(temp_msg);
      break;
    case 713:
      cold_extrusion = !(cold_extrusion);
      if(cold_extrusion) uart_print("Cold extrusion enabled\r\n");
      if(!cold_extrusion) uart_print("Cold extrusion disabled\r\n");
      break;

    default:
#ifdef SEND_WRONG_CMD_INFO
      //showString(PSTR("Unknown M-COM:"));
      //Serial.println(cmdbuffer[bufindr]);
      // printf("Unknown M-COM:%d\r\n",cmdbuffer[bufindr]);
#endif
      break;

    }
   }
    else if (code_seen('T')) {
      gcode_T(code_value());
    } else {
      sprintf(temp_msg, "Unknown command:%s\r\n", cmdbuffer[bufindr]);
      uart_print(temp_msg);
      // printf("Unknown command:%s\r\n",cmdbuffer[bufindr]);
      //showString(PSTR("Unknown command:\r\n"));
      //Serial.println(cmdbuffer[bufindr]);
    }

    ClearToSend();

  }


  void uv_led_on(){
      //uart_print("UV LED ON\r\n"); 
      _SET(ck_shields_data, UV_PIN);
      XGpio_DiscreteWrite(&CK_ShieldInst, 1, ck_shields_data);
  }

  void uv_led_off(){
      //uart_print("UV LED OFF\r\n"); 
      _CLR(ck_shields_data, UV_PIN);
      XGpio_DiscreteWrite(&CK_ShieldInst, 1, ck_shields_data);
  }

  void dispenser_on(){
      //uart_print("Dispenser ON\r\n"); 
      _SET(ck_shields_data, DISP_PIN);
      XGpio_DiscreteWrite(&CK_ShieldInst, 1, ck_shields_data);
  }

  void dispenser_off(){
      //uart_print("Dispenser OFF\r\n"); 
      _CLR(ck_shields_data , DISP_PIN);
      XGpio_DiscreteWrite(&CK_ShieldInst, 1, ck_shields_data);
  }

  void update_dispenser_pressure(int channel, int target_pressure){
     if(millis() - previous_millis_dispenser > 100){
        //String base_command = "0EPH  CH001P";
        sprintf(dispenser_cmd_msg, "0EPH  CH001P%d", target_pressure);
        int string_length = strlen(dispenser_cmd_msg);
        
        byte result = 0x00;
        for (int i=0;i<string_length;i++){
        	result = result - dispenser_cmd_msg[i];
        }
        // byte result = 0x00 - 0x30 - 0x34 - 0x44 - 0x49 - 0x20 - 0x20; //-305->two's complement of 305->0b011001111->0xCF
        byte ubyte = ((result & 0b11110000) >> 4) & 0b11111111;
        byte lbyte = result & 0b00001111;
        
        char ubyteChar[2];
        char lbyteChar[2];
        sprintf(ubyteChar,"%x",ubyte);
        sprintf(lbyteChar,"%x",lbyte);

       //uart_print("Update pressure\r\n");
       //uart_print(dispenser_cmd_msg);
       //uart_print("\r\n");
       //sprintf(temp_msg, "%lu\r\n",previous_millis_dispenser);
       //uart_print(temp_msg);

       rs232c_write(ENQ);
       usleep(100);
       rs232c_write(STX); 
       rs232c_print(dispenser_cmd_msg); //do not send null terminator
       rs232c_write(ubyteChar[0]);
       rs232c_write(lbyteChar[0]);
       rs232c_write(ETX); 

       rs232c_read(returned_message);
       //uart_print((char *)returned_message); //do not send null terminator
       //uart_print("\r\n"); //do not send null terminator
       bool rs232c_error = false;
       for(int i=0;i<strlen(a0_string);i++){
          if ((char) returned_message[i] != a0_string[i]){
            rs232c_error = true;
          }
       }
       if(rs232c_error){
         //uart_print("RS232C error\r\n");
         rs232c_write(STX); 
         rs232c_print(can_string); 
         rs232c_write(ETX); 
         //dispenser_delay_time = 600;
        //sprintf(temp_msg, "res:%d,0\r\n", millis()); 
        ///uart_print(temp_msg);
       } else {
         rs232c_write(EOT);
         //dispenser_delay_time = 100;
         success_count++;
         //sprintf(temp_msg, "res:%d,1\r\n", millis()); 
         //uart_print(temp_msg);
       }
       total_count++;
      //sprintf(temp_msg, "success:%d total:%d\r\n", success_count, total_count); 
      //uart_print(temp_msg);
      previous_millis_dispenser = millis();
    }
 
  }

  void FlushSerialRequestResend() {
    //always commented out below
    //char cmdbuffer[bufindr][100]="Resend:";
    //  Serial.flush();
    // printf("Resend:%d\r\n",gcode_LastN+1);
    //  //showString(PSTR("Resend:"));
    //  //Serial.println(gcode_LastN + 1);
    ClearToSend();
  }

  void ClearToSend() {
    previous_millis_cmd = millis();
#ifdef SDSUPPORT
    if(fromsd[bufindr])
      return;
#endif
  clear_to_send = true;
  MAILBOX_CMD_ADDR = 0x0;
  uart_print(ok_msg);
  //uart_write(uart_dev0,ok_msg,strlen(ok_msg));
    // printf("ok\r\n");
    //  //showString(PSTR("ok\r\n"));
    ////Serial.println("ok");
  }

  FORCE_INLINE void get_coordinates() {
    for (int i = 0; i < NUM_AXIS; i++) {
      if (code_seen(axis_codes[i]))
        destination[i] = (float) code_value()
        + (axis_relative_modes[i] || relative_mode)
        * current_position[i];
      else
        destination[i] = current_position[i]; //Are these else lines really needed?
    }

    if (code_seen('F')) {
      next_feedrate = code_value();
      if (next_feedrate > 0.0)
        feedrate = next_feedrate;
    }
  }

#ifdef USE_ARC_FUNCTION
  void get_arc_coordinates()
  {
    get_coordinates();
    if(code_seen('I')) {
      offset[0] = code_value();
    }
    else {
      offset[0] = 0.0;
    }
    if(code_seen('J')) {
      offset[1] = code_value();
    }
    else {
      offset[1] = 0.0;
    }
  }
#endif

  void prepare_move() {
    long help_feedrate = 0;

    if (!is_homing) {
      if (min_software_endstops) {
        if (destination[X_AXIS] < 0)
          destination[X_AXIS] = 0.0;
        if (destination[Y_AXIS] < 0)
          destination[Y_AXIS] = 0.0;
        if (destination[Z_AXIS] < 0)
          destination[Z_AXIS] = 0.0;
      }

      if (max_software_endstops) {
        if (destination[X_AXIS] > X_MAX_LENGTH)
          destination[X_AXIS] = X_MAX_LENGTH;
        if (destination[Y_AXIS] > Y_MAX_LENGTH)
          destination[Y_AXIS] = Y_MAX_LENGTH;
        if (destination[Z_AXIS] > Z_MAX_LENGTH)
          destination[Z_AXIS] = Z_MAX_LENGTH;
      }
    }

    if (destination[E_AXIS] > current_position[E_AXIS]) {
      help_feedrate = ((long) feedrate * (long) feedmultiply);
    } else {
      help_feedrate = ((long) feedrate * (long) 100);
    }

    #ifdef DELTA
    //TODO: IK calculation should be segmented by fraction
    inverse_kinematics(destination);
    //plan_buffer_line(destination[X_AXIS], destination[Y_AXIS],
    //    destination[Z_AXIS], destination[E_AXIS], help_feedrate / 6000.0);
    plan_buffer_line(delta[X_AXIS], delta[Y_AXIS],
        delta[Z_AXIS], destination[E_AXIS], help_feedrate / 6000.0);
    #else 
      plan_buffer_line(destination[X_AXIS], destination[Y_AXIS],
          destination[Z_AXIS], destination[E_AXIS], help_feedrate / 6000.0);
    #endif

    for (int i = 0; i < NUM_AXIS; i++) {
      current_position[i] = destination[i];
    }
  }


  void inverse_kinematics(const float cartesian[3]) {

    delta[TOWER_1] = sqrt(delta_diagonal_rod_2_tower_1
        - sq(delta_tower1_x - cartesian[X_AXIS])
        - sq(delta_tower1_y - cartesian[Y_AXIS])
    ) + cartesian[Z_AXIS];
    delta[TOWER_2] = sqrt(delta_diagonal_rod_2_tower_2
        - sq(delta_tower2_x - cartesian[X_AXIS])
        - sq(delta_tower2_y - cartesian[Y_AXIS])
    ) + cartesian[Z_AXIS];
    delta[TOWER_3] = sqrt(delta_diagonal_rod_2_tower_3
        - sq(delta_tower3_x - cartesian[X_AXIS])
        - sq(delta_tower3_y - cartesian[Y_AXIS])
    ) + cartesian[Z_AXIS];

    //for(int i=0;i<3;i++){
     // printf("cartesian[%d]: %f, delta[%d]: %f\r\n",i,cartesian[i],i,delta[i]);
    //}

  }


#ifdef USE_ARC_FUNCTION
  void prepare_arc_move(char isclockwise)
  {

    float r = hypot(offset[X_AXIS], offset[Y_AXIS]); // Compute arc radius for mc_arc
    long help_feedrate = 0;

    if(destination[E_AXIS] > current_position[E_AXIS])
    {
      help_feedrate = ((long)feedrate*(long)feedmultiply);
    }
    else
    {
      help_feedrate = ((long)feedrate*(long)100);
    }

    // Trace the arc
    mc_arc(current_position, destination, offset, X_AXIS, Y_AXIS, Z_AXIS, help_feedrate/6000.0, r, isclockwise);

    // As far as the parser is concerned, the position is now == target. In reality the
    // motion control system might still be processing the action and the real tool position
    // in any intermediate location.
    for(int8_t i=0; i < NUM_AXIS; i++)
    {
      current_position[i] = destination[i];
    }
  }
#endif

  FORCE_INLINE void kill() {
#if TEMP_0_PIN > -1
    target_raw = 0;
    //WRITE(HEATER_0_PIN, LOW);
#endif

#if TEMP_1_PIN > -1
    target_bed_raw = 0;
    //    if(HEATER_1_PIN > -1) WRITE(HEATER_1_PIN,LOW);
#endif

    disable_x();
    disable_y();
    disable_z();
    disable_e();

    if (PS_ON_PIN > -1)
      pinMode(PS_ON_PIN, INPUT);

  }

  FORCE_INLINE void manage_inactivity(byte debug) {
    if ((millis() - previous_millis_cmd) > max_inactive_time)
      if (max_inactive_time)
        kill();

    if ((millis() - previous_millis_cmd) > stepper_inactive_time)
      if (stepper_inactive_time) {
        disable_x();
        disable_y();
        disable_z();
        disable_e();
      }
    check_axes_activity();
  }

#if (MINIMUM_FAN_START_SPEED > 0)
  void manage_fan_start_speed(void)
  {
    if(fan_org_start_speed > 0)
    {
      if((millis() - previous_millis_fan_start) > MINIMUM_FAN_START_TIME )
      {
#if FAN_PIN > -1
#if defined(FAN_SOFT_PWM)
        g_fan_pwm_val = fan_org_start_speed;
#else
        WRITE(FAN_PIN, HIGH);
        analogWrite_check(FAN_PIN, fan_org_start_speed;
#endif
#endif

        fan_org_start_speed = 0;
      }
    }
  }
#endif

  // Planner with Interrupt for Stepper

  /*
 Reasoning behind the mathematics in this module (in the key of 'Mathematica'):

 s == speed, a == acceleration, t == time, d == distance

 Basic definitions:

 Speed[s_, a_, t_] := s + (a*t) 
 Travel[s_, a_, t_] := Integrate[Speed[s, a, t], t]

 Distance to reach a specific speed with a constant acceleration:

 Solve[{Speed[s, a, t] == m, Travel[s, a, t] == d}, d, t]
 d -> (m^2 - s^2)/(2 a) --> estimate_acceleration_distance()

 Speed after a given distance of travel with constant acceleration:

 Solve[{Speed[s, a, t] == m, Travel[s, a, t] == d}, m, t]
 m -> Sqrt[2 a d + s^2]    

 DestinationSpeed[s_, a_, d_] := Sqrt[2 a d + s^2]

 When to start braking (di) to reach a specified destionation speed (s2) after accelerating
 from initial speed s1 without ever stopping at a plateau:

 Solve[{DestinationSpeed[s1, a, di] == DestinationSpeed[s2, a, d - di]}, di]
 di -> (2 a d - s1^2 + s2^2)/(4 a) --> intersection_distance()

 IntersectionDistance[s1_, s2_, a_, d_] := (2 a d - s1^2 + s2^2)/(4 a)
   */


  //===========================================================================
  //=============================private variables ============================
  //===========================================================================

  // Returns the index of the next block in the ring buffer
  // NOTE: Removed modulo (%) operator, which uses an expensive divide and multiplication.
  static int8_t next_block_index(int8_t block_index) {
    block_index++;
    if (block_index == BLOCK_BUFFER_SIZE) {
      block_index = 0;
    }
    return (block_index);
  }

  // Returns the index of the previous block in the ring buffer
  static int8_t prev_block_index(int8_t block_index) {
    if (block_index == 0) {
      block_index = BLOCK_BUFFER_SIZE;
    }
    block_index--;
    return (block_index);
  }


  // Calculates the distance (not time) it takes to accelerate from initial_rate to target_rate using the
  // given acceleration:
  FORCE_INLINE float estimate_acceleration_distance(float initial_rate,
      float target_rate, float acceleration) {
    if (acceleration != 0) {
      return ((target_rate * target_rate - initial_rate * initial_rate)
          / (2.0 * acceleration));
    } else {
      return 0.0;  // acceleration was 0, set acceleration distance to 0
    }
  }

  // This function gives you the point at which you must start braking (at the rate of -acceleration) if
  // you started at speed initial_rate and accelerated until this point and want to end at the final_rate after
  // a total travel of distance. This can be used to compute the intersection point between acceleration and
  // deceleration in the cases where the trapezoid has no plateau (i.e. never reaches maximum speed)

  FORCE_INLINE float intersection_distance(float initial_rate, float final_rate,
      float acceleration, float distance) {
    if (acceleration != 0) {
      return ((2.0 * acceleration * distance - initial_rate * initial_rate
          + final_rate * final_rate) / (4.0 * acceleration));
    } else {
      return 0.0;  // acceleration was 0, set intersection distance to 0
    }
  }

  // Calculates trapezoid parameters so that the entry- and exit-speed is compensated by the provided factors.

  void calculate_trapezoid_for_block(block_t *block, float entry_factor,
      float exit_factor) {
    unsigned long initial_rate = ceil(block->nominal_rate * entry_factor); // (step/min)
    unsigned long final_rate = ceil(block->nominal_rate * exit_factor); // (step/min)

    // Limit minimal step rate (Otherwise the timer will overflow.)
    if (initial_rate < 120) {
      initial_rate = 120;
    }
    if (final_rate < 120) {
      final_rate = 120;
    }

    long acceleration = block->acceleration_st;
    int32_t accelerate_steps = ceil(
        estimate_acceleration_distance(block->initial_rate,
            block->nominal_rate, acceleration));
    int32_t decelerate_steps = floor(
        estimate_acceleration_distance(block->nominal_rate,
            block->final_rate, -acceleration));

    // Calculate the size of Plateau of Nominal Rate.
    int32_t plateau_steps = block->step_event_count - accelerate_steps
        - decelerate_steps;

    // Is the Plateau of Nominal Rate smaller than nothing? That means no cruising, and we will
    // have to use intersection_distance() to calculate when to abort acceleration and start breaking
    // in order to reach the final_rate exactly at the end of this block.
    if (plateau_steps < 0) {
      accelerate_steps = ceil(
          intersection_distance(block->initial_rate, block->final_rate,
              acceleration, block->step_event_count));
      accelerate_steps = max(accelerate_steps, 0); // Check limits due to numerical round-off
      accelerate_steps = min(accelerate_steps, block->step_event_count);
      plateau_steps = 0;
    }

#ifdef ADVANCE
    volatile long initial_advance = block->advance*entry_factor*entry_factor;
    volatile long final_advance = block->advance*exit_factor*exit_factor;
#endif // ADVANCE

    // block->accelerate_until = accelerate_steps;
    // block->decelerate_after = accelerate_steps+plateau_steps;
    ///CRITICAL_SECTION_START
    ;  // Fill variables used by the stepper in a critical section
    if (block->busy == false) { // Don't update variables if block is busy.
      block->accelerate_until = accelerate_steps;
      block->decelerate_after = accelerate_steps + plateau_steps;
      block->initial_rate = initial_rate;
      block->final_rate = final_rate;
#ifdef ADVANCE
      block->initial_advance = initial_advance;
      block->final_advance = final_advance;
#endif //ADVANCE
    }
    //  CRITICAL_SECTION_END;
  }

  // Calculates the maximum allowable speed at this point when you must be able to reach target_velocity using the
  // acceleration within the allotted distance.
  FORCE_INLINE float max_allowable_speed(float acceleration,
      float target_velocity, float distance) {
    return sqrt(target_velocity * target_velocity - 2 * acceleration * distance);
  }

  // "Junction jerk" in this context is the immediate change in speed at the junction of two blocks.
  // This method will calculate the junction jerk as the euclidean distance between the nominal
  // velocities of the respective blocks.
  //inline float junction_jerk(block_t *before, block_t *after) {
  //  return sqrt(
  //    pow((before->speed_x-after->speed_x), 2)+pow((before->speed_y-after->speed_y), 2));
  //}

  // The kernel called by planner_recalculate() when scanning the plan from last to first entry.
  void planner_reverse_pass_kernel(block_t *previous, block_t *current,
      block_t *next) {
    if (!current) {
      return;
    }

    if (next) {
      // If entry speed is already at the maximum entry speed, no need to recheck. Block is cruising.
      // If not, block in state of acceleration or deceleration. Reset entry speed to maximum and
      // check for maximum allowable speed reductions to ensure maximum possible planned speed.
      if (current->entry_speed != current->max_entry_speed) {

        // If nominal length true, max junction speed is guaranteed to be reached. Only compute
        // for max allowable speed if block is decelerating and nominal length is false.
        if ((!current->nominal_length_flag)
            && (current->max_entry_speed > next->entry_speed)) {
          current->entry_speed = min(current->max_entry_speed,
              max_allowable_speed(-current->acceleration,
                  next->entry_speed, current->millimeters));
        } else {
          current->entry_speed = current->max_entry_speed;
        }
        current->recalculate_flag = true;

      }
    } // Skip last block. Already initialized and set for recalculation.
  }

  // planner_recalculate() needs to go over the current plan twice. Once in reverse and once forward. This
  // implements the reverse pass.
  void planner_reverse_pass() {
    uint8_t block_index = block_buffer_head;

    //Make a local copy of block_buffer_tail, because the interrupt can alter it
    //CRITICAL_SECTION_START

    unsigned char tail = block_buffer_tail;
    //CRITICAL_SECTION_END;

    if (((block_buffer_head - tail + BLOCK_BUFFER_SIZE)
        & (BLOCK_BUFFER_SIZE - 1)) > 3) {
      block_index = (block_buffer_head - 3) & (BLOCK_BUFFER_SIZE - 1);
      block_t *block[3] = { NULL, NULL, NULL };
      while (block_index != tail) {
        block_index = prev_block_index(block_index);
        block[2] = block[1];
        block[1] = block[0];
        block[0] = &block_buffer[block_index];
        planner_reverse_pass_kernel(block[0], block[1], block[2]);
      }
    }
  }

  // The kernel called by planner_recalculate() when scanning the plan from first to last entry.
  void planner_forward_pass_kernel(block_t *previous, block_t *current,
      block_t *next) {
    if (!previous) {
      return;
    }

    // If the previous block is an acceleration block, but it is not long enough to complete the
    // full speed change within the block, we need to adjust the entry speed accordingly. Entry
    // speeds have already been reset, maximized, and reverse planned by reverse planner.
    // If nominal length is true, max junction speed is guaranteed to be reached. No need to recheck.
    if (!previous->nominal_length_flag) {
      if (previous->entry_speed < current->entry_speed) {
        double entry_speed = min(current->entry_speed,
            max_allowable_speed(-previous->acceleration,
                previous->entry_speed, previous->millimeters));

        // Check for junction speed change
        if (current->entry_speed != entry_speed) {
          current->entry_speed = entry_speed;
          current->recalculate_flag = true;
        }
      }
    }
  }

  // planner_recalculate() needs to go over the current plan twice. Once in reverse and once forward. This
  // implements the forward pass.
  void planner_forward_pass() {
    uint8_t block_index = block_buffer_tail;
    block_t *block[3] = { NULL, NULL, NULL };

    while (block_index != block_buffer_head) {
      block[0] = block[1];
      block[1] = block[2];
      block[2] = &block_buffer[block_index];
      planner_forward_pass_kernel(block[0], block[1], block[2]);
      block_index = next_block_index(block_index);
    }
    planner_forward_pass_kernel(block[1], block[2], NULL);
  }

  // Recalculates the trapezoid speed profiles for all blocks in the plan according to the
  // entry_factor for each junction. Must be called by planner_recalculate() after
  // updating the blocks.
  void planner_recalculate_trapezoids() {
    int8_t block_index = block_buffer_tail;
    block_t *current;
    block_t *next = NULL;

    while (block_index != block_buffer_head) {
      current = next;
      next = &block_buffer[block_index];
      if (current) {
        // Recalculate if current block entry or exit junction speed has changed.
        if (current->recalculate_flag || next->recalculate_flag) {
          // NOTE: Entry and exit factors always > 0 by all previous logic operations.
          calculate_trapezoid_for_block(current,
              current->entry_speed / current->nominal_speed,
              next->entry_speed / current->nominal_speed);
          current->recalculate_flag = false; // Reset current only to ensure next trapezoid is computed
        }
      }
      block_index = next_block_index(block_index);
    }
    // Last/newest block in buffer. Exit speed is set with MINIMUM_PLANNER_SPEED. Always recalculated.
    if (next != NULL) {
      calculate_trapezoid_for_block(next,
          next->entry_speed / next->nominal_speed,
          MINIMUM_PLANNER_SPEED / next->nominal_speed);
      next->recalculate_flag = false;
    }
  }

  // Recalculates the motion plan according to the following algorithm:
  //
  //   1. Go over every block in reverse order and calculate a junction speed reduction (i.e. block_t.entry_factor)
  //      so that:
  //     a. The junction jerk is within the set limit
  //     b. No speed reduction within one block requires faster deceleration than the one, true constant
  //        acceleration.
  //   2. Go over every block in chronological order and dial down junction speed reduction values if
  //     a. The speed increase within one block would require faster accelleration than the one, true
  //        constant acceleration.
  //
  // When these stages are complete all blocks have an entry_factor that will allow all speed changes to
  // be performed using only the one, true constant acceleration, and where no junction jerk is jerkier than
  // the set limit. Finally it will:
  //
  //   3. Recalculate trapezoids for all blocks.

  void planner_recalculate() {
    planner_reverse_pass();
    planner_forward_pass();
    planner_recalculate_trapezoids();
  }

  void plan_init() {
    block_buffer_head = 0;
    block_buffer_tail = 0;
    memset(position, 0, sizeof(position)); // clear position
    previous_speed[0] = 0.0;
    previous_speed[1] = 0.0;
    previous_speed[2] = 0.0;
    previous_speed[3] = 0.0;
    previous_nominal_speed = 0.0;
  }

  FORCE_INLINE void plan_discard_current_block() {
    if (block_buffer_head != block_buffer_tail) {
      block_buffer_tail = (block_buffer_tail + 1) & BLOCK_BUFFER_MASK;
    }
  }

  FORCE_INLINE block_t *plan_get_current_block() {
    if (block_buffer_head == block_buffer_tail) {
      return (NULL);
    }
    block_t *block = &block_buffer[block_buffer_tail];
    block->busy = true;
    return (block);
  }

  // Gets the current block. Returns NULL if buffer empty
  // FORCE_INLINE bool blocks_queued() {
  bool blocks_queued() {
    if (block_buffer_head == block_buffer_tail) {
      return false;
    } else {
      return true;
    }
  }

  void check_axes_activity() {
    unsigned char x_active = 0;
    unsigned char y_active = 0;
    unsigned char z_active = 0;
    unsigned char e_active = 0;
    block_t *block;

    if (block_buffer_tail != block_buffer_head) {
      uint8_t block_index = block_buffer_tail;
      while (block_index != block_buffer_head) {
        block = &block_buffer[block_index];
        if (block->steps_x != 0)
          x_active++;
        if (block->steps_y != 0)
          y_active++;
        if (block->steps_z != 0)
          z_active++;
        if (block->steps_e != 0)
          e_active++;
        block_index = (block_index + 1) & (BLOCK_BUFFER_SIZE - 1);
      }
    }
    if ((DISABLE_X) &&(x_active == 0))
      disable_x();
    if ((DISABLE_Y) &&(y_active == 0))
      disable_y();
    if ((DISABLE_Z) &&(z_active == 0))
      disable_z();
    if ((DISABLE_E) &&(e_active == 0))
      disable_e();
  }

  // Add a new linear movement to the buffer. steps_x, _y and _z is the absolute position in
  // mm. Microseconds specify how many microseconds the move should take to perform. To aid acceleration
  // calculation the caller must also provide the physical length of the line in millimeters.
  void plan_buffer_line(float x, float y, float z, float e, float feed_rate) {
    // Calculate the buffer head after we push this byte
    int next_buffer_head = next_block_index(block_buffer_head); // 0 to BLOCK_BUFFER_SIZE-1(=15)

    // If the buffer is full: good! That means we are well ahead of the robot.
    // Rest here until there is room in the buffer.
    /*  comment out: since we cannot run this code inside the AXI timer
    while (block_buffer_tail == next_buffer_head) {
      //manage_heater();
      manage_heater(SysMonInstPtr);
      manage_inactivity(1);
#if (MINIMUM_FAN_START_SPEED > 0)
      manage_fan_start_speed();
#endif
    }
    */

    // The target position of the tool in absolute steps
    // Calculate target position in absolute steps
    //this should be done after the wait, because otherwise a M92 code within the gcode disrupts this calculation somehow
    long target[4];
    target[X_AXIS] = lround(x * axis_steps_per_unit[X_AXIS]);
    target[Y_AXIS] = lround(y * axis_steps_per_unit[Y_AXIS]);
    target[Z_AXIS] = lround(z * axis_steps_per_unit[Z_AXIS]);
    target[E_AXIS] = lround(e * axis_steps_per_unit[E_AXIS]);

    // Prepare to set up new block
    block_t *block = &block_buffer[block_buffer_head];

    // Mark block as not busy (Not executed by the stepper interrupt)
    block->busy = false;

    // Number of steps for each axis
    block->steps_x = labs(target[X_AXIS] - position[X_AXIS]);
    block->steps_y = labs(target[Y_AXIS] - position[Y_AXIS]);
    block->steps_z = labs(target[Z_AXIS] - position[Z_AXIS]);
    block->steps_e = labs(target[E_AXIS] - position[E_AXIS]);
    block->steps_e *= extrudemultiply;
    block->steps_e /= 100;
    last_e_steps = block->steps_e;
    block->step_event_count = max(block->steps_x,
        max(block->steps_y, max(block->steps_z, block->steps_e)));
    // printf("step x: %d, step y: %d, step z: %d, step e: %d\r\n", block->steps_x, block->steps_y, block->steps_z, block->steps_e);
    //sprintf(temp_msg, "x: %d, y: %d, z: %d, e: %d\r\n", block->steps_x, block->steps_y, block->steps_z, block->steps_e);
    //uart_print(temp_msg);

    // Bail if this is a zero-length block
    if (block->step_event_count <= dropsegments) {
      return;
    };


    // Compute direction bits for this block
    block->direction_bits = 0;
    if (target[X_AXIS] < position[X_AXIS]) {
      block->direction_bits |= (1 << X_AXIS);
    }
    if (target[Y_AXIS] < position[Y_AXIS]) {
      block->direction_bits |= (1 << Y_AXIS);
    }
    if (target[Z_AXIS] < position[Z_AXIS]) {
      block->direction_bits |= (1 << Z_AXIS);
    }
    if (target[E_AXIS] < position[E_AXIS]) {
      block->direction_bits |= (1 << E_AXIS);
      //High Feedrate for retract
      max_E_feedrate_calc = MAX_RETRACT_FEEDRATE;
      retract_feedrate_aktiv = true;
    } else {
      if (retract_feedrate_aktiv) {
        if (block->steps_e > 0)
          retract_feedrate_aktiv = false;
      } else {
        max_E_feedrate_calc = max_feedrate[E_AXIS];
      }
    }

// #ifdef DELAY_ENABLE
//     if(block->steps_x != 0)
//     {
//       enable_x();
//       delayMicroseconds(DELAY_ENABLE);
//     }
//     if(block->steps_y != 0)
//     {
//       enable_y();
//       delayMicroseconds(DELAY_ENABLE);
//     }
//     if(block->steps_z != 0)
//     {
//       enable_z();
//       delayMicroseconds(DELAY_ENABLE);
//     }
//     if(block->steps_e != 0)
//     {
//       enable_e();
//       delayMicroseconds(DELAY_ENABLE);
//     }
// #else
    //enable active axes
    if (block->steps_x != 0)
      enable_x();
    if (block->steps_y != 0)
      enable_y();
    if (block->steps_z != 0)
      enable_z();
    if (block->steps_e != 0)
      enable_e();
// #endif

    if (block->steps_e == 0) {
      if (feed_rate < mintravelfeedrate)
        feed_rate = mintravelfeedrate;
    } else {
      if (feed_rate < minimumfeedrate)
        feed_rate = minimumfeedrate;
    }

    // slow down when the buffer starts to empty, rather than wait at the corner for a buffer refill
    int moves_queued = (block_buffer_head - block_buffer_tail
        + BLOCK_BUFFER_SIZE) & (BLOCK_BUFFER_SIZE - 1);
#ifdef SLOWDOWN  
    if (moves_queued < (BLOCK_BUFFER_SIZE * 0.5) && moves_queued > 1)
      feed_rate = feed_rate * moves_queued / (BLOCK_BUFFER_SIZE * 0.5);
#endif

    float delta_mm[4];
    delta_mm[X_AXIS] = (target[X_AXIS] - position[X_AXIS])
              / axis_steps_per_unit[X_AXIS];
    delta_mm[Y_AXIS] = (target[Y_AXIS] - position[Y_AXIS])
              / axis_steps_per_unit[Y_AXIS];
    delta_mm[Z_AXIS] = (target[Z_AXIS] - position[Z_AXIS])
              / axis_steps_per_unit[Z_AXIS];
    //delta_mm[E_AXIS] = (target[E_AXIS]-position[E_AXIS])/axis_steps_per_unit[E_AXIS];
    delta_mm[E_AXIS] = ((target[E_AXIS] - position[E_AXIS])
        / axis_steps_per_unit[E_AXIS]) * extrudemultiply / 100.0;

    if (block->steps_x <= dropsegments && block->steps_y <= dropsegments
        && block->steps_z <= dropsegments) {
      block->millimeters = fabs(delta_mm[E_AXIS]);
    } else {
      block->millimeters = sqrt(
          (delta_mm[X_AXIS] * delta_mm[X_AXIS]) + (delta_mm[Y_AXIS]*delta_mm[Y_AXIS])
          + (delta_mm[Z_AXIS]*delta_mm[Z_AXIS]));
    }

    float inverse_millimeters = 1.0 / block->millimeters; // Inverse millimeters to remove multiple divides

    // Calculate speed in mm/second for each axis. No divide by zero due to previous checks.
    float inverse_second = feed_rate * inverse_millimeters;

    block->nominal_speed = block->millimeters * inverse_second; // (mm/sec) Always > 0
    block->nominal_rate = ceil(block->step_event_count * inverse_second); // (step/sec) Always > 0

    float extruder_speed = delta_mm[E_AXIS] / block->millimeters;
    block->dispenser_multiplier = extruder_speed / extruder_base_speed;

    
    /*
   //  segment time im micro seconds
   long segment_time = lround(1000000.0/inverse_second);
   if ((blockcount>0) && (blockcount < (BLOCK_BUFFER_SIZE - 4))) {
   if (segment_time<minsegmenttime)  { // buffer is draining, add extra time.  The amount of time added increases if the buffer is still emptied more.
   segment_time=segment_time+lround(2*(minsegmenttime-segment_time)/blockcount);
   }
   }
   else {
   if (segment_time<minsegmenttime) segment_time=minsegmenttime;
   }
   //  END OF SLOW DOWN SECTION
     */

    // Calculate and limit speed in mm/sec for each axis
    float current_speed[4];
    float speed_factor = 1.0; //factor <=1 do decrease speed
    for (int i = 0; i < 3; i++) {
      current_speed[i] = delta_mm[i] * inverse_second;
      if (fabs(current_speed[i]) > max_feedrate[i])
        speed_factor = min(speed_factor,
            max_feedrate[i] / fabs(current_speed[i]));
    }

    current_speed[E_AXIS] = delta_mm[E_AXIS] * inverse_second;
    if (fabs(current_speed[E_AXIS]) > max_E_feedrate_calc)
      speed_factor = min(speed_factor,
          max_E_feedrate_calc / fabs(current_speed[E_AXIS]));

    // Correct the speed
    if (speed_factor < 1.0) {
      for (unsigned char i = 0; i < 4; i++) {
        current_speed[i] *= speed_factor;
      }
      block->nominal_speed *= speed_factor;
      block->nominal_rate *= speed_factor;
    }

    // Compute and limit the acceleration rate for the trapezoid generator.
    float steps_per_mm = block->step_event_count / block->millimeters;
    if (block->steps_x == 0 && block->steps_y == 0 && block->steps_z == 0) {
      block->acceleration_st = ceil(retract_acceleration * steps_per_mm); // convert to: acceleration steps/sec^2
    } else {
      block->acceleration_st = ceil(move_acceleration * steps_per_mm); // convert to: acceleration steps/sec^2
      // Limit acceleration per axis
      if (((float) block->acceleration_st * (float) block->steps_x
          / (float) block->step_event_count)
          > axis_steps_per_sqr_second[X_AXIS])
        block->acceleration_st = axis_steps_per_sqr_second[X_AXIS];
      if (((float) block->acceleration_st * (float) block->steps_y
          / (float) block->step_event_count)
          > axis_steps_per_sqr_second[Y_AXIS])
        block->acceleration_st = axis_steps_per_sqr_second[Y_AXIS];
      if (((float) block->acceleration_st * (float) block->steps_e
          / (float) block->step_event_count)
          > axis_steps_per_sqr_second[E_AXIS])
        block->acceleration_st = axis_steps_per_sqr_second[E_AXIS];
      if (((float) block->acceleration_st * (float) block->steps_z
          / (float) block->step_event_count)
          > axis_steps_per_sqr_second[Z_AXIS])
        block->acceleration_st = axis_steps_per_sqr_second[Z_AXIS];
    }
    block->acceleration = block->acceleration_st / steps_per_mm;
    block->acceleration_rate =
        (long) ((float) block->acceleration_st * 8.388608);

#if 0  // Use old jerk for now
    // Compute path unit vector
    double unit_vec[3];

    unit_vec[X_AXIS] = delta_mm[X_AXIS]*inverse_millimeters;
    unit_vec[Y_AXIS] = delta_mm[Y_AXIS]*inverse_millimeters;
    unit_vec[Z_AXIS] = delta_mm[Z_AXIS]*inverse_millimeters;

    // Compute maximum allowable entry speed at junction by centripetal acceleration approximation.
    // Let a circle be tangent to both previous and current path line segments, where the junction
    // deviation is defined as the distance from the junction to the closest edge of the circle,
    // colinear with the circle center. The circular segment joining the two paths represents the
    // path of centripetal acceleration. Solve for max velocity based on max acceleration about the
    // radius of the circle, defined indirectly by junction deviation. This may be also viewed as
    // path width or max_jerk in the previous grbl version. This approach does not actually deviate
    // from path, but used as a robust way to compute cornering speeds, as it takes into account the
    // nonlinearities of both the junction angle and junction velocity.
    double vmax_junction = MINIMUM_PLANNER_SPEED;// Set default max junction speed

    // Skip first block or when previous_nominal_speed is used as a flag for homing and offset cycles.
    if ((block_buffer_head != block_buffer_tail) && (previous_nominal_speed > 0.0)) {
      // Compute cosine of angle between previous and current path. (prev_unit_vec is negative)
      // NOTE: Max junction velocity is computed without sin() or acos() by trig half angle identity.
      double cos_theta = - previous_unit_vec[X_AXIS] * unit_vec[X_AXIS]
                                    - previous_unit_vec[Y_AXIS] * unit_vec[Y_AXIS]
                                                       - previous_unit_vec[Z_AXIS] * unit_vec[Z_AXIS];

      // Skip and use default max junction speed for 0 degree acute junction.
      if (cos_theta < 0.95) {
        vmax_junction = min(previous_nominal_speed,block->nominal_speed);
        // Skip and avoid divide by zero for straight junctions at 180 degrees. Limit to min() of nominal speeds.
        if (cos_theta > -0.95) {
          // Compute maximum junction velocity based on maximum acceleration and junction deviation
          double sin_theta_d2 = sqrt(0.5*(1.0-cos_theta));// Trig half angle identity. Always positive.
          vmax_junction = min(vmax_junction,
              sqrt(block->acceleration * junction_deviation * sin_theta_d2/(1.0-sin_theta_d2)) );
        }
      }
    }
#endif
    // Start with a safe speed
    float vmax_junction = max_xy_jerk / 2;
    float vmax_junction_factor = 1.0;

    if (fabs(current_speed[Z_AXIS]) > max_z_jerk / 2)
      vmax_junction = min(vmax_junction, max_z_jerk / 2);

    if (fabs(current_speed[E_AXIS]) > max_e_jerk / 2)
      vmax_junction = min(vmax_junction, max_e_jerk / 2);

    if (G92_reset_previous_speed == 1) {
      vmax_junction = 0.1;
      G92_reset_previous_speed = 0;
    }

    vmax_junction = min(vmax_junction, block->nominal_speed);
    float safe_speed = vmax_junction;

    if ((moves_queued > 1) && (previous_nominal_speed > 0.0001)) {
      float jerk = sqrt(
          pow((current_speed[X_AXIS] - previous_speed[X_AXIS]), 2)
          + pow((current_speed[Y_AXIS] - previous_speed[Y_AXIS]),
              2));
      //    if((fabs(previous_speed[X_AXIS]) > 0.0001) || (fabs(previous_speed[Y_AXIS]) > 0.0001)) {
      vmax_junction = block->nominal_speed;
      //    }
      if (jerk > max_xy_jerk) {
        vmax_junction_factor = (max_xy_jerk / jerk);
      }
      if (fabs(current_speed[Z_AXIS] - previous_speed[Z_AXIS]) > max_z_jerk) {
        vmax_junction_factor =
            min(vmax_junction_factor,
                (max_z_jerk/fabs(current_speed[Z_AXIS] - previous_speed[Z_AXIS])));
      }
      if (fabs(current_speed[E_AXIS] - previous_speed[E_AXIS]) > max_e_jerk) {
        vmax_junction_factor =
            min(vmax_junction_factor,
                (max_e_jerk/fabs(current_speed[E_AXIS] - previous_speed[E_AXIS])));
      }
      vmax_junction = min(previous_nominal_speed,
          vmax_junction * vmax_junction_factor); // Limit speed to max previous speed
    }
    block->max_entry_speed = vmax_junction;

    // Initialize block entry speed. Compute based on deceleration to user-defined MINIMUM_PLANNER_SPEED.
    double v_allowable = max_allowable_speed(-block->acceleration,
        MINIMUM_PLANNER_SPEED, block->millimeters);
    block->entry_speed = min(vmax_junction, v_allowable);

    // Initialize planner efficiency flags
    // Set flag if block will always reach maximum junction speed regardless of entry/exit speeds.
    // If a block can de/ac-celerate from nominal speed to zero within the length of the block, then
    // the current block and next block junction speeds are guaranteed to always be at their maximum
    // junction speeds in deceleration and acceleration, respectively. This is due to how the current
    // block nominal speed limits both the current and next maximum junction speeds. Hence, in both
    // the reverse and forward planners, the corresponding block junction speed will always be at the
    // the maximum junction speed and may always be ignored for any speed reduction checks.
    if (block->nominal_speed <= v_allowable) {
      block->nominal_length_flag = true;
    } else {
      block->nominal_length_flag = false;
    }
    block->recalculate_flag = true; // Always calculate trapezoid for new block

    // Update previous path unit_vector and nominal speed
    memcpy(previous_speed, current_speed, sizeof(previous_speed)); // previous_speed[] = current_speed[]
    previous_nominal_speed = block->nominal_speed;

#ifdef ADVANCE
    // Calculate advance rate
    if((block->steps_e == 0) || (block->steps_x == 0 && block->steps_y == 0 && block->steps_z == 0)) {
      block->advance_rate = 0;
      block->advance = 0;
    }
    else {
      long acc_dist = estimate_acceleration_distance(0, block->nominal_rate, block->acceleration_st);
      float advance = (STEPS_PER_CUBIC_MM_E * EXTRUDER_ADVANCE_K) *
          (current_speed[E_AXIS] * current_speed[E_AXIS] * EXTRUTION_AREA * EXTRUTION_AREA)*256;
      block->advance = advance;
      if(acc_dist == 0) {
        block->advance_rate = 0;
      }
      else {
        block->advance_rate = advance / (float)acc_dist;
      }
    }

#endif // ADVANCE


    calculate_trapezoid_for_block(block,
        block->entry_speed / block->nominal_speed,
        safe_speed / block->nominal_speed);


    // Move buffer head
    block_buffer_head = next_buffer_head;

    // Update position
    memcpy(position, target, sizeof(target)); // position[] = target[]

    planner_recalculate();

#ifdef AUTOTEMP
    getHighESpeed();
#endif
    st_wake_up();
  }

  int calc_plannerpuffer_fill(void) {
    int moves_queued = (block_buffer_head - block_buffer_tail
        + BLOCK_BUFFER_SIZE) & (BLOCK_BUFFER_SIZE - 1);
    return (moves_queued);
  }

  void plan_set_position(float x, float y, float z, float e) {
    position[X_AXIS] = lround(x * axis_steps_per_unit[X_AXIS]);
    position[Y_AXIS] = lround(y * axis_steps_per_unit[Y_AXIS]);
    position[Z_AXIS] = lround(z * axis_steps_per_unit[Z_AXIS]);
    position[E_AXIS] = lround(e * axis_steps_per_unit[E_AXIS]);

    virtual_steps_x = 0;
    virtual_steps_y = 0;
    virtual_steps_z = 0;

    previous_nominal_speed = 0.0; // Resets planner junction speeds. Assumes start from rest.
    previous_speed[0] = 0.0;
    previous_speed[1] = 0.0;
    previous_speed[2] = 0.0;
    previous_speed[3] = 0.0;

    G92_reset_previous_speed = 1;
  }

#ifdef AUTOTEMP
  void getHighESpeed()
  {
    static float oldt=0;
    if(!autotemp_enabled)
      return;
    if((target_temp+2) < autotemp_min) //probably temperature set to zero.
      return;//do nothing

    float high=0.0;
    uint8_t block_index = block_buffer_tail;

    while(block_index != block_buffer_head) {
      if((block_buffer[block_index].steps_x != 0) ||
          (block_buffer[block_index].steps_y != 0) ||
          (block_buffer[block_index].steps_z != 0)) {
        float se=(float(block_buffer[block_index].steps_e)/float(block_buffer[block_index].step_event_count))*block_buffer[block_index].nominal_speed;
        //se; units steps/sec;
        if(se>high)
        {
          high=se;
        }
      }
      block_index = (block_index+1) & (BLOCK_BUFFER_SIZE - 1);
    }

    float t=autotemp_min+high*autotemp_factor;

    if(t<autotemp_min)
      t=autotemp_min;

    if(t>autotemp_max)
      t=autotemp_max;

    if(oldt>t)
    {
      t=AUTOTEMP_OLDWEIGHT*oldt+(1-AUTOTEMP_OLDWEIGHT)*t;
    }
    oldt=t;
    autotemp_setpoint = (int)t;

  }
#endif

  // Stepper

  // intRes = intIn1 * intIn2 >> 16
  // uses:
  // r26 to store 0
  // r27 to store the byte 1 of the 24 bit result
#define MultiU16X8toH16(intRes, charIn1, intIn2) \
    asm volatile ( \
        "clr r26 \n\t" \
        "mul %A1, %B2 \n\t" \
        "movw %A0, r0 \n\t" \
        "mul %A1, %A2 \n\t" \
        "add %A0, r1 \n\t" \
        "adc %B0, r26 \n\t" \
        "lsr r0 \n\t" \
        "adc %A0, r26 \n\t" \
        "adc %B0, r26 \n\t" \
        "clr r1 \n\t" \
        : \
          "=&r" (intRes) \
          : \
          "d" (charIn1), \
          "d" (intIn2) \
          : \
            "r26" \
    )

  uint16_t MultiU16X8toH16_emulation(uint8_t charIn1, uint16_t intIn2){
    uint8_t r26 = 0;
    uint8_t a0,b0;    // output
    uint8_t a1,a2,b2; //input
    a1 = charIn1;
    a2 = intIn2;
    b2 = intIn2 >> 8;
    uint16_t r1r0 = (uint16_t)a1*(uint16_t)b2;
    uint8_t r1 = r1r0 >> 8;
    uint8_t r0 = r1r0;
    a0 = r0;
    r1r0 = (int)a1*(int)a2;
    r1 = r1r0 >> 8;
    // r0 = r1r0; //not needed
    a0 = a0 + r1;
    b0 = r26;
    // r0 = r0 >> 1; //not needed
    // a0 = a0 + r26; //this does nothing
    // b0 = b0 + r26; //this does nothing
    // r1 = 0; //not needed
    uint16_t result = (b0<<8) + a0;
    return result;
  }

  // intRes = longIn1 * longIn2 >> 24
  // uses:
  // r26 to store 0
  // r27 to store the byte 1 of the 48bit result
#define MultiU24X24toH16(intRes, longIn1, longIn2) \
    asm volatile ( \
        "clr r26 \n\t" \
        "mul %A1, %B2 \n\t" \
        "mov r27, r1 \n\t" \
        "mul %B1, %C2 \n\t" \
        "movw %A0, r0 \n\t" \
        "mul %C1, %C2 \n\t" \
        "add %B0, r0 \n\t" \
        "mul %C1, %B2 \n\t" \
        "add %A0, r0 \n\t" \
        "adc %B0, r1 \n\t" \
        "mul %A1, %C2 \n\t" \
        "add r27, r0 \n\t" \
        "adc %A0, r1 \n\t" \
        "adc %B0, r26 \n\t" \
        "mul %B1, %B2 \n\t" \
        "add r27, r0 \n\t" \
        "adc %A0, r1 \n\t" \
        "adc %B0, r26 \n\t" \
        "mul %C1, %A2 \n\t" \
        "add r27, r0 \n\t" \
        "adc %A0, r1 \n\t" \
        "adc %B0, r26 \n\t" \
        "mul %B1, %A2 \n\t" \
        "add r27, r1 \n\t" \
        "adc %A0, r26 \n\t" \
        "adc %B0, r26 \n\t" \
        "lsr r27 \n\t" \
        "adc %A0, r26 \n\t" \
        "adc %B0, r26 \n\t" \
        "clr r1 \n\t" \
        : \
          "=&r" (intRes) \
          : \
          "d" (longIn1), \
          "d" (longIn2) \
          : \
            "r26" , "r27" \
    )

  uint16_t MultiU24X24toH16_emulation(uint32_t longIn1, uint32_t longIn2){
    uint8_t a0,b0;    // output 16bit
    uint8_t a1,b1,c1,a2,b2,c2; //input 32bit
    a1 = longIn1;
    b1 = longIn1>>8;
    c1 = longIn1>>16;
    a2 = longIn2;
    b2 = longIn2>>8;
    c2 = longIn2>>16;

    uint8_t r26 = 0;
    uint8_t r27;

    uint16_t r1r0 = (uint16_t)a1*(uint16_t)b2;
    uint8_t r1 = r1r0 >> 8;
    uint8_t r0 = r1r0;

    r27 = r1;

    r1r0 = (uint16_t)b1*(uint16_t)c2;
    r1 = r1r0 >> 8;
    r0 = r1r0;

    a0 = r0;
    b0 = r1;

    r1r0 = (uint16_t)c1*(uint16_t)c2;
    r1 = r1r0 >> 8;
    r0 = r1r0;

    b0 = b0 + r0;

    r1r0 = (uint16_t)c1*(uint16_t)b2;
    r1 = r1r0 >> 8;
    r0 = r1r0;

    a0 = a0 + r0;

    b0 = b0 + r1;

    r1r0 = (uint16_t)a1*(uint16_t)c2;
    r1 = r1r0 >> 8;
    r0 = r1r0;

    r27 = r27 + r0;

    a0 = a0 + r1;

    b0 = b0 + r26;

    r1r0 = (uint16_t)b1*(uint16_t)b2;
    r1 = r1r0 >> 8;
    r0 = r1r0;

    r27 = r27 + r0;

    a0 = a0 + r1;

    b0 = b0 + r26;

    r1r0 = (uint16_t)c1*(uint16_t)a2;
    r1 = r1r0 >> 8;
    r0 = r1r0;

    r27 = r27 + r0;

    a0 = a0 + r1;

    b0 = b0 + r26;

    r1r0 = (uint16_t)b1*(uint16_t)a2;
    r1 = r1r0 >> 8;
    r0 = r1r0;

    r27 = r27 + r1;

    a0 = a0 + r26;

    b0 = b0 + r26;

    r27 = r27 >> 1;

    a0 = a0 + r26;

    b0 = b0 + r26;

    r1 = 0;

    uint16_t result = (b0<<8) + a0;
    return result;
  }



  // Some useful constants

  //#define ENABLE_STEPPER_DRIVER_INTERRUPT()  TIMSK1 |= (1<<OCIE1A)
  //#define DISABLE_STEPPER_DRIVER_INTERRUPT() TIMSK1 &= ~(1<<OCIE1A)

#ifdef ENDSTOPS_ONLY_FOR_HOMING
#define CHECK_ENDSTOPS  if(check_endstops)
#else
#define CHECK_ENDSTOPS
#endif


  //         __________________________
  //        /|                        |\     _________________         ^
  //       / |                        | \   /|               |\        |
  //      /  |                        |  \ / |               | \       s
  //     /   |                        |   |  |               |  \      p
  //    /    |                        |   |  |               |   \     e
  //   +-----+------------------------+---+--+---------------+----+    e
  //   |               BLOCK 1            |      BLOCK 2          |    d
  //
  //                           time ----->
  //
  //  The trapezoid is the shape of the speed curve over time. It starts at block->initial_rate, accelerates
  //  first block->accelerate_until step_events_completed, then keeps going at constant speed until
  //  step_events_completed reaches block->decelerate_after after which it decelerates until the trapezoid generator is reset.
  //  The slope of acceleration is calculated with the leib ramp alghorithm.

  void enable_stepper_driver_interrupt(){

  }

  void st_wake_up() {
    //  TCNT1 = 0;
    if (busy == false)
      enable_stepper_driver_interrupt();
    // ENABLE_STEPPER_DRIVER_INTERRUPT();
  }


  FORCE_INLINE unsigned short calc_timer(unsigned short step_rate) {
    unsigned short timer;
    if (step_rate > MAX_STEP_FREQUENCY)
      step_rate = MAX_STEP_FREQUENCY;

    if (step_rate > 20000) { // If steprate > 20kHz >> step 4 times
      step_rate = (step_rate >> 2) & 0x3fff;
      step_loops = 4;
    } else if (step_rate > 10000) { // If steprate > 10kHz >> step 2 times
      step_rate = (step_rate >> 1) & 0x7fff;
      step_loops = 2;
    } else {
      step_loops = 1;
    }

    if (step_rate < (F_CPU / 500000))
      step_rate = (F_CPU / 500000);
    step_rate -= (F_CPU / 500000); // Correct for minimal speed

    if (step_rate >= (8 * 256)) // higher step rate
    { // higher step rate
      // unsigned short table_address = (uint32_t) &speed_lookuptable_fast[(unsigned char) (step_rate >> 8)][0];
      unsigned char tmp_step_rate = (step_rate & 0x00ff);
      // unsigned short gain = (unsigned short) pgm_read_word_near(table_address + 2);
      unsigned short gain = (unsigned short) speed_lookuptable_fast[(unsigned char) (step_rate  >> 8)][1];
      //  MultiU16X8toH16(timer, tmp_step_rate, gain);
      timer = MultiU16X8toH16_emulation(tmp_step_rate, gain);
      // timer = (unsigned short) pgm_read_word_near(table_address) - timer;
      timer = (unsigned short) speed_lookuptable_fast[(unsigned char) (step_rate  >> 8)][0] - timer;
    } else { // lower step rates
      // unsigned short table_address = (uint32_t) &speed_lookuptable_slow[0][0];
      // table_address += ((step_rate) >> 1) & 0xfffc;
      unsigned short table_index = (((step_rate) >> 1) & 0xfffc) / 4;
      // timer = (unsigned short) pgm_read_word_near(table_address);
      timer = (unsigned short) speed_lookuptable_slow[table_index][0];
      //timer -= (((unsigned short) pgm_read_word_near(table_address + 2) * (unsigned char) (step_rate & 0x0007)) >> 3);
      timer -= (((unsigned short) speed_lookuptable_slow[table_index][1] * (unsigned char) (step_rate & 0x0007)) >> 3);
    }
    if (timer < 100) {
      timer = 100;
    } //(20kHz this should never happen)
    return timer;
  }

  // Initializes the trapezoid generator from the current block. Called whenever a new
  // block begins.
  FORCE_INLINE void trapezoid_generator_reset() {
#ifdef ADVANCE
    advance = current_block->initial_advance;
    final_advance = current_block->final_advance;
    // Do E steps + advance steps
    e_steps += ((advance >>8) - old_advance);
    old_advance = advance >>8;
#endif
    deceleration_time = 0;

    // step_rate to timer interval
    acc_step_rate = current_block->initial_rate;
    acceleration_time = calc_timer(acc_step_rate);
    // OCR1A = acceleration_time;
    XTmrCtr_SetResetValue(&TimerInstancePtr,
        0, //Change with generic value
        acceleration_time*50); //(OCR1A=2000) => 1kHz. 100MHz(FPGA)/2MHz(Mega2560 pre-scaled)=50.
    OCR1A_nominal = calc_timer(current_block->nominal_rate);
  }

  void wait_for_1_8us(){
    /* NOP inserted:So that high time is 1.8us approximately */
    // edited: wait is 1.9us according to the Pololu website.
    // Below is supposed to wait 2.0us
    for(int i=0;i<22;i++){
      asm volatile("nop");
    }
  }


  // "The Stepper Driver Interrupt" - This timer interrupt is the workhorse.
  // It pops blocks from the block_buffer and executes them by pulsing the stepper pins appropriately.
  /*
ISR(TIMER1_COMPA_vect)
{        
  // If there is no current block, attempt to pop one from the buffer
  if (current_block == NULL) {
    // Anything in the buffer?
    current_block = plan_get_current_block();
    if (current_block != NULL) {
      trapezoid_generator_reset();
      counter_x = -(current_block->step_event_count >> 1);
      counter_y = counter_x;
      counter_z = counter_x;
      counter_e = counter_x;
      step_events_completed = 0;
//      #ifdef ADVANCE
//      e_steps = 0;
//      #endif
    } 
    else {
        OCR1A=2000; // 1kHz.
    }    
  } 

  if (current_block != NULL) {
    // Set directions TO DO This should be done once during init of trapezoid. Endstops -> interrupt
    out_bits = current_block->direction_bits;

    // Set direction and check limit switches
    if ((out_bits & (1<<X_AXIS)) != 0) {   // -direction
      WRITE(X_DIR_PIN, INVERT_X_DIR);CHECK_ENDSTOPS
      {
        #if X_MIN_PIN > -1
          bool x_min_endstop=(READ(X_MIN_PIN)!= X_ENDSTOP_INVERT);
          if(x_min_endstop && old_x_min_endstop && (current_block->steps_x > 0)) {
            if(!is_homing)
              endstop_x_hit=true;
            else  
              step_events_completed = current_block->step_event_count;
          }
          else
          {
            endstop_x_hit=false;
          }
          old_x_min_endstop = x_min_endstop;
        #else
          endstop_x_hit=false;
        #endif
      }
    }
    else { // +direction 
      WRITE(X_DIR_PIN, !INVERT_X_DIR);CHECK_ENDSTOPS
      {
        #if X_MAX_PIN > -1
          bool x_max_endstop=(READ(X_MAX_PIN) != X_ENDSTOP_INVERT);
          if(x_max_endstop && old_x_max_endstop && (current_block->steps_x > 0)){
            if(!is_homing)
              endstop_x_hit=true;
            else    
              step_events_completed = current_block->step_event_count;
          }
          else
          {
            endstop_x_hit=false;
          }
          old_x_max_endstop = x_max_endstop;
        #else
          endstop_x_hit=false;
        #endif
      }
    }

    if ((out_bits & (1<<Y_AXIS)) != 0) {   // -direction
      WRITE(
    Y_DIR_PIN, INVERT_Y_DIR);CHECK_ENDSTOPS
      {
        #if Y_MIN_PIN > -1
          bool y_min_endstop=(READ(Y_MIN_PIN)!= Y_ENDSTOP_INVERT);
          if(y_min_endstop && old_y_min_endstop && (current_block->steps_y > 0)) {
            if(!is_homing)
              endstop_y_hit=true;
            else
              step_events_completed = current_block->step_event_count;
          }
          else
          {
            endstop_y_hit=false;
          }
          old_y_min_endstop = y_min_endstop;
        #else
          endstop_y_hit=false;  
        #endif
      }
    }
    else { // +direction
      WRITE(Y_DIR_PIN, !INVERT_Y_DIR);CHECK_ENDSTOPS
      {
        #if Y_MAX_PIN > -1
          bool y_max_endstop=(READ(Y_MAX_PIN) != Y_ENDSTOP_INVERT);
          if(y_max_endstop && old_y_max_endstop && (current_block->steps_y > 0)){
            if(!is_homing)
              endstop_y_hit=true;
            else  
              step_events_completed = current_block->step_event_count;
          }
          else
          {
            endstop_y_hit=false;
          }
          old_y_max_endstop = y_max_endstop;
        #else
          endstop_y_hit=false;  
        #endif
      }
    }

    if ((out_bits & (1<<Z_AXIS)) != 0) {   // -direction
      WRITE(
    Z_DIR_PIN, INVERT_Z_DIR);CHECK_ENDSTOPS
      {
        #if Z_MIN_PIN > -1
          bool z_min_endstop=(READ(Z_MIN_PIN)!= Z_ENDSTOP_INVERT);
          if(z_min_endstop && old_z_min_endstop && (current_block->steps_z > 0)) {
            if(!is_homing)  
              endstop_z_hit=true;
            else  
              step_events_completed = current_block->step_event_count;
          }
          else
          {
            endstop_z_hit=false;
          }
          old_z_min_endstop = z_min_endstop;
        #else
          endstop_z_hit=false;  
        #endif
      }
    }
    else { // +direction
      WRITE(Z_DIR_PIN, !INVERT_Z_DIR);CHECK_ENDSTOPS
      {
        #if Z_MAX_PIN > -1
          bool z_max_endstop=(READ(Z_MAX_PIN) != Z_ENDSTOP_INVERT);
          if(z_max_endstop && old_z_max_endstop && (current_block->steps_z > 0)) {
            if(!is_homing)
              endstop_z_hit=true;
            else  
              step_events_completed = current_block->step_event_count;
          }
          else
          {
            endstop_z_hit=false;
          }
          old_z_max_endstop = z_max_endstop;
        #else
          endstop_z_hit=false;  
        #endif
      }
    }

    #ifndef ADVANCE
      if ((out_bits & (1<<E_AXIS)) != 0) {  // -direction
        WRITE(
    E_DIR_PIN, INVERT_E_DIR);}
      else { // +direction
        WRITE(E_DIR_PIN, !INVERT_E_DIR);}
    #endif //!ADVANCE



    for(int8_t i=0; i < step_loops; i++) { // Take multiple steps per interrupt (For high speed moves) 

      #ifdef ADVANCE
      counter_e += current_block->steps_e;
      if (counter_e > 0) {
        counter_e -= current_block->step_event_count;
        if ((out_bits & (1<<E_AXIS)) != 0) { // - direction
          e_steps--;
        }
        else {
          e_steps++;
        }
      }    
      #endif //ADVANCE


      counter_x += current_block->steps_x;
      if (counter_x > 0) {
        if(!endstop_x_hit)
        {
          if(virtual_steps_x)
            virtual_steps_x--;
          else
            WRITE(
    X_STEP_PIN, HIGH);}
        else
          virtual_steps_x++;

        counter_x -= current_block->step_event_count;
        WRITE(X_STEP_PIN, LOW);}

      counter_y += current_block->steps_y;
      if (counter_y > 0) {
        if(!endstop_y_hit)
        {
          if(virtual_steps_y)
            virtual_steps_y--;
          else
            WRITE(Y_STEP_PIN, HIGH);}
        else
          virtual_steps_y++;

        counter_y -= current_block->step_event_count;
        WRITE(
    Y_STEP_PIN, LOW);}

      counter_z += current_block->steps_z;
      if (counter_z > 0) {
        if(!endstop_z_hit)
        {
          if(virtual_steps_z)
            virtual_steps_z--;
          else
            WRITE(Z_STEP_PIN, HIGH);}
        else
          virtual_steps_z++;

        counter_z -= current_block->step_event_count;
        WRITE(Z_STEP_PIN, LOW);}

      #ifndef ADVANCE
        counter_e += current_block->steps_e;
        if (counter_e > 0) {
          WRITE(
    E_STEP_PIN, HIGH);counter_e -= current_block->step_event_count;
          WRITE(E_STEP_PIN, LOW);}
      #endif //!ADVANCE

      step_events_completed += 1;  
      if(step_events_completed >= current_block->step_event_count) break;

    }
    // Calculare new timer value
    unsigned short timer;
    unsigned short step_rate;
    if (step_events_completed <= (unsigned long int)current_block->accelerate_until) {

      MultiU24X24toH16(acc_step_rate,
    acceleration_time, current_block->acceleration_rate);acc_step_rate += current_block->initial_rate;

      // upper limit
      if(acc_step_rate > current_block->nominal_rate)
        acc_step_rate = current_block->nominal_rate;

      // step_rate to timer interval
      timer = calc_timer(acc_step_rate);
      OCR1A = timer;
      acceleration_time += timer;
      #ifdef ADVANCE
        for(int8_t i=0; i < step_loops; i++) {
          advance += advance_rate;
        }
        //if(advance > current_block->advance) advance = current_block->advance;
        // Do E steps + advance steps
        e_steps += ((advance >>8) - old_advance);
        old_advance = advance >>8;  

      #endif
    } 
    else if (step_events_completed > (unsigned long int)current_block->decelerate_after) {   
      MultiU24X24toH16(
    step_rate, deceleration_time, current_block->acceleration_rate);

if(step_rate > acc_step_rate) { // Check step_rate stays positive
  step_rate = current_block->final_rate;
}
else {
  step_rate = acc_step_rate - step_rate; // Decelerate from aceleration end point.
}

// lower limit
if(step_rate < current_block->final_rate)
step_rate = current_block->final_rate;

// step_rate to timer interval
timer = calc_timer(step_rate);
OCR1A = timer;
deceleration_time += timer;
#ifdef ADVANCE
for(int8_t i=0; i < step_loops; i++) {
  advance -= advance_rate;
}
if(advance < final_advance) advance = final_advance;
// Do E steps + advance steps
e_steps += ((advance >>8) - old_advance);
old_advance = advance >>8;
#endif //ADVANCE
}
else {
OCR1A = OCR1A_nominal;
}

 // If current block is finished, reset pointer
if (step_events_completed >= current_block->step_event_count) {
current_block = NULL;
plan_discard_current_block();
}
}
}
   */

#ifdef ADVANCE

  unsigned char old_OCR0A;
  // Timer interrupt for E. e_steps is set in the main routine;
  // Timer 0 is shared with millies
  /*
ISR(TIMER0_COMPA_vect)
{
old_OCR0A += 52; // ~10kHz interrupt (250000 / 26 = 9615kHz)
OCR0A = old_OCR0A;
// Set E direction (Depends on E direction + advance)
for(unsigned char i=0; i<4;i++)
{
if (e_steps != 0)
{
WRITE(E0_STEP_PIN, LOW);
if (e_steps < 0) {
  WRITE(E0_DIR_PIN, INVERT_E0_DIR);
  e_steps++;
  WRITE(E0_STEP_PIN, HIGH);
}
else if (e_steps > 0) {
  WRITE(E0_DIR_PIN, !INVERT_E0_DIR);
  e_steps--;
  WRITE(E0_STEP_PIN, HIGH);
}
}
}
}
   */
#endif // ADVANCE


  void Timer_OVF_vect(){
  #ifdef PID_SOFT_PWM
    if(g_heater_pwm_val >= 2)
    {
      //      WRITE(HEATER_0_PIN,HIGH);
      //XGpio_DiscreteWrite(&HeaterInst, 1, 0x01);
      _SET(ck_shields_data , HTR0_PIN);
      XGpio_DiscreteWrite(&CK_ShieldInst, 1, ck_shields_data);
      if(g_heater_pwm_val <= 253){
        //        OCR2A = g_heater_pwm_val;
        XTmrCtr_SetResetValue(&TimerInstancePtr2,
            1, //Change with generic value
            PULSES_500HZ*g_heater_pwm_val/256);
      }else{
        //        OCR2A = 192;
        XTmrCtr_SetResetValue(&TimerInstancePtr2,
            1, //Change with generic value
            PULSES_500HZ*192/256);
      }
    }
    else
    {
      //      WRITE(HEATER_0_PIN,LOW);
      _CLR(ck_shields_data , HTR0_PIN);
      XGpio_DiscreteWrite(&CK_ShieldInst, 1, ck_shields_data);
      //XGpio_DiscreteWrite(&HeaterInst, 1, 0x00);
      //      OCR2A = 192;
      XTmrCtr_SetResetValue(&TimerInstancePtr2,
          1, //Change with generic value
          PULSES_500HZ*192/256);
    }
#endif
  }

  void Timer_COMP_vect(){
    if(g_heater_pwm_val > 253)
       {
    //     WRITE(HEATER_0_PIN,HIGH);
      _SET(ck_shields_data , HTR0_PIN);
      XGpio_DiscreteWrite(&CK_ShieldInst, 1, ck_shields_data);
      //  XGpio_DiscreteWrite(&HeaterInst, 1, 0x01);
       }
       else
       {
    //     WRITE(HEATER_0_PIN,LOW);
        _CLR(ck_shields_data , HTR0_PIN);
        XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);
        //XGpio_DiscreteWrite(&HeaterInst, 1, 0x00);
       }
  }

//  XScuGic InterruptController; /* Instance of the Interrupt Controller */
//  static XScuGic_Config *GicConfig;/* The configuration parameters of the controller */
  XIntc IntcInstancePtr;

  void Timer_InterruptHandler(void *data, u8 TmrCtrNumber)
  {
    // If there is no current block, attempt to pop one from the buffer
    if (current_block == NULL) {
      // Anything in the buffer?
      current_block = plan_get_current_block();
      if (current_block != NULL) {
        trapezoid_generator_reset();
        counter_x = -(current_block->step_event_count >> 1);
        counter_y = counter_x;
        counter_z = counter_x;
        counter_e = counter_x;
        step_events_completed = 0;
              #ifdef ADVANCE
              e_steps = 0;
              #endif
      }
      else {
        // OCR1A=2000; // 1kHz.
        XTmrCtr_SetResetValue(&TimerInstancePtr,
            0, //Change with generic value
            2000*50); //1kHz
        //0xf8000000);
      }
    }

    if (current_block != NULL) {
      // Set directions TO DO This should be done once during init of trapezoid. Endstops -> interrupt
      out_bits = current_block->direction_bits;

      // Set direction and check limit switches
      if ((out_bits & (1<<X_AXIS)) != 0) {   // -direction
        if(INVERT_X_DIR){
          _SET(shields_data , DIR_X_PIN);
          XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);
        } else {
          _CLR(shields_data , DIR_X_PIN);
          XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);
        }
        // WRITE(X_DIR_PIN, INVERT_X_DIR);
        CHECK_ENDSTOPS
        {
#if X_MIN_PIN > -1
          // bool x_min_endstop=(READ(X_MIN_PIN)!= X_ENDSTOP_INVERT);
          int x_min_pin_read = _CHK(XGpio_DiscreteRead(&ShieldInst, 1), X_MIN_PIN);
          bool x_min_endstop= (x_min_pin_read != X_ENDSTOP_INVERT);
          if(x_min_endstop && old_x_min_endstop && (current_block->steps_x > 0)) {
            if(!is_homing)
              endstop_x_hit=true;
            else
              step_events_completed = current_block->step_event_count;
          }
          else
          {
            endstop_x_hit=false;
          }
          old_x_min_endstop = x_min_endstop;
#else
          endstop_x_hit=false;
#endif
        }
      }
      else { // +direction
        if(INVERT_X_DIR){
          _CLR(shields_data , DIR_X_PIN);
          XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);
        } else {
          _SET(shields_data , DIR_X_PIN);
          XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);
        }
        // WRITE(X_DIR_PIN, !INVERT_X_DIR);
        CHECK_ENDSTOPS
        {
#if X_MAX_PIN > -1
          int x_max_pin_read = _CHK(XGpio_DiscreteRead(&ShieldInst, 1), X_MAX_PIN);
          //x_max_endstop : false if LOW, true if HIGH
          bool x_max_endstop=( x_max_pin_read != X_ENDSTOP_INVERT);
          // bool x_max_endstop=(READ(X_MAX_PIN) != X_ENDSTOP_INVERT);
          if(x_max_endstop && old_x_max_endstop && (current_block->steps_x > 0)){
            // xil_printf("x max endstop hit!\r\n");
            if(!is_homing){
              endstop_x_hit=true;
            }
            else {
              step_events_completed = current_block->step_event_count;
            }
          }
          else
          {
            endstop_x_hit=false;
          }
          old_x_max_endstop = x_max_endstop;
#else
          endstop_x_hit=false;
#endif
        }
      }

      if ((out_bits & (1<<Y_AXIS)) != 0) {   // -direction
        if(INVERT_Y_DIR){
          _SET(shields_data , DIR_Y_PIN);
          XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);
        } else {
          _CLR(shields_data , DIR_Y_PIN);
          XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);
        }
        // WRITE(Y_DIR_PIN, INVERT_Y_DIR);
        CHECK_ENDSTOPS
        {
#if Y_MIN_PIN > -1
          int y_min_pin_read = _CHK(XGpio_DiscreteRead(&ShieldInst, 1), Y_MIN_PIN);
          bool y_min_endstop=( y_min_pin_read != Y_ENDSTOP_INVERT);
          // bool y_min_endstop=(READ(Y_MIN_PIN)!= Y_ENDSTOP_INVERT);
          if(y_min_endstop && old_y_min_endstop && (current_block->steps_y > 0)) {
            if(!is_homing)
              endstop_y_hit=true;
            else
              step_events_completed = current_block->step_event_count;
          }
          else
          {
            endstop_y_hit=false;
          }
          old_y_min_endstop = y_min_endstop;
#else
          endstop_y_hit=false;
#endif
        }
      }
      else { // +direction
        if(INVERT_Y_DIR){
          _CLR(shields_data , DIR_Y_PIN);
          XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);
        } else {
          _SET(shields_data , DIR_Y_PIN);
          XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);
        }
        // WRITE(Y_DIR_PIN, !INVERT_Y_DIR);
        CHECK_ENDSTOPS
        {
#if Y_MAX_PIN > -1
          int y_max_pin_read = _CHK(XGpio_DiscreteRead(&ShieldInst, 1), Y_MAX_PIN);
          bool y_max_endstop=( y_max_pin_read != Y_ENDSTOP_INVERT);
          // bool y_max_endstop=(READ(Y_MAX_PIN) != Y_ENDSTOP_INVERT);
          // bool y_max_endstop=(READ(Y_MAX_PIN) != Y_ENDSTOP_INVERT);
          if(y_max_endstop && old_y_max_endstop && (current_block->steps_y > 0)){
            // xil_printf("y max endstop hit!\r\n");
            if(!is_homing)
              endstop_y_hit=true;
            else
              step_events_completed = current_block->step_event_count;
          }
          else
          {
            endstop_y_hit=false;
          }
          old_y_max_endstop = y_max_endstop;
#else
          endstop_y_hit=false;
#endif
        }
      }

      if ((out_bits & (1<<Z_AXIS)) != 0) {   // -direction
        if(INVERT_Z_DIR){
          _SET(shields_data , DIR_Z_PIN);
          XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);
        } else {
          _CLR(shields_data , DIR_Z_PIN);
          XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);
        }
        // WRITE(Z_DIR_PIN, INVERT_Z_DIR);
        CHECK_ENDSTOPS
        {
#if Z_MIN_PIN > -1
          int z_min_pin_read = _CHK(XGpio_DiscreteRead(&ShieldInst, 1), Z_MIN_PIN);
          bool z_min_endstop=( z_min_pin_read != Z_ENDSTOP_INVERT);
          // bool z_min_endstop=(READ(Z_MIN_PIN)!= Z_ENDSTOP_INVERT);
          if(z_min_endstop && old_z_min_endstop && (current_block->steps_z > 0)) {
            if(!is_homing)
              endstop_z_hit=true;
            else
              step_events_completed = current_block->step_event_count;
          }
          else
          {
            endstop_z_hit=false;
          }
          old_z_min_endstop = z_min_endstop;
#else
          endstop_z_hit=false;
#endif
        }
      }
      else { // +direction
        if(INVERT_Z_DIR){
          _CLR(shields_data , DIR_Z_PIN);
          XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);
        } else {
          _SET(shields_data , DIR_Z_PIN);
          XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);
        }
        // WRITE(Z_DIR_PIN, !INVERT_Z_DIR);
        CHECK_ENDSTOPS
        {
#if Z_MAX_PIN > -1
          int z_max_pin_read = _CHK(XGpio_DiscreteRead(&ShieldInst, 1), Z_MAX_PIN);
          bool z_max_endstop=( z_max_pin_read != Z_ENDSTOP_INVERT);
          // bool z_max_endstop=(READ(Z_MAX_PIN) != Z_ENDSTOP_INVERT);
          if(z_max_endstop && old_z_max_endstop && (current_block->steps_z > 0)) {
            // xil_printf("z max endstop hit!\r\n");
            if(!is_homing)
              endstop_z_hit=true;
            else
              step_events_completed = current_block->step_event_count;
          }
          else
          {
            endstop_z_hit=false;
          }
          old_z_max_endstop = z_max_endstop;
#else
          endstop_z_hit=false;
#endif
        }
      }

#ifndef ADVANCE
          if ((out_bits & (1<<E_AXIS)) != 0) {  // -direction
            //WRITE(E_DIR_PIN, INVERT_E_DIR);}
            _CLR(shields_data , DIR_E_PIN);
            XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);
          }else { // +direction
              //WRITE(E_DIR_PIN, !INVERT_E_DIR);}
            _SET(shields_data , DIR_E_PIN);
            XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);
          }
#endif //!ADVANCE

      for(int8_t i=0; i < step_loops; i++) { // Take multiple steps per interrupt (For high speed moves)

#ifdef ADVANCE
        counter_e += current_block->steps_e;
        if (counter_e > 0) {
          counter_e -= current_block->step_event_count;
          if ((out_bits & (1<<E_AXIS)) != 0) { // - direction
            e_steps--;
          }
          else {
            e_steps++;
          }
        }
#endif //ADVANCE


        counter_x += current_block->steps_x;
        if (counter_x > 0) {
          if(!endstop_x_hit)
          {
            if(virtual_steps_x)
              virtual_steps_x--;
            else {
              // WRITE(X_STEP_PIN, HIGH);}
              _SET(shields_data , STEP_X_PIN);
              XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);}
            //hack for stepper motor driver
            wait_for_1_8us();
          } else
            virtual_steps_x++;

          counter_x -= current_block->step_event_count;
          // WRITE(X_STEP_PIN, LOW);
          _CLR(shields_data , STEP_X_PIN);
          XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);
        }

        counter_y += current_block->steps_y;
        if (counter_y > 0) {
          if(!endstop_y_hit)
          {
            if(virtual_steps_y)
              virtual_steps_y--;
            else {
              // WRITE(Y_STEP_PIN, HIGH);}
              _SET(shields_data , STEP_Y_PIN);
              XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);}
            wait_for_1_8us();
          }
          else
            virtual_steps_y++;

          counter_y -= current_block->step_event_count;
          // WRITE(Y_STEP_PIN, LOW);
          _CLR(shields_data , STEP_Y_PIN);
          XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);
        }

        counter_z += current_block->steps_z;
        if (counter_z > 0) {
          if(!endstop_z_hit)
          {
            if(virtual_steps_z)
              virtual_steps_z--;
            else {
              // WRITE(Z_STEP_PIN, HIGH);}
              _SET(shields_data , STEP_Z_PIN);
              XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);}
            wait_for_1_8us();
          }
          else
            virtual_steps_z++;

          counter_z -= current_block->step_event_count;
          // WRITE(Z_STEP_PIN, LOW);
          _CLR(shields_data , STEP_Z_PIN);
          XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);
        }

#ifndef ADVANCE
        counter_e += current_block->steps_e;
        if (counter_e > 0) {

         if(current_tool_number==1){
            e_pulse_counter++; // counter for dispenser and UV LED

            if(is_first_block_element){
              if(!cold_extrusion){
                uv_led_on();
                dispenser_on();
              }
              is_dispenser_running = true;
              current_dispenser_pressure = ceil(DISPENSER_BASE_PRESSURE * current_block->dispenser_multiplier);
              if(current_dispenser_pressure>7500) current_dispenser_pressure = 7500;
              if(current_dispenser_pressure<200) current_dispenser_pressure = 200;

              is_first_block_element = false;
            }
         } 

         if(current_tool_number==0){
            // WRITE(E_STEP_PIN, HIGH);
            _SET(shields_data , STEP_E_PIN);
            XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);

            wait_for_1_8us();
          }

          counter_e -= current_block->step_event_count;
          if(current_tool_number==0){
            // WRITE(E_STEP_PIN, LOW);
            _CLR(shields_data , STEP_E_PIN);
            XGpio_DiscreteWrite(&ShieldInst, 1, shields_data);
          }
        }
#endif //!ADVANCE

        step_events_completed += 1;
        if(step_events_completed >= current_block->step_event_count) break;

      }
      // Calculare new timer value
      unsigned short timer;
      unsigned short step_rate;
      //while acceleration
      if (step_events_completed <= (unsigned long int)current_block->accelerate_until) {

        // MultiU24X24toH16(acc_step_rate, acceleration_time, current_block->acceleration_rate);
        acc_step_rate = MultiU24X24toH16_emulation(acceleration_time, current_block->acceleration_rate);
        // below not working
        // acc_step_rate = (acceleration_time * current_block->acceleration_rate) >> 24;
        acc_step_rate += current_block->initial_rate;

        // upper limit
        if(acc_step_rate > current_block->nominal_rate)
          acc_step_rate = current_block->nominal_rate;

        // step_rate to timer interval
        timer = calc_timer(acc_step_rate);
        // OCR1A = timer;
        XTmrCtr_SetResetValue(&TimerInstancePtr,
            0, //Change with generic value
            timer*50); //(OCR1A=2000) => 1kHz. 100MHz(FPGA)/2MHz(Mega2560 pre-scaled)=50.
        acceleration_time += timer;
#ifdef ADVANCE
        for(int8_t i=0; i < step_loops; i++) {
          advance += advance_rate;
        }
        //if(advance > current_block->advance) advance = current_block->advance;
        // Do E steps + advance steps
        e_steps += ((advance >>8) - old_advance);
        old_advance = advance >>8;

#endif
      }
      //while deceleration
      else if (step_events_completed > (unsigned long int)current_block->decelerate_after) {
        // MultiU24X24toH16(step_rate, deceleration_time, current_block->acceleration_rate);
        step_rate = MultiU24X24toH16_emulation(deceleration_time, current_block->acceleration_rate);
        // below not working
        // step_rate = (deceleration_time * current_block->acceleration_rate) >> 24;

        if(step_rate > acc_step_rate) { // Check step_rate stays positive
          step_rate = current_block->final_rate;
        }
        else {
          step_rate = acc_step_rate - step_rate; // Decelerate from aceleration end point.
        }

        // lower limit
        if(step_rate < current_block->final_rate)
          step_rate = current_block->final_rate;

        // step_rate to timer interval
        timer = calc_timer(step_rate);
        // OCR1A = timer;
        XTmrCtr_SetResetValue(&TimerInstancePtr,
            0, //Change with generic value
            timer*50); //(OCR1A=2000) => 1kHz. 100MHz(FPGA)/2MHz(Mega2560 pre-scaled)=50.
        deceleration_time += timer;

#ifdef ADVANCE
        for(int8_t i=0; i < step_loops; i++) {
          advance -= advance_rate;
        }
        if(advance < final_advance)
          advance = final_advance;
        // Do E steps + advance steps
        e_steps += ((advance >>8) - old_advance);
        old_advance = advance >>8;
#endif //ADVANCE

      }
      //while plateau slew rate
      else {
        //OCR1A = OCR1A_nominal;
        XTmrCtr_SetResetValue(&TimerInstancePtr,
            0, //Change with generic value
            OCR1A_nominal*50);
      }

      // If current block is finished, reset pointer
      if (step_events_completed >= current_block->step_event_count) {
        current_block = NULL;
        plan_discard_current_block();

        is_first_block_element = true;
        is_dispenser_running = false;
        previous_discarded_millis = millis();
        //free buffer
        int buflen_accum_finished = MAILBOX_DATA(BUFLEN_ACCUM_FINISHED_DATA_ADDR); 
        MAILBOX_DATA(BUFLEN_ACCUM_FINISHED_DATA_ADDR) = buflen_accum_finished + 1;
      }
    }
  }

  // 500Hz
  void Timer_InterruptHandler2(void *data, u8 TmrCtrNumber)
  {
    //500Hz cycle; this is used for the heater management
    //xil_printf("Interrupt acknowledged.\r\n");
    u32 reg0 = Xil_In32(TimerInstancePtr2.BaseAddress + XTmrCtr_Offsets[0] + XTC_TCSR_OFFSET);
    u32 reg1 = Xil_In32(TimerInstancePtr2.BaseAddress + XTmrCtr_Offsets[1] + XTC_TCSR_OFFSET);
    if(_CHK(reg0,8)){
      //printf("PWM ON\r\n");
      Timer_OVF_vect();
    } else if(_CHK(reg1,8)){
      //printf("PWM OFF\r\n");
      Timer_COMP_vect();
    }
  }

  //20Hz
  void Timer_InterruptHandler3(void *data, u8 TmrCtrNumber)
  {

    //check heater every n milliseconds
      manage_heater(SysMonInstPtr);
      manage_inactivity(1);
    #if (MINIMUM_FAN_START_SPEED > 0)
      manage_fan_start_speed();
    #endif

    //20Hz cycle; this is used as main loop (which is parallel to main) 
  	if(is_homing){
           #ifdef DELTA
  	 if(block_buffer_head==block_buffer_tail){ //pulse generation completed (alternative of st_synchronize)
	 int min_pin, max_pin, home_dir, max_length, home_bump_mm;
  	 float axis_homing_feedrate_mm_m;

  	   if(homing_status==0){ //X_AXIS 1st
         	endstop_x_hit = false;
                endstop_y_hit = false;
                endstop_z_hit = false;
                current_position[X_AXIS] = current_position[Y_AXIS] = current_position[Z_AXIS] = 0.0;

                //Read X_AXIS settings
                min_pin = X_MIN_PIN;
                max_pin = X_MAX_PIN;
                home_dir = X_HOME_DIR;
                max_length = Z_MAX_LENGTH;
                home_bump_mm = X_HOME_BUMP_MM;
                axis_homing_feedrate_mm_m = homing_feedrate_mm_m[Z_AXIS];

                // Set the axis position as setup for the move
                current_position[X_AXIS] = 0;
                sync_plan_position();
                // Move towards the endstop until an endstop is triggered
                line_to_axis_pos(X_AXIS, 1.5 * max_length * home_dir);
                homing_status++;
            } else if(homing_status==1) {  //X_AXIS 2nd
                //Read X_AXIS settings
                min_pin = X_MIN_PIN;
                max_pin = X_MAX_PIN;
                home_dir = X_HOME_DIR;
                max_length = Z_MAX_LENGTH;
                home_bump_mm = X_HOME_BUMP_MM;
                axis_homing_feedrate_mm_m = homing_feedrate_mm_m[Z_AXIS];
	
            	current_position[X_AXIS] = 0;
		sync_plan_position();
  		// Move away from the endstop by the axis HOME_BUMP_MM
  		line_to_axis_pos(X_AXIS, -home_bump_mm * home_dir);
                homing_status++;
            } else if(homing_status==2) { //X_AXIS 3rd
                //Read X_AXIS settings
                min_pin = X_MIN_PIN;
                max_pin = X_MAX_PIN;
                home_dir = X_HOME_DIR;
                max_length = Z_MAX_LENGTH;
                home_bump_mm = X_HOME_BUMP_MM;
                axis_homing_feedrate_mm_m = homing_feedrate_mm_m[Z_AXIS];
	
            	// Move slowly towards the endstop until triggered
  		line_to_axis_pos(X_AXIS, 2 * home_bump_mm * home_dir, axis_homing_feedrate_mm_m / 10);
  		// reset current_position to 0 to reflect hitting endpoint
  		current_position[X_AXIS] = 0;
  		sync_plan_position();
  		// not necessary
  		//set_axis_is_at_home(axis);
  		//sync_plan_position_delta();
  		sync_plan_position();
  		destination[X_AXIS] = current_position[X_AXIS];
    		endstop_x_hit = false; // clear endstop hit flags
                homing_status++;
            } else if(homing_status==3){ // Y_AXIS 1st
                //Read Y_AXIS settings
            	min_pin = Y_MIN_PIN;
		max_pin = Y_MAX_PIN;
    		home_dir = Y_HOME_DIR;
    		max_length = Z_MAX_LENGTH;
    		home_bump_mm = Y_HOME_BUMP_MM;
    		axis_homing_feedrate_mm_m = homing_feedrate_mm_m[Z_AXIS];

    		// Set the axis position as setup for the move
                current_position[Y_AXIS] = 0;
                sync_plan_position();
                // Move towards the endstop until an endstop is triggered
                line_to_axis_pos(Y_AXIS, 1.5 * max_length * home_dir);
                homing_status++;
            } else if(homing_status==4) { // Y_AXIS 2nd
            	//Read Y_AXIS settings
            	min_pin = Y_MIN_PIN;
		max_pin = Y_MAX_PIN;
    		home_dir = Y_HOME_DIR;
    		max_length = Z_MAX_LENGTH;
    		home_bump_mm = Y_HOME_BUMP_MM;
    		axis_homing_feedrate_mm_m = homing_feedrate_mm_m[Z_AXIS];

            	current_position[Y_AXIS] = 0;
		sync_plan_position();
  		// Move away from the endstop by the axis HOME_BUMP_MM
  		line_to_axis_pos(Y_AXIS, -home_bump_mm * home_dir);
                homing_status++;
            } else if(homing_status==5) { // Y_AXIS 3rd
            	//Read Y_AXIS settings
            	min_pin = Y_MIN_PIN;
		max_pin = Y_MAX_PIN;
    		home_dir = Y_HOME_DIR;
    		max_length = Z_MAX_LENGTH;
    		home_bump_mm = Y_HOME_BUMP_MM;
    		axis_homing_feedrate_mm_m = homing_feedrate_mm_m[Z_AXIS];

    		// Move slowly towards the endstop until triggered
  		line_to_axis_pos(Y_AXIS, 2 * home_bump_mm * home_dir, axis_homing_feedrate_mm_m / 10);
  		// reset current_position to 0 to reflect hitting endpoint
  		current_position[Y_AXIS] = 0;
  		sync_plan_position();
  		// not necessary
  		//set_axis_is_at_home(axis);
  		//sync_plan_position_delta();
  		sync_plan_position();
  		destination[Y_AXIS] = current_position[Y_AXIS];
    		endstop_y_hit = false; // clear endstop hit flags
                homing_status++;
            } else if(homing_status==6){ // Z_AXIS 1st
            	//Read Z_AXIS settings
            	min_pin = Z_MIN_PIN;
		max_pin = Z_MAX_PIN;
    		home_dir = Z_HOME_DIR;
    		max_length = Z_MAX_LENGTH;
    		home_bump_mm = Z_HOME_BUMP_MM;
    		axis_homing_feedrate_mm_m = homing_feedrate_mm_m[Z_AXIS];

    		// Set the axis position as setup for the move
                current_position[Z_AXIS] = 0;
                sync_plan_position();
                // Move towards the endstop until an endstop is triggered
                line_to_axis_pos(Z_AXIS, 1.5 * max_length * home_dir);

                homing_status++;
            } else if(homing_status==7){ // Z_AXIS 2nd 
            	//Read Z_AXIS settings
            	min_pin = Z_MIN_PIN;
		max_pin = Z_MAX_PIN;
    		home_dir = Z_HOME_DIR;
    		max_length = Z_MAX_LENGTH;
    		home_bump_mm = Z_HOME_BUMP_MM;
    		axis_homing_feedrate_mm_m = homing_feedrate_mm_m[Z_AXIS];

    		current_position[Z_AXIS] = 0;
		sync_plan_position();
  		// Move away from the endstop by the axis HOME_BUMP_MM
  		line_to_axis_pos(Z_AXIS, -home_bump_mm * home_dir);

                homing_status++;
            } else if(homing_status==8){ // Z_AXIS 3rd
            	//Read Z_AXIS settings
            	min_pin = Z_MIN_PIN;
		max_pin = Z_MAX_PIN;
    		home_dir = Z_HOME_DIR;
    		max_length = Z_MAX_LENGTH;
    		home_bump_mm = Z_HOME_BUMP_MM;
    		axis_homing_feedrate_mm_m = homing_feedrate_mm_m[Z_AXIS];

    		// Move slowly towards the endstop until triggered
  		line_to_axis_pos(Z_AXIS, 2 * home_bump_mm * home_dir, axis_homing_feedrate_mm_m / 10);
  		// reset current_position to 0 to reflect hitting endpoint
  		current_position[Z_AXIS] = 0;
  		sync_plan_position();
  		// not necessary
  		//set_axis_is_at_home(axis);
  		//sync_plan_position_delta();
  		sync_plan_position();
  		destination[Z_AXIS] = current_position[Z_AXIS];
    		endstop_z_hit = false; // clear endstop hit flags

                homing_status++;
            } else if(homing_status==9){
                // sync_plan_position_delta();
                sync_plan_position();
                // set the current position as top
                // trigger sync_plan_position_delta() to exclude orthogonal coordinates
                current_position[X_AXIS] = current_position[Y_AXIS] = 0.0;
                current_position[Z_AXIS] = Z_MAX_POS;
                destination[X_AXIS] = current_position[X_AXIS];
                destination[Y_AXIS] = current_position[Y_AXIS];
                destination[Z_AXIS] = current_position[Z_AXIS];
                destination[E_AXIS] = current_position[E_AXIS];
                inverse_kinematics(destination);
                sync_plan_position_delta();

                // step back to avoid the switch
                destination[X_AXIS] = current_position[X_AXIS];
                destination[Y_AXIS] = current_position[Y_AXIS];
                destination[Z_AXIS] = current_position[Z_AXIS] - Z_HOME_BUMP_MM;
                destination[E_AXIS] = current_position[E_AXIS];
                inverse_kinematics(destination);
                plan_buffer_line(delta[X_AXIS], delta[Y_AXIS],delta[Z_AXIS], destination[E_AXIS], feedrate_mm_m);
                for (int i = 0; i < NUM_AXIS; i++) {
                  current_position[i] = destination[i];
                }
                sync_plan_position_delta();

          #ifdef ENDSTOPS_ONLY_FOR_HOMING
                enable_endstops(false);
          #endif

                is_homing = false;
                feedrate = saved_feedrate;
                feedmultiply = saved_feedmultiply;

                previous_millis_cmd = millis();

            	homing_status = 0;
            	clear_to_send = true;
            	MAILBOX_CMD_ADDR = 0x0;
                uart_print(ok_msg);  
            }
          }
        // Gantry type
        #else
  	    if(block_buffer_head==block_buffer_tail){ //pulse generation completed (alternative of st_synchronize)
		int min_pin, max_pin, home_dir, max_length, home_bump_mm;
  		float axis_homing_feedrate_mm_m;

  	    if(home_x_axis){ //X_AXIS 1st
              //Read X_AXIS settings
              min_pin = X_MIN_PIN;
              max_pin = X_MAX_PIN;
              home_dir = X_HOME_DIR;
              max_length = X_MAX_LENGTH;
              home_bump_mm = X_HOME_BUMP_MM;
              axis_homing_feedrate_mm_m = homing_feedrate_mm_m[X_AXIS];

              if(homing_status==0){
         	endstop_x_hit = false;
                current_position[X_AXIS] = current_position[Y_AXIS] = current_position[Z_AXIS] = 0.0;

                // Set the axis position as setup for the move
                current_position[X_AXIS] = 0;
                sync_plan_position();
                // Move towards the endstop until an endstop is triggered
                line_to_axis_pos(X_AXIS, 1.5 * max_length * home_dir);
                homing_status++;
              } else if(homing_status==1) {  //X_AXIS 2nd
              	current_position[X_AXIS] = 0;
	          sync_plan_position();
  	          // Move away from the endstop by the axis HOME_BUMP_MM
  	          line_to_axis_pos(X_AXIS, -home_bump_mm * home_dir);
                  homing_status++;
              } else if(homing_status==2) { //X_AXIS 3rd
              	// Move slowly towards the endstop until triggered
  	          line_to_axis_pos(X_AXIS, 2 * home_bump_mm * home_dir, axis_homing_feedrate_mm_m / 10);
  	          // reset current_position to 0 to reflect hitting endpoint
  	          current_position[X_AXIS] = 0;
  	          sync_plan_position();
  	          // not necessary
  	          //set_axis_is_at_home(axis);
  	          //sync_plan_position_delta();
  	          //sync_plan_position();
  	          destination[X_AXIS] = current_position[X_AXIS];
    	          endstop_x_hit = false; // clear endstop hit flags
                  homing_status = 0;
                  home_x_axis = false;
              } 
           } else if (home_y_axis) { // Y_AXIS 1st
              //Read Y_AXIS settings
              min_pin = Y_MIN_PIN;
	      max_pin = Y_MAX_PIN;
    	      home_dir = Y_HOME_DIR;
    	      max_length = Y_MAX_LENGTH;
    	      home_bump_mm = Y_HOME_BUMP_MM;
    	      axis_homing_feedrate_mm_m = homing_feedrate_mm_m[Y_AXIS];

              if(homing_status==0){
                endstop_y_hit = false;
    		// Set the axis position as setup for the move
                current_position[Y_AXIS] = 0;
                sync_plan_position();
                // Move towards the endstop until an endstop is triggered
                line_to_axis_pos(Y_AXIS, 1.5 * max_length * home_dir);
                homing_status++;
            } else if(homing_status==1) { // Y_AXIS 2nd
            	current_position[Y_AXIS] = 0;
		sync_plan_position();
  		// Move away from the endstop by the axis HOME_BUMP_MM
  		line_to_axis_pos(Y_AXIS, -home_bump_mm * home_dir);
                homing_status++;
            } else if(homing_status==2) { // Y_AXIS 3rd
    		// Move slowly towards the endstop until triggered
  		line_to_axis_pos(Y_AXIS, 2 * home_bump_mm * home_dir, axis_homing_feedrate_mm_m / 10);
  		// reset current_position to 0 to reflect hitting endpoint
  		current_position[Y_AXIS] = 0;
  		sync_plan_position();
  		// not necessary
  		//set_axis_is_at_home(axis);
  		//sync_plan_position_delta();
  		//sync_plan_position();
  		destination[Y_AXIS] = current_position[Y_AXIS];
    		endstop_y_hit = false; // clear endstop hit flags
                homing_status = 0;
                home_y_axis = false;
              }
            } else if (home_z_axis) {
               //Read Z_AXIS settings
               min_pin = Z_MIN_PIN;
               max_pin = Z_MAX_PIN;
               home_dir = Z_HOME_DIR;
               max_length = Z_MAX_LENGTH;
               home_bump_mm = Z_HOME_BUMP_MM;
               axis_homing_feedrate_mm_m = homing_feedrate_mm_m[Z_AXIS];

              if(homing_status==0) { // Z_AXIS 1st
    		// Set the axis position as setup for the move
                current_position[Z_AXIS] = 0;
                sync_plan_position();
                // Move towards the endstop until an endstop is triggered
                line_to_axis_pos(Z_AXIS, 1.5 * max_length * home_dir);
                homing_status++;
              } else if(homing_status==1){ // Z_AXIS 2nd 
    	        current_position[Z_AXIS] = 0;
	        sync_plan_position();
  	        // Move away from the endstop by the axis HOME_BUMP_MM
  	        line_to_axis_pos(Z_AXIS, -home_bump_mm * home_dir);

                homing_status++;
              } else if(homing_status==2) { // Z_AXIS 3rd
    	        // Move slowly towards the endstop until triggered
  	        line_to_axis_pos(Z_AXIS, 2 * home_bump_mm * home_dir, axis_homing_feedrate_mm_m / 10);
  	        // reset current_position to 0 to reflect hitting endpoint
  	        current_position[Z_AXIS] = 0;
  	        sync_plan_position();
  	        // not necessary
  	        //set_axis_is_at_home(axis);
  	        //sync_plan_position_delta();
  	        //sync_plan_position();
  	        destination[Z_AXIS] = current_position[Z_AXIS];
    	        endstop_z_hit = false; // clear endstop hit flags

                homing_status = 0;
                home_z_axis = false;
              }
            } else  { //all axis homed
                // sync_plan_position_delta();
                //sync_plan_position();
                // set the current position as top
                // trigger sync_plan_position_delta() to exclude orthogonal coordinates
                current_position[X_AXIS] = current_position[Y_AXIS] = current_position[Z_AXIS] = 0.0;
                destination[X_AXIS] = current_position[X_AXIS];
                destination[Y_AXIS] = current_position[Y_AXIS];
                destination[Z_AXIS] = current_position[Z_AXIS];
                destination[E_AXIS] = current_position[E_AXIS];
                sync_plan_position();

          #ifdef ENDSTOPS_ONLY_FOR_HOMING
                enable_endstops(false);
          #endif

                is_homing = false;
                feedrate = saved_feedrate;
                feedmultiply = saved_feedmultiply;

                previous_millis_cmd = millis();

            	homing_status = 0;
            	clear_to_send = true;
            	MAILBOX_CMD_ADDR = 0x0;
                uart_print(ok_msg);  
            }
          }
        #endif
  	} else if(waiting_until_setpoint) {
          if ((millis() - codenum_heater) > 1000) { //Print Temp Reading every 1 second while heating up/cooling down
             bool still_waiting = target_direction_heating ? (current_raw < target_raw) : (current_raw > target_raw);
             if(still_waiting) {
               //showString(PSTR("T:"));
               //Serial.println( analog2temp(current_raw) );
               // printf("T:%d\r\n",analog2temp(current_raw));
                codenum_heater = millis();
             } else {
             	 waiting_until_setpoint = false;
             	 clear_to_send = true;
                   MAILBOX_CMD_ADDR = 0x0;	
                   uart_print(ok_msg);  
             }
          }
       }

    int next_buffer_head_temp = block_buffer_head + 1;
    if (next_buffer_head_temp == BLOCK_BUFFER_SIZE) {
      next_buffer_head_temp = 0;
    }
    MAILBOX_DATA(NEXT_BUFFER_HEAD_ADDR) = next_buffer_head_temp;

  if(!is_homing){
    if (block_buffer_tail != next_buffer_head_temp)  {
       if(buflen) {
     // #ifdef SDSUPPORT
     //     if(savetosd)
     //     {
     //       if(strstr(cmdbuffer[bufindr],"M29") == NULL)
     //       {
     //         write_command(cmdbuffer[bufindr]);
     //         // printf("ok\r\n");
     //         //            //showString(PSTR("ok\r\n"));
     //       }
     //       else
     //       {
     //         file.sync();
     //         file.close();
     //         savetosd = false;
     //         // printf("Done saving file.\r\n");
     //         //            //showString(PSTR("Done saving file.\r\n"));
     //       }
     //     }
     //     else
     //     {
     //       process_commands();
     //     }
     // #else
         process_commands();
     // #endif
     
         buflen = (buflen - 1);
         MAILBOX_DATA(BUFLEN_DATA_ADDR) = buflen;
         bufindr++;
         if (bufindr == BUFSIZE)
           bufindr = 0;
       }
    }
  }
  
    //update current temperature
    MAILBOX_DATA_FLOAT(HOTEND_TEMP_ADDR) =  analog2temp(current_raw);
    // MAILBOX_DATA(HOTEND_TEMP_RAW_ADDR) =  current_raw2;
    // MAILBOX_DATA_FLOAT(X_DATA_ADDR) = current_position[0];
    // MAILBOX_DATA_FLOAT(Y_DATA_ADDR) = current_position[1];
    // MAILBOX_DATA_FLOAT(Z_DATA_ADDR) = current_position[2];
    // MAILBOX_DATA_FLOAT(E_DATA_ADDR) = current_position[3];





  }

  // Handler for dispenser and UV LED control
  // 1Hz 
  void Timer_InterruptHandler4(void *data, u8 TmrCtrNumber){
    if(!cold_extrusion){
      if(last_dispenser_pressure != current_dispenser_pressure){
      	if(use_dispenser){
          update_dispenser_pressure(1, current_dispenser_pressure);
      	}
        last_dispenser_pressure = current_dispenser_pressure;
       }
    }
  }

//20Hz
  void Timer_InterruptHandler5(void *data, u8 TmrCtrNumber){

        if(!is_dispenser_running &&  (millis() - previous_discarded_millis > 100 ) && e_pulse_counter==0){
          uv_led_off();
          dispenser_off();
        } 
        e_pulse_counter = 0;

    // Wait until any of the conditions satisfied
    if( MAILBOX_CMD_ADDR!=0 && clear_to_send){
       if(MAILBOX_CMD_ADDR!=0){
           u32 cmd = MAILBOX_CMD_ADDR;
           if (cmd==PRINT_STRING) {
             clear_to_send = false;
             get_command_mailbox();

             comment_mode = false; //this is required!!

             for(int i=0;i<current_command_length;i++){
               parse_command(current_command[i],i);
             }

           }
        }
     }
  }


  void initSteppers(){
    //no use of enable pin
    //  _CLR(shields_data , X_EN_PIN);
    // XGpio_SetDataDirection(&ShieldInst, 1, 0x00);

  }

  void initializeXADC(){
    SysMonConfigPtr = XSysMon_LookupConfig(SYSMON_DEVICE_ID);
    if(SysMonConfigPtr == NULL) {
      // printf("XsysMon LookupConfig FAILED\n\r");
    }
    int xStatus;
    xStatus = XSysMon_CfgInitialize(SysMonInstPtr, SysMonConfigPtr,SysMonConfigPtr->BaseAddress);
    if(XST_SUCCESS != xStatus) {
      // printf("XsysMon CfgInitialize FAILED\r\n");
    }
    XSysMon_GetStatus(SysMonInstPtr); // Clear the old status
  }

  void initializeGPIO(){
    int xStatus;
    shields_data = 0;
    xStatus = XGpio_Initialize(&ShieldInst, SHIELDS_DEVICE_ID);
    if (xStatus != XST_SUCCESS)
      // return XST_FAILURE;
      return;
//    xStatus = XGpio_Initialize(&HeaterInst, HEATER_DEVICE_ID);
//    if (xStatus != XST_SUCCESS)
//      // return XST_FAILURE;
//      return;
    // Set LEDs direction to outputs
//    XGpio_SetDataDirection(&HeaterInst, 1, 0x00);

    u32 shield_dir = 0x00;
    #ifdef DELTA
      _SET(shield_dir, X_MAX_PIN);
      _SET(shield_dir, Y_MAX_PIN);
      _SET(shield_dir, Z_MAX_PIN);
    #else
      _SET(shield_dir, X_MIN_PIN);
      _SET(shield_dir, Y_MIN_PIN);
      _SET(shield_dir, Z_MIN_PIN);
    #endif
    _SET(shield_dir, THERMISTOR_PIN);
    // printf("shield_dir: %d\r\n",shield_dir);
    XGpio_SetDataDirection(&ShieldInst, 1, shield_dir);
    //All are initialized as low
    // printf("<a>shield value: %d\r\n",XGpio_DiscreteRead(&ShieldInst, 1));
    XGpio_DiscreteWrite(&ShieldInst, 1, 0x00);
    // printf("<b>shield value: %d\r\n",XGpio_DiscreteRead(&ShieldInst, 1));
    //XGpio_SetDataDirection(&ShieldInst, 1, 0x00);
  }

  void initializeGPIO_ChipKit(){
    int xStatus;
    ck_shields_data = 0;
    xStatus = XGpio_Initialize(&CK_ShieldInst, CK_SHIELDS_DEVICE_ID);
    if (xStatus != XST_SUCCESS){
      //do nothing
      uart_print("UART1 error\r\n"); 
    }
    u32 shield_dir = 0x00; // all pins are output
    XGpio_SetDataDirection(&CK_ShieldInst, 1, shield_dir); 
    XGpio_DiscreteWrite(&CK_ShieldInst, 1, 0x00);

  }

#define XTC_CSR_ENABLE_PWM_MASK   0x00000200
#define XTC_CSR_EXT_GENERATE_MASK 0x00000004
  void initializeAxiTimer(){

    //print("##### Timer initialization start. #####\n\r");
    //print("\r\n");
    //**************************************************************************//
    // Timer1 init (Stepper motor pulse generation)
    //**************************************************************************//
    int xStatus = XTmrCtr_Initialize(&TimerInstancePtr,XPAR_TMRCTR_0_DEVICE_ID);
    if(XST_SUCCESS != xStatus){
      // print("TIMER INIT FAILED \n\r");
      // SeeedOled.setTextXY(0,0);SeeedOled.putString("tmr1 ng");
    }

    XTmrCtr_SetHandler(&TimerInstancePtr,
        Timer_InterruptHandler,
        &TimerInstancePtr);

    XTmrCtr_SetResetValue(&TimerInstancePtr,
        0, //Channel 0
        2000*50); //(OCR1A=2000) => 1kHz. 100MHz(FPGA)/2MHz(Mega2560 pre-scaled)=50.
        // 0x5f5e100); //1Hz -> for debug
    //      //Therefore, (2000*50) => 1kHz on AXI timer. Whatever times 50 makes AXI timer value.
    //      //0xf8000000);

    XTmrCtr_SetOptions(&TimerInstancePtr,
        0, //Channel 0
        (XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION | XTC_DOWN_COUNT_OPTION));//Timer1 is DOWN counter

    //**************************************************************************//
    // Timer2 init (Heater PWM control)
    //**************************************************************************//
    xStatus = XTmrCtr_Initialize(&TimerInstancePtr2,XPAR_TMRCTR_1_DEVICE_ID);
    if(XST_SUCCESS != xStatus){
      // SeeedOled.setTextXY(0,0);SeeedOled.putString("tmr2 ng");
      // print("TIMER INIT2 FAILED \n\r");
    }

    XTmrCtr_SetHandler(&TimerInstancePtr2,
        Timer_InterruptHandler2,
        &TimerInstancePtr2);

    //enable interrupt & auto reload
    XTmrCtr_SetOptions(&TimerInstancePtr2,
        0,
        (XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION | XTC_DOWN_COUNT_OPTION));

    u32 CounterControlReg = Xil_In32(TimerInstancePtr2.BaseAddress + XTmrCtr_Offsets[0] + XTC_TCSR_OFFSET);
    CounterControlReg = CounterControlReg | XTC_CSR_ENABLE_PWM_MASK | XTC_CSR_EXT_GENERATE_MASK;
    Xil_Out32(TimerInstancePtr2.BaseAddress + XTmrCtr_Offsets[0] + XTC_TCSR_OFFSET, CounterControlReg);

    XTmrCtr_SetOptions(&TimerInstancePtr2,
        1,
        (XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION | XTC_DOWN_COUNT_OPTION));

    CounterControlReg = Xil_In32(TimerInstancePtr2.BaseAddress + XTmrCtr_Offsets[1] + XTC_TCSR_OFFSET);
    CounterControlReg = CounterControlReg | XTC_CSR_ENABLE_PWM_MASK | XTC_CSR_EXT_GENERATE_MASK;
    Xil_Out32(TimerInstancePtr2.BaseAddress + XTmrCtr_Offsets[1] + XTC_TCSR_OFFSET, CounterControlReg);

    XTmrCtr_SetResetValue(&TimerInstancePtr2,
        0, //Channel 0
        0x30d40);//500Hz
    XTmrCtr_SetResetValue(&TimerInstancePtr2,
        1, //Channel 1
        0x0);

    //**************************************************************************//
    // Timer3 init (Command parser: Quasi main loop)
    //**************************************************************************//

    xStatus = XTmrCtr_Initialize(&TimerInstancePtr3,XPAR_TMRCTR_2_DEVICE_ID);
    if(XST_SUCCESS != xStatus){
      // SeeedOled.setTextXY(0,0);SeeedOled.putString("tmr3 ng");
      // print("TIMER3 INIT FAILED \n\r");
    }

    XTmrCtr_SetHandler(&TimerInstancePtr3,
        Timer_InterruptHandler3,
        &TimerInstancePtr3);

    XTmrCtr_SetResetValue(&TimerInstancePtr3,
        0, //Channel 0
        // 0x64);//1MHz
        // 0x186a0); //1kHz
        0x4c4b40); //20Hz 
        // 0x2faf080); //2Hz
        // 0x5f5e100); //1Hz -> for debug
        //0x1312d00);//5Hz

    XTmrCtr_SetOptions(&TimerInstancePtr3,
        0, //Channel 0
        (XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION | XTC_DOWN_COUNT_OPTION));//Timer3 is DOWN counter

    //**************************************************************************//
    // Timer4 (Diepenser and UV LED control)
    //**************************************************************************//

     xStatus = XTmrCtr_Initialize(&TimerInstancePtr4,XPAR_TMRCTR_3_DEVICE_ID);
     if(XST_SUCCESS != xStatus){
    // SeeedOled.setTextXY(0,0);SeeedOled.putString("tmr4 ng");
    // print("TIMER3 INIT FAILED \n\r");
     }

     XTmrCtr_SetHandler(&TimerInstancePtr4,
         Timer_InterruptHandler4,
         &TimerInstancePtr4);

     XTmrCtr_SetResetValue(&TimerInstancePtr4,
         0, //Channel 0
         // 0x1dcd6500); //0.2Hz
         // 0x4c4b40); //20Hz 
         0x5f5e100); //1Hz -> for debug
         //  0x1312d00);//5Hz

     XTmrCtr_SetOptions(&TimerInstancePtr4,
         0, //Channel 0
         (XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION | XTC_DOWN_COUNT_OPTION));//Timer4 is DOWN counter

    //**************************************************************************//
    // Timer5 (BRAM communication)
    //**************************************************************************//

     xStatus = XTmrCtr_Initialize(&TimerInstancePtr5,XPAR_TMRCTR_4_DEVICE_ID);
     if(XST_SUCCESS != xStatus){
    // SeeedOled.setTextXY(0,0);SeeedOled.putString("tmr4 ng");
    // print("TIMER3 INIT FAILED \n\r");
     }

     XTmrCtr_SetHandler(&TimerInstancePtr5,
         Timer_InterruptHandler5,
         &TimerInstancePtr5);

     XTmrCtr_SetResetValue(&TimerInstancePtr5,
         0, //Channel 0
         // 0x1dcd6500); //0.2Hz
         0x4c4b40); //20Hz 
         // 0x5f5e100); //1Hz -> for debug
         //  0x1312d00);//5Hz

     XTmrCtr_SetOptions(&TimerInstancePtr5,
         0, //Channel 0
         (XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION | XTC_DOWN_COUNT_OPTION));//Timer5 is DOWN counter

    //**************************************************************************//
    // Connect INTC and Timers 
    //**************************************************************************//

    xStatus = XIntc_Initialize(&IntcInstancePtr, XPAR_INTC_0_DEVICE_ID);
    if (xStatus != XST_SUCCESS){
      // SeeedOled.setTextXY(0,0);SeeedOled.putString("initc");
      // print("intc init error\n\r");
    }

    xStatus = XIntc_Connect(&IntcInstancePtr, XPAR_INTC_0_TMRCTR_0_VEC_ID,
        (XInterruptHandler)XTmrCtr_InterruptHandler,
        (void*)&TimerInstancePtr);
    if (xStatus != XST_SUCCESS){
      // SeeedOled.setTextXY(0,0);SeeedOled.putString("tmr0c");
      // print("connect timer0 error\n\r");
    }
    xStatus = XIntc_Connect(&IntcInstancePtr, XPAR_INTC_0_TMRCTR_1_VEC_ID,
        (XInterruptHandler)XTmrCtr_InterruptHandler,
        (void*)&TimerInstancePtr2);
    if (xStatus != XST_SUCCESS){
      // SeeedOled.setTextXY(0,0);SeeedOled.putString("tmr1c");
      // print("connect timer1 error\n\r");
    }
    xStatus = XIntc_Connect(&IntcInstancePtr, XPAR_INTC_0_TMRCTR_2_VEC_ID,
        (XInterruptHandler)XTmrCtr_InterruptHandler,
        (void*)&TimerInstancePtr3);
    if (xStatus != XST_SUCCESS){
      // SeeedOled.setTextXY(0,0);SeeedOled.putString("tmr3c");
      // print("connect timer3 error\n\r");
    }
     xStatus = XIntc_Connect(&IntcInstancePtr, XPAR_INTC_0_TMRCTR_3_VEC_ID,
         (XInterruptHandler)XTmrCtr_InterruptHandler,
         (void*)&TimerInstancePtr4);
     if (xStatus != XST_SUCCESS){
       // SeeedOled.setTextXY(0,0);SeeedOled.putString("tmr4c");
       // print("connect timer3 error\n\r");
     }
     xStatus = XIntc_Connect(&IntcInstancePtr, XPAR_INTC_0_TMRCTR_4_VEC_ID,
         (XInterruptHandler)XTmrCtr_InterruptHandler,
         (void*)&TimerInstancePtr5);
     if (xStatus != XST_SUCCESS){
       // SeeedOled.setTextXY(0,0);SeeedOled.putString("tmr4c");
       // print("connect timer3 error\n\r");
     }
    xStatus = XIntc_Start(&IntcInstancePtr, XIN_REAL_MODE);
    if (xStatus != XST_SUCCESS){
      // SeeedOled.setTextXY(0,0);SeeedOled.putString("intc");
      // print("intc start error\n\r");
    }
    XIntc_Enable(&IntcInstancePtr, XPAR_INTC_0_TMRCTR_0_VEC_ID);
    XIntc_Enable(&IntcInstancePtr, XPAR_INTC_0_TMRCTR_1_VEC_ID);
    XIntc_Enable(&IntcInstancePtr, XPAR_INTC_0_TMRCTR_2_VEC_ID);
    XIntc_Enable(&IntcInstancePtr, XPAR_INTC_0_TMRCTR_3_VEC_ID);
    XIntc_Enable(&IntcInstancePtr, XPAR_INTC_0_TMRCTR_4_VEC_ID);
    microblaze_enable_interrupts();

    XTmrCtr_Start(&TimerInstancePtr,0);
    // print("AXI timer1 start \n\r");
    XTmrCtr_Start(&TimerInstancePtr2,0);
    // print("AXI timer2 - timer0 start \n\r");
    XTmrCtr_Start(&TimerInstancePtr2,1);
    // print("AXI timer2 - timer1 start \n\r");
    XTmrCtr_Start(&TimerInstancePtr3,0);
    // print("AXI timer3 start \n\r");
    XTmrCtr_Start(&TimerInstancePtr4,0);
    // print("AXI timer4 start \n\r");
    XTmrCtr_Start(&TimerInstancePtr5,0);
    // print("AXI timer5 start \n\r");
  }

  void st_init() {


    // waveform generation = 0100 = CTC
    //TCCR1B &= ~(1 << WGM13);
    //TCCR1B |= (1 << WGM12);
    //TCCR1A &= ~(1 << WGM11);
    //TCCR1A &= ~(1 << WGM10);

    // output mode = 00 (disconnected)
    //TCCR1A &= ~(3 << COM1A0);
    //TCCR1A &= ~(3 << COM1B0);

    // Set the timer pre-scaler
    // Generally we use a divider of 8, resulting in a 2MHz timer
    // frequency on a 16MHz MCU. If you are going to change this, be
    // sure to regenerate speed_lookuptable.h with
    // create_speed_lookuptable.py
    //TCCR1B = (TCCR1B & ~(0x07 << CS10)) | (2 << CS10); // 2MHz timer

    //OCR1A = 0x4000;
    //TCNT1 = 0;
    //ENABLE_STEPPER_DRIVER_INTERRUPT();

#ifdef ADVANCE
#if defined(TCCR0A) && defined(WGM01)
    //TCCR0A &= ~(1<<WGM01);
    //TCCR0A &= ~(1<<WGM00);
#endif
    e_steps = 0;
    //TIMSK0 |= (1<<OCIE0A);
#endif //ADVANCE

#ifdef ENDSTOPS_ONLY_FOR_HOMING
    enable_endstops(false);
#else
    enable_endstops(true);
#endif

    //sei();
  }

  // Block until all buffered steps are executed
  void st_synchronize() {
    while (blocks_queued()) {
    // while (block_buffer_head != block_buffer_tail){
    // manage_heater();
//       manage_heater(SysMonInstPtr);
//       manage_inactivity(1);
// #if (MINIMUM_FAN_START_SPEED > 0)
//       manage_fan_start_speed();
// #endif
    }
  }

#ifdef DEBUG
  void log_message(char* message) {
    //Serial.print("DEBUG"); //Serial.println(message);
  }

  void log_bool(char* message, bool value) {
    //Serial.print("DEBUG"); Serial.print(message); Serial.print(": "); //Serial.println(value);
  }

  void log_int(char* message, int value) {
    //Serial.print("DEBUG"); Serial.print(message); Serial.print(": "); //Serial.println(value);
  }

  void log_long(char* message, long value) {
    //Serial.print("DEBUG"); Serial.print(message); Serial.print(": "); //Serial.println(value);
  }

  void log_float(char* message, float value) {
    //Serial.print("DEBUG"); Serial.print(message); Serial.print(": "); //Serial.println(value);
  }

  void log_uint(char* message, unsigned int value) {
    //Serial.print("DEBUG"); Serial.print(message); Serial.print(": "); //Serial.println(value);
  }

  void log_ulong(char* message, unsigned long value) {
    //Serial.print("DEBUG"); Serial.print(message); Serial.print(": "); //Serial.println(value);
  }

  void log_int_array(char* message, int value[], int array_lenght) {
    //Serial.print("DEBUG"); Serial.print(message); Serial.print(": {");
    for(int i=0; i < array_lenght; i++) {
      //Serial.print(value[i]);
      if(i != array_lenght-1) Serial.print(", ");
    }
    //Serial.println("}");
  }

  void log_long_array(char* message, long value[], int array_lenght) {
    //Serial.print("DEBUG"); Serial.print(message); Serial.print(": {");
    for(int i=0; i < array_lenght; i++) {
      //Serial.print(value[i]);
      if(i != array_lenght-1) Serial.print(", ");
    }
    //Serial.println("}");
  }

  void log_float_array(char* message, float value[], int array_lenght) {
    //Serial.print("DEBUG"); Serial.print(message); Serial.print(": {");
    for(int i=0; i < array_lenght; i++) {
      Serial.print(value[i]);
      if(i != array_lenght-1) Serial.print(", ");
    }
    //Serial.println("}");
  }

    //Serial.print("DEBUG "); Serial.print(message); Serial.print(": {");
    for(int i=0; i < array_lenght; i++) {
      //Serial.print(value[i]);
      if(i != array_lenght-1) Serial.print(", ");
    }
    //Serial.println("}");
  }

  void log_ulong_array(char* message, unsigned long value[], int array_lenght) {
    //Serial.print("DEBUG"); Serial.print(message); Serial.print(": {");
    for(int i=0; i < array_lenght; i++) {
      //Serial.print(value[i]);
      if(i != array_lenght-1) Serial.print(", ");
    }
    //Serial.println("}");
  }
#endif


