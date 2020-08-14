#ifndef DRC_H
#define DRC_H

#include <stdint.h>

#define MAX_DRC_FILTER_CNT_SUPPORT 2

#define DRC_BAND_NUM (3)

struct DrcBandConfig
{
    int     threshold;
    float   makeup_gain;
    int     ratio;              // set 1, bypass, reduce mips
    int     attack_time;
    int     release_time;
    float   scale_fact;         // invalid
};

typedef struct
{
    int knee;
    int filter_type[MAX_DRC_FILTER_CNT_SUPPORT];
    int band_num;
    int look_ahead_time;
    struct DrcBandConfig band_settings[DRC_BAND_NUM];
} DrcConfig;

struct DrcState_;

typedef struct DrcState_ DrcState;

#ifdef __cplusplus
extern "C" {
#endif

DrcState *drc_create(int sample_rate, int frame_size, int sample_bit, int ch_num, const DrcConfig *config);

int32_t drc_destroy(DrcState *st);

int32_t drc_set_config(DrcState *st, const DrcConfig *config);

int32_t drc_process(DrcState *st, uint8_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif