//
// Created by Angel Dust on 18/02/2017.
//

#ifndef UTILS_H
#define UTILS_H

#include "stm32f4xx.h"
#include <stdio.h>

#define ENABLE_LOGGER 1

#ifndef constrain
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#endif

#define delayUS_ASM(us)                                                                                                                                        \
    do {                                                                                                                                                       \
        asm volatile("MOV R0,%[loops]\n\t"                                                                                                                     \
                     "1: \n\t"                                                                                                                                 \
                     "SUB R0, #1\n\t"                                                                                                                          \
                     "CMP R0, #0\n\t"                                                                                                                          \
                     "BNE 1b \n\t"                                                                                                                             \
                     :                                                                                                                                         \
                     : [loops] "r"(16 * us)                                                                                                                    \
                     : "memory");                                                                                                                              \
    } while (0)

unsigned long millis();

unsigned long micros();

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// DAC Pins
#define CS_PIN 10
#define SCK_PIN 13
#define DIN_PIN 11
#define SCK2_PIN 2
#define DIN2_PIN 4
#define DAC_MIN 0
#define DAC_MAX 4096
#define DAC_BITS 10
#define ADC_BITS 10

#define min2(a, b) ((a) < (b) ? (a) : (b))
#define max2(a, b) ((a) > (b) ? (a) : (b))

// static double PRECISION = 0.00000000000001;
// static int MAX_NUMBER_STRING_SIZE = 32;

typedef float float32_t;

void enableTimers();

void disableTimers();

char *format_long(int64_t x, char *buf);

void format_long(int64_t x, char *buf, uint8_t length, char thou_separator = '.', int max_length = 20);

int strcicmp(char const *a, char const *b);

void removePunct(char *str);

void removeChars(char *str, const char *chars);

float format_eng(char *dest, float value, const char *units, char *new_units);

float format_eng(char *dest, float value, const char *units, char *new_units, uint8_t dec_places, bool trailing_zero);

float fasterlog2(float);

void extract_file_and_path(const char *fileandpath, char *path, char *file, size_t size);

float fasterlog(float);

float fastpow2(const float val);

void trim(char *);

int endsWith(const char *str, const char *suffix);

void print_vector_f32(float *v, uint16_t len);

void print_vector_complex_f32(float *v, uint16_t len);

void min_max_f32(float *v, uint16_t size, float *min, float *max);

void min_max_f32(float *v, uint16_t size, float *min, float *max, float discard);

char *ftoa(char *dest, size_t size, double val, int dec);

bool parse_int(const char *str, int &result);

#if ENABLE_LOGGER

#define LOG_MAX_ITEMS 40
#define MAX_LOGGERS 1

extern volatile uint8_t logEventIndex;
extern bool scopeLog;
extern volatile bool eventLogEnabled;
enum LogEventType { ADC_READ, PID_CALC, SCOPE_LOOP };

typedef struct {

    uint8_t type;
    uint32_t time;
    uint32_t elapsed;
    bool end;
    float data;
} logevent_st_t;

/*
typedef void (*logger_t)(const char*,bool);

extern int num_loggers;

int addLogger(logger_t logger);

// Log a message with a variable number of arguments
void logMessage(int, ...);

// Log a message with a variable number of pairs char * label, float value, ...
void logMessageValues(int num_args, ...);

// Log a label and it's value
void logValue(const char *,float value);

extern logevent_st_t logEvents[LOG_MAX_ITEMS];
*/

void logEvent(uint8_t type, float value, uint8_t end);

void logEvent(uint8_t type, float value);

void printLog(logevent_st_t *logEventsClone);

void printLog();

#endif // ENABLE_LOGGER

uint16_t mod(int a, int b);

int mostSignificantDecimal(long i);

int analogMedian(int pin, int n);

float roundDownToNearest(float d, float t);

int splitString(char *str, char **parts, int length, char separator);

void printMemory();

float mapFloat(float, float, float, float, float);

char *format_double(double v, char *dest, char decimal_separator = '.', char thousand_separator = ' ', uint8_t frac_digits = 8, bool trailing_zero = true,
                    uint8_t max_length = 20);

int readVcc();

float truncate_float(float v, int decimals);

float adc_to_mv(int adc_value, float adc_vref, int adc_max);

float mv_to_adc(int millivolts, float adc_vref, int adc_max);

/**
 * Double to ASCII
 */
char *dtoa(char *s, double n);

#endif // UTILS_H
