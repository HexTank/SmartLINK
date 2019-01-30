////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Virtual SD command table for SmartLINK by Paul Tankard - 2018
//
// TODO:
//   check if the map table is really baking in to stm ROM
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

uint8_t cmdpacket[]                 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
constexpr size_t  cmdpacket_size    = std::extent<decltype(cmdpacket)>::value;

// GO_IDLE_STATE                    - 0x40 - R1
constexpr uint8_t cmd0[]            = {0x01};
constexpr size_t  cmd0_size         = std::extent<decltype(cmd0)>::value;
// SEND_OP_COND                     - 0x41 - R1
constexpr uint8_t cmd1[]            = {0x00};
constexpr size_t  cmd1_size         = std::extent<decltype(cmd1)>::value;
// END_IF_COND                      - 0x48 - R7
constexpr uint8_t cmd8[]            = {0x00, 0x00, 0x00, 0x01, 0xAA};
constexpr size_t  cmd8_size         = std::extent<decltype(cmd8)>::value;
// SEND_CSD                         - 0x49 - R1
constexpr uint8_t cmd9[]            = {0x00, 0xfe, 0x00, 0x7f, 0x00, 0x32, 0x5b, 0x5a, 0x83, 0xbf, 0xff, 0xff, 0xcf, 0x80, 0x16, 0x80, 0x00, 0x37};
constexpr size_t  cmd9_size         = std::extent<decltype(cmd9)>::value;
// SEND_CID                         - 0x4a - R1
constexpr uint8_t cmd10[]           = {0x00, 0xfe, 0x88, 0x03, 0x02, 0x31, 0x32, 0x33, 0x32, 0x20, 0x10, 0x00, 0x00, 0xce, 0xa4, 0x00, 0x71, 0xb9};
constexpr size_t  cmd10_size        = std::extent<decltype(cmd10)>::value;
// READ_SINGLE_BLOCK                - 0x51 - R1
constexpr uint8_t cmd17[]           = {0x00, 0xfe};
constexpr size_t  cmd17_size        = std::extent<decltype(cmd17)>::value;
// SD_SEND_OP_COND                  - 0x69 - R1
constexpr uint8_t acmd41[]          = {0x00};
constexpr size_t  acmd41_size       = std::extent<decltype(acmd41)>::value;
// APP_CMD                          - 0x77 - R1
constexpr uint8_t cmd55[]           = {0x00};
constexpr size_t  cmd55_size        = std::extent<decltype(cmd55)>::value;
// READ_OCR                         - 0x7a - R3
constexpr uint8_t cmd58[]           = {0x01, 0x00, 0xff, 0x80, 0x00};
constexpr size_t  cmd58_size        = std::extent<decltype(cmd58)>::value;

// Not supported R1 - may need to add for R2, R3 and R7, depending how funky we want to get. 
constexpr uint8_t cmdNoSupport[]    = {0x05};
constexpr size_t  cmdNoSupport_size = std::extent<decltype(cmdNoSupport)>::value;

