//		$Id$

/*

    Bubblehelp class Copyright (C) 1998 Marco Nelissen <marcone@xs4all.nl>
    Freely usable in non-commercial applications, as long as proper credit
    is given.

    Usage:
	- Add the file BubbleHelper.cpp to your project
	- #include "BubbleHelper.h" in your files where needed
    - Create a single instance of BubbleHelper (it will serve your entire
      application). It is safe to create one on the stack or as a global.
    - Call SetHelp(view,text) for each view to which you wish to attach a text.
    - Use SetHelp(view,NULL) to remove text from a view.
    
    This could be implemented as a BMessageFilter as well, but that means using
    one bubblehelp-instance for each window to which you wish to add help-bubbles.
    Using a single looping thread for everything turned out to be the most practical
    solution.

*/

#ifndef _BUBBLEHELPER_H
#define _BUBBLEHELPER_H

#include <OS.h>

class BubbleHelper
{
	public:
		BubbleHelper();
		virtual ~BubbleHelper();

		void SetHelp(BView *view, const char *text);
		void EnableHelp(bool enable=true);

	private:
		thread_id	helperthread;
		BList		*helplist;
		BWindow		*textwin;
		BTextView	*textview;
		void		DisplayHelp(char *text,BPoint where);
		void		Helper();
		char		*GetHelp(BView *view);
		static long _helper(void *arg);
		BView		*FindView(BPoint where);
		bool		enabled;
		
		void HideBubble();
		void ShowBubble(BPoint dest);
		
		static long runcount;
};

extern BubbleHelper TheBubbleHelper;

#endif
