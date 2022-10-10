#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#include "project_config.h"
#include "gpio.h"
#include "debug.h"
#include "utils.h"
#include "gui.h"

FILENUM(4);

// The LCD driver. 
U8G2_ST7920_128X64_F_HW_SPI u8g2(U8G2_R2, GPIO_PIN_LCD_SEL);

// Local data.
static struct {
	const gui_w_def_t* current_window_defs;
	gui_w_context_t* current_window_ctx;
	uint8_t current_window_len;
	uint8_t enables;
	bool is_dirty;
	uint8_t s_phase;							// Service phase, incremented every call to do_service(). Multipart widgets use it mod item-count so that they stay in sync with each other.
	int8_t selection;
} f_gui_locals;

// Selection.
//

// Debug function for verifying that there are no widgets flagged with the mask.
static bool check_widget_flag_clear(uint16_t mask) {
	fori (f_gui_locals.current_window_len) {
		if (f_gui_locals.current_window_ctx[i].flags & mask)
			return false;
	}
	return true;
}

static bool is_w_selectable(uint16_t f) { return ((f & GUI_W_FLAGS_MASK_SELECTABLE) && !(f & GUI_W_FLAGS_MASK_DISABLED)); }
static void wrap_selection(int8_t bump) { utilsBump<int8_t>(&f_gui_locals.selection, bump, 0, f_gui_locals.current_window_len-1, true); }

int8_t guiGetSelection(int8_t dir) { 			// int8_t guiGetSelection(int8_t dir=0);
	if (f_gui_locals.selection < 0) {			// If no selection indicated, 
		f_gui_locals.selection = 0;				// If the window asks for selection, and we have nothing selected, move to first item.
	}
	else {  // If selection indicated, the widget should be flagged as selected. 
		ASSERT(f_gui_locals.selection < f_gui_locals.current_window_len);  // Selection must be in range. 
		ASSERT(f_gui_locals.current_window_ctx[f_gui_locals.selection].flags & GUI_W_FLAGS_MASK_SELECTED);
		guiWidgetClearFlags(f_gui_locals.selection, GUI_W_FLAGS_MASK_SELECTED);
	}
	ASSERT(check_widget_flag_clear(GUI_W_FLAGS_MASK_SELECTED));		// Verify that no widgets are flagged as selected. 
	
	// Apply offset.
	wrap_selection(dir);

	// Scan until we find a selectable widget. 
	fori (f_gui_locals.current_window_len) {		// Iterate over every widget. 
		if (is_w_selectable(f_gui_locals.current_window_ctx[f_gui_locals.selection].flags)) { // Is selectable?
			guiWidgetSetFlags(f_gui_locals.selection, GUI_W_FLAGS_MASK_SELECTED);		// Set it to selected. 
			return f_gui_locals.selection;					// Back home. 
		}
		wrap_selection((dir < 0) ? -1 : +1);
	}
	
	// Nothing selectable
	f_gui_locals.selection = GUI_NO_SEL;
	return f_gui_locals.selection;
}

// Useful functions
void guiCtxSetDirty(gui_w_context_t* ctx) {
	ctx->flags |= GUI_W_FLAGS_MASK_DIRTY;
	f_gui_locals.is_dirty = true;
}

// TODO: Merge with other load default func. 
static void load_widget_data_from_default(const gui_w_def_t* w, gui_w_context_t* ctx) {
	guiGenericUpdateWidgetData(ctx, &ctx->data, (void*)pgm_read_word(&w->default_data));
}

static char read_pointer_generic(const gui_w_context_t* ctx, const char* s) {
	return (ctx->flags & GUI_W_FLAGS_MASK_PROGMEM) ? pgm_read_byte(s) : *s;
}

