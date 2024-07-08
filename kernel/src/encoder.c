#include "encoder.h"
#include "lib/include/rpi.h"
#include "lib/include/timer.h"

static int8_t change_lookup[16]; // -1 backward, +1 forward

void encoder_mgr_init(struct kQuadEncoderMgr * mgr) {
    mgr->encoders_n = 0;
    for (int i = 0; i < 64; i += 1) {
        mgr->pin_lookup[i] = -1;
    }

    for (int i = 0; i < 16; i += 1) {
        change_lookup[i] = 0;
    }
    change_lookup[0b0010] = 1;
    change_lookup[0b1011] = 1;
    change_lookup[0b1101] = 1;
    change_lookup[0b0100] = 1;
    change_lookup[0b0001] = -1;
    change_lookup[0b0111] = -1;
    change_lookup[0b1110] = -1;
    change_lookup[0b1000] = -1;
}

int encoder_init(struct kQuadEncoderMgr * mgr, uint32_t pin_a, uint32_t pin_b)
{
    if (mgr->encoders_n >= ENCODER_MAX) {
        return -1;
    }

    int idx = mgr->encoders_n;
    mgr->encoders_n += 1;

    mgr->encoders[idx].pin_a = pin_a;
    mgr->encoders[idx].pin_b = pin_b;

    uint64_t gpio_status = gpio_read();
    mgr->encoders[idx].last_a = (gpio_status & (1ULL << pin_a)) ? 1 : 0;
    mgr->encoders[idx].last_b = (gpio_status & (1ULL << pin_b)) ? 1 : 0;

    ke_quad_encoder_stat_clear(&mgr->encoders[idx].stat);

    mgr->pin_lookup[pin_a] = idx;
    mgr->pin_lookup[pin_b] = idx;

    // Setup the GPIO Pins
    setup_gpio(pin_a, GPIO_INPUT, GPIO_PDP);
    setup_gpio(pin_b, GPIO_INPUT, GPIO_PDP);
    gpio_set_edge_trigger(pin_a, true, true);
    gpio_set_edge_trigger(pin_b, true, true);

    return idx;
}

struct kQuadEncoder * get_encoder(struct kQuadEncoderMgr * mgr, int id)
{
    if (id >= 0 && id < mgr->encoders_n) {
        return &mgr->encoders[id];
    }
    return NULL;
}

struct kQuadEncoder * get_encoder_by_pin(struct kQuadEncoderMgr * mgr, uint32_t pin) {
    int id = mgr->pin_lookup[pin];
    if (id == -1) {
        return NULL;
    }
    return get_encoder(mgr, id);
}

void encoder_update(struct kQuadEncoder * encoder, uint8_t a, uint8_t b) {
    if (encoder == NULL) {
        return;
    }

    // static uint64_t update_cnt = 0;
    // update_cnt += 1;
    // if (update_cnt == 10000) {
    //     printf("UC %llu\r\n", get_current_time_u64());
    //     update_cnt = 0;
    // }

    uint8_t change = (encoder->last_a << 3) | (encoder->last_b << 2) | (a << 1) | b;

    int8_t change_result = change_lookup[change];

    if (change_result == 1) {
        encoder->stat.forward_cnt += 1;
    } else if (change_result == -1) {
        encoder->stat.backward_cnt += 1;
    } else {
        if (encoder->last_a == a && encoder->last_b == b) {
            encoder->stat.invalid_1_cnt += 1;
        } else {
            encoder->stat.invalid_2_cnt += 1;
        }
    }

    encoder->last_a = a;
    encoder->last_b = b;
}