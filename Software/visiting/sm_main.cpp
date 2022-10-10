#include <Arduino.h>
#include <U8g2lib.h>

#include "project_config.h"
#include "debug.h"
#include "types.h"
#include "event.h"
#include "driver.h"
#include "utils.h"
#include "gui.h"
#include "led_driver.h"
#include "version.h"
#include "led_driver.h"
#include "regs.h"
#include "packet_defs.h"
#include "hc12.h"
#include "msg_mgr.h"
#include "sm_main.h"

FILENUM(5);

const uint16_t ONE_SECOND = EVENT_TIMER_RATE;
const uint16_t STARTUP_DISPLAY_PERIOD_SECS = 3;
const uint16_t MESSAGES_DISPLAY_PERIOD_SECS = 3;	  
const uint16_t HEARTBEAT_PERIOD_SECS = 10;
const uint16_t SELECT_TIMEOUT_SECS = 5;
const uint16_t CONFIRM_DIALOG_DURATION_SECS = 3;
const uint16_t UNPAIR_TIMEOUT_SECS = 5;
const uint16_t REMOTE_OFFLINE_TIMEOUT_SECS = 30;

const uint32_t CALL_STATION_OFFLINE_TIMEOUT_MS = 30UL*1000UL;

const uint16_t RETRY_COUNT = 2;

STATIC_ASSERT((int)CFG_REMOTE_COUNT >= (int)GPIO_NUM_LEDS); // Assume that we can deal with no less than the number of LEDs.
STATIC_ASSERT(CFG_REMOTE_COUNT >= HC12_NUM_INACTIVITY_TIMERS); // HC12 lib should be configured with timers for us. 

// Assign timers.
enum { 
	TIMER_0 = TIMER_SM_MAIN, 
	TIMER_HEARTBEAT = TIMER_0, 
	TIMER_SEL,
	TIMER_MSG,
};
#define start_timer(tid_, timeout_) \
	my_context->timer_cookie[(tid_) - TIMER_0] = eventTimerStart((tid_), (timeout_))
#define case_timer(tid_)															\
	case EVENT_MK_TIMER_EVENT_ID(tid_):												\
		if (event_get_param16(*ev) != my_context->timer_cookie[(tid_) - TIMER_0])  	\
			break;																	
		
/*
	Model to hold states of remote Call Stations and support presentation to user with simple GUI. 
	State can be:
	  0x0000: idle
	  0x00xx: call station alarm
	  0x8000: unused
	  0x0400: offline
*/

// Holds state of ALL call stations.
static struct {
	alarm_state_t statemasks[CFG_REMOTE_COUNT];		 // Same values in SEND_EVENT_MASK_xxx in packet_defs.h. A value of 0 implies unused, a value with bit 15 set is offline.
	bool updated_by_inactivity;
} f_cs_model;

// Check validity of all statemasks.
#define ASSERT_STATEMASK(sm_)															\
 ASSERT(																				\
		(A_OFFLINE == (sm_)) ||															\
		(A_UNUSED ==(sm_)) ||															\
		(0U == ((sm_) & ~(A_ALL_ALARMS|A_OFFLINE|A_UNUSED)))							\
		)
  
// Reset the model, so all used stations states get set to OFFLINE. Clears any alarm state. 
static void csm_reset() {
	fori (CFG_REMOTE_COUNT) {
		f_cs_model.statemasks[i] = ((REGS[REGS_IDX_REMOTE_MASK] & _BV(i)) ? A_OFFLINE : A_UNUSED);  // Set all used states to OFFLINE or UNUSED.
		if (REGS[REGS_IDX_REMOTE_MASK] & _BV(i)) 
			hc12TimerForceTimeout(i);				// Force Master to try contacting all used remotes (currently offline). 
		ASSERT_STATEMASK(f_cs_model.statemasks[i]);
	}
	f_cs_model.updated_by_inactivity = true;
}

// Update statemask for a station.
bool csmUpdateState(uint8_t idx, alarm_state_t setmask, alarm_state_t clearmask) {
	if (idx < CFG_REMOTE_COUNT) {	 // Only if index in range, we might get anything over the radio.
		const alarm_state_t sm = (f_cs_model.statemasks[idx] & ~clearmask) | setmask;  // SET overrides CLEAR. 
		if (sm != f_cs_model.statemasks[idx]) { // Has it actually changed?
			f_cs_model.statemasks[idx] = sm;
			ASSERT_STATEMASK(f_cs_model.statemasks[idx]);
			return true;
		}
	}
	return false; // No change in state.
}

