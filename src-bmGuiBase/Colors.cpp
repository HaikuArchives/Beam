//		$Id$
//Useful until be gets around to making these sorts of things
//globals akin to be_plain_font, etc.


//******************************************************************************************************
//**** SYSTEM HEADER FILES
//******************************************************************************************************
#include <Debug.h>
#include <View.h>

#include "BmString.h"
#include "Colors.h"

//******************************************************************************************************
//**** CONSTANT DEFINITIONS
//******************************************************************************************************
//Be standard UI colors
const rgb_color BeBackgroundColor = ui_color( B_UI_PANEL_BACKGROUND_COLOR);

// standard shadow-modification:
const int BeShadowMod = 3;

// color settings for R5:
static const rgb_color BeControlBackground = { 245, 245, 245, 255};
static const rgb_color Black =					{   0,   0,   0, 255};
static const rgb_color White =					{ 255, 255, 255, 255};
static const rgb_color BeControlHighlight =  {  75,  96, 175, 255};
static const rgb_color BeTooltipBackground = { 240, 240, 100, 255};
static const rgb_color BeShadow =				{   0,   0,   0, 255};
static const rgb_color BeShine = 				{ 255, 255, 255, 255};
static const rgb_color BeSuccess	=				{   0, 255,   0, 255};
static const rgb_color BeFailure = 				{ 255,   0,   0, 255};
static const rgb_color BeMenuSelectedBackground =	{ 178, 178, 178, 255};

/*------------------------------------------------------------------------------*\
	Functions for retrieving UI-colors, independent from whether or not the
	current BeOS supports it or not (on R5 they just return the fixed values):
\*------------------------------------------------------------------------------*/
#ifdef B_BEOS_VERSION_DANO

status_t BmSetViewUIColor( BView* view, const char* uiColName) {
	if (!view)
		return B_BAD_VALUE;
	view->SetViewUIColor( uiColName);
	return B_OK;
}

status_t BmSetHighUIColor( BView* view, const char* uiColName) {
	if (!view)
		return B_BAD_VALUE;
	view->SetHighUIColor( uiColName);
	return B_OK;
}

status_t BmSetLowUIColor( BView* view, const char* uiColName) {
	if (!view)
		return B_BAD_VALUE;
	view->SetLowUIColor( uiColName);
	return B_OK;
}

#else

status_t BmSetViewUIColor( BView* view, const char* uiColName) {
	if (!view)
		return B_BAD_VALUE;
	view->SetViewColor( ui_color( uiColName));
	return B_OK;
}

status_t BmSetHighUIColor( BView* view, const char* uiColName) {
	if (!view)
		return B_BAD_VALUE;
	view->SetHighColor( ui_color( uiColName));
	return B_OK;
}

status_t BmSetLowUIColor( BView* view, const char* uiColName) {
	if (!view)
		return B_BAD_VALUE;
	view->SetLowColor( ui_color( uiColName));
	return B_OK;
}

// backport of UI-colors to R5:
rgb_color ui_color( const char* uiColName) {
	BmString uiCol( uiColName);
	if (uiCol == B_UI_PANEL_BACKGROUND_COLOR)
		return ui_color( B_PANEL_BACKGROUND_COLOR);
	else if (uiCol == B_UI_MENU_BACKGROUND_COLOR)
		return ui_color( B_MENU_BACKGROUND_COLOR);
	else if (uiCol == B_UI_MENU_SELECTED_BACKGROUND_COLOR)
		return BeMenuSelectedBackground;
	else if (uiCol == B_UI_MENU_ITEM_TEXT_COLOR)
		return ui_color( B_MENU_ITEM_TEXT_COLOR);
	else if (uiCol == B_UI_MENU_SELECTED_ITEM_TEXT_COLOR)
		return ui_color( B_MENU_SELECTED_ITEM_TEXT_COLOR);
	else if (uiCol == B_UI_NAVIGATION_BASE_COLOR)
		return ui_color( B_KEYBOARD_NAVIGATION_COLOR);
	else if (uiCol == B_UI_PANEL_TEXT_COLOR)
		return Black;
	else if (uiCol == B_UI_DOCUMENT_BACKGROUND_COLOR)
		return White;
	else if (uiCol == B_UI_DOCUMENT_TEXT_COLOR)
		return Black;
	else if (uiCol == B_UI_CONTROL_BACKGROUND_COLOR)
		return BeControlBackground;
	else if (uiCol == B_UI_CONTROL_TEXT_COLOR)
		return Black;
	else if (uiCol == B_UI_CONTROL_BORDER_COLOR)
		return Black;
	else if (uiCol == B_UI_CONTROL_HIGHLIGHT_COLOR)
		return BeControlHighlight;
	else if (uiCol == B_UI_NAVIGATION_PULSE_COLOR)
		return Black;
	else if (uiCol == B_UI_SHINE_COLOR)
		return BeShine;
	else if (uiCol == B_UI_SHADOW_COLOR)
		return BeShadow;
	else if (uiCol == B_UI_TOOLTIP_BACKGROUND_COLOR)
		return BeTooltipBackground;
	else if (uiCol == B_UI_TOOLTIP_TEXT_COLOR)
		return Black;
	else if (uiCol == B_UI_MENU_SELECTED_BORDER_COLOR)
		return Black;
	else if (uiCol == B_UI_SUCCESS_COLOR)
		return BeSuccess;
	else if (uiCol == B_UI_FAILURE_COLOR)
		return BeFailure;
	DEBUGGER(("B,GetUIColor called with unknown color!"));
	return Black;
}

