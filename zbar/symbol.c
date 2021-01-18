/*------------------------------------------------------------------------
 *  Copyright 2007-2010 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the ZBar Bar Code Reader.
 *
 *  The ZBar Bar Code Reader is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The ZBar Bar Code Reader is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the ZBar Bar Code Reader; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zbar
 *------------------------------------------------------------------------*/

#include <assert.h>
#include <config.h>
#include <stdio.h>
#include <string.h>

#include "symbol.h"
#include <zbar.h>

const char* zbar_get_symbol_name(zbar_symbol_type_t sym) {
    switch (sym & ZBAR_SYMBOL) {
    case ZBAR_EAN2:
        return ("EAN-2");
    case ZBAR_EAN5:
        return ("EAN-5");
    case ZBAR_EAN8:
        return ("EAN-8");
    case ZBAR_UPCE:
        return ("UPC-E");
    case ZBAR_ISBN10:
        return ("ISBN-10");
    case ZBAR_UPCA:
        return ("UPC-A");
    case ZBAR_EAN13:
        return ("EAN-13");
    case ZBAR_ISBN13:
        return ("ISBN-13");
    case ZBAR_COMPOSITE:
        return ("COMPOSITE");
    case ZBAR_I25:
        return ("I2/5");
    case ZBAR_DATABAR:
        return ("DataBar");
    case ZBAR_DATABAR_EXP:
        return ("DataBar-Exp");
    case ZBAR_CODABAR:
        return ("Codabar");
    case ZBAR_CODE39:
        return ("CODE-39");
    case ZBAR_CODE93:
        return ("CODE-93");
    case ZBAR_CODE128:
        return ("CODE-128");
    case ZBAR_PDF417:
        return ("PDF417");
    case ZBAR_QRCODE:
        return ("QR-Code");
    case ZBAR_SQCODE:
        return ("SQ-Code");
    default:
        return ("UNKNOWN");
    }
}

const char* zbar_get_addon_name(zbar_symbol_type_t sym) { return (""); }

const char* zbar_get_config_name(zbar_config_t cfg) {
    switch (cfg) {
    case ZBAR_CFG_ENABLE:
        return ("ENABLE");
    case ZBAR_CFG_ADD_CHECK:
        return ("ADD_CHECK");
    case ZBAR_CFG_EMIT_CHECK:
        return ("EMIT_CHECK");
    case ZBAR_CFG_ASCII:
        return ("ASCII");
    case ZBAR_CFG_BINARY:
        return ("BINARY");
    case ZBAR_CFG_MIN_LEN:
        return ("MIN_LEN");
    case ZBAR_CFG_MAX_LEN:
        return ("MAX_LEN");
    case ZBAR_CFG_UNCERTAINTY:
        return ("UNCERTAINTY");
    case ZBAR_CFG_POSITION:
        return ("POSITION");
    case ZBAR_CFG_X_DENSITY:
        return ("X_DENSITY");
    case ZBAR_CFG_Y_DENSITY:
        return ("Y_DENSITY");
    default:
        return ("");
    }
}

const char* zbar_get_modifier_name(zbar_modifier_t mod) {
    switch (mod) {
    case ZBAR_MOD_GS1:
        return ("GS1");
    case ZBAR_MOD_AIM:
        return ("AIM");
    default:
        return ("");
    }
}

const char* zbar_get_orientation_name(zbar_orientation_t orient) {
    switch (orient) {
    case ZBAR_ORIENT_UP:
        return ("UP");
    case ZBAR_ORIENT_RIGHT:
        return ("RIGHT");
    case ZBAR_ORIENT_DOWN:
        return ("DOWN");
    case ZBAR_ORIENT_LEFT:
        return ("LEFT");
    default:
        return ("UNKNOWN");
    }
}

int _zbar_get_symbol_hash(zbar_symbol_type_t sym) {
    static const signed char hash[ZBAR_CODE128 + 1] = {
        [0] = -1,

        /* [ZBAR_FOO] = 0, is empty */
        [ZBAR_SQCODE] = 1,
        [ZBAR_CODE128] = 2,
        [ZBAR_EAN13] = 3,
        [ZBAR_UPCA] = 4,
        [ZBAR_EAN8] = 5,
        [ZBAR_UPCE] = 6,
        [ZBAR_ISBN13] = 7,
        [ZBAR_ISBN10] = 8,
        [ZBAR_CODE39] = 9,
        [ZBAR_I25] = 10,
        [ZBAR_PDF417] = 11,
        [ZBAR_QRCODE] = 12,
        [ZBAR_DATABAR] = 13,
        [ZBAR_DATABAR_EXP] = 14,
        [ZBAR_CODE93] = 15,
        [ZBAR_EAN2] = 16,
        [ZBAR_EAN5] = 17,
        [ZBAR_COMPOSITE] = 18,
        [ZBAR_CODABAR] = 19,

        /* Please update NUM_SYMS accordingly */
    };
    int h;

    assert(sym >= ZBAR_PARTIAL && sym <= ZBAR_CODE128);

    h = hash[sym];
    assert(h >= 0 && h < NUM_SYMS);

    return h;
}