// Maps set bits in statemask to what should be displayed.
typedef struct {
	uint16_t mask;
	const char* announce;
	uint8_t led_blink;
} led_states_t;
static const char S_UNUSED[] PROGMEM = "---";
static const char S_OFFLINE[] PROGMEM = "Offline";
static const char S_ALERT[] PROGMEM = "ALERT";
static const char S_CALL[] PROGMEM = "Call";
static const char S_ONLINE[] PROGMEM = "OK";
static const led_states_t LED_STATES[] PROGMEM = {
	{ A_UNUSED,				S_UNUSED,		LED_DRIVER_BLINK_OFF		 },
	{ A_OFFLINE,			S_OFFLINE,		LED_DRIVER_BLINK_OFFLINE	 },	 // Offline 2nd highest priority.
	{ A_ALERT,				S_ALERT,		LED_DRIVER_BLINK_ALERT		 },	 // Alert higher than Call
	{ A_CALL,				S_CALL,			LED_DRIVER_BLINK_CALL		 },	 // Call
	{ A_BATTERY_LOW,		NULL,			LED_DRIVER_BLINK_BATTERY_LOW },	 // Low battery alert.
	{ A_EXT_POWER_FAIL,		NULL,			LED_DRIVER_BLINK_NO_EXT_PWR	 },	 // No external power, must be running on batteries. 
	{ 0,					S_ONLINE,		LED_DRIVER_BLINK_ONLINE		 },	 
};
	
void csmUpdateLedState(uint8_t idx) {
	if (idx < GPIO_NUM_LEDS) {		// We only have a few LEDs...
		fori (ELEMENT_COUNT(LED_STATES)-1) {
			const alarm_state_t tm = utilsReadProgmem(&LED_STATES[i].mask); 
			if (f_cs_model.statemasks[idx] & tm) { // If set bits in message match set bits in table...
				ledDriverSet(idx, utilsReadProgmem(&LED_STATES[i].led_blink)); // Set LED corresponding to match, with last item (off) as a default.
				return;
			}
		}
	ledDriverSet(idx, utilsReadProgmem(&LED_STATES[ELEMENT_COUNT(LED_STATES)-1].led_blink));
	}
}

void csmGetAlarmStr(uint8_t idx, char* str) {		// No check for overflow.
	char* s = str;
	if (idx < CFG_REMOTE_COUNT) {
		fori (ELEMENT_COUNT(LED_STATES)-1) { 
			const alarm_state_t tm = utilsReadProgmem(&LED_STATES[i].mask);
			if (tm == (f_cs_model.statemasks[idx] & tm)) { // If set bits in message match set bits in table...
				const char* p = (const char*)utilsReadProgmem(&LED_STATES[i].announce); 
				if (p) {
					char c;
					while ('\0' != (c = utilsReadProgmem(p++)))
						*s++ = c;
					*s++ = '|';
				}
				if ((A_UNUSED == tm) || (A_OFFLINE == tm))	// If unused or offline only show unused announcement.
					break;
			}
		}
		if (s != str) {
			s -= 1; // Back up over last separator. 
			*s = '\0';	// You are terminated!
		}
		else 
			strcpy_P(str, utilsReadProgmem(&LED_STATES[ELEMENT_COUNT(LED_STATES)-1].announce));  
	}
	
}

static int8_t f_sel_idx; // Negative for no select. 
const int8_t NO_SEL = -1;
static char f_alarms[CFG_REMOTE_COUNT][25];		  // Gui has room for 10 stations. 

// GUI
//

enum {
	COL_0 = 0,			// LH margin.
	COL_1 = 0,
	COL_2 = 10,
	COL_3 = COL_2 + 8,
	COL_4 = 69,
	COL_5 = COL_4 + 8,
	COL_R = 128,		// RH margin. 
	COUNT_COL = 3,
	COL_1_W = COL_2 - COL_1,
	COL_2_W = COL_3 - COL_2,
	COL_3_W = COL_4 - COL_3,
	COL_4_W = COL_5 - COL_4,
	COL_5_W = COL_R - COL_5,
};
enum {			// Chose suitable for font. 
	HEIGHT = 10,
	START_ROW = 1,
};
enum { NUM_ROWS = CFG_REMOTE_COUNT/2 };
#define ROW(n_) (n_)*HEIGHT+START_ROW +1		// 
void w_main_draw_grid(const gui_w_def_t* w, gui_w_context_t* ctx) {
	(void)ctx;
	u8g2.setDrawColor(1);
	u8g2.drawHLine(w->x, w->y, w->w);
	fori (NUM_ROWS+1)
		u8g2.drawHLine(w->x, ROW(i+1), w->w);
	fori (NUM_ROWS) {
		u8g2.setCursor(COL_1+1+2, ROW(i+2)-1);
		u8g2.print(i+1);
	}
	u8g2.drawVLine(w->x, w->y, w->h);		// Vertical dividers.
	u8g2.drawVLine(COL_2, w->y, w->h);
	u8g2.drawVLine(COL_4, w->y, w->h);
	u8g2.drawVLine(w->x+w->w-1, w->y, w->h);
}

// Gui system - main window definition.
//
#define char const uint8_t PROGMEM
#include "graphics/batt_0.xbm"
#include "graphics/batt_1.xbm"
#include "graphics/batt_2.xbm"
#include "graphics/batt_3.xbm"
#include "graphics/batt_4.xbm"
#include "graphics/batt_none.xbm"
#include "graphics/power_fail.xbm"
#include "graphics/ani1.xbm"
#include "graphics/ani2.xbm"
#include "graphics/ani3.xbm"
#undef char

