#include "keeloq.h"
#include "keeloq_common.h"

#include "../subghz_keystore.h"
#include <m-string.h>
#include <m-array.h>

#include "../blocks/const.h"
#include "../blocks/decoder.h"
#include "../blocks/encoder.h"
#include "../blocks/generic.h"
#include "../blocks/math.h"

#define TAG "SubGhzProtocolkeeloq"

static const SubGhzBlockConst subghz_protocol_keeloq_const = {
    .te_short = 400,
    .te_long = 800,
    .te_delta = 140,
    .min_count_bit_for_found = 64,
};

struct SubGhzProtocolDecoderKeeloq {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;

    uint16_t header_count;
    SubGhzKeystore* keystore;
    const char* manufacture_name;
};

struct SubGhzProtocolEncoderKeeloq {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;

    SubGhzKeystore* keystore;
    const char* manufacture_name;
};

typedef enum {
    KeeloqDecoderStepReset = 0,
    KeeloqDecoderStepCheckPreambula,
    KeeloqDecoderStepSaveDuration,
    KeeloqDecoderStepCheckDuration,
} KeeloqDecoderStep;

const SubGhzProtocolDecoder subghz_protocol_keeloq_decoder = {
    .alloc = subghz_protocol_decoder_keeloq_alloc,
    .free = subghz_protocol_decoder_keeloq_free,

    .feed = subghz_protocol_decoder_keeloq_feed,
    .reset = subghz_protocol_decoder_keeloq_reset,

    .serialize = subghz_protocol_decoder_keeloq_serialization,
    .save_file = subghz_protocol_keeloq_save_file,
};

const SubGhzProtocolEncoder subghz_protocol_keeloq_encoder = {
    .alloc = subghz_protocol_encoder_keeloq_alloc,
    .free = subghz_protocol_encoder_keeloq_free,

    .load = subghz_protocol_encoder_keeloq_load,
    .stop = subghz_protocol_encoder_keeloq_stop,
    .yield = subghz_protocol_encoder_keeloq_yield,
    .load_file = subghz_protocol_keeloq_load_file,
};

const SubGhzProtocol subghz_protocol_keeloq = {
    .name = SUBGHZ_PROTOCOL_KEELOQ_NAME,
    .type = SubGhzProtocolTypeDynamic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_868 | SubGhzProtocolFlag_315 |
            SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable,

    .decoder = &subghz_protocol_keeloq_decoder,
    .encoder = &subghz_protocol_keeloq_encoder,
};

void* subghz_protocol_encoder_keeloq_alloc(SubGhzEnvironment* environment) {
    SubGhzProtocolEncoderKeeloq* instance = malloc(sizeof(SubGhzProtocolEncoderKeeloq));

    instance->base.protocol = &subghz_protocol_keeloq;
    instance->generic.protocol_name = instance->base.protocol->name;
    instance->keystore = subghz_environment_get_keystore(environment);

    instance->encoder.repeat = 10;
    instance->encoder.size_upload = 256;
    instance->encoder.upload = malloc(instance->encoder.size_upload * sizeof(LevelDuration));
    instance->encoder.is_runing = false;
    return instance;
}

void subghz_protocol_encoder_keeloq_free(void* context) {
    furi_assert(context);
    SubGhzProtocolEncoderKeeloq* instance = context;
    free(instance->encoder.upload);
    free(instance);
}

