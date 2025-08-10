//
// Created by Angel Dust on 18/02/2017.
//

//#define ENABLE_LOGGER 0

#if defined(NRF51)

#define MEMORYFREE_ENABLED false

#else

#include "utils.hpp"
#include "../printf/printf.h"
#include "MemoryFree.h"
#include <c++/7.2.1/cstring>
#include <ctype.h>
#include <math.h>

#define MEMORYFREE_ENABLED false
#define RAW_LOG false

#endif

#ifdef ENABLE_LOGGER

volatile uint8_t logEventIndex = 0;
volatile bool printingLog = false;
int num_loggers = 0;
volatile bool eventLogEnabled = true;
bool scopeLog = false;
logevent_st_t logEvents[LOG_MAX_ITEMS];
logevent_st_t logEventsClone[LOG_MAX_ITEMS];
uint32_t logEventTypeStartTimes[LOG_MAX_ITEMS];

uint32_t POWSOF10[] = {1, 10, 100, 1000, 10000, 100000, 1000000};

/*
logger_t loggers[MAX_LOGGERS];
int addLogger(logger_t logger) {

    if (num_loggers == MAX_LOGGERS) return -1;
    else {
        loggers[num_loggers++] = logger;
        return 1;
    }

}

void logMessage(int num_args, ...) {

    va_list ap;
    va_start(ap, num_args);

    for (size_t i = 0; i < num_args; i++) {

        char *msg = va_arg(ap, char *);

        printf(msg);
        if (i == num_args - 1) printf("\n");

        for (int il = 0; il < num_loggers; il++) {

            loggers[il](msg, i == num_args - 1);
        }

    }

}

void logMessageValues(int num_args, ...) {

    va_list ap;
    va_start(ap, num_args);

    for (size_t i = 0; i < num_args; i += 2) {

        char buffer[20];

        char *label = va_arg(ap, char *);

        double value = va_arg(ap, double);

        sprintf(buffer, "%16s: %8.2f  ", label, value);

        printf(buffer);

        if (i == num_args - 2) printf("\n");

        for (int il = 0; il < num_loggers; il++) {

            loggers[il](buffer, i == num_args - 1);

        }

    }

}

void logValue(const char *label, float value) {

    char buffer[20];

    sprintf(buffer, "%s: %f", label, value);

    logMessage(1, buffer);
}
 */

void logEvent(uint8_t type, float value) {
    logEvent(type, value, 0);
}

void logEvent(uint8_t type, float value, uint8_t end = 0) {

    if (eventLogEnabled && !printingLog) {

        //   disableTimers();
        uint8_t index = logEventIndex++;
        if (logEventIndex == LOG_MAX_ITEMS) {
            logEventIndex = 0;
        }
        //  enableTimers();

        uint32_t m = micros();
        logEvents[index].time = m;
        logEvents[index].type = type;
        logEvents[index].data = value;
        logEvents[index].end = end;

        if (index == LOG_MAX_ITEMS - 1) {

            printLog();
        }
    }
}

// Function to trim leading and trailing spaces fom a char*
void trim(char *str) {

    if (str == nullptr) {
        return;
    }

    // Leading spaces
    char *start = str;
    while (*start && *start == ' ') {
        ++start;
    }

    // If the string is empty after trimming leading spaces, return it
    if (*start == '\0') {
        *str = '\0'; // Set to empty string
        return;
    }

    // Trim trailing spaces by finding the end of the string
    char *end = start + std::strlen(start) - 1;
    while (end > start && *end == ' ') {
        --end;
    }

    // Null-terminate the trimmed string
    *(end + 1) = '\0';

    // Copy the trimmed string back to the original position
    if (start != str) {
        std::memmove(str, start, end - start + 2); // +2 to include the null terminator
    }
}

void printLog() {

    //  disableTimers();

    printingLog = true;

    logEventIndex = 0;
    logevent_st_t logEventsClone[LOG_MAX_ITEMS];
    for (int i = 0; i < LOG_MAX_ITEMS; i++) {

        logEventsClone[i] = logEvents[i];
    }
    printingLog = false;

    //  enableTimers();

    printLog(logEventsClone);
}