static const char HEADING_COL_2[] PROGMEM = "Room";
static const char HEADING_COL_4[] PROGMEM = "Remote";
enum { 
	GRID_H = ROW(NUM_ROWS+1)-START_ROW,
	ROW_H = HEIGHT,
};

static gui_bitmap_state f_bmps_anim;
static gui_bitmap_state f_bmps_remote[CFG_REMOTE_COUNT];
static void init_bmps() {
	memset(&f_bmps_anim, 0, sizeof(f_bmps_anim));
	memset(f_bmps_remote, 0, sizeof(f_bmps_remote));
}
static const gui_w_def_t W_MAIN[] PROGMEM = {
	{ COL_0,	ROW(0),		COL_R,		GRID_H,		w_main_draw_grid,	NULL,					NULL,									0 },	 						// Table borders. 
	{ COL_1+1,	ROW(0)+1,	8,			ROW_H-1,	guiDrawBmp,			guiServiceMpBitmap,		(void*)&f_bmps_anim,					0 },	 						// Animation top LH corner.
	{ COL_2+1,	ROW(0),		COL_2_W-1,	ROW_H,		guiDrawText,		NULL,					(void*)HEADING_COL_2,					GUI_W_FLAGS_MASK_PROGMEM },	 	// Heading "Room". 
	{ COL_4+1,	ROW(0),		COL_4_W-2,	ROW_H,		guiDrawText,		NULL,					(void*)HEADING_COL_4,					GUI_W_FLAGS_MASK_PROGMEM },	 	// Heading "Mobile". 
		
#define MK_ROW(n_)\
	{ COL_3+1,	ROW(n_),	COL_3_W-1,	ROW_H,		guiDrawText,		guiServiceMpText,		&f_alarms[(n_-1)*2],					0 }, \
	{ COL_5+1,	ROW(n_),	COL_5_W-2,	ROW_H,		guiDrawText,		guiServiceMpText,		&f_alarms[(n_-1)*2+1],					0 },
		
	MK_ROW(1)
	MK_ROW(2)
	MK_ROW(3)
	MK_ROW(4)
	MK_ROW(5)
	
#define MK_BATT_COND(n_)\
	{ COL_2+1,	ROW(n_)+1,	COL_2_W-1,	ROW_H-1,	guiDrawBmp,			guiServiceMpBitmap,		(void*)&f_bmps_remote[(n_-1)*2],		0 }, \
	{ COL_4+1,	ROW(n_)+1,	COL_4_W-1,	ROW_H-1,	guiDrawBmp,			guiServiceMpBitmap,		(void*)&f_bmps_remote[(n_-1)*2+1],		0 }, \

	// Declare these after the alarms text as all the alarms need to be together as we index them as 'W_MAIN_W_IDX_ALERTS_REMOTE_0 + i'.
	MK_BATT_COND(1)
	MK_BATT_COND(2)
	MK_BATT_COND(3)
	MK_BATT_COND(4)
	MK_BATT_COND(5)

};
static gui_w_context_t f_gui_contexts[ELEMENT_COUNT(W_MAIN)];
#include "repeat.h" //	   REPEAT(count, macro, data) => macro(0, data) macro(1, data) ... macro(count - 1, data)
#define MK_ENUM(n_, x_) x_##n_,
enum {
	W_MAIN_W_IDX_GRID,
	W_MAIN_W_IDX_ANIMATION,
	W_MAIN_W_IDX_HEADING_1,
	W_MAIN_W_IDX_HEADING_2,
	REPEAT(10, MK_ENUM, W_MAIN_W_IDX_ALERTS_REMOTE_)		// Generates 'W_MAIN_W_IDX_ALERTS_REMOTE_n' items.
	REPEAT(10, MK_ENUM, W_MAIN_W_IDX_BAT_COND_REMOTE_)		// Generates 'W_MAIN_W_IDX_BAT_COND_REMOTE_n' items.
};

static const gui_animation_t ANI_BMP_BATT_CHARGING[] PROGMEM = { batt_0_bits, batt_1_bits, batt_2_bits, batt_3_bits, batt_4_bits, batt_4_bits, NULL };
static const gui_animation_t ANI_BMP_BATT_NONE[] PROGMEM = { NULL };
static const gui_animation_t ANI_BMP_BATT_CHARGED[] PROGMEM = { batt_4_bits, NULL };
static const gui_animation_t ANI_BMP_BATT_LOW[] PROGMEM = { batt_0_bits, batt_0_bits, batt_none_bits, batt_none_bits, NULL };
static const gui_animation_t ANI_BMP_BATT_POWER_FAIL[] PROGMEM = { batt_0_bits, batt_0_bits, power_fail_bits, power_fail_bits, NULL };
static const gui_animation_t ANI_BMP_HAPPY[] PROGMEM = { ani1_bits, ani1_bits, ani1_bits, ani2_bits, ani1_bits, ani1_bits, ani3_bits, NULL };

