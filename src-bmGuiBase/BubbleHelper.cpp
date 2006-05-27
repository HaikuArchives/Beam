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

#include <string.h>
#include <malloc.h>

#include <map>

#include <Application.h>
#include <Window.h>
#include <TextView.h>
#include <Region.h>
#include <Screen.h>
#include <Cursor.h>
#include <List.h>
#include <Locker.h>

#include "BubbleHelper.h"
#include "Colors.h"

long BubbleHelper::runcount=0;
BubbleHelper* TheBubbleHelper = NULL;

void BubbleHelper::CreateInstance()
{
	if (!TheBubbleHelper)
		TheBubbleHelper = new BubbleHelper();
}

BubbleHelper::BubbleHelper()
	:	infoList( new BList(100))
	,	infoLocker( new BLocker( "BubbleHelperLock"))
{
    // You only need one instance per application.
    if(atomic_add(&runcount,1)==0)
    {
        helperthread=spawn_thread(_helper,"helper",B_NORMAL_PRIORITY,this);
        if(helperthread>=0)
            resume_thread(helperthread);
        enabled=true;
    }
    else
    {
        // Since you shouldn't be creating more than one instance
        // you may want to jump straight into the debugger here.
        debugger("only one BubbleHelper instance allowed/necessary");
    }
}

BubbleHelper::~BubbleHelper()
{
    if(helperthread>=0)
    {
        // force helper thread into a known state
        bool locked=textwin->Lock();
        // Be rude...
        kill_thread(helperthread);
        // dispose of window
        if(locked)
        {
	        textwin->PostMessage(B_QUIT_REQUESTED);
    	    textwin->Unlock();
    	}
    }
    delete infoLocker;
    delete infoList;
    TheBubbleHelper = NULL;
    atomic_add(&runcount,-1);
}

BubbleHelper::BubbleInfo* BubbleHelper::FindInfo( BView *view)
{
	BubbleInfo* info;
	for( int i=0; i<infoList->CountItems(); ++i) {
		info = (BubbleInfo*)infoList->ItemAt( i);
		if (info && info->view == view)
			return info;
	}
	return NULL;
}

void BubbleHelper::SetHelp(BView *view, const char *text)
{
    if (!text)
        DropInfo( view);
    else if (this && view) {
    	infoLocker->Lock();
    	BubbleInfo* info = FindInfo( view);
    	if (info) {
    	  	info->text = text;
    	} else {
    		info = new BubbleInfo( view, text, NULL);
    	  	infoList->AddItem( info);
		}
		infoLocker->Unlock();
    }
}

void BubbleHelper::SetCursor(BView *view, const BCursor* cursor)
{
    if (this && view) {
    	infoLocker->Lock();
    	BubbleInfo* info = FindInfo( view);
    	if (info) {
    	  	info->cursor = cursor;
    	} else {
    		info = new BubbleInfo( view, NULL, cursor);
    	  	infoList->AddItem( info);
		}
		infoLocker->Unlock();
    }
}

void BubbleHelper::DropInfo( BView *view)
{
    if (this && view) {
    	infoLocker->Lock();
    	BubbleInfo* info = FindInfo( view);
    	if (info)
    		infoList->RemoveItem( info);
    	delete info;
		infoLocker->Unlock();
    }
}

const char *BubbleHelper::GetHelp(BView *view)
{
  	const char* text = NULL;
  	infoLocker->Lock();
   	BubbleInfo* info = FindInfo( view);
   	if (info)
   	  	text = info->text;
	infoLocker->Unlock();
	return text;
}

const BCursor* BubbleHelper::GetCursor(BView *view)
{
  	const BCursor* cursor = NULL;
  	infoLocker->Lock();
   	BubbleInfo* info = FindInfo( view);
   	if (info)
   	  	cursor = info->cursor;
	infoLocker->Unlock();
	return cursor;
}

long BubbleHelper::_helper(void *arg)
{
    ((BubbleHelper*)arg)->Helper();
    return 0;
}

