#pragma once

#include "base.h"

#define SUBGHZ_PROTOCOL_SOMFY_TELIS_NAME "Somfy Telis"

typedef struct SubGhzProtocolDecoderSomfyTelis SubGhzProtocolDecoderSomfyTelis;
typedef struct SubGhzProtocolEncoderSomfyTelis SubGhzProtocolEncoderSomfyTelis;

extern const SubGhzProtocolDecoder subghz_protocol_somfy_telis_decoder;
extern const SubGhzProtocolEncoder subghz_protocol_somfy_telis_encoder;
extern const SubGhzProtocol subghz_protocol_somfy_telis;

void* subghz_protocol_decoder_somfy_telis_alloc(SubGhzEnvironment* environment);
void subghz_protocol_decoder_somfy_telis_free(void* context);
void subghz_protocol_decoder_somfy_telis_reset(void* context);
void subghz_protocol_decoder_somfy_telis_feed(void* context, bool level, uint32_t duration);
void subghz_protocol_decoder_somfy_telis_serialization(void* context, string_t output);