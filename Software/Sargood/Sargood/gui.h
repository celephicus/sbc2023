#ifndef GUI_H__
#define GUI_H__

// Initialise the gui system, call at startup.
void guiInit();

// Call as frequently as possible.
void guiService();

// Call every 100ms.
void guiService10hz();

#endif	// GUI_H__
