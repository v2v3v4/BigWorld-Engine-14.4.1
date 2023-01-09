/**
 *	GUI Tearoff panel framework - Content class
 */

#ifndef GUITABS_CONTENT_HPP
#define GUITABS_CONTENT_HPP

class CWnd;

/*
 *	This macro implements the default behaviour for a Content class, except
 *	that the user must declare and implement the isClonable(), clone(),
 *	onClose(), load() and save().
 *	The contentID here MUST match the contentID of the corresponding factory.
 *  See sample at the end of the file.
 *  methods.
 */
#define IMPLEMENT_ROOT_CONTENT_NOLOAD( shortName, longName, width, height, icon ) \
   public:\
   static const BW::wstring contentID;\
   BW::wstring getContentID() { return contentID; }\
   BW::wstring getDisplayString() { return (longName); }\
   BW::wstring getTabDisplayString() { return (shortName); }\
   HICON getIcon() { return (icon); }\
   CWnd* getCWnd() { return this; }\
   void getPreferredSize( int& w, int& h )\
   {\
       w = (width);\
       h = (height);\
   }\
   void handleRightClick( int x, int y ) { }\
   void OnOK() { }\
   void OnCancel() { }\
   void PostNcDestroy() { }


/*
 *	This macro implements the default behaviour for a Content class, except
 *	that the user must declare and implement the isClonable(), clone() and
 *	onClose();
 *	The contentID here MUST match the contentID of the corresponding factory.
 *  See sample at the end of the file.
 *  methods.
 */
#define IMPLEMENT_ROOT_CONTENT( shortName, longName, width, height, icon ) \
   IMPLEMENT_ROOT_CONTENT_NOLOAD( shortName, longName, width, height, icon ) \
   bool load( DataSectionPtr section ) { return true; }\
   bool save( DataSectionPtr section ) { return true; }


/*
 *	This macro implements the default behaviour for a non-clonable Content
 *	class.
 *  See sample at the end of the file.
 */
#define IMPLEMENT_BASIC_CONTENT( shortName, longName, width, height, icon ) \
   IMPLEMENT_ROOT_CONTENT( shortName, longName, width, height, icon ) \
   bool isClonable() { return false; }\
   GUITABS::ContentPtr clone() { return 0; }\
   OnCloseAction onClose( bool isLastContent ) { return isLastContent?CONTENT_HIDE:CONTENT_DESTROY; }


/*
 *	This macro implements the default behaviour for a non-clonable Content
 *	class that implements the load and save methods.
 *  See sample at the end of the file.
 */
#define IMPLEMENT_LOADABLE_CONTENT( shortName, longName, width, height, icon ) \
   IMPLEMENT_ROOT_CONTENT_NOLOAD( shortName, longName, width, height, icon ) \
   bool isClonable() { return false; }\
   GUITABS::ContentPtr clone() { return 0; }\
   OnCloseAction onClose( bool isLastContent ) { return isLastContent?CONTENT_HIDE:CONTENT_DESTROY; }


/*
 *	This macro implements the default behaviour for a clonable Content
 *	class. The user must only declare and implement the clone() method.
 *  See sample at the end of the file.
 */
#define IMPLEMENT_CLONABLE_CONTENT( shortName, longName, width, height, icon ) \
   IMPLEMENT_ROOT_CONTENT( shortName, longName, width, height, icon ) \
   bool isClonable() { return true; }\
   OnCloseAction onClose( bool isLastContent ) { return isLastContent?CONTENT_HIDE:CONTENT_DESTROY; }


/*
 *	This macro implements the default behaviour for a clonable Content
 *	class which can recieve close events. The user must declare and
 *  implement the clone() and onClose(bool) methods.
 *  See sample at the end of the file.
 */
#define IMPLEMENT_CLONE_CLOSE_CONTENT( shortName, longName, width, height, icon ) \
   IMPLEMENT_ROOT_CONTENT( shortName, longName, width, height, icon ) \
   bool isClonable() { return true; }


BW_BEGIN_NAMESPACE

namespace GUITABS
{

/**
 *	This abstract class must be inherited by dialogs, property sheets, and 
 *  other kinds of windows that wish to be inserted as a tearoff panel.
 *  Tipically, classes inheriting from this class also inherit from a MFC's
 *  CFormView class, but could also inherit from other MFC classes such as
 *  CDialog, CWnd, CView, etc, and since this class is base and abstract,
 *  there should be no problem with multiple inheritance. The only thing to
 *  care about is that classes that inherit from this class must not also
 *  inherit from ReferenceCount.
 *  IMPORTANT NOTES:
 *	If your class inherits from CFormView, you must set the dialog resource
 *  property "Visible" to False.
 *  Make sure you override the PostNcDestroy() method for CView and CFrameWnd
 *  derived classes. In these classes, the default behaviour of this method is
 *	to do a "delete this", which is incompatible to the way Content objects
 *	are deallocated by the Manager.
 *  For more information, please see manager.hpp and content_factory.hpp
 */
class Content : public ReferenceCount
{
public:
	enum OnCloseAction
	{
		CONTENT_KEEP,
		CONTENT_HIDE,
		CONTENT_DESTROY
	};

	/**
	 *  Returns the contentID string of the class.
	 *  @return the BW::wstring representing the ID of the class.
	 *  @see Manager,ContentFactory
	 */
	virtual BW::wstring getContentID() = 0;