void BubbleHelper::Helper()
{
    // Wait until the BApplication becomes valid, in case
    // someone creates this as a global variable.
    while(!be_app_messenger.IsValid())
        snooze(200000);
    // wait a little longer, until the BApplication is really up to speed
    while(be_app->IsLaunching())
        snooze(200000);

    textwin=new BWindow(BRect(-100,-100,-50,-50),"",B_BORDERED_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL,
                B_NOT_MOVABLE|B_AVOID_FOCUS);

    textview=new BTextView(BRect(0,0,50,50),"",BRect(2,2,48,48),B_FOLLOW_ALL_SIDES,B_WILL_DRAW);
    textview->MakeEditable(false);
    textview->MakeSelectable(false);
    textview->SetWordWrap(false);
    BmSetLowUIColor( textview, B_UI_TOOLTIP_BACKGROUND_COLOR);
    BmSetViewUIColor( textview, B_UI_TOOLTIP_BACKGROUND_COLOR);
    BmSetHighUIColor( textview, B_UI_TOOLTIP_TEXT_COLOR);
    textwin->AddChild(textview);
    textwin->Run();
    textwin->Lock();
    textwin->Activate(false);
    rename_thread(textwin->Thread(),"bubble");
    textwin->Unlock();

    ulong delaycounter=0;
    BPoint lastwhere;

    while(be_app_messenger.IsValid())
    {
        BPoint where;
        ulong buttons;
        if(enabled)
        {
            if(textwin->Lock())
            {
                textview->GetMouse(&where,&buttons);
                textview->ConvertToScreen(&where);
                if(lastwhere!=where || buttons)
                {
                    delaycounter=0;

                    BView *view = FindView(where);
                    const BCursor* cursor = GetCursor( view);
                    if (!cursor)
                        cursor = B_CURSOR_SYSTEM_DEFAULT;
                    be_app->SetCursor( cursor, true);
                }
                else
                {
                    // mouse didn't move
                    if(delaycounter++>5)
                    {
                        delaycounter=0;
                        // mouse didn't move for a while
                        BView *view = FindView(where);
                        const char *text = NULL;
                        while(view && (text=GetHelp(view))==NULL)
                            view=view->Parent();
                        if(text)
                        {
                            DisplayHelp(text,where);
                            // wait until mouse moves out of view, or wait
                            // for timeout
                            long displaycounter=0;
                            BPoint where2;
                            long displaytime=max_c(20,strlen(text));
                            do
                            {
                                textwin->Unlock();
                                snooze(100000);
                                if(!textwin->Lock())
                                    goto end; //window is apparently gone
                                textview->GetMouse(&where2,&buttons);
                                textview->ConvertToScreen(&where2);
                            } while(!buttons && where2==where && (displaycounter++<displaytime));
                        
                            HideBubble();
                            do
                            {
                                textwin->Unlock();
                                snooze(100000);
                                if(!textwin->Lock())
                                    goto end; //window is apparently gone
                                textview->GetMouse(&where2,&buttons);
                                textview->ConvertToScreen(&where2);
                            } while(where2==where);
                        }
                    }
                }
                lastwhere=where;
                textwin->Unlock();
            }
        }
end:
        snooze(100000);
    }
    // (this thread normally gets killed by the destructor before arriving here)
}

void BubbleHelper::HideBubble()
{
    textwin->MoveTo(-1000,-1000);                    // hide it
    if(!textwin->IsHidden())
        textwin->Hide();
}

void BubbleHelper::ShowBubble(BPoint dest)
{
    textwin->MoveTo(dest);
    textwin->SetWorkspaces(B_CURRENT_WORKSPACE);
    if(textwin->IsHidden())
        textwin->Show();
}

BView *BubbleHelper::FindView(BPoint where)
{
    BView *winview=NULL;
    BWindow *win;
    long windex=0;
    while((winview==NULL)&&((win=be_app->WindowAt(windex++))!=NULL))
    {
        if(win!=textwin)
        {
            // lock with timeout, in case somebody has a non-running window around
            // in their app.
            if(win->LockWithTimeout(1E6)==B_OK)
            {
                BRect frame=win->Frame();
                if(frame.Contains(where))
                {
                    BPoint winpoint;
                    winpoint=where-frame.LeftTop();
                    winview=win->FindView(winpoint);
                    if(winview)
                    {
                        BRegion region;
                        BPoint newpoint=where;
                        winview->ConvertFromScreen(&newpoint);
                        winview->GetClippingRegion(&region);
                        if(!region.Contains(newpoint))
                            winview=0;
                    }
                }
                win->Unlock();
            }
        }
    }
    return winview;
}

void BubbleHelper::DisplayHelp(const char *text, BPoint where)
{
    textview->SetText(text);
    
    float height=textview->TextHeight(0,2E6)+4;
    float width=0;
    int numlines=textview->CountLines();
    int linewidth;
    for(int i=0;i<numlines;i++)
        if((linewidth=int(textview->LineWidth(i)))>width)
            width=linewidth;
    textwin->ResizeTo(width+4,height);
    textview->SetTextRect(BRect(2,2,width+2,height+2));
    
    BScreen screen;
    BPoint dest=where+BPoint(0,20);
    BRect screenframe=screen.Frame();
    if((dest.y+height)>(screenframe.bottom-3))
        dest.y=dest.y-(16+height+8);

    if((dest.x+width+4)>(screenframe.right))
        dest.x=dest.x-((dest.x+width+4)-screenframe.right);

    ShowBubble(dest);
}

void BubbleHelper::EnableHelp(bool enable)
{
    enabled=enable;
}