static void init_gui_main() {
	guiSetWindow(W_MAIN, f_gui_contexts, ELEMENT_COUNT(W_MAIN));
	//guiSetEnables(GUI_ENABLE_MASK_DRAW_EXTENTS_ONLY);
	fori (CFG_REMOTE_COUNT)
	f_alarms[i][0] = '\0'; // Clear alarm text.
	f_bmps_anim.base = f_bmps_anim.current = (gui_animation_t)ANI_BMP_HAPPY;
}

static void set_animation(uint8_t bat_idx, const gui_animation_t* anim) {
	ASSERT(bat_idx < CFG_REMOTE_COUNT);
	// Can't get this to compile.
	//guiGenericUpdateWidgetData(&f_gui_contexts[bat_idx + W_MAIN_W_IDX_BAT_COND_REMOTE_0], &f_bmps_remote[bat_idx].current, anim);
	//guiGenericUpdateWidgetData(&f_gui_contexts[bat_idx + W_MAIN_W_IDX_BAT_COND_REMOTE_0], &f_bmps_remote[bat_idx].base, anim);

	f_bmps_remote[bat_idx].current = f_bmps_remote[bat_idx].base = (gui_animation_t)anim;
	guiWidgetSetDirty(bat_idx + W_MAIN_W_IDX_BAT_COND_REMOTE_0);
}

// Startup window definition.
//


#define W_DIALOG_ROW_H  	11
#define W_DIALOG_ROW(n_) 	((n_) * W_DIALOG_ROW_H)

#define W_STARTUP_NUM_ROWS 	4
#define W_STARTUP_W			100
#define W_STARTUP_H 		W_DIALOG_ROW(W_STARTUP_NUM_ROWS) + 1
#define W_STARTUP_Y 		((64-W_STARTUP_H)/2)
#define W_STARTUP_X 		((128-W_STARTUP_W)/2)

static void w_dialog_draw_border(const gui_w_def_t* w, gui_w_context_t* ctx) {
	(void)ctx;
	//const dialog_def_t* def = (dialog_def_t*)(&ctx->data);
	u8g2.drawFrame(w->x, w->y, w->w, w->h);
	u8g2.drawHLine(w->x, w->y+W_DIALOG_ROW_H, w->w);
}
static const char W_STARTUP_HEADING_TITLE[] PROGMEM = "TSA Call System";
static const char W_STARTUP_VERSION_INFO[]  PROGMEM = "V" VERSION_VER_STR " build " VERSION_BUILD_NUMBER_STR;
static const char W_STARTUP_BUILD_DATE[]    PROGMEM = VERSION_BUILD_TIMESTAMP;
static const char W_STARTUP_SYSTEM[]    	PROGMEM = CFG_SYSTEM;
static const gui_w_def_t W_STARTUP[] PROGMEM = {
	{ W_STARTUP_X, 		W_DIALOG_ROW(0)+W_STARTUP_Y, 	W_STARTUP_W, 	W_STARTUP_H, 		w_dialog_draw_border,	NULL,	 NULL, 								0 						 },			
	{ W_STARTUP_X+2, 	W_DIALOG_ROW(0)+W_STARTUP_Y,	W_STARTUP_W-4, 	W_DIALOG_ROW_H-1, 	guiDrawText,			NULL,	 (void*)W_STARTUP_HEADING_TITLE, 	GUI_W_FLAGS_MASK_CENTRED | GUI_W_FLAGS_MASK_PROGMEM },		   
	{ W_STARTUP_X+2, 	W_DIALOG_ROW(1)+W_STARTUP_Y,	W_STARTUP_W-4, 	W_DIALOG_ROW_H-1, 	guiDrawText,			NULL,	 (void*)W_STARTUP_SYSTEM, 			GUI_W_FLAGS_MASK_CENTRED | GUI_W_FLAGS_MASK_PROGMEM },			  
	{ W_STARTUP_X+2, 	W_DIALOG_ROW(2)+W_STARTUP_Y,	W_STARTUP_W-4, 	W_DIALOG_ROW_H-1, 	guiDrawText,			NULL,	 (void*)W_STARTUP_VERSION_INFO, 	GUI_W_FLAGS_MASK_CENTRED | GUI_W_FLAGS_MASK_PROGMEM },			  
	{ W_STARTUP_X+2, 	W_DIALOG_ROW(3)+W_STARTUP_Y,	W_STARTUP_W-4, 	W_DIALOG_ROW_H-1, 	guiDrawText,			NULL,	 (void*)W_STARTUP_BUILD_DATE, 		GUI_W_FLAGS_MASK_CENTRED | GUI_W_FLAGS_MASK_PROGMEM },			  
};

enum {
	W_STARTUP_W_IDX_BORDER,
	W_STARTUP_W_IDX_TITLE,
	W_STARTUP_W_IDX_SYSTEM,
	W_STARTUP_W_IDX_VERSION,
	W_STARTUP_W_IDX_BUILD,
};
static void init_gui_startup() {
	guiSetWindow(W_STARTUP, f_gui_contexts, ELEMENT_COUNT(W_STARTUP));
	// guiSetEnables(GUI_ENABLE_MASK_DRAW_EXTENTS_ONLY);
}

// Remote config window definition.
//

