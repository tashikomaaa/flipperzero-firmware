#include "princeton.h"

#include "../blocks/const.h"
#include "../blocks/decoder.h"
#include "../blocks/encoder.h"
#include "../blocks/generic.h"
#include "../blocks/math.h"

/*
 * Help
 * https://phreakerclub.com/447
 *
 */

#define TAG "SubGhzProtocolCAME"

static const SubGhzBlockConst subghz_protocol_princeton_const = {
    .te_short = 400,
    .te_long = 1200,
    .te_delta = 250,
    .min_count_bit_for_found = 24,
};

struct SubGhzProtocolDecoderPrinceton {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;

    uint32_t te;
};

struct SubGhzProtocolEncoderPrinceton {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;

    uint32_t te;
};

typedef enum {
    PrincetonDecoderStepReset = 0,
    PrincetonDecoderStepSaveDuration,
    PrincetonDecoderStepCheckDuration,
} PrincetonDecoderStep;

const SubGhzProtocolDecoder subghz_protocol_princeton_decoder = {
    .alloc = subghz_protocol_decoder_princeton_alloc,
    .free = subghz_protocol_decoder_princeton_free,

    .feed = subghz_protocol_decoder_princeton_feed,
    .reset = subghz_protocol_decoder_princeton_reset,

    .serialize = subghz_protocol_decoder_princeton_serialization,
    .save_file = subghz_protocol_princeton_save_file,
};

const SubGhzProtocolEncoder subghz_protocol_princeton_encoder = {
    .alloc = subghz_protocol_encoder_princeton_alloc,
    .free = subghz_protocol_encoder_princeton_free,

    .load = subghz_protocol_encoder_princeton_load,
    .stop = subghz_protocol_encoder_princeton_stop,
    .yield = subghz_protocol_encoder_princeton_yield,
    .load_file = subghz_protocol_princeton_load_file,
};

const SubGhzProtocol subghz_protocol_princeton = {
    .name = SUBGHZ_PROTOCOL_PRINCETON_NAME,
    .type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_868 | SubGhzProtocolFlag_315 |
            SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable,

    .decoder = &subghz_protocol_princeton_decoder,
    .encoder = &subghz_protocol_princeton_encoder,
};

void* subghz_protocol_encoder_princeton_alloc(SubGhzEnvironment* environment) {
    SubGhzProtocolEncoderPrinceton* instance = furi_alloc(sizeof(SubGhzProtocolEncoderPrinceton));

    instance->base.protocol = &subghz_protocol_princeton;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->encoder.repeat = 10;
    instance->encoder.size_upload = 52; //max 24bit*2 + 2 (start, stop)
    instance->encoder.upload = furi_alloc(instance->encoder.size_upload * sizeof(LevelDuration));
    instance->encoder.is_runing = false;
    return instance;
}

void subghz_protocol_encoder_princeton_free(void* context) {
    furi_assert(context);
    SubGhzProtocolEncoderPrinceton* instance = context;
    free(instance->encoder.upload);
    free(instance);
}

static bool
    subghz_protocol_princeton_encoder_get_upload(SubGhzProtocolEncoderPrinceton* instance) {
    furi_assert(instance);

    size_t index = 0;
    size_t size_upload = (instance->generic.data_count_bit * 2) + 2;
    if(size_upload > instance->encoder.size_upload) {
        FURI_LOG_E(TAG, "Size upload exceeds allocated encoder buffer.");
        return false;
    } else {
        instance->encoder.size_upload = size_upload;
    }

    //Send key data
    for(uint8_t i = instance->generic.data_count_bit; i > 0; i--) {
        if(bit_read(instance->generic.data, i - 1)) {
            //send bit 1
            instance->encoder.upload[index++] =
                level_duration_make(true, (uint32_t)instance->te * 3);
            instance->encoder.upload[index++] = level_duration_make(false, (uint32_t)instance->te);
        } else {
            //send bit 0
            instance->encoder.upload[index++] = level_duration_make(true, (uint32_t)instance->te);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)instance->te * 3);
        }
    }

    //Send Stop bit
    instance->encoder.upload[index++] = level_duration_make(true, (uint32_t)instance->te);
    //Send PT_GUARD
    instance->encoder.upload[index++] = level_duration_make(false, (uint32_t)instance->te * 30);

    return true;
}

bool subghz_protocol_encoder_princeton_load(
    void* context,
    uint64_t key,
    uint8_t count_bit,
    size_t repeat) {
    furi_assert(context);
    SubGhzProtocolEncoderPrinceton* instance = context;
    instance->te = 400;
    instance->generic.data = key;
    instance->generic.data_count_bit = 24;
    instance->encoder.repeat = repeat;
    subghz_protocol_princeton_encoder_get_upload(instance);
    instance->encoder.is_runing = true;
    return true;
}

void subghz_protocol_encoder_princeton_stop(void* context) {
    SubGhzProtocolEncoderPrinceton* instance = context;
    instance->encoder.is_runing = false;
}

LevelDuration subghz_protocol_encoder_princeton_yield(void* context) {
    SubGhzProtocolEncoderPrinceton* instance = context;

    if(instance->encoder.repeat == 0 || !instance->encoder.is_runing) {
        instance->encoder.is_runing = false;
        return level_duration_reset();
    }

    LevelDuration ret = instance->encoder.upload[instance->encoder.front];

    if(++instance->encoder.front == instance->encoder.size_upload) {
        instance->encoder.repeat--;
        instance->encoder.front = 0;
    }

    return ret;
}

