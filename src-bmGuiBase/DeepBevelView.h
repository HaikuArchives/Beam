//Name:		DeepBevelView.h
//Author:	Brian Tietz
//Copyright 1999
//Conventions:
//	Global constants (declared with const) and #defines - begin with "c_" followed by lowercase
//		words separated by underscores.
//		(E.G., #define c_my_constant 5).
//		(E.G., const int c_my_constant = 5;).
//	Global variables - begin with "g_" followed by lowercase words separated by underscores.
//		(E.G., int g_my_global;).
//	New data types (classes, structs, typedefs, etc.) - begin with an uppercase letter followed by
//		lowercase words separated by uppercase letters.  Enumerated constants contain a prefix
//		associating them with a particular enumerated set.
//		(E.G., typedef int MyTypedef;).
//		(E.G., enum MyEnumConst {c_mec_one, c_mec_two};)
//	Private member variables - begin with "m_" followed by lowercase words separated by underscores.
//		(E.G., int m_my_member;).
//	Public or friend-accessible member variables - all lowercase words separated by underscores.
//		(E.G., int public_member;).
//	Argument and local variables - begin with a lowercase letter followed by
//		lowercase words separated by underscores.  If the name is already taken by a public member
//		variable, prefix with a_ or l_
//		(E.G., int my_local; int a_my_arg, int l_my_local).
//	Functions (member or global) - begin with an uppercase letter followed by lowercase words
//		separated by uppercase letters.
//		(E.G., void MyFunction(void);).
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


//******************************************************************************************************
//**** DeepBevelView
//******************************************************************************************************
class DeepBevelView : public BView
{
	public:
		DeepBevelView(BRect frame, const char* name, uint32 resize_mask, uint32 flags);

		//BView overrides
		virtual void Draw(BRect update_rect);
		virtual void FrameResized(float width, float height);

	private:
		BRect m_cached_bounds;
		rgb_color m_background_color;
		rgb_color m_dark_1_color;
		rgb_color m_dark_2_color;
};


#endif //_DEEP_BEVEL_VIEW_H_
