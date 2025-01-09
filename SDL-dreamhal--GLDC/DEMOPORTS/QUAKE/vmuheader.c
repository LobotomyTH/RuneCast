#include "vmuheader.h"

struct vmuheader vmuheader = {
	"QUAKE SAVE      ", // char desc_vm[16];
	"" , // char desc_dc[32];
	"QUAKE", // char application[16];
	1, // uint16le n_icons;
	0, // uint16le anime_speed;
	0, // uint16le eyecache;
	0, // uint16le crc;
	0, // uint32le filesize;
	"", // char reserved[20];
	{
	0xF000,0xFE91,0xF850,0xF999,0xF555,0xF530,0xFFFD,0xFFA8,0xFF74,0xFF86,0xF777,0xFB70,0xFF88,0xFFB4,0xFCCC,0xF222,
	},  // uint32le icon_pal[16];
	{
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0xE0,0x00,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x0E,0x00,0x00,0x0E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x0E,0x00,0x00,0x0E,0x00,0x00,0x00,0x0F,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x0E,0x30,0xE0,0x3E,0x00,0x0F,0x44,0x4F,0xA4,0x44,0x4F,0x00,0x00,0x00,0x00,0x00,
	0x00,0xEE,0xEE,0xE0,0x04,0x4A,0x44,0xA3,0x73,0xA4,0x44,0x4F,0x00,0x00,0x00,0x00,
	0x00,0x00,0xE0,0x00,0x0F,0xA4,0xF5,0x44,0xA4,0x45,0x54,0x45,0x00,0x00,0x00,0x00,
	0x00,0x00,0x30,0x00,0x04,0x40,0xF4,0x44,0x44,0x44,0x55,0xF4,0x00,0x00,0x00,0x00,
	0x00,0x00,0x30,0x00,0x05,0x0F,0x4F,0x44,0xA4,0x4F,0xF0,0xF5,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x0F,0x0F,0x0F,0xFF,0xFF,0xFF,0xFF,0x05,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x05,0xF0,0x28,0x99,0x89,0x78,0x20,0xF5,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x0F,0x50,0xB5,0x59,0x99,0x25,0x20,0xF5,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x05,0xF5,0x22,0x52,0x92,0x52,0x25,0xFF,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x0F,0x02,0x97,0x97,0x67,0x97,0x92,0x05,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x05,0xF5,0x29,0xB2,0x92,0x29,0x82,0x0F,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x05,0xF5,0x42,0x4A,0x44,0x42,0x52,0xF5,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x05,0x43,0x4A,0x42,0x88,0x42,0x24,0xA5,0x00,0x00,0x00,0x00,
	0x04,0x00,0x00,0x00,0x0F,0x54,0x48,0x5A,0x93,0x52,0xA4,0xFF,0x00,0x00,0x00,0x40,
	0xF9,0xFF,0x0B,0x1B,0xFF,0x5F,0xFA,0x28,0x98,0x2A,0x4F,0x5F,0x0B,0xBB,0xF0,0xAA,
	0x4C,0xFF,0x0B,0x11,0x2F,0x0F,0x04,0xAA,0x4A,0xA4,0x0F,0xFF,0x5B,0x1B,0xFF,0x9A,
	0x47,0xF4,0x02,0xB1,0xB2,0xFF,0x0F,0x05,0x0F,0x05,0x0F,0x05,0xBD,0x1B,0x0F,0x7A,
	0x43,0xFF,0x51,0x11,0xB1,0xBB,0x22,0x25,0xFF,0x52,0x22,0xB1,0xBB,0x1B,0x0F,0xA4,
	0x47,0xFF,0x21,0x1D,0x11,0xD1,0xD1,0x12,0x55,0x11,0x11,0x11,0x11,0x1B,0x5F,0xA4,
	0x43,0xFF,0x2B,0xBB,0x1D,0x11,0xDD,0xD2,0x22,0x1D,0xD1,0x1D,0x1B,0xBB,0x0F,0x34,
	0x4A,0xA0,0x2B,0x1B,0xBB,0xB2,0x22,0x88,0x88,0x82,0x2B,0xBB,0xBB,0xBB,0xFF,0x8A,
	0xA4,0xAF,0x2B,0x2B,0xB2,0x22,0x25,0x22,0x25,0xB2,0x2B,0x2B,0x1B,0x2B,0x04,0xA4,
	0x44,0x4F,0x5B,0xBB,0xB1,0x11,0x11,0x25,0x05,0x51,0x1B,0x12,0x2B,0xBB,0xF4,0xAF,
	0x4F,0x44,0x2B,0x2B,0xBB,0x21,0xBB,0xB5,0x55,0x21,0xB1,0x1B,0xBB,0xBB,0xFA,0x4F,
	0x4F,0x4F,0x22,0xBB,0xBB,0x22,0x25,0x88,0x88,0x85,0x52,0x2B,0x1B,0xBB,0xF2,0xFF,
	0xFF,0xAF,0x2B,0xBB,0xB1,0x11,0x12,0x52,0x22,0x22,0xB1,0x1B,0xBB,0x2B,0xF4,0xBF,
	} // unsigned char icon_data[32*32/2];
};