const char *B_UI_PANEL_BACKGROUND_COLOR = "be:c:PanBg";
const char *B_UI_PANEL_TEXT_COLOR = "be:c:PanTx";
const char *B_UI_DOCUMENT_BACKGROUND_COLOR = "be:c:DocBg";
const char *B_UI_DOCUMENT_TEXT_COLOR = "be:c:DocTx";
const char *B_UI_CONTROL_BACKGROUND_COLOR = "be:c:CtlBg";
const char *B_UI_CONTROL_TEXT_COLOR = "be:c:CtlTx";
const char *B_UI_CONTROL_BORDER_COLOR = "be:c:CtlBr";
const char *B_UI_CONTROL_HIGHLIGHT_COLOR = "be:c:CtlHg";
const char *B_UI_NAVIGATION_BASE_COLOR = "be:c:NavBs";
const char *B_UI_NAVIGATION_PULSE_COLOR = "be:c:NavPl";
const char *B_UI_SHINE_COLOR = "be:c:Shine";
const char *B_UI_SHADOW_COLOR = "be:c:Shadow";
const char *B_UI_TOOLTIP_BACKGROUND_COLOR = "be:c:TipBg";
const char *B_UI_TOOLTIP_TEXT_COLOR = "be:c:TipTx";
const char *B_UI_MENU_BACKGROUND_COLOR = "be:c:MenBg";
const char *B_UI_MENU_SELECTED_BACKGROUND_COLOR = "be:c:MenSBg";
const char *B_UI_MENU_ITEM_TEXT_COLOR = "be:c:MenTx";
const char *B_UI_MENU_SELECTED_ITEM_TEXT_COLOR = "be:c:MenSTx";
const char *B_UI_MENU_SELECTED_BORDER_COLOR = "be:c:MenSBr";
const char *B_UI_SUCCESS_COLOR = "be:c:Success";
const char *B_UI_FAILURE_COLOR = "be:c:Failure";

#endif 	// B_BEOS_VERSION_DANO

rgb_color BmFixupColor( const char* uiColName, int level, bool weaken) {
	if (level > 0) {
		float tint = B_NO_TINT;
		rgb_color col = ui_color( uiColName);
		int avg = (col.red+col.green+col.blue)/3;
		if ((weaken && avg < 128) || (!weaken && avg >= 128)) {
			switch (level) {
				case 1: tint = 0.980F; break;
				case 2: tint = B_LIGHTEN_1_TINT; break;
				case 3: tint = B_LIGHTEN_2_TINT; break;
				default: tint = B_LIGHTEN_MAX_TINT;
			}
		} else {
			switch (level) {
				case 1: tint = 1.072F; break;
				case 2: tint = B_DARKEN_1_TINT; break;
				case 3: tint = B_DARKEN_2_TINT; break;
				case 4: tint = B_DARKEN_3_TINT; break;
				case 5: tint = B_DARKEN_4_TINT; break;
				default: tint = B_DARKEN_MAX_TINT;
			}
		}
		return tint_color( col, tint);
	} else
		return ui_color( uiColName);
}

bool operator== (const rgb_color& left, const rgb_color& right) {
	return left.red == right.red
				&& left.green == right.green
				&& left.blue == right.blue
				&& left.alpha == right.alpha;
}

BFont bm_plain_font;
BFont bm_bold_font;
