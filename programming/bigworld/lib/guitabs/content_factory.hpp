/**
 *	GUI Tearoff panel framework - ContentFactory class
 */

#ifndef GUITABS_CONTENT_FACTORY_HPP
#define GUITABS_CONTENT_FACTORY_HPP


/*
 *	This macro implements the default behaviour for a ContentFactory class. If
 *	the Content class inherits from CDialog, you should manually declare your
 *	factory class and implement the create method as in Sample 3 at the end of
 *  the file, or use the IMPLEMENT_CDIALOG_CONTENT_FACTORY macro.
 *	The contentID MUST match the contentID of the corresponding content class.
 *	NOTE: This macro must be placed after the Content-derived class is declared
 *	See content.hpp for additional information.
 */
#define IMPLEMENT_BASIC_CONTENT_FACTORY( CONTENT ) \
	class CONTENT##Factory : public GUITABS::ContentFactory\
	{\
	public:\
	CONTENT##Factory() {}\
	GUITABS::ContentPtr create()\
	{\
		return new CONTENT();\
	}\
	BW::wstring getContentID() { return CONTENT::contentID; }\
	}; 


/*
 *	This macro implements the default behaviour for a ContentFactory class that
 *	corresponds to Content classes that inherit from CDialog. This macro is
 *	declares a special create method that calls the Create() method of the
 *	CDialog-derived class in order to properly create the dialog window.
 *	NOTE: This macro must be placed after the Content-derived class is declared.
 *	See content.hpp for additional information.
 */
#define IMPLEMENT_CDIALOG_CONTENT_FACTORY( CONTENT, DIALOG_RESOURCE_ID ) \
	class CONTENT##Factory : public GUITABS::ContentFactory\
	{\
	public:\
	CONTENT##Factory() {}\
	GUITABS::ContentPtr create()\
	{\
		CONTENT* pContent = new CONTENT();\
		pContent->Create( DIALOG_RESOURCE_ID );\
		return pContent;\
	}\
	BW::wstring getContentID() { return CONTENT::contentID; }\
	}; 


BW_BEGIN_NAMESPACE

namespace GUITABS
{


/**
 *	This abstract class must be inherited by factories that create the desired
 *  dialogs, property sheets, and other kinds of windows to be inserted as
 *  tearoff panels. Since this class is an interface, there should be no
 *  problem with multiple inheritance.
 *  IMPORTANT NOTES:
 *  - Make sure you override the PostNcDestroy() method for CView and CFrameWnd
 *  derived classes. In these classes, the default behaviour of this method is
 *	to do a "delete this", which is incompatible to the way Content objects
 *	are deallocated by the Manager.
 *	- Classes that inherit from this class must not also inherit from
 *  ReferenceCount.
 *  For more information, please see manager.hpp and content.hpp
 */
class ContentFactory : public ReferenceCount
{
public:
	/**
	 *  Returns the contentID string of the class.
	 *  @return the BW::wstring representing the ID of the class.
	 *  @see Manager,ContentFactory
	 */
	virtual BW::wstring getContentID() = 0;

	/**
	 *  Returns a new Content derived object.
	 *  @return the new Content derived object.
	 *  @see Manager,ContentFactory
	 */
	virtual ContentPtr create() = 0;
};


} // namespace
BW_END_NAMESPACE


/**
 *
 *	Sample 1: ContentFactory declared by macro, perfect for CFormView and CWnd
 *	derived classes, not for CDialog derived classes:
 *	
 *	#include "guitabs/guitabs_content.hpp"
 *
 *	IMPLEMENT_BASIC_CONTENT_FACTORY( TestContent )
 *
 *	
 *	Sample 2: ContentFactory derived class:
 *
 *	#include "guitabs/guitabs_content.hpp"
 *	
 *	class TestFactory : public GUITABS::ContentFactory
 *	{
 *	public:
 *		GUITABS::ContentPtr create() { return new TestContent(); }
 *		BW::string getContentID() { return TestContent::contentID; }  // see the sample for Content in content.hpp
 *	};
 *	
 *
 *  Sample 3: ContentFactory create method for classes that require special
 *	window creation like CDialog derived classes.
 *
 *	#include "guitabs/guitabs_content.hpp"
 *
 *	GUITABS::ContentPtr CDialogContentFactory::create()
 *	{
 *		CDialogContent* p = new CDialogContent(); // CDialogContent inherits from Content and CDialog
 *		p->Create( CDialogContent::IDD );  // the resource ID is asumed to be defined in a const inside CDialogContent in this example
 *		return p;
 *	};
 *	
 *	NOTE: Factory class declaration via macro must be done after the content
 *	class declaration, be it in the same file or in another file.
 *	NOTE: For CFormView and other classes is usually not necesary to create the
 *	window inside the factory's create method. If you don't create the window,
 *	GUITABS will try to create a default window for you. If you create the
 *	window, calling CWnd::Create or CWnd::CreateEx for example, GUITABS will
 *	use your created window. GUITABS only creates a window if the GetSafeHwnd() member
 *	is not a valid window handle ( it's tested using Win32's ::IsWindow() ).
 *
 *********************************************************
 *
 *  Sample 4: factory registration call in the cpp file that creates the panels, etc:
 *
 *	GUITABS::Manager::instance().registerFactory( new TestFactory() );
 *	
 */


#endif // GUITABS_CONTENT_FACTORY_HPP
