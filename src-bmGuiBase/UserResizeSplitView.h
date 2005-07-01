//		$Id$
//Name:		UserResizeSplitView.h
//Author:	Brian Tietz
//Copyright 1999


#ifndef _USER_RESIZE_SPLIT_VIEW_H_
#define _USER_RESIZE_SPLIT_VIEW_H_


//******************************************************************************************************
//**** System header files
//******************************************************************************************************
#include <assert.h>
#include <BeBuild.h>
#ifdef B_BEOS_VERSION_DANO
	class BFont;
	class BMessage;
	class BMessageRunner;
	class BRect;
	class BStatusBar;
#endif
#include <layout.h>
#include <MGroup.h>

#include "BmGuiBase.h"

//******************************************************************************************************
//**** UserResizeSplitView
//******************************************************************************************************
class IMPEXPBMGUIBASE UserResizeSplitView : public MGroup, public BView
{
	public:
		UserResizeSplitView( MView* top_or_left_child, MView* right_or_bottom_child, 
									const char* name, float divider_left_or_top,
									orientation posture = B_HORIZONTAL, 
									bool should_resize_left_or_top = true,
									bool should_resize_right_or_bottom = true, 
									bool can_hide_left_or_top = false,
									bool can_hide_right_or_bottom = false, 
									bool move_slider_on_frame_resize = false,
									uint32 resize_mask = B_FOLLOW_LEFT|B_FOLLOW_TOP, 
									uint32 flags = B_WILL_DRAW|B_FRAME_EVENTS);
			//posture: B_HORIZONTAL indicates a horizontal divider, and therefore two vertically stacked
			//BViews inside.  B_VERTICAL indicates a vertical divider and side-by-side BViews.
			//should_resize_left_or_top and should_resize_right_or_bottom indicate which views should
			//be resized when the user drags the divider.  If these flags are set to false, the right or
			//bottom views will be moved, but not resized, and the left or top views will stay at the
			//same size and position.  The resize mode of these views will be ignored, since you may
			//want both resized when the user drags the divider, however both cannot be set to
			//B_FOLLOW_ALL_SIDES, B_FOLLOW_TOP_BOTTOM for horizontal posture, or B_FOLLOW_LEFT_RIGHT for
			//vertical posture, since that would give strange behavior during resizing by the user
			//dragging the window corner.  Below there, the resize mode of sub-views will be used to
			//determine whether and how to resize them.  The move_slider_on_frame_resize flag indicates
			//whether the divider should be moved to follow the bottom edge of the window when the user
			//resizes the window.  If this flag is set, make sure that the UserResizeSplitView is set to
			//have B_FOLLOW_TOP_BOTTOM for a horizontal divider, or B_FOLLOW_LEFT_RIGHT for a vertical
			//divider, and make sure the bottom child has the B_FOLLOW_BOTTOM flag set for a horizontal
			//divider, or that the right child has the B_FOLLOW_RIGHT set for a vertical divider.  Note
			//that the parent window should have the B_ASYNCHRONOUS_CONTROLS flag set.

//		void AddChildren(CLVContainerView* top_or_left, CLVContainerView* right_or_bottom);
		inline MView* LeftChild() {assert(m_posture == B_HORIZONTAL); return m_left_or_top;}
		inline MView* TopChild() {assert(m_posture == B_VERTICAL); return m_left_or_top;}
		inline MView* RightChild() {assert(m_posture == B_HORIZONTAL); return m_right_or_bottom;}
		inline MView* BottomChild() {assert(m_posture == B_VERTICAL); return m_right_or_bottom;}
		void SetMinimumChildAreaSizes(float left_or_top_minimum_size,
			float right_or_bottom_minimum_size);

		virtual void SetPreferredDividerLeftOrTop(float divider_left_or_top);
		virtual void SetDividerLeftOrTop(float divider_left_or_top, 
													bool force = false);
		inline float DividerLeftOrTop() {return m_divider_left_or_top;}

		virtual void ResizeLeftOrTopChildTo(float width_or_height);
		virtual void ResizeRightOrBottomChildTo(float width_or_height);

		//BView overrides
		virtual void Draw(BRect updateRect);
		virtual void MouseDown(BPoint where);
		virtual void MouseMoved(BPoint where, uint32 code, const BMessage *message);
		virtual void MouseUp(BPoint where);

		virtual void FrameResized(float new_width, float new_height);

		// adapted for liblayout
		virtual minimax layoutprefs();
		virtual BRect layout(BRect);
		// (end of adaptation)

		float LeftOrTopMinSize();
		float LeftOrTopMaxSize();
		float RightOrBottomMinSize();
		float RightOrBottomMaxSize();

	private:
		float m_divider_left_or_top;
		orientation m_posture;
		bool m_should_resize_left_or_top;
		bool m_should_resize_right_or_bottom;
		bool m_can_hide_left_or_top;
		bool m_can_hide_right_or_bottom;
		bool m_move_slider_on_frame_resize;
		bool m_dragging;
		MView* m_left_or_top;
		MView* m_right_or_bottom;
		BView* m_left_or_top_BV;
		BView* m_right_or_bottom_BV;
		float m_cached_width;
		float m_cached_height;
		float m_drag_mouse_offset;
};


#endif //_USER_RESIZE_SPLIT_VIEW_H_
