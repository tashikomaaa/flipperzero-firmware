#pragma once

#include "base.h"

#define SUBGHZ_PROTOCOL_NERO_RADIO_NAME "Nero Radio"

typedef struct SubGhzProtocolDecoderNeroRadio SubGhzProtocolDecoderNeroRadio;
typedef struct SubGhzProtocolEncoderNeroRadio SubGhzProtocolEncoderNeroRadio;

extern const SubGhzProtocolDecoder subghz_protocol_nero_radio_decoder;
extern const SubGhzProtocolEncoder subghz_protocol_nero_radio_encoder;
extern const SubGhzProtocol subghz_protocol_nero_radio;

void* subghz_protocol_encoder_nero_radio_alloc(SubGhzEnvironment* environment);
void subghz_protocol_encoder_nero_radio_free(void* context);
bool subghz_protocol_encoder_nero_radio_load(
    void* context,
    uint64_t key,
    uint8_t count_bit,
    size_t repeat);
void subghz_protocol_encoder_nero_radio_stop(void* context);
LevelDuration subghz_protocol_encoder_nero_radio_yield(void* context);
void* subghz_protocol_decoder_nero_radio_alloc(SubGhzEnvironment* environment);
void subghz_protocol_decoder_nero_radio_free(void* context);
void subghz_protocol_decoder_nero_radio_reset(void* context);
void subghz_protocol_decoder_nero_radio_feed(void* context, bool level, uint32_t duration);
void subghz_protocol_decoder_nero_radio_serialization(void* context, string_t output);
bool subghz_protocol_nero_radio_save_file(void* context, FlipperFile* flipper_file);
bool subghz_protocol_nero_radio_load_file(
    void* context,
    FlipperFile* flipper_file,
    const char* file_path);