/*
	TestFolders.cpp
		$Id$
*/

#include <memory>

#include <Application.h>

#include "BmMailFolderList.h"
#include "BmPrefs.h"

void TestFolders();
void TestQuery();

class GenericApp : public BApplication
{
public:
	GenericApp() : BApplication("application/x-vnd.OT-Generic") {}
};

int main()
{
	Beam beam;
	GenericApp *testApp = new GenericApp;
	try {
		auto_ptr<BmMailFolderList> fl( BmMailFolderList::Init());
		fl->Store();
//		be_app->Run();
	} 
	catch( exception &e) {
		BM_LOGERR( BString("Oops: ") << e.what());
	}
	delete testApp;
}
