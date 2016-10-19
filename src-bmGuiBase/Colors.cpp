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
const rgb_color BeBackgroundColor = ui_color( B_PANEL_BACKGROUND_COLOR);

// standard shadow-modification:
const int BeShadowMod = 3;

rgb_color BmFixupColor( color_which uiColName, int level, bool weaken) {
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
	}
	return ui_color( uiColName);
}

BFont bm_plain_font;
BFont bm_bold_font;
