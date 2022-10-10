#ifndef GUI_H__
#define GUI_H__

// Generic GUI.

class U8G2_ST7920_128X64_F_HW_SPI;
extern U8G2_ST7920_128X64_F_HW_SPI u8g2;

// Widget's local data.
typedef struct {
    uint16_t flags;  		// Modify the behaviour of the widget.
    void* data;     		// What the widget displays, loaded from a default value in gui_w_def_t but freely writable as it's in RAM.
} gui_w_context_t;

// User function that draws the widget. 
typedef struct gui_w_def_t gui_w_def_t;  // Clever, forward declaration of struct combined with typedef.
typedef void (*gui_w_drawfunc_t)(const gui_w_def_t* w, gui_w_context_t* ctx);      // Draw function, passed a CONST pointer to a widget_t in RAM, and a MUTABLE pointer to the context. 

// User function that services multipart widgets. 
typedef void (*gui_w_servicefunc_t)(const gui_w_def_t* w, gui_w_context_t* ctx, uint8_t phase);      // Like the Draw function, but passed the phase counter as well to know which item to display next. 

// User function to SERVICE widget. What it does is up to you. 
void guiLocalService(const gui_w_def_t* w, gui_w_context_t* ctx, uint8_t phase);     

// Widget definition in Flash. 
typedef struct gui_w_def_t {
    uint8_t x, y, w, h;             // Region.    
    gui_w_drawfunc_t draw;		    // Do the draw, knows how to access data. 
	gui_w_servicefunc_t service;
    void* default_data;             // Default value for widget data member. 
    uint16_t default_flags;			// Always need these, note that they are constant. 
} gui_w_def_t;

// Sets new window, loads data in context from defs, clears widget flags.
void guiSetWindow(const gui_w_def_t* wnd_def, gui_w_context_t* wnd_ctx, uint8_t wnd_len);

// Useful drawing functions.
void guiDrawText(const gui_w_def_t* w, gui_w_context_t* ctx); // Draws text in RAM/PROGMEM pointed to by data.
void guiServiceMpText(const gui_w_def_t* w, gui_w_context_t* ctx, uint8_t phase); // Data is RAM address of string, items separated by '|'. 
void guiDrawBmp(const gui_w_def_t* w, gui_w_context_t* ctx);

// TODO: Animations have gone a bit crusty.

typedef const uint8_t * gui_animation_t;		// Confused about types? Simply add a typedef.

typedef struct {
	gui_animation_t base;
	gui_animation_t current;
} gui_bitmap_state;
void guiServiceMpBitmap(const gui_w_def_t* w, gui_w_context_t* ctx, uint8_t phase); // Data points to a gui_bitmap_state.

// Widget Flags. Altering the value will make the widget dirty. 
enum {
   GUI_W_FLAGS_MASK_INVERT =		_BV(0),		// Draws the widget inverted.
   GUI_W_FLAGS_MASK_BLINK =			_BV(1),		// Toggles the inverted state every so often for a blinking effect. 
   GUI_W_FLAGS_MASK_DISABLED =		_BV(2),		// If disabled the widget will not be drawn and cannot be selected. 
   GUI_W_FLAGS_MASK_RESYNC =		_BV(3),		// Used for multipart widgets that might want to sync their changes to something. Cleared on draw. 
   GUI_W_FLAGS_MASK_CENTRED =		_BV(4),		// Draw text centred.
   GUI_W_FLAGS_MASK_PROGMEM =		_BV(5),		// Data member points to PROGMEM.
   GUI_W_FLAGS_MASK_SELECTABLE =	_BV(6),		// Widget may be selected.
   GUI_W_FLAGS_MASK_SELECTED =		_BV(7),		// Widget _is_ selected. 
   
   GUI_W_FLAGS_MASK_DIRTY =		_BV(15),
};
uint16_t guiWidgetGetFlags(uint8_t widx);
void guiWidgetWriteFlags(uint8_t widx, uint16_t flags);
void guiWidgetFlipFlags(uint8_t widx, uint16_t mask);
void guiWidgetClearFlags(uint8_t widx, uint16_t mask);
void guiWidgetSetFlags(uint8_t widx, uint16_t mask);
void guiWidgetSetClearFlags(uint8_t widx, uint16_t mask, bool f);

// Mark the widget as dirty so that it will be redrawn. Note that it will never CLEAR the dirty state. A negative index sets all widgets to dirty. 
void guiWidgetSetDirty(int8_t widx=-1, bool f=true);		

// Mark widget or all widgets as clean.
void guiWidgetSetClean(uint8_t widx=-1);	

// Read generic data for widget. 
void* guiWidgetReadData(uint8_t widx);

// Write widget data, sets dirty flag if has changed.
void guiWidgetWriteData(uint8_t widx, void* data, uint16_t mask=0U, bool set=true);

// Load default data in to widget's data, sets dirty flag if has changed.
void guiWidgetLoadDefaultData(uint8_t widx);

// Set widget's context to dirty, quick version. 
void guiCtxSetDirty(gui_w_context_t* ctx);

// Universal updater for widgets: reads old value, writes new value, if different set the dirty flag in the supplied context. 
template <typename T>
void guiGenericUpdateWidgetData(gui_w_context_t* ctx, T* var, T val) {
	T old_val = *var;
	*var = val;
	if (old_val != val)
		guiCtxSetDirty(ctx);
}	

// Initialise, probably not much in here. 
void guiInit();

// Enables, controls global behaviour.
enum {
    GUI_ENABLE_MASK_CLEAR_BEFORE_DRAW = _BV(7),
    GUI_ENABLE_MASK_ALWAYS_REDRAW_ALL = _BV(0),
    GUI_ENABLE_MASK_DRAW_EXTENTS_ONLY = _BV(1),
};
void guiSetEnables(uint8_t x);
uint8_t guiGetEnables();

enum { GUI_NO_SEL = -1 };
// Return current selection if selectable bit set, or next item in sequennce, or NO_SEL if none. 
int8_t guiGetSelection(int8_t dir=0);

enum { 
    GUI_SERVICE_INTERVAL_MS = 100,				// Redraw done at this interval.
    GUI_BLINK_INTERVAL_MS = 250,				// Widgets blink period = 2 x interval.
    GUI_MULTIPART_DISPLAY_INTERVAL_MS = 500,	// Multipart messages cycled at this rate, actually done at closest multiple of blink interval to maintain synchronisation. 
};    

// Redraws the gui if required. Servicing is carried out by GUI_SERVICE_INTERVAL_MS.
void guiService();

#endif // GUI_H__

