#ifndef K_ENCODER_H
#define K_ENCODER_H

#include <stdint.h>
#include <stddef.h>

#include "lib/include/uapi.h"

#define ENCODER_MAX 2

struct kQuadEncoder {
    uint32_t pin_a;
    uint32_t pin_b;
    
    uint8_t last_a;
    uint8_t last_b;

    struct keQuadEncoderStat stat;
};

struct kQuadEncoderMgr {
    struct kQuadEncoder encoders[ENCODER_MAX];
    int encoders_n;
    int pin_lookup[64]; // From Pin Number to encoder id
};

void encoder_mgr_init(struct kQuadEncoderMgr * mgr);

int encoder_init(struct kQuadEncoderMgr * mgr, uint32_t pin_a, uint32_t pin_b);

struct kQuadEncoder * get_encoder(struct kQuadEncoderMgr * mgr, int id);
struct kQuadEncoder * get_encoder_by_pin(struct kQuadEncoderMgr * mgr, uint32_t pin);

void encoder_update(struct kQuadEncoder * encoder, uint8_t a, uint8_t b);
#endif