void guiServiceMpText(const gui_w_def_t* w, gui_w_context_t* ctx, uint8_t phase) {
	(void)phase;
	const char* str = (const char*)ctx->data;  
	if ((NULL != str) && ('\0' != read_pointer_generic(ctx, str))) {					// We could be handed a NULL pointer or it might point to a NULL. The draw func will handle this. 
		// Attempt to select next portion, else set to zero and select that. 
		do {		// Do loop so go past nul or '|' on first pass. 
			 str += 1;	
		} while (('\0' != read_pointer_generic(ctx, str)) && ('|' != read_pointer_generic(ctx, str)));
		if ('\0' == read_pointer_generic(ctx, str))
			 load_widget_data_from_default(w, ctx);	 // Will set dirty if data changes.
		else {
			if ('\0' == read_pointer_generic(ctx, str + 1))				// Fix if source ends in a '|'.
				load_widget_data_from_default(w, ctx);
			else {
				ctx->data = (void*)(str + 1);  // Move past '|' at end. 
				guiCtxSetDirty(ctx);
			}
		}
	}
}	
	
enum { X_OFF = 2, Y_OFF = -1 };  
void guiDrawText(const gui_w_def_t* w, gui_w_context_t* ctx) {
	const char* str = (const char*)ctx->data;
	char buf[20];
	char* b = buf;
	if (NULL != str) {
		while (('\0' != read_pointer_generic(ctx, str)) && ('|' != read_pointer_generic(ctx, str)))
			*b++ = read_pointer_generic(ctx, str++);
		*b = '\0';
	
		volatile int8_t offset = 0;
		if (ctx->flags & GUI_W_FLAGS_MASK_CENTRED) {
			volatile int8_t str_w = u8g2.getStrWidth(buf);
			offset = (w->w - X_OFF - str_w) / 2;
			if (offset < 0) offset = 0;
		}
	
		u8g2.drawStr(w->x+X_OFF+offset, w->y+Y_OFF+w->h, buf);
	}
}

// Bitmaps expect to have the data item point to a gui_bitmap_state object that holds the addresses of the base of the animation and the current position, terminated with a NULL. 
static gui_bitmap_state* get_bitmap_state(gui_w_context_t* ctx) {
	gui_bitmap_state* pbmp = (gui_bitmap_state*)ctx->data;
	return ((NULL != pbmp) && (NULL != pbmp->base) && (NULL != pbmp->current) && (0 != pgm_read_word(pbmp->current))) ? pbmp : NULL;
}
void guiDrawBmp(const gui_w_def_t* w, gui_w_context_t* ctx) {
	gui_bitmap_state* pbmp = get_bitmap_state(ctx);
	if (pbmp)
		u8g2.drawXBMP(w->x, w->y, w->w, w->h, (const uint8_t*)pgm_read_word(pbmp->current));
}
void guiServiceMpBitmap(const gui_w_def_t* w, gui_w_context_t* ctx, uint8_t phase) { // Data is PROGMEM address Null terminated array of bitmaps. 
	(void)w;
	(void)phase;
	gui_bitmap_state* pbmp = get_bitmap_state(ctx);
	if (pbmp) {
		const gui_animation_t pnext = pbmp->current + 2;		// Why 2?? TODO: Fixme.
		guiGenericUpdateWidgetData(ctx, &pbmp->current, (const uint8_t*)((0 != pgm_read_word(pnext)) ? pnext : pbmp->base));	 // Will set dirty if data changes.
	}
}

// Current window.
void guiSetWindow(const gui_w_def_t* wnd_def, gui_w_context_t* wnd_ctx, uint8_t wnd_len) {
	f_gui_locals.selection = GUI_NO_SEL;
	f_gui_locals.current_window_defs = wnd_def;
	f_gui_locals.current_window_ctx = wnd_ctx;
	memset(f_gui_locals.current_window_ctx, 0, sizeof(gui_w_context_t) * wnd_len); // Just to be sure. 
	f_gui_locals.current_window_len = wnd_len;
	fori (f_gui_locals.current_window_len)		// Write default flags to context. 
		f_gui_locals.current_window_ctx[i].flags = pgm_read_word(&f_gui_locals.current_window_defs[i].default_flags);
	fori (f_gui_locals.current_window_len)	   // Write default data to context. 
		guiWidgetLoadDefaultData(i);
	guiWidgetSetDirty();
	u8g2.clearBuffer();	
}

