#include "main.h"
#include "ad9851.h"
#include "sinwave.h"
#include "rotary_switch.h"

#define MAX_LINE_LENGTH 256
#define PROMPT "cli> "

#define ABS(x) (x < 0) ? (-x) : x
#define L3G_Sensitivity_250dps     (float)     114.285f        /*!< gyroscope sensitivity with 250 dps full scale [LSB/dps] */
#define L3G_Sensitivity_500dps     (float)     57.1429f        /*!< gyroscope sensitivity with 500 dps full scale [LSB/dps] */
#define L3G_Sensitivity_2000dps    (float)     14.285f         /*!< gyroscope sensitivity with 2000 dps full scale [LSB/dps] */
#define PI                         (float)     3.14159265f

#define LSM_Acc_Sensitivity_2g     (float)     1.0f            /*!< accelerometer sensitivity with 2 g full scale [LSB/mg] */
#define LSM_Acc_Sensitivity_4g     (float)     0.5f            /*!< accelerometer sensitivity with 4 g full scale [LSB/mg] */
#define LSM_Acc_Sensitivity_8g     (float)     0.25f           /*!< accelerometer sensitivity with 8 g full scale [LSB/mg] */
#define LSM_Acc_Sensitivity_16g    (float)     0.0834f         /*!< accelerometer sensitivity with 12 g full scale [LSB/mg] */

extern char* _strtok(char *, const char *);
extern size_t _strlen(const char *);
extern int _strncmp(const char *, const char *, size_t);

typedef struct command_list_s {
    char *name;
    void (*command)(char *);
    char *help;
} command_list_t;

void do_help(char *str);
void do_test(char *str);
void do_reset(char *str);
void do_peek(char *args);
void do_poke(char *args);

void do_accelerometer(char *str);
void do_gyro(char *args);
void do_pio(char *args);
void do_dds(char *args);
void do_dds_scan(char *args);
void do_sin(char *args);
void do_roto(char *args);

command_list_t commands[] = {
    {"help", do_help, "Print help"},
    {"?", do_help, "Print help"},
    {"test", do_test, "Test some functionality"},
    {"reset", do_reset, "Reset"},
    {"peek", do_peek, "peek <address>"},
    {"poke", do_poke, "poke <address> <value>"},
    {"acc", do_accelerometer, "Read accelerometer"},
    {"gyro", do_gyro, "Read gyro"},
    {"pio", do_pio, "Set GPIO <port> <value>"},
    {"dds", do_dds, "Set DDS frequency <frequency>"},
    {"ddsscan", do_dds_scan, "Automatic frequency change"},
    {"sin", do_sin, "Get/set sinus mesh <slot> <frequency>"},
    {"roto", do_roto, "Get rotary position"},
    {"\0", NULL, "\0"}
};

void cli_parse(char *str)
{
    char *pch;
    int i;
    int found = 0;

    while (*str == ' ') {
        str++;
    }

    if (*str == '\0') {
        return;
    }

    pch = _strtok((char *)str, " ,.-");
    for (i = 0; commands[i].command != NULL; i++) {
        if (!_strncmp(pch, commands[i].name, _strlen(pch))) {
            commands[i].command(pch);
            found = 1;
            break;
        }
    }

    if (!found) {
        _dprintf("Command not found\r\n");
    }
}

void cli_run(void)
{
    char line[MAX_LINE_LENGTH];
    int line_idx = 0;
    char cspecial = 0;
    int autocomplete = 0;
    char c;

    line[line_idx] = 0;
    _dprintf("%s", PROMPT);
    while (1) {
        c = _getchar();
        STM_EVAL_LEDToggle(LED9);
        if (cspecial) { /* +history */
            cspecial = 0;
            continue;
        }
        switch (c) {
        case '\r':
            _dprintf("\r\n");
            line[line_idx] = 0;
            cli_parse(line);
            line_idx = 0;
            _dprintf("%s", PROMPT);
            break;
        case '\b':
            if (line_idx > 0) {
                line[--line_idx] = 0;
                _dprintf("\b \b");
            }
            break;
        case 0x1b: /* skip special characters */
            _dprintf("special\r\n");
            cspecial = 1;
            break;
        default:
            if (line_idx >= MAX_LINE_LENGTH) {
                line_idx = 0;
                _dprintf("input line too long\r\n");
                _dprintf("%s", PROMPT);
                break;
            }
            line[line_idx++] = c;
            _putchar(c);
        }
    }
}

void do_help(char *str)
{
    int i;

    _dprintf("Available commands:\r\n");
    for (i = 0; commands[i].command; i++) {
        _dprintf("%-16s - %s\r\n", commands[i].name, commands[i].help);
    }
}

void do_test(char *str)
{
    _dprintf("do_test\r\n");
}

void do_reset(char *str)
{
    _dprintf("Bye!\r\n");
}

int _atoi(char *str)
{
    int n = 0;

    while (*str) {
        if ((*str >= '0') && (*str <= '9')) {
            n = (n << 4) | (*str - '0');
        } else if ((*str >= 'a') && (*str <= 'f')) {
            n = (n << 4) | (*str - 'a' + 10);
        } else if ((*str >= 'A') && (*str <= 'F')) {
            n = (n << 4) | (*str - 'A' + 10);
        } else {
            return n;
        }
        str++;
    }
    return n;
}

int _atoid(char *str)
{
    int n = 0;

    if (!str) {
        return -1;
    }

    while (*str) {
        if ((*str >= '0') && (*str <= '9')) {
            n = (n * 10) + (*str - '0');
        } else {
            return n;
        }
        str++;
    }
    return n;
}

