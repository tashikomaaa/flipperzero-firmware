#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "irda.h"
#include "common/irda_common_i.h"

/***************************************************************************************************
*   NEC protocol description
*   https://radioparty.ru/manuals/encyclopedia/213-ircontrol?start=1
****************************************************************************************************
*     Preamble   Preamble      Pulse Distance/Width          Pause       Preamble   Preamble  Stop
*       mark      space            Modulation             up to period    repeat     repeat    bit
*                                                                          mark       space
*
*        9000      4500         32 bit + stop bit         ...110000         9000       2250
*     __________          _ _ _ _  _  _  _ _ _  _  _ _ _                ___________            _
* ____          __________ _ _ _ __ __ __ _ _ __ __ _ _ ________________           ____________ ___
*
***************************************************************************************************/

#define IRDA_NEC_PREAMBLE_MARK 9000
#define IRDA_NEC_PREAMBLE_SPACE 4500
#define IRDA_NEC_BIT1_MARK 560
#define IRDA_NEC_BIT1_SPACE 1690
#define IRDA_NEC_BIT0_MARK 560
#define IRDA_NEC_BIT0_SPACE 560
#define IRDA_NEC_REPEAT_PERIOD 110000
#define IRDA_NEC_SILENCE IRDA_NEC_REPEAT_PERIOD
#define IRDA_NEC_MIN_SPLIT_TIME IRDA_NEC_REPEAT_PAUSE_MIN
#define IRDA_NEC_REPEAT_PAUSE_MIN 4000
#define IRDA_NEC_REPEAT_PAUSE_MAX 150000
#define IRDA_NEC_REPEAT_MARK 9000
#define IRDA_NEC_REPEAT_SPACE 2250
#define IRDA_NEC_PREAMBLE_TOLERANCE 200 // us
#define IRDA_NEC_BIT_TOLERANCE 120 // us

void* irda_decoder_nec_alloc(void);
void irda_decoder_nec_reset(void* decoder);
void irda_decoder_nec_free(void* decoder);
IrdaMessage* irda_decoder_nec_check_ready(void* decoder);
IrdaMessage* irda_decoder_nec_decode(void* decoder, bool level, uint32_t duration);
void* irda_encoder_nec_alloc(void);
IrdaStatus irda_encoder_nec_encode(void* encoder_ptr, uint32_t* duration, bool* level);
void irda_encoder_nec_reset(void* encoder_ptr, const IrdaMessage* message);
void irda_encoder_nec_free(void* encoder_ptr);
bool irda_decoder_nec_interpret(IrdaCommonDecoder* decoder);
IrdaStatus irda_decoder_nec_decode_repeat(IrdaCommonDecoder* decoder);
IrdaStatus
    irda_encoder_nec_encode_repeat(IrdaCommonEncoder* encoder, uint32_t* duration, bool* level);
const IrdaProtocolSpecification* irda_nec_get_spec(IrdaProtocol protocol);

extern const IrdaCommonProtocolSpec protocol_nec;

/***************************************************************************************************
*   SAMSUNG32 protocol description
*   https://www.mikrocontroller.net/articles/IRMP_-_english#SAMSUNG
****************************************************************************************************
*  Preamble   Preamble     Pulse Distance/Width        Pause       Preamble   Preamble  Bit1  Stop
*    mark      space           Modulation                           repeat     repeat          bit
*                                                                    mark       space
*
*     4500      4500        32 bit + stop bit       40000/100000     4500       4500
*  __________          _  _ _  _  _  _ _ _  _  _ _                ___________            _    _
* _          __________ __ _ __ __ __ _ _ __ __ _ ________________           ____________ ____ ___
*
***************************************************************************************************/

#define IRDA_SAMSUNG_PREAMBLE_MARK 4500
#define IRDA_SAMSUNG_PREAMBLE_SPACE 4500
#define IRDA_SAMSUNG_BIT1_MARK 550
#define IRDA_SAMSUNG_BIT1_SPACE 1650
#define IRDA_SAMSUNG_BIT0_MARK 550
#define IRDA_SAMSUNG_BIT0_SPACE 550
#define IRDA_SAMSUNG_REPEAT_PAUSE_MIN 30000
#define IRDA_SAMSUNG_REPEAT_PAUSE1 46000
#define IRDA_SAMSUNG_REPEAT_PAUSE2 97000
/* Samsung silence have to be greater than REPEAT MAX
 * otherwise there can be problems during unit tests parsing
 * of some data. Real tolerances we don't know, but in real life
 * silence time should be greater than max repeat time. This is
 * because of similar preambule timings for repeat and first messages. */
#define IRDA_SAMSUNG_MIN_SPLIT_TIME 5000
#define IRDA_SAMSUNG_SILENCE 145000
#define IRDA_SAMSUNG_REPEAT_PAUSE_MAX 140000
#define IRDA_SAMSUNG_REPEAT_MARK 4500
#define IRDA_SAMSUNG_REPEAT_SPACE 4500
#define IRDA_SAMSUNG_PREAMBLE_TOLERANCE 200 // us
#define IRDA_SAMSUNG_BIT_TOLERANCE 120 // us

