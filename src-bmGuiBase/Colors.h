//		$Id$
//Useful until be gets around to making these sorts of things
//globals akin to be_plain_font, etc.


#ifndef _SGB_COLORS_H_
#define _SGB_COLORS_H_


//******************************************************************************************************
//**** SYSTEM HEADER FILES
//******************************************************************************************************
#include <InterfaceDefs.h>

#include "SantaPartsForBeam.h"

//******************************************************************************************************
//**** CONSTANT DEFINITIONS
//******************************************************************************************************
//Be standard UI colors
extern IMPEXPSANTAPARTSFORBEAM const rgb_color BeBackgroundColor;
extern IMPEXPSANTAPARTSFORBEAM const int BeShadowMod;

class BView;
/*------------------------------------------------------------------------------*\
	Functions for retrieving UI-colors, independent from whether or not the
	current BeOS supports it or not (on R5 they just return the fixed values):
\*------------------------------------------------------------------------------*/
status_t BmSetViewUIColor( BView* view, const char* uiColName);
status_t BmSetHighUIColor( BView* view, const char* uiColName);
status_t BmSetLowUIColor( BView* view, const char* uiColName);

rgb_color BmFixupColor( const char* uiColName, int level, bool weaken);

inline rgb_color BmWeakenColor( const char* uiColName, int level) {
	return BmFixupColor( uiColName, level, true);
}

inline rgb_color BmStrengthenColor( const char* uiColName, int level) {
	return BmFixupColor( uiColName, level, false);
}

#ifndef B_BEOS_VERSION_DANO

// backport of UI-colors to R5:
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_PANEL_BACKGROUND_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_PANEL_TEXT_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_DOCUMENT_BACKGROUND_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_DOCUMENT_TEXT_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_CONTROL_BACKGROUND_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_CONTROL_TEXT_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_CONTROL_BORDER_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_CONTROL_HIGHLIGHT_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_NAVIGATION_BASE_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_NAVIGATION_PULSE_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_SHINE_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_SHADOW_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_TOOLTIP_BACKGROUND_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_TOOLTIP_TEXT_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_MENU_BACKGROUND_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_MENU_SELECTED_BACKGROUND_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_MENU_ITEM_TEXT_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_MENU_SELECTED_ITEM_TEXT_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_MENU_SELECTED_BORDER_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_SUCCESS_COLOR;
extern IMPEXPSANTAPARTSFORBEAM const char *B_UI_FAILURE_COLOR;

rgb_color ui_color( const char* uiColName);

// [HACK-HACK-HACK]:
// 	map calls to SetXXXUIColor to use BmSetXXXUIColor automagically:
#define SetViewUIColor(col) BmSetViewUIColor(this,(col))
#define SetHighUIColor(col) BmSetHighUIColor(this,(col))
#define SetLowUIColor(col) BmSetLowUIColor(this,(col))

#endif

#endif