	/**
	 *  Clone the content class, and return the new cloned object, if clone
	 *  is supported by the class. This method will not be called if the
	 *  class' isClonable() method returns false.
	 *  @return the new cloned object.
	 *  @see isClonable()
	 */
	virtual ContentPtr clone() = 0;

	/**
	 *  Load any class specific information previously saved.
	 *  @param section data section pointer where the data should be loaded from.
	 *  @return true if successful, false otherwise.
	 */
	virtual bool load( DataSectionPtr section ) = 0;

	/**
	 *  Save any class specific information for later load.
	 *  @param section data section pointer where the data should be saved to.
	 *  @return true if successful, false otherwise.
	 */
	virtual bool save( DataSectionPtr section ) = 0;

	/**
	 *  The deriving class should return here the display string, or window
	 *  title/caption that it wishes to have in its window caption bar.
	 *  @return wstring containing the desired window title text.
	 */
	virtual BW::wstring getDisplayString() = 0;

	/**
	 *  The deriving class should return here a short display string that can
	 *  displayed in the small tabs inside panels.
	 *  @return wstring containing the desired tab title short text.
	 */
	virtual BW::wstring getTabDisplayString() = 0;

	/**
	 *  The deriving class should return here the icon it wants displayed
	 *  on its tab inside a panel ( like in VS tabs ).
	 *  @return HICON containing the desired icon handle, or 0 for no icon.
	 */
	virtual HICON getIcon() = 0;

	/**
	 *  Returns if the class supports cloning.
	 *  @return true if the class supports cloning through the clone() method, false otherwise.
	 *  @see Content::clone()
	 */
	virtual bool isClonable() = 0;

	/**
	 *  Returns a pointer to the main CWnd object of the class. Typically, a
	 *  derived class will return "this".
	 *  @return a pointer to the class' main window CWnd.
	 */
	virtual CWnd* getCWnd() = 0;

	/**
	 *  Returns the preferred size of this class' main window.
	 *  @param width the preferred width is returned in this parameter
	 *  @param height the preferred height is returned in this parameter
	 */
	virtual void getPreferredSize( int& width, int& height ) = 0;

	/**
	 *  This method is called when the close button of the panel containing the
	 *	content is clicked. If the this function returns true, the Manager will
	 *	look for other content classes with the same ID. If there are, this
	 *	content will be permanently removed, but if not, it will be hidden, and
	 *	if the application wants to completely destroy it, it can later remove
	 *	the panel/tab.
	 *  @param isLastContent true if it's the last content with this ID, false if not.
	 *  @return true if the Content wishes to be removed/hidden, false if not.
	 */
	virtual OnCloseAction onClose( bool isLastContent ) = 0;

	/**
	 *  This method is called when the right mouse button is clicked in the
	 *	panel's title bar or in the tab associated with the content.
	 *  @param x x coord where the click occured.
	 *  @param y y coord where the click occured.
	 */
	virtual void handleRightClick( int x, int y ) = 0;
};


} // namespace

BW_END_NAMESPACE

/*
 *	Sample 1: derived class with macros:
 *	
 *	#include "guitabs/guitabs_content.hpp"
 *
 *	class TestContent : public CFormView, public GUITABS::Content
 *	{
 *		IMPLEMENT_BASIC_CONTENT( L"A Test!", L"A Test Content Panel Class!", 265, 430, NULL )
 *	public:
 *
 *		TestContent() : CFormView(IDD_TESTDLG2) {}
 *		~TestContent() {}
 *	};
 *	
 *	const BW::string TestContent::contentID = "TestContentID";
 *	
 *	
 *	
 *	Sample 2: derived class declared/implemented manually:
 *	
 *	#include "guitabs/guitabs_content.hpp"
 *	
 *	class TestContent : public CFormView, public GUITABS::Content
 *	{
 *	public:
 *		static const BW::string TestID;
 *		
 *		TestContent() : CFormView(IDD_TESTDLG2) {}
 *		~TestContent() {}
 *	
 *		BW::string getContentID() { return TestContent::TestID; }
 *		GUITABS::ContentPtr clone() { return 0; }
 *		bool load( DataSectionPtr section ) { return true; }
 *		bool save( DataSectionPtr section ) { return true; }
 *		BW::string getDisplayString() { return "A Test Content Panel Class!"; }
 *		BW::string getTabDisplayString() { return "A Test!"; }
 *		HICON getIcon() { return 0; }
 *		bool isClonable() { return false; }
 *		CWnd* getCWnd() { return this; }
 *		void getPreferredSize( int& width, int& height )
 *		{
 *			width = 265;
 *			height = 430;
 *		}
 *		GUITABS::OnCloseAction onClose( bool isLastContent )
 *		{
 *			if ( isLastContent )
 *				return GUITABS::OnCloseAction::CONTENT_HIDE; // if it's the last content of this class, keep it hidden.
 *			else
 *				return GUITABS::OnCloseAction::CONTENT_DESTROY; // if it's not the last, then just delete it.
 *		}
 *		void handleRightClick( int x, int y ) { }
 *		void OnOK() { }
 *		void OnCancel() { }
 *		void PostNcDestroy() { }  // VERY IMPORTANT: required for CView and CFrameWnd derived windows, to avoid the object doing a "delete this"
 *	};
 *	
 *	const BW::string TestContent::contentID = "TestContentID";
 *	
 *	
 */

#endif // GUITABS_CONTENT_HPP
