
#ifndef _LE_GMA_H
#define _LE_GMA_H

#include <stdint.h>
#include "bt_common.h"
#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_GMA)

//
// gatt profile include file, generated by jieli gatt_inc_generator.exe
//

const uint8_t profile_data[] = {
    //////////////////////////////////////////////////////
    //
    // 0x0001 PRIMARY_SERVICE  1800
    //
    //////////////////////////////////////////////////////
    0x0a, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x28, 0x00, 0x18,

    /* CHARACTERISTIC,  2a00, READ | DYNAMIC, */
    // 0x0002 CHARACTERISTIC 2a00 READ | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x02, 0x00, 0x03, 0x28, 0x02, 0x03, 0x00, 0x00, 0x2a,
    // 0x0003 VALUE 2a00 READ | DYNAMIC
    0x08, 0x00, 0x02, 0x01, 0x03, 0x00, 0x00, 0x2a,

    //////////////////////////////////////////////////////
    //
    // 0x0004 PRIMARY_SERVICE  1801
    //
    //////////////////////////////////////////////////////
    0x0a, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x28, 0x01, 0x18,

    /* CHARACTERISTIC,  2a05, INDICATE, */
    // 0x0005 CHARACTERISTIC 2a05 INDICATE
    0x0d, 0x00, 0x02, 0x00, 0x05, 0x00, 0x03, 0x28, 0x20, 0x06, 0x00, 0x05, 0x2a,
    // 0x0006 VALUE 2a05 INDICATE
    0x08, 0x00, 0x20, 0x00, 0x06, 0x00, 0x05, 0x2a,
    // 0x0007 CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x07, 0x00, 0x02, 0x29, 0x00, 0x00,

    //////////////////////////////////////////////////////
    //
    // 0x0008 PRIMARY_SERVICE  feb3
    //
    //////////////////////////////////////////////////////
    0x0a, 0x00, 0x02, 0x00, 0x08, 0x00, 0x00, 0x28, 0xb3, 0xfe,

    /* CHARACTERISTIC,  fed4, READ | DYNAMIC, */
    // 0x0009 CHARACTERISTIC fed4 READ | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x09, 0x00, 0x03, 0x28, 0x02, 0x0a, 0x00, 0xd4, 0xfe,
    // 0x000a VALUE fed4 READ | DYNAMIC
    0x08, 0x00, 0x02, 0x01, 0x0a, 0x00, 0xd4, 0xfe,

    /* CHARACTERISTIC,  fed5, READ | WRITE | DYNAMIC, */
    // 0x000b CHARACTERISTIC fed5 READ | WRITE | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x0b, 0x00, 0x03, 0x28, 0x0a, 0x0c, 0x00, 0xd5, 0xfe,
    // 0x000c VALUE fed5 READ | WRITE | DYNAMIC
    0x08, 0x00, 0x0a, 0x01, 0x0c, 0x00, 0xd5, 0xfe,

    /* CHARACTERISTIC,  fed6, READ | INDICATE | DYNAMIC, */
    // 0x000d CHARACTERISTIC fed6 READ | INDICATE | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x0d, 0x00, 0x03, 0x28, 0x22, 0x0e, 0x00, 0xd6, 0xfe,
    // 0x000e VALUE fed6 READ | INDICATE | DYNAMIC
    0x08, 0x00, 0x22, 0x01, 0x0e, 0x00, 0xd6, 0xfe,
    // 0x000f CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x0f, 0x00, 0x02, 0x29, 0x00, 0x00,

    /* CHARACTERISTIC,  fed7, READ | WRITE_WITHOUT_RESPONSE | DYNAMIC, */
    // 0x0010 CHARACTERISTIC fed7 READ | WRITE_WITHOUT_RESPONSE | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x10, 0x00, 0x03, 0x28, 0x06, 0x11, 0x00, 0xd7, 0xfe,
    // 0x0011 VALUE fed7 READ | WRITE_WITHOUT_RESPONSE | DYNAMIC
    0x08, 0x00, 0x06, 0x01, 0x11, 0x00, 0xd7, 0xfe,

    /* CHARACTERISTIC,  fed8, READ | NOTIFY | DYNAMIC, */
    // 0x0012 CHARACTERISTIC fed8 READ | NOTIFY | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x12, 0x00, 0x03, 0x28, 0x12, 0x13, 0x00, 0xd8, 0xfe,
    // 0x0013 VALUE fed8 READ | NOTIFY | DYNAMIC
    0x08, 0x00, 0x12, 0x01, 0x13, 0x00, 0xd8, 0xfe,
    // 0x0014 CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x14, 0x00, 0x02, 0x29, 0x00, 0x00,

    // END
    0x00, 0x00,
};
//
// characteristics <--> handles
//
#define ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE 0x0003
#define ATT_CHARACTERISTIC_2a05_01_VALUE_HANDLE 0x0006
#define ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE 0x0007
#define ATT_CHARACTERISTIC_fed4_01_VALUE_HANDLE 0x000a
#define ATT_CHARACTERISTIC_fed5_01_VALUE_HANDLE 0x000c
#define ATT_CHARACTERISTIC_fed6_01_VALUE_HANDLE 0x000e
#define ATT_CHARACTERISTIC_fed6_01_CLIENT_CONFIGURATION_HANDLE 0x000f
#define ATT_CHARACTERISTIC_fed7_01_VALUE_HANDLE 0x0011
#define ATT_CHARACTERISTIC_fed8_01_VALUE_HANDLE 0x0013
#define ATT_CHARACTERISTIC_fed8_01_CLIENT_CONFIGURATION_HANDLE 0x0014


#endif

#endif