void printLog(logevent_st_t *logEventsClone) {

    char buf[10];

    printf("0-\n"); // leading '0' to 'see' where the block starts

    for (int i = 0; i < LOG_MAX_ITEMS; i++) {

        uint8_t type = (uint8_t)logEventsClone[i].type;
        bool end = (bool)logEventsClone[i].end;

        // Adjust for the average time spent in logEvent (2ms)
        // logEvents[i][0]-=2*i;

        // format_long(logEventsClone[i].time,buf);
        printf("%lu,", logEventsClone[i].time);

        if (type < 20) {

            logEventsClone[i].elapsed = logEventsClone[i].time - logEventTypeStartTimes[type];
            logEventTypeStartTimes[type] = logEventsClone[i].time;

            format_long(logEventsClone[i].elapsed, buf);
            printf("%s", buf);
        } else if (i > 0) {

            printf("%lu", logEventsClone[i].time - logEventsClone[i - 1].time);
        } else {

            printf("-");
        }

#if (!RAW_LOG)

        printf(" (+%lu", i > 0 ? logEventsClone[i].time - logEventsClone[i - 1].time : 0);

        if (type < 10 && end) {
            printf("us., +");
            printf("%d", (uint16_t)logEventsClone[i].elapsed);
        }

        printf("us.):");

        switch (logEventsClone[i].type) {

            default:
                printf("%d", logEventsClone[i].type);
        }

        printf(end ? " e" : " s");
        printf(": ");

#else
        Serial.print(F(","));
        Serial.print(logEventsClone[i].type);
        Serial.print(F(","));
#endif

        printf("%.2f\n", logEventsClone[i].data);
    }

#if (!RAW_LOG)
    printf("--------------\n");
#endif

    printf("0!\n"); // leading '0' to 'see' where the block starts
}

#endif

int endsWith(const char *str, const char *suffix) {
    if (!str || !suffix) {
        return 0;
    }
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix > lenstr) {
        return 0;
    }
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

void print_vector_f32(float32_t *v, uint16_t len) {

    for (int i = 0; i < len; i++) {
        if (i && ((i) % 4 == 0)) {
            printf("\r\n");
        }
        printf("%14.10f;  ", v[i]);
        for (uint32_t xx = 0; xx < 1000; xx++) {
            ;
        }
    }
    printf("\r\n");
    printf("\r\n");
    for (uint32_t xx = 0; xx < 1000000; xx++) {
        ;
    }
}

void print_vector_complex_f32(float32_t *v, uint16_t len) {

    printf("x=transpose(complex([");
    for (int i = 0; i < len * 2; i += 2) {
        if (i && ((i) % 8 == 0)) {
            printf("\r\n");
        }
        printf("%9.5f; ", v[i]);
        for (uint32_t xx = 0; xx < 1000; xx++) {
            ;
        }
    }
    printf("]\r\n,\r\n[");
    for (int i = 1; i < len * 2; i += 2) {
        if ((i > 1) && ((i - 1) % 8) == 0) {
            printf("\r\n");
        }
        printf("%9.5f; ", v[i]);
        for (uint32_t xx = 0; xx < 1000; xx++) {
            ;
        }
    }
    printf("]));");
    printf("\r\n");
    printf("\r\n");
    for (uint64_t xx = 0; xx < 1000000; xx++) {
        ;
    }
}

unsigned long millis() {
    return uwTick;
}

unsigned long micros() {
    return uwTick * 1000 + (1000 - SysTick->VAL / 72);
}

float fasterlog2(float x) {
    union {
        float f;
        uint32_t i;
    } vx = {x};
    float y = vx.i;
    y *= 1.0 / (1 << 23);
    return y - 126.94269504f;
}

float fasterlog(float x) {

    // log2(x) = log10(x)/log10(2)

    // return  0.69314718f * fasterlog2(x); // Ln(x)
    return 0.3010299f * fasterlog2(x); // Log10(x)
}

float fastpow2(const float val) {
    union {
        float f;
        uint32_t n;
    } u;
    u.n = val * 8388608 + (0x3f800000 - 60801 * 8);
    return u.f;
}

void min_max_f32(float *v, uint16_t size, float *min, float *max) {
    min_max_f32(v, size, min, max, (float)0xFFFFFFFF);
}

void min_max_f32(float *v, uint16_t size, float *min, float *max, float discard) {

    *min = 1e6;
    *max = -1e6;

    for (int i = 0; i < size; i++) {
        if (v[i] != discard) {
            if (v[i] > *max) {
                *max = v[i];
            }
            if (v[i] < *min) {
                *min = v[i];
            }
        }
    }
}

