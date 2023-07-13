
/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.

FILE NAME
    config_definition.c

DESCRIPTION 
    This file contains the configuration definition for the application.
    
    The const data structure contains all the default values for the
    different modules included in the build and must be passed to the 
    config store library as part of initialisation.

*/

/*********************************/
/* GENERATED FILE - DO NOT EDIT! */
/*********************************/

#include "config_definition.h"

#include <config_store.h>
#include <config_data.h>

static const uint16 const_config_data[] =
{
(CONFIG_DATA_CONFIG_SET_TYPE << 12) | 1110,
    (CONFIG_DATA_CONFIG_SET_METADATA_TYPE << 12) | 29,
    0x6537, 0x3466, 0x3731, 0x3032, 0x3265, 0x6338, 0x6233, 0x3664, 0x3264, 0x3931, 0x3638, 0x3733, 0x3033, 0x3364, 0x3234, 0x3639, 
    0, 40490,
    20,
    0x636f, 0x6e66, 0x6967, 0x5f64, 0x6566, 0x696e, 0x6974, 0x696f, 0x6e2e, 0x677a, 
    (CONFIG_DATA_PS_CONFIG_SET_DATA_TYPE << 12) | 1,
    171,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 7,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 4,
        0x0000, 0x003c, 0x003c, 0x0850,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        7,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 4,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        0x17f0,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        8,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 23,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 20,
        0x1200, 0x000a, 0x000a, 0x0005, 0x0005, 0xffff, 0xff00, 0x1f40, 0x0000, 0x000c, 0x0000, 0x0002, 0xafe0, 0x0003, 0x0003, 0x0080, 0x4000, 0xdfda, 0x0a0a, 0xa000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        9,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 3,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 0,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        10,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 3,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 0,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        11,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 13,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 10,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        12,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 4,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        0x0000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        13,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 16,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 13,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0xff00,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        14,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 3,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 0,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        15,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 26,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 23,
        0x0000, 0x0003, 0x0000, 0x003c, 0x0000, 0x0000, 0x000f, 0x0064, 0x0000, 0x0003, 0x001e, 0x0f0f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0008, 0x0014, 0x2020, 0x1040,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        16,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 4,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        0x8000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        17,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 31,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 28,
        0x4000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x4000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x4000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x4000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        18,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 10,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 7,
        0x01f4, 0x03e8, 0x09c4, 0x0320, 0x1388, 0x040f, 0xe000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        19,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 21,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 18,
        0x0003, 0x01ff, 0x0204, 0x0305, 0x04ff, 0x05ff, 0x06ff, 0x07ff, 0x08ff, 0x09ff, 0x0aff, 0x0bff, 0x0cff, 0x0dff, 0x0eff, 0x0fff, 0x1802, 0x1919,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        20,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 69,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 66,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        21,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 69,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 66,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        22,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 69,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 66,
        0x0001, 0x0038, 0xbe03, 0x0001, 0x0038, 0xbd08, 0x0000, 0xfffd, 0x9506, 0x0000, 0xfffd, 0x9607, 0x0000, 0xfff8, 0x0d01, 0x0000, 0xfff8, 0x0d05, 0x0000, 0xfff8, 0x0e01, 0x0000, 0xfff8, 0x0e05, 0x0000, 0x0038, 0x0c01, 0x0001, 0x7ffe, 0x8f02, 0x0003, 0x0006, 0x220b, 0x0000, 0xfffa, 0x0203, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        23,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 17,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 14,
        0xffba, 0xffc9, 0x0005, 0x0000, 0x0000, 0xffdd, 0x0005, 0x080a, 0x0080, 0x0800, 0x0012, 0x0800, 0x0012, 0x0078,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        24,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 19,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 16,
        0x0000, 0x0400, 0x0000, 0x2480, 0x0000, 0x4000, 0x0000, 0x6080, 0x0000, 0x8000, 0x0000, 0xa000, 0x0000, 0xc080, 0x0000, 0x1000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        25,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 11,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 8,
        0x0001, 0x7788, 0x0414, 0x0414, 0xffff, 0xffff, 0xffff, 0xffff,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        26,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 4,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        0xff00,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        27,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 52,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 49,
        0x0010, 0xffb0, 0x0004, 0xffd8, 0x000a, 0xfff6, 0x0000, 0x0000, 0x0010, 0xffb0, 0x0004, 0xffd8, 0x000a, 0xfff6, 0x0000, 0x8000, 0x0000, 0x0010, 0x0f00, 0x0ffe, 0x0100, 0x0120, 0x0200, 0x0231, 0x0300, 0x0342, 0x0400, 0x0453, 0x0500, 0x0564, 0x0600, 0x0675, 0x0700, 0x0786, 0x0800, 0x0897, 0x0900, 0x09a8, 0x0a00, 0x0ab9, 0x0b00, 0x0bca, 0x0c00, 0x0cdb, 0x0d00, 0x0dec, 0x0e00, 0x0efd, 0x0000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        28,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 5,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 2,
        0x0005, 0x4000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        29,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 32,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 29,
        0x0001, 0x0001, 0x0010, 0x0000, 0x0000, 0x0004, 0x8000, 0x0058, 0x8000, 0x0058, 0x8000, 0x0058, 0x8000, 0x0058, 0x8000, 0x0058, 0x8000, 0x7800, 0x0000, 0x0000, 0x000a, 0x000a, 0x05dc, 0x0000, 0x0bff, 0xffff, 0x1267, 0x5218, 0x00dc,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        30,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 25,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 22,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        31,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 6,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 3,
        0x0003, 0x0001, 0x0000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        32,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 5,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 2,
        0x0007, 0x0800,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        33,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 9,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 6,
        0x4001, 0xfffc, 0x0000, 0x4002, 0xfffc, 0x0100,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        34,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 91,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 88,
        0x4001, 0x0b00, 0x4002, 0x0c00, 0x4003, 0x0d00, 0x4004, 0x0200, 0x4006, 0x0200, 0x4008, 0x0d00, 0x4009, 0x0e00, 0x400a, 0x0200, 0x400b, 0x0200, 0x400f, 0x4600, 0x4010, 0x0200, 0x4011, 0x0200, 0x4012, 0x0200, 0x4013, 0x0200, 0x4014, 0x0200, 0x4019, 0x0e00, 0x401a, 0x0d00, 0x401e, 0x0200, 0x401f, 0x0d00, 0x4020, 0x0e00, 0x4085, 0x4400, 0x4701, 0x0200, 0x4704, 0x1800, 0x4717, 0x1600, 0x471b, 0x1a00, 0x4736, 0x1700, 0x4744, 0x0a00, 0x4745, 0x0100, 0x474b, 0x5000, 0x4751, 0x1d00, 0x4752, 0x5600, 0x4753, 0x1300, 0x4754, 0x1000, 0x4789, 0x1e00, 0x478c, 0x2000, 0x478e, 0x4400, 0x47ed, 0x5000, 0x47ef, 0x0d00, 0x47f0, 0x4400, 0x47f1, 0x5000, 0x473d, 0x3c00, 0x400d, 0x0400, 0x400e, 0x0300, 0x4018, 0x1400,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        35,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 4,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        0x0000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        36,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 4,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        0x6c80,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        37,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 103,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 100,
        0x0013, 0x0f0f, 0x0600, 0x0062, 0x6058, 0x0001, 0x0a0a, 0x2800, 0x0060, 0x6114, 0x0002, 0x1414, 0x0100, 0x0060, 0x6124, 0x0003, 0x140a, 0x0100, 0x0060, 0x6224, 0x0004, 0x0a0a, 0x2800, 0x0060, 0x6124, 0x0005, 0x0a0a, 0x2800, 0x0060, 0x6224, 0x0006, 0x0a0a, 0x2800, 0x0061, 0x6228, 0x0009, 0x6464, 0x2800, 0x4b60, 0x6114, 0x000b, 0x6464, 0x0100, 0x0060, 0x6112, 0x000c, 0x6464, 0x0100, 0x0060, 0x6112, 0x000d, 0x6464, 0x0100, 0x0060, 0x6112, 0x000f, 0x6464, 0x0100, 0x0060, 0x6112, 0x0007, 0x3232, 0x0100, 0x0060, 0x6114, 0x0008, 0x3232, 0x0100, 0x0060, 0x6114, 0x0010, 0x3264, 0x0100, 0x0060, 0x6124, 0x0012, 0x0a0a, 0x0100, 0x0061, 0x6226, 0x0014, 0x0a0a, 0x2800, 0x0060, 0x6124, 0x0015, 0x1414, 0x2800, 0x0060, 0x6134, 0x0016, 0x1414, 0x0100, 0x0060, 0x6124, 0x0017, 0x1414, 0x0100, 0x0060, 0x6124,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        38,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 28,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 25,
        0x4001, 0x6400, 0x0000, 0x0060, 0x6114, 0x4002, 0x6400, 0x0000, 0x0060, 0x6112, 0x408f, 0x0a0a, 0x0000, 0x0060, 0x6138, 0x4021, 0x0a00, 0x0000, 0x0060, 0x6109, 0x4726, 0x0c0c, 0x0000, 0x0060, 0x6187,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        39,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 67,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 64,
        0x4704, 0x0000, 0x0044, 0x0000, 0x4095, 0x0000, 0x0810, 0x0000, 0x470a, 0x0000, 0x0810, 0x0000, 0x4709, 0x6000, 0x0030, 0x0000, 0x4708, 0x6100, 0x0030, 0x0000, 0x4708, 0x0000, 0x2010, 0x0000, 0x4709, 0x0000, 0x2810, 0x0000, 0x4096, 0x0000, 0x2010, 0x0000, 0x4096, 0x0000, 0x2810, 0x0000, 0x472b, 0x0003, 0x0020, 0x0000, 0x4728, 0x0002, 0x0020, 0x0000, 0x4728, 0x0000, 0x5010, 0x0000, 0x472a, 0x0000, 0x5810, 0x0000, 0x4729, 0x6200, 0x0030, 0x0000, 0x47e7, 0x0000, 0x7010, 0x0000, 0x47e8, 0x0000, 0x5010, 0x0000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        40,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 6,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 3,
        0x6061, 0x6200, 0xff10,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        41,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 4,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        0x8000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        42,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 4,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        0xc000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        43,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 5,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 2,
        0x0bb8, 0x1120,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        44,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 4,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        0x8000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        49,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 5,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 2,
        0x0005, 0x0800,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        150,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 6,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 3,
        0x0000, 0x0100, 0x0200,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        151,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 5,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 2,
        0x001f, 0x1000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        152,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 5,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 2,
        0x000f, 0x000f,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        153,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 25,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 22,
        0x9602, 0x0000, 0x0011, 0x4000, 0x0020, 0x4000, 0x5a33, 0x0000, 0x5a42, 0x0000, 0x9652, 0x0000, 0x9662, 0x0000, 0x9672, 0x0000, 0x9682, 0x0000, 0x9692, 0x0000, 0x96a2, 0x0000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        154,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 4,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        0x8000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        155,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 5,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 2,
        0x0000, 0x0000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        156,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 10,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 7,
        0x000a, 0x0064, 0x0000, 0x0000, 0x0000, 0x0a32, 0x3c00,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        157,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 7,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 4,
        0x11c6, 0x122a, 0x0000, 0x0000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        158,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 5,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 2,
        0x0014, 0x0414,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        159,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 24,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 21,
        0x0014, 0x0514, 0x0000, 0x470b, 0x8706, 0xe000, 0x4704, 0x911e, 0xe000, 0x470d, 0x9b00, 0x6000, 0x470e, 0xa500, 0x6000, 0x470f, 0xaf00, 0x6000, 0x4710, 0xff00, 0x6000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        160,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 14,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 11,
        0x0014, 0x0114, 0x013d, 0x02d9, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0fff, 0x0000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        161,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 19,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 16,
        0x0000, 0x0200, 0x9600, 0x1020, 0x0000, 0x2200, 0x0000, 0x3200, 0x0000, 0x4200, 0x0000, 0x5200, 0x0000, 0x6200, 0x0000, 0x7200,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        162,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 5,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 2,
        0x0014, 0x0614,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        163,
    (CONFIG_DATA_CONFIG_BLOCK_TYPE << 12) | 5,
        (CONFIG_DATA_CONFIG_BLOCK_DATA_TYPE << 12) | 2,
        0x109a, 0x0000,
        (CONFIG_DATA_PS_CONFIG_BLOCK_DATA_TYPE << 12) | 1,
        164
};

const void *ConfigDefinitionGetConstData(void)
{
    return (const void *) const_config_data;
}

uint16 ConfigDefinitionGetConstDataSize(void)
{
    return (sizeof(const_config_data) / sizeof(const_config_data[0]));
}
