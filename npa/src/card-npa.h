/*
 * Copyright (C) 2014 Frank Morgner
 *
 * This file is part of npa.
 *
 * npa is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * npa is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * npa.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CARD_NPA_H
#define _CARD_NPA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libopensc/opensc.h"

enum {
    SC_CARD_TYPE_NPA = 42000,
    SC_CARD_TYPE_NPA_TEST,
    SC_CARD_TYPE_NPA_ONLINE,
};

#define NPA_PIN_ID_ESIGN_PIN 0x83

const unsigned char esign_chat[] = {
    0x7F, 0x4C, 0x0E,
        0x06, 0x09, 0x04, 0x00, 0x7F, 0x00, 0x07, 0x03, 0x01, 0x02, 0x03,
        0x53, 0x01, 0x03,
};

static const unsigned char df_esign_aid[]  = { 0xa0, 0x00, 0x00, 0x01, 0x67, 0x45, 0x53, 0x49, 0x47, 0x4e};
static const unsigned char df_esign_path[] = { 0x3f, 0x00, 0x50, 0x15, 0x1f, 0xff};
static const unsigned char ef_cardaccess_path[] = { 0x3f, 0x00, 0x01, 0x1c};

#ifdef  __cplusplus
}
#endif
#endif
