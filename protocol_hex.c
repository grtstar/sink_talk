#include <stdio.h>
#include <string.h>
#include <vm.h>
#include <panic.h>
#include <connection.h>
#include <ps.h>
#include <panic.h>
#include <stream.h>
#include <source.h>
#include <sink.h>
#include <message.h>
#include <boot.h>
#include <pio.h>


#include "protocol_hex.h"

enum
{
    PH_CHECK_HEADER,
    PH_CHECK_SEG
};

void ProtoHexInit(ProtoHex* ph)
{
    int i= 0;
    ph->varDataPos = 0;
    ph->varDataLen = 0;
    for (i = 0; i < ph->segCount; i++)
    {
        if (ph->segs[i].type == SEG_VARDATE_LEN)
        {
            ph->varDataLenPos = ph->segs[i].pos;
            ph->varDataLenLen = ph->segs[i].len;
        }
        if (ph->segs[i].type == SEG_VARDATA)
        {
            ph->varDataPos = ph->segs[i].pos;
        }
        ph->fixedDataLen += ph->segs[i].len;
    }
    ph->fixedDataLen += ph->header.len;
    if (ph->sendBuf)
    {
        memcpy(ph->sendBuf, ph->header.header, ph->header.len);
    }
}

static ProtoSeg* ProtoHexGetSeg(ProtoHex* ph, ProtoSegType type)
{
    int i= 0;
    ProtoSeg* seg = NULL;
    for (i = 0; i < ph->segCount; i++)
    {
        if (ph->segs[i].type == type)
        {
            seg = &ph->segs[i];
            return seg;
        }
    }
    return NULL;
}

int ProtoHexGetSegValue(ProtoHex* ph, ProtoSegType type)
{
    int pos = 0;
    int value = 0;
    
    ProtoSeg* seg = ProtoHexGetSeg(ph, type);
    if (seg == NULL)
    {
        return 0;
    }

    pos = seg->pos;
    if (seg->pos >= ph->varDataPos && seg->type != SEG_VARDATA)
    {
        pos = ph->varDataLenPos + ph->varDataLen + (seg->pos - ph->varDataPos);
    }
    
    if (ph->byteOrder == LITTLE_ENDIAN)
    {
        switch (seg->len)
        {
        case 1:
            value = ph->buff[pos];
            break;
        case 2:
            value = ph->buff[pos] | ((uint32)ph->buff[pos + 1]) << 8;
            break;
        case 3:
            value = ph->buff[pos] | ((uint32)ph->buff[pos + 1]) << 8 | ((uint32)ph->buff[pos + 2]) << 16;
            break;
        case 4:
            value = ph->buff[pos] | ((uint32)ph->buff[pos + 1]) << 8 | ((uint32)ph->buff[pos + 2]) << 16 | ((uint32)ph->buff[pos + 3]) << 24;
            break;
        default:
            break;
        }
    }
    if (ph->byteOrder == BIG_ENDIAN)
    {
        switch (seg->len)
        {
        case 1:
            value = ph->buff[pos];
            break;
        case 2:
            value = (uint32)ph->buff[pos] << 8 | ph->buff[pos + 1];
            break;
        case 3:
            value = (uint32)ph->buff[pos] << 16 | (uint32)ph->buff[pos + 1] << 8 | ph->buff[pos + 2];
            break;
        case 4:
            value = (uint32)ph->buff[pos] << 24 | (uint32)ph->buff[pos + 1] << 16 | (uint32)ph->buff[pos + 2] << 8 | ph->buff[pos + 3];
            break;
        default:
            break;
        }
    }
    return value;
}

uint8_t* ProtoHexGetVarData(ProtoHex* ph)
{
    return &ph->buff[ph->varDataPos];
}

int ProtoHexGetVarLen(ProtoHex* ph)
{
    return ph->varDataLen;
}