static bool subghz_protocol_keeloq_gen_data(SubGhzProtocolEncoderKeeloq* instance, uint8_t btn) {
    instance->generic.cnt++;
    uint32_t fix = btn << 28 | instance->generic.serial;
    uint32_t decrypt = btn << 28 |
                       (instance->generic.serial & 0x3FF)
                           << 16 | //ToDo in some protocols the discriminator is 0
                       instance->generic.cnt;
    uint32_t hop = 0;
    uint64_t man = 0;
    int res = 0;

    for
        M_EACH(manufacture_code, *subghz_keystore_get_data(instance->keystore), SubGhzKeyArray_t) {
            res = strcmp(string_get_cstr(manufacture_code->name), instance->manufacture_name);
            if(res == 0) {
                switch(manufacture_code->type) {
                case KEELOQ_LEARNING_SIMPLE:
                    //Simple Learning
                    hop = subghz_protocol_keeloq_common_encrypt(decrypt, manufacture_code->key);
                    break;
                case KEELOQ_LEARNING_NORMAL:
                    //Simple Learning
                    man =
                        subghz_protocol_keeloq_common_normal_learning(fix, manufacture_code->key);
                    hop = subghz_protocol_keeloq_common_encrypt(decrypt, man);
                    break;
                case KEELOQ_LEARNING_MAGIC_XOR_TYPE_1:
                    man = subghz_protocol_keeloq_common_magic_xor_type1_learning(
                        instance->generic.serial, manufacture_code->key);
                    hop = subghz_protocol_keeloq_common_encrypt(decrypt, man);
                    break;
                case KEELOQ_LEARNING_UNKNOWN:
                    hop = 0; //todo
                    break;
                }
                break;
            }
        }
    if(hop) {
        uint64_t yek = (uint64_t)fix << 32 | hop;
        instance->generic.data =
            subghz_protocol_blocks_reverse_key(yek, instance->generic.data_count_bit);
        return true;
    } else {
        instance->manufacture_name = "Unknown";
        return false;
    }
}

static bool
    subghz_protocol_keeloq_encoder_get_upload(SubGhzProtocolEncoderKeeloq* instance, uint8_t btn) {
    furi_assert(instance);

    //gen new key
    if(subghz_protocol_keeloq_gen_data(instance, btn)) {
        //ToDo Update display data
        // if(instance->common.callback)
        //     instance->common.callback((SubGhzProtocolCommon*)instance, instance->common.context);
    } else {
        return false;
    }

    size_t index = 0;
    size_t size_upload = 11 * 2 + 2 + (instance->generic.data_count_bit * 2) + 4;
    if(size_upload > instance->encoder.size_upload) {
        FURI_LOG_E(TAG, "Size upload exceeds allocated encoder buffer.");
        return false;
    } else {
        instance->encoder.size_upload = size_upload;
    }

    //Send header
    for(uint8_t i = 11; i > 0; i--) {
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_keeloq_const.te_short);
        instance->encoder.upload[index++] =
            level_duration_make(false, (uint32_t)subghz_protocol_keeloq_const.te_short);
    }
    instance->encoder.upload[index++] =
        level_duration_make(true, (uint32_t)subghz_protocol_keeloq_const.te_short);
    instance->encoder.upload[index++] =
        level_duration_make(false, (uint32_t)subghz_protocol_keeloq_const.te_short * 10);

    //Send key data
    for(uint8_t i = instance->generic.data_count_bit; i > 0; i--) {
        if(bit_read(instance->generic.data, i - 1)) {
            //send bit 1
            instance->encoder.upload[index++] =
                level_duration_make(true, (uint32_t)subghz_protocol_keeloq_const.te_short);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_keeloq_const.te_long);
        } else {
            //send bit 0
            instance->encoder.upload[index++] =
                level_duration_make(true, (uint32_t)subghz_protocol_keeloq_const.te_long);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_keeloq_const.te_short);
        }
    }
    // +send 2 status bit
    instance->encoder.upload[index++] =
        level_duration_make(true, (uint32_t)subghz_protocol_keeloq_const.te_short);
    instance->encoder.upload[index++] =
        level_duration_make(false, (uint32_t)subghz_protocol_keeloq_const.te_long);
    // send end
    instance->encoder.upload[index++] =
        level_duration_make(true, (uint32_t)subghz_protocol_keeloq_const.te_short);
    instance->encoder.upload[index++] =
        level_duration_make(false, (uint32_t)subghz_protocol_keeloq_const.te_short * 40);

    return true;
}

bool subghz_protocol_encoder_keeloq_load(
    void* context,
    uint64_t key,
    uint8_t count_bit,
    size_t repeat) {
    furi_assert(context);
    SubGhzProtocolEncoderKeeloq* instance = context;
    instance->generic.data = 0x65AB6CED8329BE24;
    instance->generic.serial = 0x47D94C1;
    instance->generic.btn = 0x02;
    instance->generic.cnt = 0x0319;
    instance->generic.data_count_bit = 64;
    instance->encoder.repeat = repeat;
    instance->manufacture_name = "DoorHan";
    subghz_protocol_keeloq_encoder_get_upload(instance, instance->generic.btn);
    instance->encoder.is_runing = true;
    return true;
}