void do_peek(char *args)
{
    unsigned int adr = _atoi(_strtok(NULL, " ,.-"));
    unsigned int val;

    val = *(volatile unsigned int *)adr;
    _dprintf("Memory read at 0x%x = 0x%x (%d)\r\n", adr, val, val);
}

void do_poke(char *args)
{
    unsigned int adr = _atoi(_strtok(NULL, " ,.-"));
    unsigned int val = _atoi(_strtok(NULL, " ,.-"));

    _dprintf("Writing 0x%x @ 0x%x\r\n", val, adr);
    *(volatile unsigned int *)adr = val;
}

void do_gyro(char *args)
{
    float Buffer[3] = {0.0f};
    uint8_t Xval, Yval = 0x00;

    /* Read Gyro Angular data */
    Demo_GyroReadAngRate(Buffer);

    /* Update autoreload and capture compare registers value*/
    Xval = ABS((int8_t)(Buffer[0]));
    Yval = ABS((int8_t)(Buffer[1]));
    _dprintf("X = %d\r\n", Xval);
    _dprintf("Y = %d\r\n", Yval);

}

void do_accelerometer(char *str)
{
    float MagBuffer[3] = {0.0f}, AccBuffer[3] = {0.0f};
    __IO float HeadingValue = 0.0f;
    float fNormAcc, fSinRoll, fCosRoll, fSinPitch, fCosPitch = 0.0f, RollAng = 0.0f, PitchAng = 0.0f;
    float fTiltedX, fTiltedY = 0.0f;
    int i;

    /* Read Compass data */
    Demo_CompassReadMag(MagBuffer);
    Demo_CompassReadAcc(AccBuffer);

    _dprintf("acc[0] = %d\r\n", (int)AccBuffer[0]);
    _dprintf("acc[1] = %d\r\n", (int)AccBuffer[1]);
    _dprintf("acc[2] = %d\r\n", (int)AccBuffer[2]);
    for (i = 0; i < 3; i++) {
        AccBuffer[i] /= 100.0f;
    }

    fNormAcc = sqrt((AccBuffer[0] * AccBuffer[0]) + (AccBuffer[1] * AccBuffer[1]) + (AccBuffer[2] * AccBuffer[2]));

    fSinRoll = -AccBuffer[1] / fNormAcc;
    fCosRoll = sqrt(1.0 - (fSinRoll * fSinRoll));
    fSinPitch = AccBuffer[0] / fNormAcc;
    fCosPitch = sqrt(1.0 - (fSinPitch * fSinPitch));
    if (fSinRoll > 0) {
        if (fCosRoll > 0) {
            RollAng = acos(fCosRoll)*180/PI;
        } else {
            RollAng = acos(fCosRoll)*180/PI + 180;
        }
    } else {
        if (fCosRoll > 0) {
            RollAng = acos(fCosRoll) * 180 / PI + 360;
        } else {
            RollAng = acos(fCosRoll) * 180 / PI + 180;
        }
    }

    if (fSinPitch > 0) {
        if (fCosPitch > 0) {
            PitchAng = acos(fCosPitch) * 180 / PI;
        } else {
            PitchAng = acos(fCosPitch) * 180 / PI + 180;
        }
    } else {
        if (fCosPitch > 0) {
            PitchAng = acos(fCosPitch) * 180 / PI + 360;
        } else {
            PitchAng = acos(fCosPitch) * 180 / PI + 180;
        }
    }

    if (RollAng >= 360) {
        RollAng = RollAng - 360;
    }

    if (PitchAng >= 360) {
        PitchAng = PitchAng - 360;
    }

    fTiltedX = MagBuffer[0] * fCosPitch + MagBuffer[2] * fSinPitch;
    fTiltedY = MagBuffer[0] * fSinRoll * fSinPitch + MagBuffer[1] * fCosRoll - MagBuffer[1] * fSinRoll * fCosPitch;
    HeadingValue = (float) ((atan2f((float)fTiltedY, (float)fTiltedX)) * 180) / PI;

    if (HeadingValue < 0) {
        HeadingValue = HeadingValue + 360;
    }

    _dprintf("roll = %d\r\n", (int)RollAng);
    _dprintf("pitch = %d\r\n", (int)PitchAng);

    _dprintf("fTiltedX = %d\r\n", (int)fTiltedX);
    _dprintf("fTiltedY = %d\r\n", (int)fTiltedY);
    _dprintf("HeadingValue = %d\r\n", (int)HeadingValue);
}

void do_pio(char *args)
{
    unsigned int gpio = _atoi(_strtok(NULL, " ,.-"));
    unsigned int state = _atoi(_strtok(NULL, " ,.-"));

    if ((gpio < 0) || (gpio >= LEDn)) {
        _dprintf("Unknown GPIO %d\r\n", gpio);
        return;
    }

    if (state == 1) {
        STM_EVAL_LEDOn(gpio);
    } else {
        STM_EVAL_LEDOff(gpio);
    }
}

void do_dds(char *args)
{
    int frequency = _atoid(_strtok(NULL, " ,.-"));
    int phase = _atoid(_strtok(NULL, " ,.-"));

    _dprintf("Set DDS frequency to %dHz, phase: %ddeg\r\n", frequency, phase);
    ad9851_set(frequency, phase);
}

void do_dds_scan(char *args)
{
    int i;

    while (1) {
        for (i = 1; i <= 60; i++) {
            ad9851_set(i * 1e6, 0);
        }
    }
}

void do_roto(char *args)
{
    _dprintf("pos: %d\r\n", rotary_get());
}