void* subghz_protocol_decoder_princeton_alloc(SubGhzEnvironment* environment) {
    SubGhzProtocolDecoderPrinceton* instance = furi_alloc(sizeof(SubGhzProtocolDecoderPrinceton));
    instance->base.protocol = &subghz_protocol_princeton;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void subghz_protocol_decoder_princeton_free(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderPrinceton* instance = context;
    free(instance);
}

void subghz_protocol_decoder_princeton_reset(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderPrinceton* instance = context;
    instance->decoder.parser_step = PrincetonDecoderStepReset;
}

void subghz_protocol_decoder_princeton_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    SubGhzProtocolDecoderPrinceton* instance = context;

    switch(instance->decoder.parser_step) {
    case PrincetonDecoderStepReset:
        if((!level) && (DURATION_DIFF(duration, subghz_protocol_princeton_const.te_short * 36) <
                        subghz_protocol_princeton_const.te_delta * 36)) {
            //Found Preambula
            instance->decoder.parser_step = PrincetonDecoderStepSaveDuration;
            instance->decoder.decode_data = 0;
            instance->decoder.decode_count_bit = 0;
            instance->te = 0;
        }
        break;
    case PrincetonDecoderStepSaveDuration:
        //save duration
        if(level) {
            instance->decoder.te_last = duration;
            instance->te += duration;
            instance->decoder.parser_step = PrincetonDecoderStepCheckDuration;
        }
        break;
    case PrincetonDecoderStepCheckDuration:
        if(!level) {
            if(duration >= (subghz_protocol_princeton_const.te_short * 10 +
                            subghz_protocol_princeton_const.te_delta)) {
                instance->decoder.parser_step = PrincetonDecoderStepSaveDuration;
                if(instance->decoder.decode_count_bit ==
                   subghz_protocol_princeton_const.min_count_bit_for_found) {
                    instance->te /= (instance->decoder.decode_count_bit * 4 + 1);

                    instance->generic.data = instance->decoder.decode_data;
                    instance->generic.data_count_bit = instance->decoder.decode_count_bit;
                    instance->generic.serial = instance->decoder.decode_data >> 4;
                    instance->generic.btn = (uint8_t)instance->decoder.decode_data & 0x00000F;

                    if(instance->base.callback)
                        instance->base.callback(&instance->base, instance->base.context);
                }
                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 0;
                instance->te = 0;
                break;
            }

            instance->te += duration;

            if((DURATION_DIFF(instance->decoder.te_last, subghz_protocol_princeton_const.te_short) <
                subghz_protocol_princeton_const.te_delta) &&
               (DURATION_DIFF(duration, subghz_protocol_princeton_const.te_long) <
                subghz_protocol_princeton_const.te_delta * 3)) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 0);
                instance->decoder.parser_step = PrincetonDecoderStepSaveDuration;
            } else if(
                (DURATION_DIFF(instance->decoder.te_last, subghz_protocol_princeton_const.te_long) <
                 subghz_protocol_princeton_const.te_delta * 3) &&
                (DURATION_DIFF(duration, subghz_protocol_princeton_const.te_short) <
                 subghz_protocol_princeton_const.te_delta)) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 1);
                instance->decoder.parser_step = PrincetonDecoderStepSaveDuration;
            } else {
                instance->decoder.parser_step = PrincetonDecoderStepReset;
            }
        } else {
            instance->decoder.parser_step = PrincetonDecoderStepReset;
        }
        break;
    }
}

// uint16_t subghz_protocol_princeton_get_te(void* context) {
//     SubGhzDecoderPrinceton* instance = context;
//     return instance->te;
// }

void subghz_protocol_decoder_princeton_serialization(void* context, string_t output) {
    furi_assert(context);
    SubGhzProtocolDecoderPrinceton* instance = context;

    uint32_t code_found_lo = instance->generic.data & 0x00000000ffffffff;

    uint64_t code_found_reverse = subghz_protocol_blocks_reverse_key(
        instance->generic.data, instance->generic.data_count_bit);

    uint32_t code_found_reverse_lo = code_found_reverse & 0x00000000ffffffff;

    string_cat_printf(
        output,
        "%s %dbit\r\n"
        "Key:0x%08lX\r\n"
        "Yek:0x%08lX\r\n"
        "Sn:0x%05lX BTN:%02X\r\n"
        "Te:%dus\r\n",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        code_found_lo,
        code_found_reverse_lo,
        instance->generic.serial,
        instance->generic.btn,
        instance->te);
}

bool subghz_protocol_princeton_save_file(void* context, FlipperFile* flipper_file) {
    furi_assert(context);
    SubGhzProtocolDecoderPrinceton* instance = context;
    bool res = subghz_block_generic_save_file(&instance->generic, flipper_file);
    if(res) {
        res = flipper_file_write_uint32(flipper_file, "TE", &instance->te, 1);
        if(!res) FURI_LOG_E(TAG, "Unable to add Te");
    }
    return res;
}

bool subghz_protocol_princeton_load_file(
    void* context,
    FlipperFile* flipper_file,
    const char* file_path) {
    furi_assert(context);
    SubGhzProtocolEncoderPrinceton* instance = context;
    bool res = subghz_block_generic_load_file(&instance->generic, flipper_file);
    if(res) {
        res = flipper_file_read_uint32(flipper_file, "TE", (uint32_t*)&instance->te, 1);
        if(!res) FURI_LOG_E(TAG, "Missing TE");
    }
    return res;
}

// void subghz_decoder_princeton_to_load_protocol(SubGhzDecoderPrinceton* instance, void* context) {
//     furi_assert(context);
//     furi_assert(instance);
//     SubGhzProtocolCommonLoad* data = context;
//     instance->generic.data = data->code_found;
//     instance->generic.data_count_bit = data->code_count_bit;
//     instance->te = data->param1;
//     instance->common.serial = instance->generic.data >> 4;
//     instance->common.btn = (uint8_t)instance->generic.data & 0x00000F;
// }