void subghz_protocol_encoder_keeloq_stop(void* context) {
    SubGhzProtocolEncoderKeeloq* instance = context;
    instance->encoder.is_runing = false;
}

LevelDuration subghz_protocol_encoder_keeloq_yield(void* context) {
    SubGhzProtocolEncoderKeeloq* instance = context;

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

void* subghz_protocol_decoder_keeloq_alloc(SubGhzEnvironment* environment) {
    SubGhzProtocolDecoderKeeloq* instance = malloc(sizeof(SubGhzProtocolDecoderKeeloq));
    instance->base.protocol = &subghz_protocol_keeloq;
    instance->generic.protocol_name = instance->base.protocol->name;
    instance->keystore = subghz_environment_get_keystore(environment);

    return instance;
}

void subghz_protocol_decoder_keeloq_free(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderKeeloq* instance = context;

    free(instance);
}

void subghz_protocol_decoder_keeloq_reset(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderKeeloq* instance = context;
    instance->decoder.parser_step = KeeloqDecoderStepReset;
}

void subghz_protocol_decoder_keeloq_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    SubGhzProtocolDecoderKeeloq* instance = context;

    switch(instance->decoder.parser_step) {
    case KeeloqDecoderStepReset:
        if((level) && DURATION_DIFF(duration, subghz_protocol_keeloq_const.te_short) <
                          subghz_protocol_keeloq_const.te_delta) {
            instance->decoder.parser_step = KeeloqDecoderStepCheckPreambula;
            instance->header_count++;
        }
        break;
    case KeeloqDecoderStepCheckPreambula:
        if((!level) && (DURATION_DIFF(duration, subghz_protocol_keeloq_const.te_short) <
                        subghz_protocol_keeloq_const.te_delta)) {
            instance->decoder.parser_step = KeeloqDecoderStepReset;
            break;
        }
        if((instance->header_count > 2) &&
           (DURATION_DIFF(duration, subghz_protocol_keeloq_const.te_short * 10) <
            subghz_protocol_keeloq_const.te_delta * 10)) {
            // Found header
            instance->decoder.parser_step = KeeloqDecoderStepSaveDuration;
            instance->decoder.decode_data = 0;
            instance->decoder.decode_count_bit = 0;
        } else {
            instance->decoder.parser_step = KeeloqDecoderStepReset;
            instance->header_count = 0;
        }
        break;
    case KeeloqDecoderStepSaveDuration:
        if(level) {
            instance->decoder.te_last = duration;
            instance->decoder.parser_step = KeeloqDecoderStepCheckDuration;
        }
        break;
    case KeeloqDecoderStepCheckDuration:
        if(!level) {
            if(duration >= (subghz_protocol_keeloq_const.te_short * 2 +
                            subghz_protocol_keeloq_const.te_delta)) {
                // Found end TX
                instance->decoder.parser_step = KeeloqDecoderStepReset;
                if(instance->decoder.decode_count_bit >=
                   subghz_protocol_keeloq_const.min_count_bit_for_found) {
                    if(instance->generic.data != instance->decoder.decode_data) {
                        instance->generic.data = instance->decoder.decode_data;
                        instance->generic.data_count_bit = instance->decoder.decode_count_bit;
                        if(instance->base.callback)
                            instance->base.callback(&instance->base, instance->base.context);
                    }
                    instance->decoder.decode_data = 0;
                    instance->decoder.decode_count_bit = 0;
                    instance->header_count = 0;
                }
                break;
            } else if(
                (DURATION_DIFF(instance->decoder.te_last, subghz_protocol_keeloq_const.te_short) <
                 subghz_protocol_keeloq_const.te_delta) &&
                (DURATION_DIFF(duration, subghz_protocol_keeloq_const.te_long) <
                 subghz_protocol_keeloq_const.te_delta)) {
                if(instance->decoder.decode_count_bit <
                   subghz_protocol_keeloq_const.min_count_bit_for_found) {
                    subghz_protocol_blocks_add_bit(&instance->decoder, 1);
                }
                instance->decoder.parser_step = KeeloqDecoderStepSaveDuration;
            } else if(
                (DURATION_DIFF(instance->decoder.te_last, subghz_protocol_keeloq_const.te_long) <
                 subghz_protocol_keeloq_const.te_delta) &&
                (DURATION_DIFF(duration, subghz_protocol_keeloq_const.te_short) <
                 subghz_protocol_keeloq_const.te_delta)) {
                if(instance->decoder.decode_count_bit <
                   subghz_protocol_keeloq_const.min_count_bit_for_found) {
                    subghz_protocol_blocks_add_bit(&instance->decoder, 0);
                }
                instance->decoder.parser_step = KeeloqDecoderStepSaveDuration;
            } else {
                instance->decoder.parser_step = KeeloqDecoderStepReset;
                instance->header_count = 0;
            }
        } else {
            instance->decoder.parser_step = KeeloqDecoderStepReset;
            instance->header_count = 0;
        }
        break;
    }
}

