#pragma once

//-----------------------------------------------------------------------------
// Joypad physical buttons
//-----------------------------------------------------------------------------
#define JOYPAD_DPAD_UP          0x0001
#define JOYPAD_DPAD_DOWN        0x0002
#define JOYPAD_DPAD_LEFT        0x0004
#define JOYPAD_DPAD_RIGHT       0x0008
#define JOYPAD_START            0x0010
#define JOYPAD_BACK             0x0020
#define JOYPAD_LEFT_THUMB       0x0040
#define JOYPAD_RIGHT_THUMB      0x0080
#define JOYPAD_LEFT_SHOULDER    0x0100
#define JOYPAD_RIGHT_SHOULDER   0x0200
#define JOYPAD_GUIDE			0x0400
#define JOYPAD_SHARE			0x0800
#define JOYPAD_A                0x1000
#define JOYPAD_B                0x2000
#define JOYPAD_X                0x4000
#define JOYPAD_Y                0x8000

//-----------------------------------------------------------------------------
// Gunfighter Kosmosima physical buttons
//-----------------------------------------------------------------------------
#define GF_KOS_POV_1			0x00000001
#define GF_KOS_POV_2			0x00000002
#define GF_KOS_POV_3			0x00000004
#define GF_KOS_POV_4			0x00000008
#define GF_KOS_TRIGGER_1		0x00000010
#define GF_KOS_TRIGGER_2		0x00000020
#define GF_KOS_WPN_REL			0x00000040
#define GF_KOS_R_SIDE			0x00000080
#define GF_KOS_PINKIE			0x00000100
#define GF_KOS_TMS_UP			0x00000200
#define GF_KOS_TMS_RIGHT		0x00000400
#define GF_KOS_TMS_DOWN			0x00000800
#define GF_KOS_TMS_LEFT			0x00001000
#define GF_KOS_TMS_CENTER		0x00002000
#define GF_KOS_TRIM_UP			0x00004000
#define GF_KOS_TRIM_RIGHT		0x00008000
#define GF_KOS_TRIM_DOWN		0x00010000
#define GF_KOS_TRIM_LEFT		0x00020000
#define GF_KOS_TRIM_CENTER		0x00040000
#define GF_KOS_DMS_UP			0x00080000
#define GF_KOS_DMS_RIGHT		0x00100000
#define GF_KOS_DMS_DOWN			0x00200000
#define GF_KOS_DMS_LEFT			0x00400000
#define GF_KOS_DMS_CENTER		0x00800000
#define GF_KOS_THUMB			0x10000000

//-----------------------------------------------------------------------------
// Gunfighter KG12 physical buttons
//-----------------------------------------------------------------------------
#define GF_KG12_POV_1			0x00000001
#define GF_KG12_POV_2			0x00000002
#define GF_KG12_POV_3			0x00000004
#define GF_KG12_POV_4			0x00000008
#define GF_KG12_TRIGGER			0x00000010
#define GF_KG12_TOP				0x00000020
#define GF_KG12_PINKIE			0x00000040
#define GF_KG12_HAT_UP			0x00000080
#define GF_KG12_HAT_RIGHT		0x00000100
#define GF_KG12_HAT_DOWN		0x00000200
#define GF_KG12_HAT_LEFT		0x00000400

//-----------------------------------------------------------------------------
// TWCS physical buttons
//-----------------------------------------------------------------------------
#define TWCS_COOLIESW_UP		0x00000001
#define TWCS_COOLIESW_DOWN		0x00000002
#define TWCS_COOLIESW_AFT		0x00000004
#define TWCS_COOLIESW_FORWARD	0x00000008
#define TWCS_THUMB				0x00000010
#define TWCS_FRONT_1			0x00000020
#define TWCS_FRONT_2			0x00000040
#define TWCS_ROCKER_UP			0x00000080
#define TWCS_ROCKER_DOWN		0x00000100
#define TWCS_STICK_DEPRESS		0x00000200
#define TWCS_BOATSW_UP			0x00000400
#define TWCS_BOATSW_FORWARD		0x00000800
#define TWCS_BOATSW_DOWN		0x00001000
#define TWCS_BOATSW_AFT			0x00002000
#define TWCS_CASTLESW_UP		0x00004000
#define TWCS_CASTLESW_FORWARD	0x00008000
#define TWCS_CASTLESW_DOWN		0x00010000
#define TWCS_CASTLESW_AFT		0x00020000