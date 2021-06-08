#ifndef _IAP2_H_
#define _IAP2_H_

#include "generic/typedef.h"

#define iAP2_LINK_PACKET_HEADER_LEN		9
#define iAP2_MESSAGE_LEN				6
#define iAP2_MESSAGE_PARAMETERS_LEN		4

#define MAX_PACKET_PAYLOAD			0x65525L	//means 65535
#define IAP2_START_OF_PACKET		0xFF5AL
#define START_OF_MESSAGE			0x4040L
#define PACKET_TOTAL_LEN(X)			(iAP2_LINK_PACKET_HEADER_LEN+sizeof(X)+0x1)
#define MESSAGE_TOTAL_LEN(X)		(sizeof(X))
///IAP2 HID Usage ID
#define  IAP2_HID_PLAY	 		0xB0
#define  IAP2_HID_PAUSE			0xB1
#define  IAP2_HID_NEXT			0xB5
#define  IAP2_HID_PRE			0xB6
#define  IAP2_HID_SHUFFLE		0xB9
#define  IAP2_HID_REPEAT		0xBC
#define  IAP2_HID_PP			0xCD
#define  IAP2_HID_MUTE			0xE2
#define  IAP2_HID_VOLUP			0xE9
#define  IAP2_HID_VOLDOWN		0xEA
///APPLE HID Control bit
#define APPLE_HID_VOLUP       	BIT(0)
#define APPLE_HID_VOLDOWN	 	BIT(1)
#define APPLE_HID_PP			BIT(2)
// #define APPLE_HID_SHUFFLE     	BIT(2)
#define APPLE_HID_REPEAT    	BIT(3)
#define APPLE_HID_NEXT          BIT(4)
#define APPLE_HID_PRE        	BIT(5)
#define APPLE_HID_PLAY     		BIT(6)
#define APPLE_HID_PAUSE       	BIT(7)



//Name--can alter
#define IAP2_ACCESSORY_NAME             'D','o','c','k',' ','s','p','e','a','k','e','r','\0'
#define IAP2_ACCESSORY_NAME_LEN         sizeof("Dock speaker")

//ModelIdentifier(模式描述符)--can alter
#define IAP2_ACCESSORY_MODEL_IDENTIFIER         'I','P','D','L','I','1','3','\0'
#define IAP2_ACCESSORY_MODEL_IDENTIFIER_LEN     sizeof("IPDLI13")

//Manufacturer(制造商)--can alter
#define IAP2_ACCESSORY_MANUFACTURER             'I',' ','w','a','n','t',' ','i','t','\0'
#define IAP2_ACCESSORY_MANUFACTURER_LEN         sizeof("I want it")

//SerialNumber(序列号)--can alter
#define IAP2_ACCESSORY_SERIALNUMBER             'i','A','P',' ','I','n','t','e','r','f','a','c','e','\0'
#define IAP2_ACCESSORY_SERIALNUMBER_LEN         sizeof("iAP Interface")

//FirmwareVersion(固件版本)--can alter
#define IAP2_ACCESSORY_FIRMWARE_VER             '1','.','0','.','0','\0'
#define IAP2_ACCESSORY_FIRMWARE_VER_LEN         sizeof("1.0.0")

//HardwareVersion(硬件版本)--can alter
#define IAP2_ACCESSORY_HARDWARE_VER             '1','.','0','.','0','\0'
#define IAP2_ACCESSORY_HARDWARE_VER_LEN         sizeof("1.0.0")

//CurrentLanguage--can not alter
#define IAP2_CUR_LANGUAGE           			'e','n','\0'
#define IAP2_CUR_LANGUAGE_LEN       			sizeof("en")

//SupportedLanguage--can not alter
#define IAP2_SUP_LANGUAGE           			'e','n','\0'
#define IAP2_SUP_LANGUAGE_LEN       			sizeof("en")

//USB device transport Component
#define IAP2_TRANSPORT_NAME             		'i','A','P','2','H','\0'
#define IAP2_TRANSPORT_NAME_LEN         		sizeof("iAP2H")

//iAP2HIDComponent
#define IAP2_HID_IDENTIFIER             		1
#define IAP2_HID_NAME                   		'R','e','m','o','t','e','\0'
#define IAP2_HID_NAME_LEN               		sizeof("Remote")



///variable
typedef enum {
    iAP2_SLP = 3,
    iAP2_RST,
    iAP2_EAK,
    iAP2_ACK,
    iAP2_SYN,
} iAP2_CONTROL_BYTE;

typedef enum {
    //Header
    STARTOFPACKET = 0,
    PACKETLENGTH = 2,
    CONTROLBYTE = 4,
    PACKETSEQUENCENUMBER,
    PACKETACKNUMBER,
    SESSIONIDENTIFIER,
    HEADERCHECKSUM,
    //Payload
    LINKVERSION,
    MAXNUMOFOUTSTANDINGPKTS,
    MAXPKTLENGTH = 11,
    RETRANSMISSIONTIMEOUT = 13,
    CUMULATIVEASKTIMEOUT = 15,
    MAXNUMOFRETRANSMISSIONS = 17,
    MAXCUMULATIVEACK,

    STARTOFMESSAGE = 9,
    MESSAGELENGTH = 11,
    MESSAGEID = 13,
} iAP2_PKT_INDEX;

typedef enum {
    CONTROL_SESSION = 0,
    FILE_TRANSFER_SESSION,
    EXTERNAL_ACCESSORY_SESSION,
} iAP2_SESSION_TYPE;

typedef union _U16_U8 {
    u16	wWord;
    u8	bByte[2];
} U16_U8;


typedef struct _iAP2_LINK_PACKET_HEADER {
    U16_U8	sStartofPacket;
    U16_U8	sPacketLength;
    u8	bControlByte;
    u8	bPacketSequenceNum;
    u8	bPacketAckNum;
    u8	bSessionIdentifier;
    u8	bHeaderChecksum;
} iAP2_LINK_PACKET_HEADER;

typedef struct _iAP2_SESSION {
    u8	bSessionIdentifier;
    u8	bSessionType;
    u8	bSessionVersion;
} iAP2_SESSION;

typedef struct _iAP2_LINK_PACKET_PAYLOAD {
    u8	bLinkVersion;	   			//both device & accessory must agree on !!!
    u8	bMaxNumofOutstandingPkts;
    U16_U8	sMaxPktLength;
    U16_U8	sRetransmissionTimeout;	//both device & accessory must agree on !!!
    U16_U8	sCumulativeAckTimeout;	//both device & accessory must agree on !!!
    u8	bMaxNumofRetransmissions;	//both device & accessory must agree on !!!
    u8	bMaxCumulativeAck; 			//both device & accessory must agree on !!!
} iAP2_LINK_PACKET_PAYLOAD;

typedef struct _iAP2_MESSAGE {
    U16_U8 sStartofMessage;
    U16_U8 sMessageLength;
    U16_U8 sMessageID;
} iAP2_MESSAGE;

typedef struct _iAP2_MESSAGE_PARAMETERS {
    u16 wParameterLength;
    u16 wParameterId;
} iAP2_MESSAGE_PARAMETERS;


///inside call

///outside call
bool iAP2_link(void);
void iap2_hid_key(u16 key);


#endif	/*	_IAP2_H_  */