static inline bool subghz_protocol_keeloq_check_decrypt(
    SubGhzBlockGeneric* instance,
    uint32_t decrypt,
    uint8_t btn,
    uint32_t end_serial) {
    furi_assert(instance);
    if((decrypt >> 28 == btn) && (((((uint16_t)(decrypt >> 16)) & 0xFF) == end_serial) ||
                                  ((((uint16_t)(decrypt >> 16)) & 0xFF) == 0))) {
        instance->cnt = decrypt & 0x0000FFFF;
        return true;
    }
    return false;
}

/** Checking the accepted code against the database manafacture key
 * 
 * @param instance SubGhzProtocolKeeloq instance
 * @param fix fix part of the parcel
 * @param hop hop encrypted part of the parcel
 * @return true on successful search
 */
static uint8_t subghz_protocol_keeloq_check_remote_controller_selector(
    SubGhzBlockGeneric* instance,
    uint32_t fix,
    uint32_t hop,
    SubGhzKeystore* keystore,
    const char** manufacture_name) {
    // protocol HCS300 uses 10 bits in discriminator, HCS200 uses 8 bits, for backward compatibility, we are looking for the 8-bit pattern
    // HCS300 -> uint16_t end_serial = (uint16_t)(fix & 0x3FF);
    // HCS200 -> uint16_t end_serial = (uint16_t)(fix & 0xFF);

    uint16_t end_serial = (uint16_t)(fix & 0xFF);
    uint8_t btn = (uint8_t)(fix >> 28);
    uint32_t decrypt = 0;
    uint64_t man;
    uint32_t seed = 0;

    for
        M_EACH(manufacture_code, *subghz_keystore_get_data(keystore), SubGhzKeyArray_t) {
            switch(manufacture_code->type) {
            case KEELOQ_LEARNING_SIMPLE:
                // Simple Learning
                decrypt = subghz_protocol_keeloq_common_decrypt(hop, manufacture_code->key);
                if(subghz_protocol_keeloq_check_decrypt(instance, decrypt, btn, end_serial)) {
                    *manufacture_name = string_get_cstr(manufacture_code->name);
                    return 1;
                }
                break;
            case KEELOQ_LEARNING_NORMAL:
                // Normal Learning
                // https://phreakerclub.com/forum/showpost.php?p=43557&postcount=37
                man = subghz_protocol_keeloq_common_normal_learning(fix, manufacture_code->key);
                decrypt = subghz_protocol_keeloq_common_decrypt(hop, man);
                if(subghz_protocol_keeloq_check_decrypt(instance, decrypt, btn, end_serial)) {
                    *manufacture_name = string_get_cstr(manufacture_code->name);
                    return 1;
                }
                break;
            case KEELOQ_LEARNING_SECURE:
                man = subghz_protocol_keeloq_common_secure_learning(
                    fix, seed, manufacture_code->key);
                decrypt = subghz_protocol_keeloq_common_decrypt(hop, man);
                if(subghz_protocol_keeloq_check_decrypt(instance, decrypt, btn, end_serial)) {
                    *manufacture_name = string_get_cstr(manufacture_code->name);
                    return 1;
                }
                break;
            case KEELOQ_LEARNING_MAGIC_XOR_TYPE_1:
                man = subghz_protocol_keeloq_common_magic_xor_type1_learning(
                    fix, manufacture_code->key);
                decrypt = subghz_protocol_keeloq_common_decrypt(hop, man);
                if(subghz_protocol_keeloq_check_decrypt(instance, decrypt, btn, end_serial)) {
                    *manufacture_name = string_get_cstr(manufacture_code->name);
                    return 1;
                }
                break;
            case KEELOQ_LEARNING_UNKNOWN:
                // Simple Learning
                decrypt = subghz_protocol_keeloq_common_decrypt(hop, manufacture_code->key);
                if(subghz_protocol_keeloq_check_decrypt(instance, decrypt, btn, end_serial)) {
                    *manufacture_name = string_get_cstr(manufacture_code->name);
                    return 1;
                }
                // Check for mirrored man
                uint64_t man_rev = 0;
                uint64_t man_rev_byte = 0;
                for(uint8_t i = 0; i < 64; i += 8) {
                    man_rev_byte = (uint8_t)(manufacture_code->key >> i);
                    man_rev = man_rev | man_rev_byte << (56 - i);
                }
                decrypt = subghz_protocol_keeloq_common_decrypt(hop, man_rev);
                if(subghz_protocol_keeloq_check_decrypt(instance, decrypt, btn, end_serial)) {
                    *manufacture_name = string_get_cstr(manufacture_code->name);
                    return 1;
                }
                //###########################
                // Normal Learning
                // https://phreakerclub.com/forum/showpost.php?p=43557&postcount=37
                man = subghz_protocol_keeloq_common_normal_learning(fix, manufacture_code->key);
                decrypt = subghz_protocol_keeloq_common_decrypt(hop, man);
                if(subghz_protocol_keeloq_check_decrypt(instance, decrypt, btn, end_serial)) {
                    *manufacture_name = string_get_cstr(manufacture_code->name);
                    return 1;
                }

                // Check for mirrored man
                man = subghz_protocol_keeloq_common_normal_learning(fix, man_rev);
                decrypt = subghz_protocol_keeloq_common_decrypt(hop, man);
                if(subghz_protocol_keeloq_check_decrypt(instance, decrypt, btn, end_serial)) {
                    *manufacture_name = string_get_cstr(manufacture_code->name);
                    return 1;
                }

                // Secure Learning
                man = subghz_protocol_keeloq_common_secure_learning(
                    fix, seed, manufacture_code->key);
                decrypt = subghz_protocol_keeloq_common_decrypt(hop, man);
                if(subghz_protocol_keeloq_check_decrypt(instance, decrypt, btn, end_serial)) {
                    *manufacture_name = string_get_cstr(manufacture_code->name);
                    return 1;
                }

                // Check for mirrored man
                man = subghz_protocol_keeloq_common_secure_learning(fix, seed, man_rev);
                decrypt = subghz_protocol_keeloq_common_decrypt(hop, man);
                if(subghz_protocol_keeloq_check_decrypt(instance, decrypt, btn, end_serial)) {
                    *manufacture_name = string_get_cstr(manufacture_code->name);
                    return 1;
                }

                // Magic xor type1 learning
                man = subghz_protocol_keeloq_common_magic_xor_type1_learning(
                    fix, manufacture_code->key);
                decrypt = subghz_protocol_keeloq_common_decrypt(hop, man);
                if(subghz_protocol_keeloq_check_decrypt(instance, decrypt, btn, end_serial)) {
                    *manufacture_name = string_get_cstr(manufacture_code->name);
                    return 1;
                }

                // Check for mirrored man
                man = subghz_protocol_keeloq_common_magic_xor_type1_learning(fix, man_rev);
                decrypt = subghz_protocol_keeloq_common_decrypt(hop, man);
                if(subghz_protocol_keeloq_check_decrypt(instance, decrypt, btn, end_serial)) {
                    *manufacture_name = string_get_cstr(manufacture_code->name);
                    return 1;
                }
                break;
            }
        }

    *manufacture_name = "Unknown";
    instance->cnt = 0;

    return 0;
}