void _zbar_symbol_free(zbar_symbol_t* sym) {
    if (sym->syms) {
        zbar_symbol_set_ref(sym->syms, -1);
        sym->syms = NULL;
    }
    if (sym->pts) free(sym->pts);
    if (sym->data_alloc && sym->data) free(sym->data);
    free(sym);
}

void zbar_symbol_ref(const zbar_symbol_t* sym, int refs) {
    zbar_symbol_t* ncsym = (zbar_symbol_t*) sym;
    _zbar_symbol_refcnt(ncsym, refs);
}

zbar_symbol_type_t zbar_symbol_get_type(const zbar_symbol_t* sym) {
    return (sym->type);
}

unsigned int zbar_symbol_get_configs(const zbar_symbol_t* sym) {
    return (sym->configs);
}

unsigned int zbar_symbol_get_modifiers(const zbar_symbol_t* sym) {
    return (sym->modifiers);
}

const char* zbar_symbol_get_data(const zbar_symbol_t* sym) {
    return (sym->data);
}

unsigned int zbar_symbol_get_data_length(const zbar_symbol_t* sym) {
    return (sym->datalen);
}

int zbar_symbol_get_count(const zbar_symbol_t* sym) {
    return (sym->cache_count);
}

int zbar_symbol_get_quality(const zbar_symbol_t* sym) { return (sym->quality); }

unsigned zbar_symbol_get_loc_size(const zbar_symbol_t* sym) {
    return (sym->npts);
}

int zbar_symbol_get_loc_x(const zbar_symbol_t* sym, unsigned idx) {
    if (idx < sym->npts)
        return (sym->pts[idx].x);
    else
        return (-1);
}

int zbar_symbol_get_loc_y(const zbar_symbol_t* sym, unsigned idx) {
    if (idx < sym->npts)
        return (sym->pts[idx].y);
    else
        return (-1);
}

zbar_orientation_t zbar_symbol_get_orientation(const zbar_symbol_t* sym) {
    return (sym->orient);
}

const zbar_symbol_t* zbar_symbol_next(const zbar_symbol_t* sym) {
    return ((sym) ? sym->next : NULL);
}

const zbar_symbol_set_t* zbar_symbol_get_components(const zbar_symbol_t* sym) {
    return (sym->syms);
}

const zbar_symbol_t* zbar_symbol_first_component(const zbar_symbol_t* sym) {
    return ((sym && sym->syms) ? sym->syms->head : NULL);
}

unsigned base64_encode(char* dst, const char* src, unsigned srclen) {
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char* start = dst;
    int nline = 19;
    for (; srclen; srclen -= 3) {
        unsigned int buf = *(src++) << 16;
        if (srclen > 1) buf |= *(src++) << 8;
        if (srclen > 2) buf |= *(src++);
        *(dst++) = alphabet[(buf >> 18) & 0x3f];
        *(dst++) = alphabet[(buf >> 12) & 0x3f];
        *(dst++) = (srclen > 1) ? alphabet[(buf >> 6) & 0x3f] : '=';
        *(dst++) = (srclen > 2) ? alphabet[buf & 0x3f] : '=';
        if (srclen < 3) break;
        if (!--nline) {
            *(dst++) = '\n';
            nline = 19;
        }
    }
    *(dst++) = '\n';
    *(dst++) = '\0';
    return ((unsigned) (dst - start - 1));
}

enum {
    TMPL_START,
    TMPL_MOD_START,
    TMPL_MOD_ITEM,
    TMPL_MOD_END,
    TMPL_COUNT,
    TMPL_DATA_START,
    TMPL_FORMAT,
    TMPL_CDATA,
    TMPL_NL,
    TMPL_END,
};

zbar_symbol_set_t* _zbar_symbol_set_create() {
    zbar_symbol_set_t* syms = calloc(1, sizeof(*syms));
    _zbar_refcnt(&syms->refcnt, 1);
    return (syms);
}

inline void _zbar_symbol_set_free(zbar_symbol_set_t* syms) {
    zbar_symbol_t *sym, *next;
    for (sym = syms->head; sym; sym = next) {
        next = sym->next;
        sym->next = NULL;
        _zbar_symbol_refcnt(sym, -1);
    }
    syms->head = NULL;
    free(syms);
}

void zbar_symbol_set_ref(const zbar_symbol_set_t* syms, int delta) {
    zbar_symbol_set_t* ncsyms = (zbar_symbol_set_t*) syms;
    if (!_zbar_refcnt(&ncsyms->refcnt, delta) && delta <= 0)
        _zbar_symbol_set_free(ncsyms);
}

int zbar_symbol_set_get_size(const zbar_symbol_set_t* syms) {
    return (syms->nsyms);
}

const zbar_symbol_t*
    zbar_symbol_set_first_symbol(const zbar_symbol_set_t* syms) {
    zbar_symbol_t* sym = syms->tail;
    if (sym) return (sym->next);
    return (syms->head);
}

const zbar_symbol_t*
    zbar_symbol_set_first_unfiltered(const zbar_symbol_set_t* syms) {
    return (syms->head);
}