// Global settings.
void guiSetEnables(uint8_t x) { f_gui_locals.enables = x; }
uint8_t guiGetEnables() { return f_gui_locals.enables; }

// Flags access.
void guiWidgetWriteFlags(uint8_t widx, uint16_t flags) {
	guiGenericUpdateWidgetData(&f_gui_locals.current_window_ctx[widx], &f_gui_locals.current_window_ctx[widx].flags, flags);
}
uint16_t guiWidgetGetFlags(uint8_t widx) { return f_gui_locals.current_window_ctx[widx].flags; }
void guiWidgetFlipFlags(uint8_t widx, uint16_t mask) { guiWidgetWriteFlags(widx, f_gui_locals.current_window_ctx[widx].flags ^ mask); }
void guiWidgetSetFlags(uint8_t widx, uint16_t mask)	{ guiWidgetWriteFlags(widx, f_gui_locals.current_window_ctx[widx].flags | mask); }
void guiWidgetClearFlags(uint8_t widx, uint16_t mask)  { guiWidgetWriteFlags(widx, f_gui_locals.current_window_ctx[widx].flags & ~mask); }
void guiWidgetSetClearFlags(uint8_t widx, uint16_t mask, bool f) { 
	guiWidgetWriteFlags(widx, f ? (f_gui_locals.current_window_ctx[widx].flags | mask) : (f_gui_locals.current_window_ctx[widx].flags & ~mask));
}

// Widget data access.

void guiWidgetWriteData(uint8_t widx, void* data, uint16_t mask, bool set) {			//  void guiWidgetWriteData(uint8_t widx, void* data, uint16_t mask=0U, bool set=true);	
	guiGenericUpdateWidgetData(&f_gui_locals.current_window_ctx[widx], &f_gui_locals.current_window_ctx[widx].data, data);
	guiGenericUpdateWidgetData(&f_gui_locals.current_window_ctx[widx], &f_gui_locals.current_window_ctx[widx].flags, 
	  set ? (f_gui_locals.current_window_ctx[widx].flags | mask) : (f_gui_locals.current_window_ctx[widx].flags & ~mask));
}
void* guiWidgetReadData(uint8_t widx) { 
	return f_gui_locals.current_window_ctx[widx].data; 
}
void guiWidgetLoadDefaultData(uint8_t widx) {
	guiWidgetWriteData(widx, (void*)pgm_read_word(&f_gui_locals.current_window_defs[widx].default_data));
}

void guiInit() { 
	f_gui_locals.current_window_defs = NULL;
	f_gui_locals.current_window_ctx = NULL;
	f_gui_locals.current_window_len = 0;
	f_gui_locals.enables = 0;
	u8g2.begin();  
	// u8g2.setBusClock(2000000UL);	 // ST7920 only works at 1MHz it seems. It's a bit of a slow dog.
	pinMode(GPIO_PIN_LCD_BACKLIGHT, OUTPUT);
	digitalWrite(GPIO_PIN_LCD_BACKLIGHT, true); 
}

// Dirty flag. 
void guiWidgetSetDirty(int8_t widx, bool f) {		// void guiWidgetSetDirty(int8_t widx=-1, bool f=true);		
	if (f) {
		if (widx < 0) {
			fori(f_gui_locals.current_window_len)
				guiCtxSetDirty(&f_gui_locals.current_window_ctx[i]); 
			f_gui_locals.enables |= GUI_ENABLE_MASK_CLEAR_BEFORE_DRAW;
		}
		else
			guiCtxSetDirty(&f_gui_locals.current_window_ctx[widx]); 
	}
}
void guiWidgetSetClean(uint8_t widx)  { 		// void guiWidgetSetClean(uint8_t widx=-1);	
	if (widx < 0) {
		fori(f_gui_locals.current_window_len)
			guiWidgetSetClean(i);
		f_gui_locals.is_dirty = false;
	}
	else
		f_gui_locals.current_window_ctx[widx].flags &= ~GUI_W_FLAGS_MASK_DIRTY; 
}
static bool guiWidgetIsAllClean()  {  
	return !f_gui_locals.is_dirty;
}