/** Analysis of received data
 * 
 * @param instance SubGhzProtocolKeeloq instance
 */
static void subghz_protocol_keeloq_check_remote_controller(
    SubGhzBlockGeneric* instance,
    SubGhzKeystore* keystore,
    const char** manufacture_name) {
    uint64_t key = subghz_protocol_blocks_reverse_key(instance->data, instance->data_count_bit);
    uint32_t key_fix = key >> 32;
    uint32_t key_hop = key & 0x00000000ffffffff;
    // Check key AN-Motors
    if((key_hop >> 24) == ((key_hop >> 16) & 0x00ff) &&
       (key_fix >> 28) == ((key_hop >> 12) & 0x0f) && (key_hop & 0xFFF) == 0x404) {
        *manufacture_name = "AN-Motors";
        instance->cnt = key_hop >> 16;
    } else if((key_hop & 0xFFF) == (0x000) && (key_fix >> 28) == ((key_hop >> 12) & 0x0f)) {
        *manufacture_name = "HCS101";
        instance->cnt = key_hop >> 16;
    } else {
        subghz_protocol_keeloq_check_remote_controller_selector(
            instance, key_fix, key_hop, keystore, manufacture_name);
    }

    instance->serial = key_fix & 0x0FFFFFFF;
    instance->btn = key_fix >> 28;
}

