//
// Created by Angel Dust on 18/02/2017.
//

#ifndef UTILS_H
#define UTILS_H

#include "stm32f4xx.h"
#include <stdio.h>
#include "printf.h"

#define ENABLE_LOGGER 1

#ifndef constrain
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#endif

#define STR_IN(str, ...)                                                                                                                                       \
    ({                                                                                                                                                         \
        const char *targets[] = {__VA_ARGS__, NULL};                                                                                                           \
        bool found = false;                                                                                                                                    \
        for (int i = 0; targets[i]; i++)                                                                                                                       \
            if (strcmp((str), targets[i]) == 0) {                                                                                                              \
                found = true;                                                                                                                                  \
                break;                                                                                                                                         \
            }                                                                                                                                                  \
        found;                                                                                                                                                 \
    })

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

double round_to_nearest_double(double n, double m);

void removePunct(char *str);

void removeChars(char *str, const char *chars);

float format_eng(char *dest, float value, const char *units, char *new_units);

float format_eng(char *dest, float value, const char *units, char *new_units, uint8_t dec_places, bool trailing_zero);

float fasterlog2(float);

int32_t int16_sin_s4(int32_t x);

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
bool parse_long(const char *str, int64_t &result);

int64_t safe_atoi64(const char *s);
uint64_t safe_atou64(const char *s);
uint8_t safe_atohex(const char *s);
void int_to_binary(uint64_t num, char *binary, int bits);

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

int most_significant_decimal(long i);

int analog_median(int pin, int n);

float round_down_to_nearest(float d, float t);

int ceil_multiple(int value, int mult);

int floor_multiple(int value, int mult);

int split_string(char *str, char **parts, int length, char separator);

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

/***** PROFILING *******/

// DWT (Data Watchpoint and Trace) registers
#define DWT_CTRL (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT (*(volatile uint32_t *)0xE0001004)
#define DWT_CPICNT (*(volatile uint32_t *)0xE0001008)
#define DWT_EXCCNT (*(volatile uint32_t *)0xE000100C)

// CoreDebug registers for enabling DWT
#define CoreDebug_DEMCR (*(volatile uint32_t *)0xE000EDFC)

// DWT Control register bit definitions
#define DWT_CTRL_CYCCNTENA_Pos 0U
#if !defined(DWT_CTRL_CYCCNTENA_Msk)
#define DWT_CTRL_CYCCNTENA_Msk (1UL << DWT_CTRL_CYCCNTENA_Pos)
#endif
// CoreDebug DEMCR register bit definitions
#define CoreDebug_DEMCR_TRCENA_Pos 24U
#define CoreDebug_DEMCR_TRCENA_Msk (1UL << CoreDebug_DEMCR_TRCENA_Pos)

/**
 * @brief Get current cycle count
 * @return Current cycle count value
 */
static inline uint32_t DWT_GetCycles(void) {
    return DWT_CYCCNT;
}

/*
 * @brief Reset cycle counter to zero
 */
static inline void DWT_ResetCycles(void) {
    DWT_CYCCNT = 0;
}

/**
 * @brief Convert cycles to microseconds
 * @param cycles: Number of CPU cycles
 * @param cpu_freq_mhz: CPU frequency in Hz
 * @return Time in microseconds
 */
static inline float DWT_CyclesToUs(uint32_t cycles, uint32_t cpu_freq_hz) {
    return (float)cycles / ((float)cpu_freq_hz / 100000.0f);
}

/**
 * @brief Convert cycles to milliseconds
 * @param cycles: Number of CPU cycles
 * @param cpu_freq_mhz: CPU frequency in Hz
 * @return Time in milliseconds
 */
static inline float DWT_CyclesToMs(uint32_t cycles, uint32_t cpu_freq_hz) {
    return (float)cycles / ((float)(cpu_freq_hz) / 1000.0f);
}

typedef struct {
    uint32_t start_cycles;
    const char *name;
} profile_context_t;

// Stack-based profiler for automatic scope management
typedef struct {
    profile_context_t contexts[8]; // Max 8 nested levels
    uint8_t depth;
    uint32_t cpu_freq_mhz;
} profile_stack_t;

static profile_stack_t g_profile_stack = {0};

static inline void profile_stack_init(uint32_t cpu_freq_mhz) {
    g_profile_stack.depth = 0;
    g_profile_stack.cpu_freq_mhz = cpu_freq_mhz;
}

static inline void profile_stack_push(const char *name) {
    if (g_profile_stack.depth < 8) {
        profile_context_t *ctx = &g_profile_stack.contexts[g_profile_stack.depth];
        ctx->start_cycles = DWT_GetCycles();
        ctx->name = name;
        g_profile_stack.depth++;

        // Print indented start message
        for (int i = 0; i < g_profile_stack.depth - 1; i++)
            printf_("  ");
        printf_("-> %s\n", name);
    }
}

static inline void profile_stack_pop(void) {
    if (g_profile_stack.depth > 0) {
        g_profile_stack.depth--;
        profile_context_t *ctx = &g_profile_stack.contexts[g_profile_stack.depth];
        uint32_t end_cycles = DWT_GetCycles();
        uint32_t elapsed = end_cycles - ctx->start_cycles;

        // Print indented result
        for (int i = 0; i < g_profile_stack.depth; i++)
            printf_("  ");
        printf_("<- %s: %lu cycles (%.2f us)\n", ctx->name, elapsed, DWT_CyclesToUs(elapsed, g_profile_stack.cpu_freq_mhz));
    }
}

#define PROFILE_PUSH(name) profile_stack_push(name)
#define PROFILE_POP() profile_stack_pop()

void DWT_Init(void);

#endif // UTILS_H
