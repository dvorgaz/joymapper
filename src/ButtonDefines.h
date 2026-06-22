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

//-----------------------------------------------------------------------------
// Hardware defines
//-----------------------------------------------------------------------------
#define XBOXCONTROLLER	L"Controller (Xbox One For Windows)"
#define VKBSTICK		L" VKB-Sim Space Gunfighter "
#define VKBKG12			L" VKB-Sim Gunfighter Vintage "
#define TWCSTHROTTLE	L"TWCS Throttle"

#define USE_SECOND_VJOY 0
#define USE_PEDALS 0
#define USE_KG12 0

//-----------------------------------------------------------------------------
// Gunfighter generic buttons
//-----------------------------------------------------------------------------

#if USE_KG12
#define GF_DEVICE_ID		VKBKG12

#define GF_TRIGGER_1		GF_KG12_TRIGGER
#define GF_TRIGGER_2		GF_KG12_TRIGGER
#define GF_WPN_REL			GF_KG12_TOP
#define GF_R_SIDE			0
#define GF_PINKIE			GF_KG12_PINKIE
#define GF_TRIM_UP			GF_KG12_HAT_UP
#define GF_TRIM_RIGHT		GF_KG12_HAT_RIGHT
#define GF_TRIM_DOWN		GF_KG12_HAT_DOWN
#define GF_TRIM_LEFT		GF_KG12_HAT_LEFT
#define GF_TRIM_CENTER		0
#define GF_DMS_UP			0
#define GF_DMS_RIGHT		0
#define GF_DMS_DOWN			0
#define GF_DMS_LEFT			0
#define GF_DMS_CENTER		0
#define GF_TMS_UP			0
#define GF_TMS_RIGHT		0
#define GF_TMS_DOWN			0
#define GF_TMS_LEFT			0
#define GF_TMS_CENTER		0
#define GF_EX_1				0
#define GF_EX_2				0
#define GF_EX_3				0

#define GF_MENU_ACT_1		GF_KG12_HAT_UP
#define GF_MENU_ACT_2		GF_KG12_TOP
#define GF_MENU_ACCEPT		GF_KG12_TOP
#define GF_MENU_CANCEL		GF_KG12_PINKIE
#define GF_MENU_UP			GF_KG12_HAT_UP
#define GF_MENU_DOWN		GF_KG12_HAT_DOWN
#define GF_MENU_LEFT		GF_KG12_HAT_LEFT
#define GF_MENU_RIGHT		GF_KG12_HAT_RIGHT
#else
#define GF_DEVICE_ID		VKBSTICK

#define GF_TRIGGER_1		GF_KOS_TRIGGER_1	
#define GF_TRIGGER_2		GF_KOS_TRIGGER_2	
#define GF_WPN_REL			GF_KOS_WPN_REL		
#define GF_R_SIDE			GF_KOS_R_SIDE		
#define GF_PINKIE			GF_KOS_PINKIE
#define GF_TRIM_UP			GF_KOS_TRIM_UP		
#define GF_TRIM_RIGHT		GF_KOS_TRIM_RIGHT	
#define GF_TRIM_DOWN		GF_KOS_TRIM_DOWN	
#define GF_TRIM_LEFT		GF_KOS_TRIM_LEFT	
#define GF_TRIM_CENTER		GF_KOS_TRIM_CENTER	
#define GF_DMS_UP			GF_KOS_DMS_UP		
#define GF_DMS_RIGHT		GF_KOS_DMS_RIGHT	
#define GF_DMS_DOWN			GF_KOS_DMS_DOWN		
#define GF_DMS_LEFT			GF_KOS_DMS_LEFT		
#define GF_DMS_CENTER		GF_KOS_DMS_CENTER
#define GF_TMS_UP			GF_KOS_TMS_UP		
#define GF_TMS_RIGHT		GF_KOS_TMS_RIGHT	
#define GF_TMS_DOWN			GF_KOS_TMS_DOWN		
#define GF_TMS_LEFT			GF_KOS_TMS_LEFT		
#define GF_TMS_CENTER		GF_KOS_TMS_CENTER	
#define GF_EX_1				GF_KOS_THUMB
#define GF_EX_2				0
#define GF_EX_3				0

#define GF_MENU_ACT_1		GF_KOS_R_SIDE
#define GF_MENU_ACT_2		GF_KOS_WPN_REL
#define GF_MENU_ACCEPT		GF_KOS_WPN_REL
#define GF_MENU_CANCEL		GF_KOS_R_SIDE
#define GF_MENU_UP			GF_KOS_TRIM_UP
#define GF_MENU_DOWN		GF_KOS_TRIM_DOWN
#define GF_MENU_LEFT		GF_KOS_TRIM_LEFT
#define GF_MENU_RIGHT		GF_KOS_TRIM_RIGHT
#endif