//Name:		DeepBevelView.h
//Author:	Brian Tietz
//Copyright 1999
//Usage:
//	If you can help it, don't just stick another view inside this one.  Instead, subclass this, and have
//	your class's Draw method call the DeepBevelView::Draw method, then do your subclass's drawing in the
//  view while leaving alone the outermost two pixels on each edge.


#ifndef _DEEP_BEVEL_VIEW_H_
#define _DEEP_BEVEL_VIEW_H_


//******************************************************************************************************
//**** System header files
//******************************************************************************************************
#include <View.h>

#include "BmGuiBase.h"

//******************************************************************************************************
//**** DeepBevelView
//******************************************************************************************************
class IMPEXPBMGUIBASE DeepBevelView : public BView
{
	public:
		DeepBevelView(BRect frame, const char* name, uint32 resize_mask, uint32 flags);

		//BView overrides
		virtual void Draw(BRect update_rect);
		virtual void FrameResized(float width, float height);

	protected:
		BRect m_cached_bounds;
		rgb_color m_background_color;
		rgb_color m_dark_1_color;
};


#endif //_DEEP_BEVEL_VIEW_H_
