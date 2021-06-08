#ifndef HDMI_CEC_H
#define HDMI_CEC_H

#include "typedef.h"
typedef enum {
    CEC_LOGADDR_TV          = 0x00,
    CEC_LOGADDR_RECDEV1     = 0x01,
    CEC_LOGADDR_RECDEV2     = 0x02,
    CEC_LOGADDR_TUNER1      = 0x03,     // STB1 in Spev v1.3
    CEC_LOGADDR_PLAYBACK1   = 0x04,     // DVD1 in Spev v1.3
    CEC_LOGADDR_AUDSYS      = 0x05,
    CEC_LOGADDR_TUNER2      = 0x06,     // STB2 in Spec v1.3
    CEC_LOGADDR_TUNER3      = 0x07,     // STB3 in Spec v1.3
    CEC_LOGADDR_PLAYBACK2   = 0x08,     // DVD2 in Spec v1.3
    CEC_LOGADDR_RECDEV3     = 0x09,
    CEC_LOGADDR_TUNER4      = 0x0A,     // RES1 in Spec v1.3
    CEC_LOGADDR_PLAYBACK3   = 0x0B,     // RES2 in Spec v1.3
    CEC_LOGADDR_RES3        = 0x0C,
    CEC_LOGADDR_RES4        = 0x0D,
    CEC_LOGADDR_FREEUSE     = 0x0E,
    CEC_LOGADDR_UNREGORBC   = 0x0F

} CEC_LOG_ADDR_t;

typedef enum {                  // CEC Messages
    CECOP_FEATURE_ABORT             = 0x00,
    CECOP_IMAGE_VIEW_ON             = 0x04,
    CECOP_TUNER_STEP_INCREMENT      = 0x05,     // N/A
    CECOP_TUNER_STEP_DECREMENT      = 0x06,     // N/A
    CECOP_TUNER_DEVICE_STATUS       = 0x07,     // N/A
    CECOP_GIVE_TUNER_DEVICE_STATUS  = 0x08,     // N/A
    CECOP_RECORD_ON                 = 0x09,     // N/A
    CECOP_RECORD_STATUS             = 0x0A,     // N/A
    CECOP_RECORD_OFF                = 0x0B,     // N/A
    CECOP_TEXT_VIEW_ON              = 0x0D,
    CECOP_RECORD_TV_SCREEN          = 0x0F,     // N/A
    CECOP_GIVE_DECK_STATUS          = 0x1A,
    CECOP_DECK_STATUS               = 0x1B,
    CECOP_SET_MENU_LANGUAGE         = 0x32,
    CECOP_CLEAR_ANALOGUE_TIMER      = 0x33,     // Spec 1.3A
    CECOP_SET_ANALOGUE_TIMER        = 0x34,     // Spec 1.3A
    CECOP_TIMER_STATUS              = 0x35,     // Spec 1.3A
    CECOP_STANDBY                   = 0x36,
    CECOP_PLAY                      = 0x41,
    CECOP_DECK_CONTROL              = 0x42,
    CECOP_TIMER_CLEARED_STATUS      = 0x43,     // Spec 1.3A
    CECOP_USER_CONTROL_PRESSED      = 0x44,
    CECOP_USER_CONTROL_RELEASED     = 0x45,
    CECOP_GIVE_OSD_NAME             = 0x46,
    CECOP_SET_OSD_NAME              = 0x47,
    CECOP_SET_OSD_STRING            = 0x64,
    CECOP_SET_TIMER_PROGRAM_TITLE   = 0x67,     // Spec 1.3A
    CECOP_SYSTEM_AUDIO_MODE_REQUEST = 0x70,     // Spec 1.3A
    CECOP_GIVE_AUDIO_STATUS         = 0x71,     // Spec 1.3A
    CECOP_SET_SYSTEM_AUDIO_MODE     = 0x72,     // Spec 1.3A
    CECOP_REPORT_AUDIO_STATUS       = 0x7A,     // Spec 1.3A
    CECOP_GIVE_SYSTEM_AUDIO_MODE_STATUS = 0x7D, // Spec 1.3A
    CECOP_SYSTEM_AUDIO_MODE_STATUS  = 0x7E,     // Spec 1.3A
    CECOP_ROUTING_CHANGE            = 0x80,
    CECOP_ROUTING_INFORMATION       = 0x81,
    CECOP_ACTIVE_SOURCE             = 0x82,
    CECOP_GIVE_PHYSICAL_ADDRESS     = 0x83,
    CECOP_REPORT_PHYSICAL_ADDRESS   = 0x84,
    CECOP_REQUEST_ACTIVE_SOURCE     = 0x85,
    CECOP_SET_STREAM_PATH           = 0x86,
    CECOP_DEVICE_VENDOR_ID          = 0x87,
    CECOP_VENDOR_COMMAND            = 0x89,
    CECOP_VENDOR_REMOTE_BUTTON_DOWN = 0x8A,
    CECOP_VENDOR_REMOTE_BUTTON_UP   = 0x8B,
    CECOP_GIVE_DEVICE_VENDOR_ID     = 0x8C,
    CECOP_MENU_REQUEST              = 0x8D,
    CECOP_MENU_STATUS               = 0x8E,
    CECOP_GIVE_DEVICE_POWER_STATUS  = 0x8F,
    CECOP_REPORT_POWER_STATUS       = 0x90,
    CECOP_GET_MENU_LANGUAGE         = 0x91,
    CECOP_SELECT_ANALOGUE_SERVICE   = 0x92,     // Spec 1.3A    N/A
    CECOP_SELECT_DIGITAL_SERVICE    = 0x93,     //              N/A
    CECOP_SET_DIGITAL_TIMER         = 0x97,     // Spec 1.3A
    CECOP_CLEAR_DIGITAL_TIMER       = 0x99,     // Spec 1.3A
    CECOP_SET_AUDIO_RATE            = 0x9A,     // Spec 1.3A
    CECOP_INACTIVE_SOURCE           = 0x9D,     // Spec 1.3A
    CECOP_CEC_VERSION               = 0x9E,     // Spec 1.3A
    CECOP_GET_CEC_VERSION           = 0x9F,     // Spec 1.3A
    CECOP_VENDOR_COMMAND_WITH_ID    = 0xA0,     // Spec 1.3A
    CECOP_CLEAR_EXTERNAL_TIMER      = 0xA1,     // Spec 1.3A
    CECOP_SET_EXTERNAL_TIMER        = 0xA2,     // Spec 1.3A
    CDCOP_HEADER                    = 0xF8,
    CECOP_ABORT                     = 0xFF,

    CECOP_REPORT_SHORT_AUDIO    	= 0xA3,     // Spec 1.4
    CECOP_REQUEST_SHORT_AUDIO    	= 0xA4,     // Spec 1.4

    CECOP_ARC_INITIATE              = 0xC0,
    CECOP_ARC_REPORT_INITIATED      = 0xC1,
    CECOP_ARC_REPORT_TERMINATED     = 0xC2,

    CECOP_ARC_REQUEST_INITIATION    = 0xC3,
    CECOP_ARC_REQUEST_TERMINATION   = 0xC4,
    CECOP_ARC_TERMINATE             = 0xC5,

} CEC_OPCODE_t;