int32_t int16_sin_s4(int32_t x) {
    static const int qN = 14, qA = 16, qR = 12, B = 19900, C = 3516;

    const int32_t c = x << (30 - qN); // Semi-circle info into carry.
    x -= 1 << qN;                     // sine -> cosine calc

    x = x << (31 - qN);         // Mask with PI
    x = x >> (31 - qN);         // Note: SIGNED shift! (to qN)
    x = x * x >> (2 * qN - 14); // x=x^2 To Q14

    int32_t y = B - (x * C >> 14); // B - x^2*C
    y = (1 << qA) - (x * y >> qR); // A - x^2*(B-x^2*C)

    return c >= 0 ? y : -y;
}

char prefixes[] = "num kMGT";

float format_eng(char *dest, float value, const char *units, char *new_units) {
    return format_eng(dest, value, units, new_units, 1, false);
}

float format_eng(char *dest, float value, const char *units, char *new_units, uint8_t dec_places, bool trailing_zero) {

    double tval = value;
    uint8_t order = 3;
    if (tval) {
        while (abs(tval) >= 1000.0 && order < strlen(prefixes)) {
            tval /= 1000.0;
            order++;
        }
        while (abs(tval) < 1.0 && order > 0) {
            tval *= 1000.0;
            order--;
        }
    }

    sprintf(new_units, "%c%s", prefixes[order], units);
    sprintf(dest, "%.*f ", dec_places, tval);

    if (dec_places && !trailing_zero) {
        int l = strlen(dest) - 1;
        bool end = false;
        while (--l >= 0 && !end && (dest[l] == '0' || dest[l] == '.')) {
            end = dest[l] == '.';
            dest[l] = ' ';
            dest[l + 1] = '\0';
        }
    }
    return tval;
}

char *format_long(int64_t n, char *out) {
    format_long(n, out, 0);
    return out;
}

void format_long(int64_t n, char *out, uint8_t length, char thow_separator, int max_length) {

    int c;
    char buf[max_length + 3];
    char *p;

    length = min2(length, max_length);

    if (length) {
        snprintf(buf, max_length + 3, "%0*lld", length, n);
    } else {
        snprintf(buf, max_length + 3, "%lld", n);
    }

    c = 2 - strlen(buf) % 3;
    for (p = buf; *p != 0; p++) {
        *out++ = *p;
        if (c == 1) {
            *out++ = thow_separator;
        }
        c = (c + 1) % 3;
    }
    *--out = 0;
}

int strcicmp(char const *a, char const *b) {
    const unsigned char *us1 = (const unsigned char *)a, *us2 = (const unsigned char *)b;

    while (tolower(*us1) == tolower(*us2++)) {
        if (*us1++ == '\0') {
            return (0);
        }
    }
    return (tolower(*us1) - tolower(*--us2));
}

void extract_file_and_path(const char *fileandpath, char *path, char *file, size_t size) {

    strncpy(path, fileandpath, size);
    uint16_t len = strlen(path);
    char *c = strrchr(path, '/');
    if (c) {
        uint16_t at = (c - path) + 1;      // search next index after the last '/'
        memcpy(file, path + at, len - at); // Copy last folder name
        path[at] = '\0';
    } else {
        path[0] = '\0';
        strncpy(file, fileandpath, size);
    }
}

void removePunct(char *str) {
    // To keep track of non-space character count
    int count = 0;

    // Traverse the given string. If current character
    // is not space, then place it at index 'count++'
    for (int i = 0; str[i]; i++) {
        if (str[i] != ',' && str[i] != '.' && str[i] != ' ') {
            str[count++] = str[i]; // here count is
        }
    }
    // incremented
    str[count] = '\0';
}

void removeChars(char *str, const char *chars) {
    // To keep track of non-space character count
    int count = 0;

    // Traverse the given string. If current character
    // is not space, then place it at index 'count++'
    for (int i = 0; str[i]; i++) {
        int j = 0;
        int n = strlen(chars);
        while (j < n && str[i] != chars[j]) {
            j++;
        }
        if (j == n) {
            str[count++] = str[i]; // here count is
        }
    }
    // incremented
    str[count] = '\0';
}

uint16_t mod(int a, int b) {
    int r = a % b;
    return r < 0 ? r + b : r;
}

/*
int analogMedian(int pin, int n) {

    int v = 0;

    for (int i = 0; i < n; i++) {
        v += analogRead(pin);
    }

    return round(v / n);
}
*/

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/*
float roundDownToNearest(float d, float t) {
    // 105.5 down to nearest 1 = 105
    // 105.5 down to nearest 10 = 100
    // 105.5 down to nearest 7 = 105


    //if no rounto then just pass original number back
    if (t == 0) {
        return d;
    } else {
        return floor(d / t) * t;
    }
}
*/