void* irda_decoder_samsung32_alloc(void);
void irda_decoder_samsung32_reset(void* decoder);
void irda_decoder_samsung32_free(void* decoder);
IrdaMessage* irda_decoder_samsung32_check_ready(void* ctx);
IrdaMessage* irda_decoder_samsung32_decode(void* decoder, bool level, uint32_t duration);
IrdaStatus irda_encoder_samsung32_encode(void* encoder_ptr, uint32_t* duration, bool* level);
void irda_encoder_samsung32_reset(void* encoder_ptr, const IrdaMessage* message);
void* irda_encoder_samsung32_alloc(void);
void irda_encoder_samsung32_free(void* encoder_ptr);
bool irda_decoder_samsung32_interpret(IrdaCommonDecoder* decoder);
IrdaStatus irda_decoder_samsung32_decode_repeat(IrdaCommonDecoder* decoder);
IrdaStatus irda_encoder_samsung32_encode_repeat(
    IrdaCommonEncoder* encoder,
    uint32_t* duration,
    bool* level);
const IrdaProtocolSpecification* irda_samsung32_get_spec(IrdaProtocol protocol);

extern const IrdaCommonProtocolSpec protocol_samsung32;

/***************************************************************************************************
*   RC6 protocol description
*   https://www.mikrocontroller.net/articles/IRMP_-_english#RC6_.2B_RC6A
****************************************************************************************************
*      Preamble                       Manchester/biphase                       Silence
*     mark/space                          Modulation
*
*    2666     889        444/888 - bit (x2 for toggle bit)                       2666
*
*  ________         __    __  __  __    ____  __  __  __  __  __  __  __  __
* _        _________  ____  __  __  ____    __  __  __  __  __  __  __  __  _______________
*                   | 1 | 0 | 0 | 0 |   0   |      ...      |      ...      |             |
*                     s  m2  m1  m0     T     address (MSB)   command (MSB)
*
*    s - start bit (always 1)
*    m0-2 - mode (000 for RC6)
*    T - toggle bit, twice longer
*    address - 8 bit
*    command - 8 bit
***************************************************************************************************/

#define IRDA_RC6_CARRIER_FREQUENCY 36000
#define IRDA_RC6_DUTY_CYCLE 0.33

#define IRDA_RC6_PREAMBLE_MARK 2666
#define IRDA_RC6_PREAMBLE_SPACE 889
#define IRDA_RC6_BIT 444 // half of time-quant for 1 bit
#define IRDA_RC6_PREAMBLE_TOLERANCE 200 // us
#define IRDA_RC6_BIT_TOLERANCE 120 // us
/* protocol allows 2700 silence, but it is hard to send 1 message without repeat */
#define IRDA_RC6_SILENCE (2700 * 10)
#define IRDA_RC6_MIN_SPLIT_TIME 2700

void* irda_decoder_rc6_alloc(void);
void irda_decoder_rc6_reset(void* decoder);
void irda_decoder_rc6_free(void* decoder);
IrdaMessage* irda_decoder_rc6_check_ready(void* ctx);
IrdaMessage* irda_decoder_rc6_decode(void* decoder, bool level, uint32_t duration);
void* irda_encoder_rc6_alloc(void);
void irda_encoder_rc6_reset(void* encoder_ptr, const IrdaMessage* message);
void irda_encoder_rc6_free(void* decoder);
IrdaStatus irda_encoder_rc6_encode(void* encoder_ptr, uint32_t* duration, bool* polarity);
bool irda_decoder_rc6_interpret(IrdaCommonDecoder* decoder);
IrdaStatus
    irda_decoder_rc6_decode_manchester(IrdaCommonDecoder* decoder, bool level, uint32_t timing);
IrdaStatus irda_encoder_rc6_encode_manchester(
    IrdaCommonEncoder* encoder_ptr,
    uint32_t* duration,
    bool* polarity);
const IrdaProtocolSpecification* irda_rc6_get_spec(IrdaProtocol protocol);

extern const IrdaCommonProtocolSpec protocol_rc6;

/***************************************************************************************************
*   RC5 protocol description
*   https://www.mikrocontroller.net/articles/IRMP_-_english#RC5_.2B_RC5X
****************************************************************************************************
*                                       Manchester/biphase
*                                           Modulation
*
*                              888/1776 - bit (x2 for toggle bit)
*
*                           __  ____    __  __  __  __  __  __  __  __
*                         __  __    ____  __  __  __  __  __  __  __  _
*                         | 1 | 1 | 0 |      ...      |      ...      |
*                           s  si   T   address (MSB)   command (MSB)
*
*    Note: manchester starts from space timing, so it have to be handled properly
*    s - start bit (always 1)
*    si - RC5: start bit (always 1), RC5X - 7-th bit of address (in our case always 0)
*    T - toggle bit, change it's value every button press
*    address - 5 bit
*    command - 6/7 bit
***************************************************************************************************/

