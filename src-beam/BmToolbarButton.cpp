/*
	BmToolbarButton.cpp
		$Id$
*/

#include <Bitmap.h>
#include <String.h>
#include <View.h>

#include "BubbleHelper.h"
#include "Colors.h"

#include "BmBasics.h"
#include "BmResources.h"
#include "BmToolbarButton.h"
//#include "BmLogHandler.h"
//#include "BmUtil.h"

static const float BmButtonWidth = 64;
static const float BmButtonHeight = 32;

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmToolbarButton::BmToolbarButton( const char *label, BBitmap* image, 
											 BMessage *message, BHandler *handler, 
											 const char* tipText=NULL)
	:	inherited( minimax( BmButtonWidth, BmButtonHeight,-1,-1), 
					  CreateOffPictureFor( label, image), 
					  CreateOnPictureFor( label, image), 
					  message, handler)
	,	mHighlighted( false)
{
	TheBubbleHelper.SetHelp( this, tipText);
	
	BPicture* picture = new BPicture();
	BFont font( be_plain_font);
	float labelWidth = label ? font.StringWidth( label) : 0;
	float labelHeight = label ? TheResources->FontLineHeight( &font) : 0;
	float width = MAX( BmButtonWidth, labelWidth+4);
	float height = BmButtonHeight;
	BRect rect(0,0,width-1,height-1);
	BView* view = new BView( rect, NULL, B_FOLLOW_NONE, 0);
	BBitmap* drawImage = new BBitmap( rect, B_GRAY8, true);
	drawImage->AddChild( view);
	drawImage->Lock();
	view->BeginPicture( picture);
	view->SetViewColor( BeBackgroundGrey);
	view->SetLowColor( BeBackgroundGrey);
	view->FillRect( rect, B_SOLID_LOW);
	if (image) {
		BRect imageRect = image->Bounds();
		float imageWidth = imageRect.Width();
		float imageHeight = imageRect.Height();
		view->SetDrawingMode(B_OP_ADD);
		view->DrawBitmap( image, BPoint( (width-imageWidth)/2.0, (height-labelHeight-2-imageHeight)/2.0));
		view->SetDrawingMode(B_OP_COPY);
	}
	if (label) {
		view->SetFont( &font);
		view->SetHighColor( BeShadow);
		float labelBaseOffset = TheResources->FontBaselineOffset( &font);
		view->DrawString( label, BPoint( (width-labelWidth)/2.0, height-labelHeight+labelBaseOffset));
		view->SetHighColor( Black);
	}
	view->EndPicture();
	drawImage->Unlock();
	delete drawImage;
	SetDisabledOff( picture);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmToolbarButton::~BmToolbarButton() {
	TheBubbleHelper.SetHelp( this, NULL);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BPicture* BmToolbarButton::CreateOnPictureFor( const char* label, BBitmap* image) {
	BPicture* picture = new BPicture();
	BFont font( be_plain_font);
	float labelWidth = label ? font.StringWidth( label) : 0;
	float labelHeight = label ? TheResources->FontLineHeight( &font) : 0;
	float width = MAX( BmButtonWidth, labelWidth+4);
	float height = BmButtonHeight;
	BRect rect(0,0,width-1,height-1);
	BView* view = new BView( rect, NULL, B_FOLLOW_NONE, 0);
	BBitmap* drawImage = new BBitmap( rect, B_RGBA32, true);
	drawImage->AddChild( view);
	drawImage->Lock();
	view->BeginPicture( picture);
	view->SetViewColor( BeBackgroundGrey);
	view->SetLowColor( BeBackgroundGrey);
	view->FillRect( rect, B_SOLID_LOW);
	if (image) {
		BRect imageRect = image->Bounds();
		float imageWidth = imageRect.Width();
		float imageHeight = imageRect.Height();
		view->SetDrawingMode(B_OP_OVER);
		view->DrawBitmap( image, BPoint( (width-imageWidth)/2.0, (height-labelHeight-2-imageHeight)/2.0));
		view->SetDrawingMode(B_OP_COPY);
	}
	if (label) {
		view->SetFont( &font);
		float labelBaseOffset = TheResources->FontBaselineOffset( &font);
		view->DrawString( label, BPoint( (width-labelWidth)/2.0, height-labelHeight+labelBaseOffset));
	}
	view->EndPicture();
	drawImage->Unlock();
	delete drawImage;
	return picture;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BPicture* BmToolbarButton::CreateOffPictureFor( const char* label, BBitmap* image) {
	BPicture* picture = new BPicture();
	BFont font( be_plain_font);
	float labelWidth = label ? font.StringWidth( label) : 0;
	float labelHeight = label ? TheResources->FontLineHeight( &font) : 0;
	float width = MAX( BmButtonWidth, labelWidth+4);
	float height = BmButtonHeight;
	BRect rect(0,0,width-1,height-1);
	BView* view = new BView( rect, NULL, B_FOLLOW_NONE, 0);
	BBitmap* drawImage = new BBitmap( rect, B_RGBA32, true);
	drawImage->AddChild( view);
	drawImage->Lock();
	view->BeginPicture( picture);
	view->SetViewColor( BeBackgroundGrey);
	view->SetLowColor( BeBackgroundGrey);
	view->FillRect( rect, B_SOLID_LOW);
	if (image) {
		BRect imageRect = image->Bounds();
		float imageWidth = imageRect.Width();
		float imageHeight = imageRect.Height();
		view->SetLowColor( BeShadow);
		view->SetDrawingMode(B_OP_ERASE);
		view->DrawBitmap( image, BPoint( 1+(width-imageWidth)/2.0, 1+(height-labelHeight-2-imageHeight)/2.0));
		view->DrawBitmap( image, BPoint( 0+(width-imageWidth)/2.0, 0+(height-labelHeight-2-imageHeight)/2.0));
		view->SetLowColor( BeBackgroundGrey);
		view->SetDrawingMode(B_OP_OVER);
		view->DrawBitmap( image, BPoint( -1+(width-imageWidth)/2.0, -1+(height-labelHeight-2-imageHeight)/2.0));
		view->SetDrawingMode(B_OP_COPY);
	}
	if (label) {
		view->SetFont( &font);
		float labelBaseOffset = TheResources->FontBaselineOffset( &font);
		view->DrawString( label, BPoint( (width-labelWidth)/2.0, height-labelHeight+labelBaseOffset));
	}
	view->EndPicture();
	drawImage->Unlock();
	delete drawImage;
	return picture;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmToolbarButton::Draw( BRect bounds) {
	inherited::Draw( bounds);
}