#define W_MENU_NUM_ROWS 	5
#define W_MENU_W			78
#define W_MENU_H 			W_DIALOG_ROW(W_STARTUP_NUM_ROWS) + 1
#define W_MENU_Y 			((64-W_MENU_H)/2)
#define W_MENU_X 			((128-W_MENU_W)/2)

static const gui_w_def_t W_REMOTE_MENU[] PROGMEM = {
	{ W_MENU_X, 	W_DIALOG_ROW(0)+W_MENU_Y, 	W_MENU_W, 		W_MENU_H, 			w_dialog_draw_border,	NULL,	 (void*)0, 						0 						 },			
	{ W_MENU_X+1, 	W_DIALOG_ROW(0)+W_MENU_Y,	W_MENU_W-2, 	W_DIALOG_ROW_H-1, 	guiDrawText,			NULL,	 NULL,							GUI_W_FLAGS_MASK_CENTRED },		   
	{ W_MENU_X+1, 	W_DIALOG_ROW(1)+W_MENU_Y,	W_MENU_W-2, 	W_DIALOG_ROW_H-1, 	guiDrawText,			NULL,	 NULL, 							GUI_W_FLAGS_MASK_CENTRED },		   
	{ W_MENU_X+1, 	W_DIALOG_ROW(2)+W_MENU_Y,	W_MENU_W-2, 	W_DIALOG_ROW_H-1, 	guiDrawText,			NULL,	 NULL, 							GUI_W_FLAGS_MASK_CENTRED },		   
	{ W_MENU_X+1, 	W_DIALOG_ROW(3)+W_MENU_Y,	W_MENU_W-2, 	W_DIALOG_ROW_H-1, 	guiDrawText,			NULL,	 NULL, 							GUI_W_FLAGS_MASK_CENTRED },		   
	{ W_MENU_X+1, 	W_DIALOG_ROW(4)+W_MENU_Y,	W_MENU_W-2, 	W_DIALOG_ROW_H-1, 	guiDrawText,			NULL,	 NULL, 							GUI_W_FLAGS_MASK_CENTRED },		   
};
enum {
	W_REMOTE_MENU_W_IDX_BORDER,
	W_REMOTE_MENU_W_IDX_TITLE,
	W_REMOTE_MENU_W_IDX_LINE_0,
	W_REMOTE_MENU_W_IDX_LINE_1,
	W_REMOTE_MENU_W_IDX_LINE_2,
	W_REMOTE_MENU_W_IDX_LINE_3,
};
static void init_gui_remote_menu() {
	guiSetWindow(W_REMOTE_MENU, f_gui_contexts, ELEMENT_COUNT(W_REMOTE_MENU));
}

void guiLocalService(const gui_w_def_t* w, gui_w_context_t* ctx, uint8_t phase) {
	(void)w;
	(void)ctx;
	(void)phase;
}

static void update_remote_gui(uint8_t idx) {
	// Alarms
	csmGetAlarmStr(idx, f_alarms[idx]);									// Get string describing alarms from model into buffer associated with gui widget.
	guiWidgetLoadDefaultData(W_MAIN_W_IDX_ALERTS_REMOTE_0 + idx);		// Set gui data pointer to default which is first element into buffer in case a service function has updated it.
	// guiWidgetSetClearFlags(W_MAIN_W_IDX_ALERTS_REMOTE_0 + idx, GUI_W_FLAGS_MASK_BLINK, !!(f_cs_model.statemasks[idx] & (A_CALL | A_ALERT)));
	guiWidgetSetDirty(W_MAIN_W_IDX_ALERTS_REMOTE_0 + idx);				// But guiWidgetLoadDefaultData() might not have changed the value of gui-data, so set it dirty.

	// Battery status.
	typedef struct {
		uint16_t mask;
		const gui_animation_t* anim;
	} anim_lookup_t;
		

	static const anim_lookup_t ANIMS[] PROGMEM = {
		{ A_UNUSED | A_OFFLINE,		ANI_BMP_BATT_NONE },
		{ A_BATTERY_LOW,			ANI_BMP_BATT_LOW },
		{ A_BATTERY_CHARGING,		ANI_BMP_BATT_CHARGING },
		{ A_EXT_POWER_FAIL,			ANI_BMP_BATT_POWER_FAIL },
		{ 0,						ANI_BMP_BATT_CHARGED },
	};

	uint8_t x;
	for	(x = 0; x < (ELEMENT_COUNT(ANIMS) - 1); x += 1) {
		if (utilsReadProgmem(&ANIMS[x].mask) & f_cs_model.statemasks[idx]) 
			break;
	}
	
	set_animation(idx, utilsReadProgmem(&ANIMS[x].anim));
}

