//Useful until be gets around to making these sorts of things
//globals akin to be_plain_font, etc.


#ifndef _SGB_COLORS_H_
#define _SGB_COLORS_H_


//******************************************************************************************************
//**** SYSTEM HEADER FILES
//******************************************************************************************************
#include <InterfaceDefs.h>

#include <Font.h>

#include "BmGuiBase.h"

//******************************************************************************************************
//**** CONSTANT DEFINITIONS
//******************************************************************************************************
//Be standard UI colors
extern IMPEXPBMGUIBASE const rgb_color BeBackgroundColor;
extern IMPEXPBMGUIBASE const int BeShadowMod;

class BView;

rgb_color IMPEXPBMGUIBASE BmFixupColor( color_which uiColName, int level, bool weaken);

inline rgb_color BmWeakenColor( color_which uiColName, int level) {
	return BmFixupColor( uiColName, level, true);
}

inline rgb_color BmStrengthenColor( color_which uiColName, int level) {
	return BmFixupColor( uiColName, level, false);
}

extern IMPEXPBMGUIBASE BFont bm_plain_font;
extern IMPEXPBMGUIBASE BFont bm_bold_font;

#endif