#define IRDA_RC5_CARRIER_FREQUENCY 36000
#define IRDA_RC5_DUTY_CYCLE 0.33

#define IRDA_RC5_PREAMBLE_MARK 0
#define IRDA_RC5_PREAMBLE_SPACE 0
#define IRDA_RC5_BIT 888 // half of time-quant for 1 bit
#define IRDA_RC5_PREAMBLE_TOLERANCE 200 // us
#define IRDA_RC5_BIT_TOLERANCE 120 // us
/* protocol allows 2700 silence, but it is hard to send 1 message without repeat */
#define IRDA_RC5_SILENCE (2700 * 10)
#define IRDA_RC5_MIN_SPLIT_TIME 2700

void* irda_decoder_rc5_alloc(void);
void irda_decoder_rc5_reset(void* decoder);
void irda_decoder_rc5_free(void* decoder);
IrdaMessage* irda_decoder_rc5_check_ready(void* ctx);
IrdaMessage* irda_decoder_rc5_decode(void* decoder, bool level, uint32_t duration);
void* irda_encoder_rc5_alloc(void);
void irda_encoder_rc5_reset(void* encoder_ptr, const IrdaMessage* message);
void irda_encoder_rc5_free(void* decoder);
IrdaStatus irda_encoder_rc5_encode(void* encoder_ptr, uint32_t* duration, bool* polarity);
bool irda_decoder_rc5_interpret(IrdaCommonDecoder* decoder);
const IrdaProtocolSpecification* irda_rc5_get_spec(IrdaProtocol protocol);

extern const IrdaCommonProtocolSpec protocol_rc5;

/***************************************************************************************************
*   Sony SIRC protocol description
*   https://www.sbprojects.net/knowledge/ir/sirc.php
*   http://picprojects.org.uk/
****************************************************************************************************
*      Preamble  Preamble     Pulse Width Modulation           Pause             Entirely repeat
*        mark     space                                     up to period             message..
*
*        2400      600      12/15/20 bits (600,1200)         ...45000          2400      600
*     __________          _ _ _ _  _  _  _ _ _  _  _ _ _                    __________          _ _
* ____          __________ _ _ _ __ __ __ _ _ __ __ _ _ ____________________          __________ _
*                        |    command    |   address    |
*                 SIRC   |     7b LSB    |    5b LSB    |
*                 SIRC15 |     7b LSB    |    8b LSB    |
*                 SIRC20 |     7b LSB    |    13b LSB   |
*
* No way to determine either next message is repeat or not,
* so recognize only fact message received. Sony remotes always send at least 3 messages.
* Assume 8 last extended bits for SIRC20 are address bits.
***************************************************************************************************/

#define IRDA_SIRC_CARRIER_FREQUENCY 40000
#define IRDA_SIRC_DUTY_CYCLE 0.33
#define IRDA_SIRC_PREAMBLE_MARK 2400
#define IRDA_SIRC_PREAMBLE_SPACE 600
#define IRDA_SIRC_BIT1_MARK 1200
#define IRDA_SIRC_BIT1_SPACE 600
#define IRDA_SIRC_BIT0_MARK 600
#define IRDA_SIRC_BIT0_SPACE 600
#define IRDA_SIRC_PREAMBLE_TOLERANCE 200 // us
#define IRDA_SIRC_BIT_TOLERANCE 120 // us
#define IRDA_SIRC_SILENCE 10000
#define IRDA_SIRC_MIN_SPLIT_TIME (IRDA_SIRC_SILENCE - 1000)
#define IRDA_SIRC_REPEAT_PERIOD 45000

void* irda_decoder_sirc_alloc(void);
void irda_decoder_sirc_reset(void* decoder);
IrdaMessage* irda_decoder_sirc_check_ready(void* decoder);
uint32_t irda_decoder_sirc_get_timeout(void* decoder);
void irda_decoder_sirc_free(void* decoder);
IrdaMessage* irda_decoder_sirc_decode(void* decoder, bool level, uint32_t duration);
void* irda_encoder_sirc_alloc(void);
void irda_encoder_sirc_reset(void* encoder_ptr, const IrdaMessage* message);
void irda_encoder_sirc_free(void* decoder);
IrdaStatus irda_encoder_sirc_encode(void* encoder_ptr, uint32_t* duration, bool* polarity);
bool irda_decoder_sirc_interpret(IrdaCommonDecoder* decoder);
const IrdaProtocolSpecification* irda_sirc_get_spec(IrdaProtocol protocol);
IrdaStatus
    irda_encoder_sirc_encode_repeat(IrdaCommonEncoder* encoder, uint32_t* duration, bool* level);

extern const IrdaCommonProtocolSpec protocol_sirc;