static void update_announcements(int8_t idx=-1) {
	if (idx < 0) {
		fori (CFG_REMOTE_COUNT) {
			update_remote_gui(i);
			csmUpdateLedState(i);
		}
	}
	else if (idx < CFG_REMOTE_COUNT) {
		update_remote_gui(idx);
		csmUpdateLedState(idx);
	}
}
static void gui_main_update_on_selection() {
	fori (CFG_REMOTE_COUNT) 
		guiWidgetSetClearFlags(W_MAIN_W_IDX_ALERTS_REMOTE_0 + i, GUI_W_FLAGS_MASK_BLINK | GUI_W_FLAGS_MASK_INVERT, (f_sel_idx == i));
}
static void gui_menu_update_on_selection() {
	fori (W_MENU_NUM_ROWS - 1) 
		guiWidgetSetClearFlags(W_REMOTE_MENU_W_IDX_LINE_0 + i, GUI_W_FLAGS_MASK_INVERT, (f_sel_idx == i));
}
static uint8_t menu_remote_idx;

// TODO: only send 1 ping at a time, only send if 
static void csm_service(const event_t* ev) {
	if (event_get_id(*ev) == EV_HC12_INACTIVITY) {
		const uint8_t remote_idx = event_get_param8(*ev);
		if (remote_idx < CFG_REMOTE_COUNT) {
			if (!(f_cs_model.statemasks[remote_idx] & A_UNUSED)) {	// We don't care about unused remotes. 
				uint8_t msg[PACKET_QUERY_ALARM_TX_LEN] = { PACKET_QUERY_ALARM_CMD };
				hc12Send(DEFS_STATION_ID_CALL + remote_idx, sizeof(msg), &msg[0], 0); // Send an ack message once.
				if (csmUpdateState(remote_idx, A_OFFLINE, 0U)) // Set OFFLINE state, might still have a pending alarm that we don't touch. 
					f_cs_model.updated_by_inactivity = true; 		// TODO: This might result in the gui briefly showing offline if the remote is slow.
			}
			hc12TimerRestart(remote_idx, REGS[REGS_IDX_HEARTBEAT_PERIOD_SECS]);					
		}
	}
}