// Drawing.

// Draw a single widget.
static void draw_widget(uint8_t widx) {
	gui_w_def_t w;
	memcpy_P(&w, &f_gui_locals.current_window_defs[widx], sizeof(w));

	if (guiGetEnables() & GUI_ENABLE_MASK_DRAW_EXTENTS_ONLY) {
		u8g2.setDrawColor(2);
		u8g2.drawFrame(w.x, w.y, w.w, w.h-1);
	}
	else {
		// Choose whether to set or clear the box.
		u8g2.setDrawColor((f_gui_locals.current_window_ctx[widx].flags & GUI_W_FLAGS_MASK_INVERT) ? 1 : 0);
		u8g2.drawBox(w.x, w.y+1, w.w, w.h-1);		// Draw filled or clear box.	// TODO: Magic +1?
		
		u8g2.setDrawColor((f_gui_locals.current_window_ctx[widx].flags & GUI_W_FLAGS_MASK_INVERT) ? 0 : 1); // Opposite colour
		w.draw(&w, &f_gui_locals.current_window_ctx[widx]);	// Do the draw.
	}
	
	// Assumes that draw function will take any action for resync flag, so clear it now its job is done. Also clear dirty as we have just drawn it. 
	f_gui_locals.current_window_ctx[widx].flags &= ~(GUI_W_FLAGS_MASK_RESYNC | GUI_W_FLAGS_MASK_DIRTY);
}

static void do_service_blink() {
	static bool s_blink_state;
	s_blink_state ^= 1;
	fori (f_gui_locals.current_window_len) {
		if (guiWidgetGetFlags(i) & GUI_W_FLAGS_MASK_BLINK)
			guiWidgetSetClearFlags(i, GUI_W_FLAGS_MASK_INVERT, s_blink_state);
	}
}
static void do_service_multipart() {
	fori (f_gui_locals.current_window_len) {
		const gui_w_servicefunc_t service = (gui_w_servicefunc_t)pgm_read_word(&f_gui_locals.current_window_defs[i].service);
		if (service)
			service(&f_gui_locals.current_window_defs[i], &f_gui_locals.current_window_ctx[i], f_gui_locals.s_phase);
		
		guiLocalService(&f_gui_locals.current_window_defs[i], &f_gui_locals.current_window_ctx[i], f_gui_locals.s_phase);
	}
}

static void do_draw() {
	if (guiGetEnables() & (GUI_ENABLE_MASK_CLEAR_BEFORE_DRAW|GUI_ENABLE_MASK_ALWAYS_REDRAW_ALL))
		u8g2.clearBuffer();					// clear the internal memory
	u8g2.setFont(u8g2_font_6x10_tr);
	u8g2.setDrawColor(1);

	// Call all draw functions in turn.
	fori (f_gui_locals.current_window_len) {
		if ( ((guiGetEnables() & GUI_ENABLE_MASK_ALWAYS_REDRAW_ALL) || (guiWidgetGetFlags(i) & GUI_W_FLAGS_MASK_DIRTY)) && !(guiWidgetGetFlags(i) & GUI_W_FLAGS_MASK_DISABLED) )
			draw_widget(i);
	}
	u8g2.sendBuffer();		  
	f_gui_locals.enables &= ~GUI_ENABLE_MASK_CLEAR_BEFORE_DRAW;
	f_gui_locals.is_dirty = false;
}

void guiService() {
	gpioDebugLed1Set();
	runEvery(GUI_BLINK_INTERVAL_MS) {
		do_service_blink();
	
		static uint8_t s_cnt;
		if (++s_cnt >= (GUI_MULTIPART_DISPLAY_INTERVAL_MS + GUI_BLINK_INTERVAL_MS) / GUI_BLINK_INTERVAL_MS) {
			s_cnt = 0;
			f_gui_locals.s_phase += 1;
			do_service_multipart();
		}
	}
	
	runEvery(GUI_SERVICE_INTERVAL_MS) {
		if (!guiWidgetIsAllClean()) 
			do_draw();
	}
	gpioDebugLed1Clear();
}



	