int mostSignificantDecimal(long i) {

    long d = i;
    while (d >= 10) {
        d /= 10;
    }

    return (int)d;
}

float truncate_float(float v, int decimals) {
    return (float)((int)(v * POWSOF10[decimals - 1])) / (float)POWSOF10[decimals - 1];
}
/**
 * Double to ASCII
 */
/*
char *dtoa(char *s, double n) {
   // handle special cases
   if (isnan(n)) {
       strcpy(s, "nan");
   } else if (isinf(n)) {
       strcpy(s, "inf");
   } else if (n == 0.0) {
       strcpy(s, "0");
   } else {
       int digit, m, m1;
       char *c = s;
       int neg = (n < 0);
       if (neg)
           n = -n;
       // calculate magnitude
       m = log10(n);
       int useExp = (m >= 14 || (neg && m >= 9) || m <= -9);
       if (neg)
           *(c++) = '-';
       // set up for scientific notation
       if (useExp) {
           if (m < 0)
               m -= 1.0;
           n = n / pow(10.0, m);
           m1 = m;
           m = 0;
       }
       if (m < 1.0) {
           m = 0;
       }
       // convert the number
       while (n > PRECISION || m >= 0) {
           double weight = pow(10.0, m);
           if (weight > 0 && !isinf(weight)) {
               digit = floor(n / weight);
               n -= (digit * weight);
               *(c++) = '0' + digit;
           }
           if (m == 0 && n > 0)
               *(c++) = '.';
           m--;
       }
       if (useExp) {
           // convert the exponent
           int i, j;
           *(c++) = 'e';
           if (m1 > 0) {
               *(c++) = '+';
           } else {
               *(c++) = '-';
               m1 = -m1;
           }
           m = 0;
           while (m1 > 0) {
               *(c++) = '0' + m1 % 10;
               m1 /= 10;
               m++;
           }
           c -= m;
           for (i = 0, j = m - 1; i < j; i++, j--) {
               // swap without temporary
               c[i] ^= c[j];
               c[j] ^= c[i];
               c[i] ^= c[j];
           }
           c += m;
       }
       *(c) = '\0';
   }
   return s;
}


int splitString(char *str, char **parts, int length, char separator = ':') {

   int nparts = 0;
   char *p = str;
   bool exit = false;

   do {
       parts[nparts] = strchr((const char *) p, separator);

       if (parts[nparts] != 0) {
           // split by writting null
           *parts[nparts] = 0;
           parts[nparts]++;
           while (*parts[nparts] == 32) parts[nparts]++;
           p = parts[nparts];


       } else {

           exit = true;
       }

       nparts++;

   } while (nparts < length && !exit);

   return nparts - 1;

}
*/

#if MEMORYFREE_ENABLED
void printMemory() {

#ifdef f__AVR_ATmega328P__
    Serial.print(F("M: "));
    Serial.print(freeMemory());
#else
    Serial.print(F("H: "));
    Serial.print(freeHeap());
    Serial.print(F(" S: "));
    Serial.println(freeStack());
#endif
}
#endif

char *format_double(double v, char *dest, char decimal_separator, char thousand_separator, uint8_t frac_digits, bool trailing_zero, uint8_t max_length) {

    double i;
    double fracPart = modf(v, &i);

    format_long(i, dest, 0, thousand_separator, max_length);

    if (fracPart > 0) {

        snprintf(dest + strlen(dest), max_length - strlen(dest), "%c", decimal_separator);
        if (fracPart > 0) {
            char fracStr[10];
            ftoa(fracStr, 10, fracPart, frac_digits);
            int l = strlen(fracStr);
            if (!trailing_zero) {
                while (--l >= 0 && fracStr[l] == '0') {
                    fracStr[l] = '\0';
                }
            }
            snprintf(dest + strlen(dest), max_length - strlen(dest), "%s", fracStr + 2);
        }
    }

    return dest;
}