int8_t sm_main(EventSmContextBase* context, event_t* ev) {
	enum { ST_STARTUP, ST_MESSAGES, ST_IDLE, ST_MENU, ST_CONFIRM, ST_PAIRING, };
	SmMainContext* my_context = (SmMainContext*)context;		// Downcast to derived class. 
	
	csm_service(ev);
	
	switch (context->state) {
		
	// Startup, display software version and any errors. 
	case ST_STARTUP:
		switch(event_get_id(*ev)) {
		case EV_SM_ENTRY:
			init_gui_startup();
			start_timer(TIMER_0, STARTUP_DISPLAY_PERIOD_SECS*ONE_SECOND); 
			break;
		case_timer(TIMER_0)	  
			return (msgMgrGetMessageCount() > 0) ? ST_MESSAGES: ST_IDLE;
		}
		break;
		
	case ST_MESSAGES:
		switch(event_get_id(*ev)) {
		case EV_SM_ENTRY:
			//init_gui_messages();
			start_timer(TIMER_0, MESSAGES_DISPLAY_PERIOD_SECS*ONE_SECOND); 
			break;
		case_timer(TIMER_0)	  
			return ST_IDLE;
		}
		break;
		
	// Normal operating mode.
	case ST_IDLE:
		switch(event_get_id(*ev)) {
		case EV_SM_ENTRY:
			init_bmps();
			init_gui_main();
			f_sel_idx = NO_SEL;
			gui_main_update_on_selection();
			start_timer(TIMER_HEARTBEAT, 1);
			csm_reset();
			update_announcements();
			break;
		case EV_RESET_GUI:
			return ST_IDLE;				// Executive runs exit actions for this state then entry actions so effectively a restart.
		case EV_REMOTE_EVENT:
			{
				const uint8_t csm_idx = event_get_param8(*ev);	
				//const uint8_t type_id = event_get_param16(*ev) >> 8;	
				alarm_state_t alarm_flags = (alarm_state_t)event_get_param16(*ev);
				
				if (csm_idx < CFG_REMOTE_COUNT) {
					hc12TimerRestart(csm_idx, REGS[REGS_IDX_HEARTBEAT_PERIOD_SECS]);					

					// Update state model, and update LCD & LEDs if it has changed. 
					if (csmUpdateState(csm_idx, alarm_flags, A_OFFLINE | A_ALL_ALARMS)) {	 // Set flags from message, clear offline flag as we have a message, so comms is good. 
						update_announcements(csm_idx);
					}
				}
			}
			break;
		case EV_SW_ENCODER:
			if ((event_get_param8(*ev) == EV_P8_SW_CLICK) && (f_sel_idx >= 0))
				return ST_MENU;
			break;		  
			case EV_ENCODER_BUMP:
			{
				bool update = false;
				if (f_sel_idx < 0) {
					f_sel_idx = 0;
					update = true;
				}
				else
					update = (utilsBumpU8((uint8_t*)&f_sel_idx, (int8_t)event_get_param8(*ev), 0, CFG_REMOTE_COUNT - 1, true)); // Index guaranteed in range 0..N
				if (update) {
					gui_main_update_on_selection();
					start_timer(TIMER_SEL, SELECT_TIMEOUT_SECS*ONE_SECOND);
				}
			}
			break;
		case EV_SW_CANCEL:
			if (event_get_param8(*ev) == EV_P8_SW_CLICK) {
				if (f_sel_idx >= 0) {
					uint8_t msg[PACKET_CLEAR_ALARM_TX_LEN];
					msg[0] = PACKET_CLEAR_ALARM_CMD;
					msg[1] = A_CALL | A_ALERT;
					// my_context->ack_seq = f_sel_idx;	// Record current selected remote idx, it might change by the time we get an ACK. 
					my_context->ack_seq = hc12Send(DEFS_STATION_ID_CALL + f_sel_idx, sizeof(msg), &msg[0], RETRY_COUNT); // Publishes either EV_HC12_ACK_OK or EV_HC12_ACK_FAIL
				}
			}
			break;
		case EV_HC12_ACK_OK:
			if (event_get_param16(*ev) == my_context->ack_seq) { // We got an ack to our cancel request.
				my_context->ack_seq = 0;		// Clear ack waiting status. 
			}
			break;
		case EV_HC12_ACK_FAIL:
			if (event_get_param16(*ev) == my_context->ack_seq) { // We DID NOT got an ack. 
				my_context->ack_seq = 0;		// Clear ack waiting status.		 
				csmUpdateState(my_context->ack_seq, A_OFFLINE, 0U); // Set OFFLINE state, might still have a pending alarm that we don't touch. 
				f_cs_model.updated_by_inactivity = true;
			} break;
		case_timer(TIMER_SEL)
				f_sel_idx = NO_SEL;
				gui_main_update_on_selection();
				break;
		case_timer(TIMER_HEARTBEAT)
			start_timer(TIMER_HEARTBEAT, HEARTBEAT_PERIOD_SECS*ONE_SECOND);
			if (f_cs_model.updated_by_inactivity) {
				update_announcements();
				f_cs_model.updated_by_inactivity = false;
			}
			break;
		}
		break;
		
	case ST_MENU:
		switch(event_get_id(*ev)) {
		case EV_SM_ENTRY:
		{
			menu_remote_idx = f_sel_idx;		// Save index of which remote we are menuing. 
			init_gui_remote_menu();
			
			guiWidgetWriteData(W_REMOTE_MENU_W_IDX_TITLE, (void*)PSTR("Remote Setup"), GUI_W_FLAGS_MASK_PROGMEM);							// Title.
			
			bool enabled = !!(REGS[REGS_IDX_REMOTE_MASK] & _BV(menu_remote_idx));															// Line 0.
			guiWidgetWriteData(W_REMOTE_MENU_W_IDX_LINE_0, (void*)(enabled ? PSTR("Disable") : PSTR("Enable")), GUI_W_FLAGS_MASK_PROGMEM);
			
			guiWidgetWriteData(W_REMOTE_MENU_W_IDX_LINE_1, (void*)(enabled ? PSTR("Unpair") : PSTR("Pair")), GUI_W_FLAGS_MASK_PROGMEM);		// Line 1. 
			
			if (enabled) {																													// Line 2. 
				guiWidgetWriteData(W_REMOTE_MENU_W_IDX_LINE_2, (void*)PSTR("Setup"), GUI_W_FLAGS_MASK_PROGMEM);
				guiWidgetWriteData(W_REMOTE_MENU_W_IDX_BORDER, (void*)3);
			}
			else {
				guiWidgetWriteData(W_REMOTE_MENU_W_IDX_LINE_2, NULL);
				guiWidgetWriteData(W_REMOTE_MENU_W_IDX_BORDER, (void*)2);
			}
			guiWidgetWriteData(W_REMOTE_MENU_W_IDX_LINE_3, NULL);

			f_sel_idx = 0;
			gui_menu_update_on_selection();
			start_timer(TIMER_SEL, SELECT_TIMEOUT_SECS*ONE_SECOND);
		}
			break;
		case EV_ENCODER_BUMP:
			{
				bool update = false;
				if (f_sel_idx < 0) {
					f_sel_idx = 0;
					update = true;
				}
				else {
					const uint8_t menu_item_count = (uint8_t)(uint16_t)guiWidgetReadData(W_REMOTE_MENU_W_IDX_BORDER);
					update = (utilsBumpU8((uint8_t*)&f_sel_idx, (int8_t)event_get_param8(*ev), 0, menu_item_count - 1, true)); // Index guaranteed in range 0,,N
				}

				start_timer(TIMER_SEL, SELECT_TIMEOUT_SECS*ONE_SECOND);
				if (update) {
					gui_menu_update_on_selection();
				}
			}
			break;
		case EV_SW_ENCODER:
			if ((event_get_param8(*ev) == EV_P8_SW_CLICK) && (f_sel_idx >= 0)) {
				// Clear most lines.
				guiWidgetWriteData(W_REMOTE_MENU_W_IDX_LINE_1, NULL);
				guiWidgetWriteData(W_REMOTE_MENU_W_IDX_LINE_2, NULL);
				guiWidgetWriteData(W_REMOTE_MENU_W_IDX_LINE_3, NULL);
				guiWidgetWriteData(W_REMOTE_MENU_W_IDX_BORDER, (void*)1);
				
				hc12CancelAck();
				
				// What to do? 
				switch (f_sel_idx) {
					case 0:	{	// Disable/Enable
						REGS[REGS_IDX_REMOTE_MASK] ^= _BV(menu_remote_idx);
						regsNvWrite();
						guiWidgetWriteData(W_REMOTE_MENU_W_IDX_TITLE, (void*)PSTR("Remote Setup"));
						static char msg[20];
						sprintf_P(msg, PSTR("R%u %S"), menu_remote_idx, (REGS[REGS_IDX_REMOTE_MASK] & _BV(menu_remote_idx)) ? PSTR("enabled") : PSTR("disabled"));
						guiWidgetWriteData(W_REMOTE_MENU_W_IDX_LINE_0, (void*)&msg, GUI_W_FLAGS_MASK_PROGMEM, false);
						return ST_CONFIRM;
					}
					case 1:	{	// Pair / Unpair.
						const bool paired = !!(REGS[REGS_IDX_REMOTE_MASK] & _BV(menu_remote_idx));		// For now if enabled we are assumed to be paired.
						regsWriteMask(REGS_IDX_REMOTE_MASK, _BV(menu_remote_idx), !paired);				// Flip enabled flag in registers. Model reloaded in main gui. 
						regsNvWrite();
						
						// Save some state.
						const uint8_t REMOTE_ADDRESS = DEFS_STATION_ID_CALL + menu_remote_idx;
						if (paired) {
							my_context->pairing_new_address = DEFS_STATION_ID_DEFAULT;	
							my_context->pairing_current_address = REMOTE_ADDRESS;	
						}
						else {
							my_context->pairing_new_address = REMOTE_ADDRESS;	
							my_context->pairing_current_address = DEFS_STATION_ID_DEFAULT;	
						}
						
						// Send message.
						uint8_t msg[PACKET_SET_ADDRESS_TX_LEN] = { 
							PACKET_SET_ADDRESS_CMD,  
							my_context->pairing_new_address,
						};
						my_context->ack_seq = hc12Send(my_context->pairing_current_address, sizeof(msg), &msg[0], 3); 
						
						// Tell the user. 
						guiWidgetWriteData(W_REMOTE_MENU_W_IDX_TITLE, (void*)(paired ? PSTR("Unpair") : PSTR("Pair")), GUI_W_FLAGS_MASK_PROGMEM);
						guiWidgetWriteData(W_REMOTE_MENU_W_IDX_LINE_0, (void*)PSTR("Waiting.."), GUI_W_FLAGS_MASK_PROGMEM);
						return ST_PAIRING;
					}
					case 2:		// Setup
						guiWidgetWriteData(W_REMOTE_MENU_W_IDX_TITLE, (void*)(PSTR("Alert")));
						guiWidgetWriteData(W_REMOTE_MENU_W_IDX_LINE_0, (void*)(PSTR("Not done!")));
						return ST_CONFIRM;
				}
			}
			break;
			
		case EV_SW_CANCEL:				// Cancel out.
			if (event_get_param8(*ev) == EV_P8_SW_CLICK) 
				return ST_IDLE;
			break;		  
		case_timer(TIMER_SEL)			// Timeout with no encoder action. 
				return ST_IDLE;
			break;
		}
		break;

	// Simple wait for timeout to view final message.
	case ST_CONFIRM:		
		switch(event_get_id(*ev)) {
		case EV_SM_ENTRY:
			f_sel_idx = NO_SEL;
			gui_menu_update_on_selection();
			start_timer(TIMER_SEL, CONFIRM_DIALOG_DURATION_SECS*ONE_SECOND);
			break;
		case_timer(TIMER_SEL)		
			return ST_IDLE;
		}
		break; 		

	// Master has sent PACKET_SET_ADDRESS_REQ_CMD and is waiting an ack. 
	case ST_PAIRING:
		switch(event_get_id(*ev)) {
		case EV_SM_ENTRY:
			start_timer(TIMER_SEL, UNPAIR_TIMEOUT_SECS*ONE_SECOND);
			break;
		case EV_REMOTE_SET_ADDRESS_ACK:											// ACK to PACKET_CMD_SET_ADDRESS_REQ.
			if (
			  (event_get_param8(*ev) == my_context->pairing_current_address) &&	// Is from the remote we are trying to pair?
			  (0 == event_get_param16(*ev)) 									// Did remote return success?
			) { 								
				guiWidgetWriteData(W_REMOTE_MENU_W_IDX_LINE_0, (void*)PSTR("Success!"), GUI_W_FLAGS_MASK_PROGMEM);
				return ST_CONFIRM;
			}
			else {
				guiWidgetWriteData(W_REMOTE_MENU_W_IDX_LINE_0, (void*)PSTR("Failed!"), GUI_W_FLAGS_MASK_PROGMEM);
				return ST_CONFIRM;
			}
			break;
			case_timer(TIMER_SEL)
				guiWidgetWriteData(W_REMOTE_MENU_W_IDX_LINE_0, (void*)PSTR("Failed!"), GUI_W_FLAGS_MASK_PROGMEM);
				return ST_CONFIRM;
		}
		break;
		
	}  	// Closes switch (context->state) ...
	return SM_NO_CHANGE;
}