bool ProtoHexParse(ProtoHex* ph, const uint8_t* d, int len)
{
    int i= 0;
    for (i = 0; i < len; i++)
    {
        switch (ph->state)
        {
        case PH_CHECK_HEADER:
            if (ph->header.header[ph->currIndex] == d[i])
            {
                ph->currIndex++;
            }
            else
            {
                ph->currIndex = 0;
            }

            if (ph->currIndex == ph->header.len)
            {
                ph->state = PH_CHECK_SEG;
                ph->totalLen = ph->fixedDataLen;
            }
            break;
        case PH_CHECK_SEG:
            ph->buff[ph->currIndex++] = d[i];
            if (ph->currIndex == ph->varDataLenPos + ph->varDataLenLen)
            {
              
                ph->varDataLen = ProtoHexGetSegValue(ph, SEG_VARDATE_LEN);
                ph->totalLen = ph->varDataLen + ph->fixedDataLen;
                if (ph->totalLen > ph->buffLen)
                {

                    ph->currIndex = 0;
                    ph->state = PH_CHECK_HEADER;
                    break;
                }
            }
            if (ph->totalLen == ph->currIndex)
            {
                ph->currIndex = 0;
                ph->state = PH_CHECK_HEADER;
                return TRUE;
            }
            break;
        default:
            break;
        }
    }
    return FALSE;
}

void ProtoHexPackSegValue(ProtoHex* ph, ProtoSegType type, int value)
{
    int pos = 0;
    ProtoSeg* seg = ProtoHexGetSeg(ph, type);
    if (seg == NULL)
    {
        return;
    }

    pos = seg->pos;
    if (seg->pos >= ph->varDataPos && seg->type != SEG_VARDATA)
    {
        pos += ph->varDataLen + (seg->pos - ph->varDataPos);
    }

    if (ph->byteOrder == LITTLE_ENDIAN)
    {
        switch (seg->len)
        {
        case 1:
            ph->sendBuf[pos] = value;
            break;
        case 2:
            ph->sendBuf[pos] = value;
            ph->sendBuf[pos + 1] = (uint32)value >> 8;
            break;
        case 3:
            ph->sendBuf[pos] = value;
            ph->sendBuf[pos + 1] = (uint32)value >> 8;
            ph->sendBuf[pos + 2] = (uint32)value >> 16;
            break;
        case 4:
            ph->sendBuf[pos] = value;
            ph->sendBuf[pos + 1] = (uint32)value >> 8;
            ph->sendBuf[pos + 2] = (uint32)value >> 16;
            ph->sendBuf[pos + 3] = (uint32)value >> 24;
            break;
        default:
            break;
        }
    }
    if (ph->byteOrder == BIG_ENDIAN)
    {
        switch (seg->len)
        {
        case 1:
            ph->sendBuf[pos] = value;
            break;
        case 2:
            ph->sendBuf[pos + 1] = (uint32)value;
            ph->sendBuf[pos + 0] = (uint32)value >> 8;
            break;
        case 3:
            ph->sendBuf[pos + 2] = (uint32)value;
            ph->sendBuf[pos + 1] = (uint32)value >> 8;
            ph->sendBuf[pos + 0] = (uint32)value >> 16;
            break;
        case 4:
            ph->sendBuf[pos + 3] = (uint32)value;
            ph->sendBuf[pos + 2] = (uint32)value >> 8;
            ph->sendBuf[pos + 1] = (uint32)value >> 16;
            ph->sendBuf[pos + 0] = (uint32)value >> 24;
            break;
        default:
            break;
        }
    }
}

void ProtoHexPackVarData(ProtoHex* ph, uint8_t* d, int len)
{
    if (len > ph->sendBufLen - ph->varDataPos)
    {
        len = ph->sendBufLen - ph->varDataPos;
    }
    memcpy(&ph->sendBuf[ph->varDataPos], d, len);
    ph->totalSendLen = ph->fixedDataLen + len;
    ph->varDataLen = len;

    ProtoHexPackSegValue(ph, SEG_VARDATE_LEN, len);
}


#ifdef UNIT_TEST

ProtoSeg    _seg[] = {
    {SEG_CHECKSUM, 4, 2},
    {SEG_CMD, 6, 2},
    {SEG_VARDATE_LEN, 8, 2},
    {SEG_VERSION, 10, 2},
    {SEG_VARDATA, 12, 0},      
};

uint8_t _protoBuff[128] = { 0 };

uint8_t _protoTestData[] = {
    0xAA, 0x55, 0xA5, 0x5A, 0x34, 0xFD, 0xC8, 0x05, 0x00, 0x00, 0x01, 0x00
};

ProtoHex _proto = {
    {{0xAA, 0x55, 0xA5, 0x5A}, 4},  
    BIG_ENDIAN                     
};

void ProtoHexTest(void)
{
    ProtoHexInit(&_proto, _seg, sizeof(_seg) / sizeof(_seg[0]), _protoBuff, sizeof(_protoBuff));
    if (ProtoHexParse(&_proto, _protoTestData, sizeof(_protoTestData)))
    {

    }
    else
    {

    }
}

#endif