// Smart Link commands              - 0x7f - R1
constexpr uint8_t cmdSmartLink[]    = {0x00};   // we hold the transmition data byte at this until the data is ready to send, ala, received everything from the serial buffer.
constexpr size_t  cmdSmartLink_size = std::extent<decltype(cmdSmartLink)>::value;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr std::array<std::pair<const uint8_t*, size_t>, 64> sd_response_table =
{
	/* 0x40 */	std::make_pair(cmd0, 			        cmd0_size 			),  // GO_IDLE_STATE
	/* 0x41 */	std::make_pair(cmd1, 			        cmd1_size 			),	// SEND_OP_COND
	/* 0x42 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x43 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x44 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x45 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x46 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x47 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x48 */	std::make_pair(cmd8, 			        cmd8_size 			),  // END_IF_COND
	/* 0x49 */	std::make_pair(cmd9, 			        cmd9_size 			),	// SEND_CSD
	/* 0x4a */	std::make_pair(cmd10, 			        cmd10_size 			),  // SEND_CID
	/* 0x4b */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x4c */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x4d */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x4e */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x4f */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x50 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x51 */	std::make_pair(cmd17, 			        cmd17_size 			),	// READ_SINGLE_BLOCK
	/* 0x52 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x53 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x54 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x55 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x56 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x57 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x58 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x59 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x5a */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x5b */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x5c */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x5d */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x5e */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x5f */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x60 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x61 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x62 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x63 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x64 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x65 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x66 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x67 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x68 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x69 */	std::make_pair(acmd41, 			        acmd41_size 		),	// SD_SEND_OP_COND
	/* 0x6a */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x6b */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x6c */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x6d */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x6e */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x6f */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x70 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x71 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x72 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x73 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x74 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x75 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x76 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x77 */	std::make_pair(cmd55, 			        cmd55_size 			),	// APP_CMD
	/* 0x78 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x79 */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x7a */	std::make_pair(cmd58, 			        cmd58_size 			),  // READ_OCR
	/* 0x7b */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x7c */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x7d */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),
	/* 0x7e */	std::make_pair(cmdNoSupport, 	        cmdNoSupport_size 	),  
	/* 0x7f */	std::make_pair(cmdSmartLink,            cmdSmartLink_size 	)   // SMART LINK
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr uint8_t crc7_table[256] =
{
    0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f,
    0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
    0x19, 0x10, 0x0b, 0x02, 0x3d, 0x34, 0x2f, 0x26,
    0x51, 0x58, 0x43, 0x4a, 0x75, 0x7c, 0x67, 0x6e,
    0x32, 0x3b, 0x20, 0x29, 0x16, 0x1f, 0x04, 0x0d,
    0x7a, 0x73, 0x68, 0x61, 0x5e, 0x57, 0x4c, 0x45,
    0x2b, 0x22, 0x39, 0x30, 0x0f, 0x06, 0x1d, 0x14,
    0x63, 0x6a, 0x71, 0x78, 0x47, 0x4e, 0x55, 0x5c,
    0x64, 0x6d, 0x76, 0x7f, 0x40, 0x49, 0x52, 0x5b,
    0x2c, 0x25, 0x3e, 0x37, 0x08, 0x01, 0x1a, 0x13,
    0x7d, 0x74, 0x6f, 0x66, 0x59, 0x50, 0x4b, 0x42,
    0x35, 0x3c, 0x27, 0x2e, 0x11, 0x18, 0x03, 0x0a,
    0x56, 0x5f, 0x44, 0x4d, 0x72, 0x7b, 0x60, 0x69,
    0x1e, 0x17, 0x0c, 0x05, 0x3a, 0x33, 0x28, 0x21,
    0x4f, 0x46, 0x5d, 0x54, 0x6b, 0x62, 0x79, 0x70,
    0x07, 0x0e, 0x15, 0x1c, 0x23, 0x2a, 0x31, 0x38,
    0x41, 0x48, 0x53, 0x5a, 0x65, 0x6c, 0x77, 0x7e,
    0x09, 0x00, 0x1b, 0x12, 0x2d, 0x24, 0x3f, 0x36,
    0x58, 0x51, 0x4a, 0x43, 0x7c, 0x75, 0x6e, 0x67,
    0x10, 0x19, 0x02, 0x0b, 0x34, 0x3d, 0x26, 0x2f,
    0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
    0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04,
    0x6a, 0x63, 0x78, 0x71, 0x4e, 0x47, 0x5c, 0x55,
    0x22, 0x2b, 0x30, 0x39, 0x06, 0x0f, 0x14, 0x1d,
    0x25, 0x2c, 0x37, 0x3e, 0x01, 0x08, 0x13, 0x1a,
    0x6d, 0x64, 0x7f, 0x76, 0x49, 0x40, 0x5b, 0x52,
    0x3c, 0x35, 0x2e, 0x27, 0x18, 0x11, 0x0a, 0x03,
    0x74, 0x7d, 0x66, 0x6f, 0x50, 0x59, 0x42, 0x4b,
    0x17, 0x1e, 0x05, 0x0c, 0x33, 0x3a, 0x21, 0x28,
    0x5f, 0x56, 0x4d, 0x44, 0x7b, 0x72, 0x69, 0x60,
    0x0e, 0x07, 0x1c, 0x15, 0x2a, 0x23, 0x38, 0x31,
    0x46, 0x4f, 0x54, 0x5d, 0x62, 0x6b, 0x70, 0x79
};

bool cmdPacketValid()
{
    uint8_t crc = 0;
    for (size_t i=0;  i < cmdpacket_size-1; i++)
    {
        crc = crc7_table[(crc << 1) ^ cmdpacket[i]];
    }
    return ((crc << 1) | 1) == cmdpacket[cmdpacket_size-1];
}

uint8_t crc7(const uint8_t *packet, size_t packet_len)
{
    uint8_t crc = 0;
    for (size_t i=0;  i < packet_len; i++)
    {
        crc = crc7_table[(crc << 1) ^ packet[i]];
    }
    return ((crc << 1) | 1);
}