// const char* subghz_protocol_keeloq_find_and_get_manufacture_name(void* context) {
//     SubGhzProtocolKeeloq* instance = context;
//     subghz_protocol_keeloq_check_remote_controller(instance);
//     return instance->manufacture_name;
// }

// const char* subghz_protocol_keeloq_get_manufacture_name(void* context) {
//     SubGhzProtocolKeeloq* instance = context;
//     return instance->manufacture_name;
// }

// bool subghz_protocol_keeloq_set_manufacture_name(void* context, const char* manufacture_name) {
//     SubGhzProtocolKeeloq* instance = context;
//     instance->manufacture_name = manufacture_name;
//     int res = 0;
//         for
//             M_EACH(
//                 manufacture_code,
//                 *subghz_keystore_get_data(instance->keystore),
//                 SubGhzKeyArray_t) {
//                 res = strcmp(string_get_cstr(manufacture_code->name), instance->manufacture_name);
//                 if(res == 0) return true;
//             }
//         instance->manufacture_name = "Unknown";
//         return false;
// }

void subghz_protocol_decoder_keeloq_serialization(void* context, string_t output) {
    furi_assert(context);
    SubGhzProtocolDecoderKeeloq* instance = context;
    subghz_protocol_keeloq_check_remote_controller(
        &instance->generic, instance->keystore, &instance->manufacture_name);

    uint32_t code_found_hi = instance->generic.data >> 32;
    uint32_t code_found_lo = instance->generic.data & 0x00000000ffffffff;

    uint64_t code_found_reverse = subghz_protocol_blocks_reverse_key(
        instance->generic.data, instance->generic.data_count_bit);
    uint32_t code_found_reverse_hi = code_found_reverse >> 32;
    uint32_t code_found_reverse_lo = code_found_reverse & 0x00000000ffffffff;

    string_cat_printf(
        output,
        "%s %dbit\r\n"
        "Key:%08lX%08lX\r\n"
        "Fix:0x%08lX    Cnt:%04X\r\n"
        "Hop:0x%08lX    Btn:%01lX\r\n"
        "MF:%s\r\n"
        "Sn:0x%07lX \r\n",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        code_found_hi,
        code_found_lo,
        code_found_reverse_hi,
        instance->generic.cnt,
        code_found_reverse_lo,
        instance->generic.btn,
        instance->manufacture_name,
        instance->generic.serial);
}

bool subghz_protocol_keeloq_save_file(void* context, FlipperFormat* flipper_file) {
    furi_assert(context);
    SubGhzProtocolDecoderKeeloq* instance = context;
    return subghz_block_generic_save_file(&instance->generic, flipper_file);
}

bool subghz_protocol_keeloq_load_file(
    void* context,
    FlipperFormat* flipper_file,
    const char* file_path) {
    furi_assert(context);
    SubGhzProtocolEncoderKeeloq* instance = context;
    return subghz_block_generic_load_file(&instance->generic, flipper_file);
}

// void subghz_decoder_keeloq_to_load_protocol(SubGhzProtocolKeeloq* instance, void* context) {
//     furi_assert(context);
//     furi_assert(instance);
//     SubGhzProtocolCommonLoad* data = context;
//     instance->generic.data = data->code_found;
//     instance->generic.data_count_bit = data->code_count_bit;
//     subghz_protocol_keeloq_check_remote_controller(instance);
// }