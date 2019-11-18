#include "math.h"
#include "stdint.h"
#include "circadian.h"

static float rprime   =  0.2623;
static float gprime   =  0.6547;
static float bprime   =  0.8677;

static float Aby       =  0.645073;
static float Arod      =  3.298435;
static float k         =  0.288359;
static float A         =  2.333629;

static float Wr_Smac   =  -0.099345;
static float Wg_Smac   = -0.12601;
static float Wb_Smac   = 0.440623;

static float Wr_Vmac   =  0.411945;
static float Wg_Vmac   =  0.717871;
static float Wb_Vmac   =  -0.001789;

static float Wr_M      = -0.102307;
static float Wg_M      =  0.09411;
static float Wb_M      =  0.43051;

static float Wr_Vprime = -0.062037;
static float Wg_Vprime =  0.345072;
static float Wb_Vprime =  0.271942;

static float Wr_V      =  0.451801;
static float Wg_V      =  0.667457;
static float Wb_V      = -0.071304;

float calculate_cla(uint16_t r, uint16_t g, uint16_t b) {
    float rlux = rprime * r;
    float glux = gprime * g;
    float blux = bprime * b;

    float Smac = Wr_Smac * rlux + Wg_Smac * glux + Wb_Smac * blux;
    float Vmac = Wr_Vmac * rlux + Wg_Vmac * glux + Wb_Vmac * blux;
    float M    = Wr_M * rlux + Wg_M * glux + Wb_M * blux;

    if (Smac <= (k * Vmac))
    {
        float cla = A * M;
        return cla;
    }

    float Vprime = (Wr_Vprime * rlux) + (Wg_Vprime * glux) + (Wb_Vprime * blux);

    float E = 1 - exp ( -Vprime / (6.5 * 683.0)  );

    float P1 = Aby * (Smac - k * Vmac);
    float P2 = Arod * 683 * E;

    float cla = A * (M + P1 - P2);

    return cla;
}

float calculate_cs (float CLa)
{
    float bottom = 1 + pow ((CLa / 355.7), 1.1026);
    float cs = bottom * (1 - 1 / bottom);

    return cs;
}
