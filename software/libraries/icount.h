#pragma once

#define NUM_VOLTAGES 9

static float voltages[NUM_VOLTAGES] =
{
    3.3, 3.2, 3.1, 3.0, 2.7, 2.6, 2.5, 2.4, 2.3
};

static float bias[NUM_VOLTAGES] =
{
    0.722,
    0.888,
    1.08,
    1.29,
    2.00,
    2.28,
    2.58,
    2.88,
    3.23
};

static float c_table[NUM_VOLTAGES*2] =
{
    1.17e+06,  1.18e+03,
    1.56e+06,  1.34e+03,
    2.00e+06,  1.38e+03,
    2.54e+06,  1.39e+03,
    4.47e+06,  3.61e+02,
    5.20e+06,  1.75e+02,
    6.02e+06,  3.71e+01,
    6.89e+06,  3.57e+02,
    7.87e+06, -1.48e+03

};

ret_code_t icount_init(size_t p, float* v);
void icount_enable(void);
void icount_disable(void);
void icount_pause(void);
void icount_resume(void);
void icount_clear(void);
uint32_t icount_get_count(void);