/* definition for CEC commands */
#define GIVE_PHYSICAL_ADDRESS            0x83
#define REPORT_PHYSICAL_ADDRESS          0x84
#define GIVE_DEVICE_POWER_STATUS         0x8F
#define GIVE_DEVICE_VENDOR_ID            0x8C
#define DEVICE_VENDOR_ID                 0x87
#define STANDBY                          0x36
#define  CEC_VERSION                     0x9E
#define  GET_CEC_VERSION                 0x9F

//CEC-SAC command
#define  GIVE_AUDIO_STATUS               0x71
#define  GIVE_SYSTEM_AUDIO_MODE_STATUS   0x7D
#define  REPORT_AUDIO_STATUS             0x7A
#define  REPORT_SHORT_AUDIO_DESCRIPTOR   0xA3
#define  REQUEST_SHORT AUDIO_DESCRIPTOR  0xA4
#define  SET_SYSTEM_AUDIO_MODE           0x72
#define  SYSTEM_AUDIO_MODE_REQUEST       0x70
#define  SYSTEM_AUDIO_MODE_STATUS        0x7E
#define  USER_CONTROL_PRESSED            0x44
#define  USER_CONTROL_RELEASED           0x45


////CEC-ARC command
#define  INITIATE_ARC                    0xC0
#define  REPORT_ARC_INITIATED            0xC1
#define  REPORT_ARC_TERMINATED           0xC2
#define  REQUEST_ARC_INITIATION          0xC3
#define  REQUEST_ARC_TERMINATION         0xC4
#define  TERMINATE_ARC                   0xC5


///*definition for cec*/
////bit timing
#define   CEC_START_BIT_L     37   //37*100us = 3.7ms
#define   CEC_START_BIT_H     8     //8*100us = 0.8ms
#define   CEC_BIT_1_H         18
#define   CEC_BIT_1_L         6
#define   CEC_BIT_0_H         9
#define   CEC_BIT_0_L         15
#define   CEC_MAX_RETRY       5   //Retry

#define   CEC_FREE_TIME_PI    7*24
#define   CEC_FREE_TIME_NI    5*24
#define   CEC_FREE_TIME_RS    3*24


////Destination of header block
#define   CEC_HEAD_DEST        0x05
//
////Tolerance
#define  Percent     0.05
#define  Percent0     0.13
#define  Percent1     0.33
//////Start Bit Range
////#define  STB_MAX     (CEC_START_BIT_L*4800*(1+Percent))UL
////#define  STB_MIN     (CEC_START_BIT_L*4800*(1-Percent))
////
//////Bit 0 range
////#define BIT_0_MAX       (CEC_BIT_0_L*4800*(1+Percent0))
////#define BIT_0_MIN       (CEC_BIT_0_L*4800*(1-Percent0))
//////Bit 1 range
////#define BIT_1_MAX       (CEC_BIT_1_L*4800*(1+Percent1))
////#define BIT_1_MIN       (CEC_BIT_1_L*4800*(1-Percent1))


////Start Bit Range
#define  STB_MAX     (CEC_START_BIT_L*1050)
#define  STB_MIN     (CEC_START_BIT_L*950)

////Bit 0 range
#define BIT_0_MAX       (CEC_BIT_0_L*(1130))
#define BIT_0_MIN       (CEC_BIT_0_L*(870))
////Bit 1 range
#define BIT_1_MAX       (CEC_BIT_1_L*(1330))
#define BIT_1_MIN       (CEC_BIT_1_L*(670))


#endif
