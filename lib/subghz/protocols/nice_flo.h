#pragma once

#include "base.h"

#define SUBGHZ_PROTOCOL_NICE_FLO_NAME "Nice FLO"

typedef struct SubGhzProtocolDecoderNiceFlo SubGhzProtocolDecoderNiceFlo;
typedef struct SubGhzProtocolEncoderNiceFlo SubGhzProtocolEncoderNiceFlo;

extern const SubGhzProtocolDecoder subghz_protocol_nice_flo_decoder;
extern const SubGhzProtocolEncoder subghz_protocol_nice_flo_encoder;
extern const SubGhzProtocol subghz_protocol_nice_flo;

void* subghz_protocol_encoder_nice_flo_alloc(SubGhzEnvironment* environment);
void subghz_protocol_encoder_nice_flo_free(void* context);
bool subghz_protocol_encoder_nice_flo_load(
    void* context,
    uint64_t key,
    uint8_t count_bit,
    size_t repeat);
void subghz_protocol_encoder_nice_flo_stop(void* context);
LevelDuration subghz_protocol_encoder_nice_flo_yield(void* context);
void* subghz_protocol_decoder_nice_flo_alloc(SubGhzEnvironment* environment);
void subghz_protocol_decoder_nice_flo_free(void* context);
void subghz_protocol_decoder_nice_flo_reset(void* context);
void subghz_protocol_decoder_nice_flo_feed(void* context, bool level, uint32_t duration);
void subghz_protocol_decoder_nice_flo_serialization(void* context, string_t output);
bool subghz_protocol_nice_flo_save_file(void* context, FlipperFormat* flipper_file);
bool subghz_protocol_nice_flo_load_file(
    void* context,
    FlipperFormat* flipper_file,
    const char* file_path);