/*
int readVcc() {
#if defined(ARDUINO_ARCH_AVR)
    const long int scaleConst = 1156.300 * 1000;
    // Read 1.1V reference against Avcc
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) ||
defined(__AVR_ATmega2560__) ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) |
_BV(MUX2) | _BV(MUX1); #elif defined (__AVR_ATtiny24__) ||
defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__) ADMUX = _BV(MUX5) |
_BV(MUX0); #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) ||
defined(__AVR_ATtiny85__) ADMUX = _BV(MUX3) | _BV(MUX2); #else ADMUX =
_BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1); #endif delay(2); // Wait for
Vref to settle ADCSRA |= _BV(ADSC); // Start conversion
    while(bit_is_set(ADCSRA,ADSC)); // measuring
    uint8_t low = ADCL; // must read ADCL first - it then locks ADCH
    uint8_t high = ADCH; // unlocks both
    long int result = (high<<8) | low;
    result = scaleConst / result;
    // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
    return(int)result; // Vcc in millivolts
#else
    return -1;
#endif

}
 */

char *ftoa(char *dest, size_t size, double val, int dec) {
    char *p = dest;
    char *q = dest + size;
    long long mul = 1;
    long long num;
    int i;
    if (size == 0) {
        return NULL;
    }
    *--q = '\0';
    if (size == 1) {
        return 0;
    }

    if (val < 0) {
        val = -val;
        if (p >= q) {
            return 0;
        }
        *p++ = '-';
    }
    for (i = 0; i < dec; i++) {
        mul *= 10;
    }
    num = (long long)(val * mul + 0.5);
    for (i = 1; i < dec + 2 || num > 0; i++) {
        if (p >= q) {
            return 0;
        }

        *--q = '0' + (num % 10);
        num = num / 10;
        if (i == dec) {
            if (p >= q) {
                return 0;
            }
            *--q = '.';
        }
    }
    memmove(p, q, dest + size - q);
    return dest;
}

float adc_to_mv(int adc_value, float adc_vref, int adc_max) {
    return ((float)adc_value / (float)adc_max) * adc_vref;
}

float mv_to_adc(int millivolts, float adc_vref, int adc_max) {
    return ((float)millivolts / adc_vref) * (float)adc_max;
}

bool parse_long(const char *v, int64_t &result) {
    if (!v) {
        return false;
    }

    // Skip whitespace using ARM SIMD where possible
    while (*v <= ' ' && *v > '\0') {
        v++;
    }

    if (*v == '\0') {
        return false;
    }

    // Handle sign
    if (*v == '+') {
        v++;
    } else if (*v == '-') {
        return false;
    }

    if (*v == '\0') {
        return false;
    }

    uint64_t value = 0;
    bool found_digit = false;

    // Unroll loop for better performance on ARM
    while (*v != '\0') {
        uint32_t c = *v;

        // Branchless digit check
        uint32_t is_digit = (c - '0' < 10) ? 1 : 0;
        if (!is_digit) {
            break;
        }

        found_digit = true;

        // Quick overflow check
        if (value > 1844674407370955161ULL) { // UINT64_MAX/10
            return false;
        }

        value = value * 10 + (c - '0');
        v++;
    }

    if (!found_digit) {
        return false;
    }

    // Skip trailing whitespace
    while (*v <= ' ' && *v > '\0') {
        v++;
    }

    if (*v != '\0') {
        return false;
    }

    result = (int64_t)value;
    return true;
}

bool parse_int(const char *str, int &result) {
    if (!str[0]) {
        return false;
    }

    result = 0;
    bool negative = false;
    size_t start = 0;

    // Handle sign
    if (str[0] == '-') {
        negative = true;
        start = 1;
    } else if (str[0] == '+') {
        start = 1;
    }

    if (start >= strlen(str)) {
        return false;
    }

    // Parse digits
    for (size_t i = start; i < strlen(str); ++i) {
        if (str[i] < '0' || str[i] > '9') {
            return false; // Invalid character
        }

        // Check for overflow before multiplying
        if (result > (INT32_MAX - (str[i] - '0')) / 10) {
            return false; // Overflow
        }

        result = result * 10 + (str[i] - '0');
    }

    if (negative) {
        result = -result;
    }
    return true;
}

void int_to_binary(uint64_t num, char *binary, int bits) {
    binary[bits] = '\0'; // Null terminate first

    for (int i = 0; i < bits; i++) {
        binary[bits - 1 - i] = ((num >> i) & 1) ? '1' : '0';
    }
}

/**
 * @brief Initialize DWT cycle counter for profiling
 */
void DWT_Init(void) {
    // Enable trace and debug blocks
    CoreDebug_DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    // Reset cycle counter
    DWT_CYCCNT = 0;

    // Enable cycle counter
    DWT_CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}
