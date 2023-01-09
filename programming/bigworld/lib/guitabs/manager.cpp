/**
 *	GUI Tearoff panel framework - Manager class implementation
 */


#include "pch.hpp"
#include "guitabs.hpp"
#include "cstdmf/string_utils.hpp"
#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

/// GUITabs Manager Singleton
BW_SINGLETON_STORAGE( GUITABS::Manager )

namespace GUITABS
{


/**
 *	Constructor.
 */
Manager::Manager() :
	dragMgr_( new DragManager() ),
	lastLayoutFile_( L"" )
{
	BW_GUARD;

	registerFactory( new ContentContainerFactory() );
}


/**
 *	Destructor.
 */
Manager::~Manager()
{
	BW_GUARD;

	removeDock();

	factoryList_.clear();
}


/**
 *  Register a factory object that will be responsible for creating the
 *  actual content panel.
 *
 *  @param factory the ContentFactory-derived object.
 *  @return true if the factory was successfully registered, false otherwise.
 *  @see ContentFactory
 */
bool Manager::registerFactory( ContentFactoryPtr factory )
{
	BW_GUARD;

	if ( !factory )
		return false;

	factoryList_.push_back( factory );
	return true;
}


/**
 *  Insert/register the MainFrame and the main view windows. The Manager
 *  will dock panels in the client area of the MainFrame window, and will
 *  resize/reposition the main view window to make room for docked panels.
 *  The main frame window must be the parent of the main view window.
 *
 *  @param mainFrame the Main Frame window of the application. Must inherit from CFrameWnd.
 *  @param mainFrame the Main View window (ie. the 3D window in WorldEditor). Must inherit from CWnd.
 *  @return true if the windows were successfully registered, false otherwise.
 */
bool Manager::insertDock( CFrameWnd* mainFrame, CWnd* mainView )
{
	BW_GUARD;

	if ( !mainFrame || !mainView )
		return false;

	if ( dock_ )
		return false;

	dock_ = new Dock( mainFrame, mainView );

	return true;
}


/**
 *  Unregister the MainFrame and the main view windows. The Manager
 *  will destroy all panels, tabs, content panels, and all other resources,
 *  and will set the Main Frame window as the parent of the Main View window.
 */
void Manager::removeDock()
{
	BW_GUARD;

	if ( !dock_ )
		return;

	dock_ = 0;
}


/**
 *  Utility method to create a panel displaying the content corresponding
 *  to a registered factory with the code contentID. The contentID will be
 *  searched for in the list of ContentFactory factories.
 *  Ideally, panels should be created with the load() method and managed
 *  automatically by the framework, but this method allows manual use.
 *  IMPORTANT NOTE: Panel handles can be no longer valid in some situations
 *  such as when the related panel is destroyed (i.e. after a remove dock).
 *  To know if a Panel Handle is is still valid, use the isValid() method.
 *
 *  @param contentID ID of the content to display in the new panel.
 *  @param insertAt enum indicating where the panel must be inserted.
 *  @param destPanel destination panel to insert the panel into. If ommitted, the manager will layout the panel automatically.
 *  @return a PanelHandle to the panel that was created, 0 otherwise.
 *  @see ContentFactory, Content, Manager::isValid(), Manager::load(), Manager::save()
 */
PanelHandle Manager::insertPanel( const BW::wstring & contentID, InsertAt insertAt, PanelHandle destPanel )
{
	BW_GUARD;

	if ( !dock_ )
		return 0;

	PanelPtr panel = dock_->insertPanel( contentID, destPanel, insertAt );

	// this code should be improved so in the rare case of having two contents
	// with the same contentID in one panel, it returns the last inserted one.
	// (probably using the getContent method that takes an 'index' param?)
	return panel->getContent( contentID ).getObject();
}


/**
 *  Utility method to remove previously created panel by Panel Handle
 *  Ideally, panels should be created with the load() method and managed
 *  automatically by the framework, but this method allows manual use.
 *
 *  @param contentID ID of the content to display in the new panel.
 *  @return true if the panel was removed, false otherwise.
 *  @see ContentFactory, Content, Manager::load(), Manager::save()
 */
bool Manager::removePanel( PanelHandle panel )
{
	BW_GUARD;

	if ( !dock_ )
		return false;

	PanelPtr p = dock_->getPanelByHandle( panel );

	if ( p )
		dock_->removePanel( p );

	return ( !!p );
}


/**
 *  Utility method to remove previously created panel by content ID.
 *  Ideally, panels should be created with the load() method and managed
 *  automatically by the framework, but this method allows manual use.
 *  IMPORTANT NOTE: If there are several content objects with the same
 *  contentID (for instance, panels that are cloned), this method will
 *	remove them all.
 *
 *  @param contentID ID of the content to display in the new panel.
 *  @return true if the panel was removed, false otherwise.
 *  @see ContentFactory, Content, Manager::load(), Manager::save()
 */
bool Manager::removePanel( const BW::wstring & contentID )
{
	BW_GUARD;

	if ( !dock_ )
		return false;

	dock_->removePanel( contentID );

	return true;
}


/**
 *  Remove all panels.
 *
 *  @return true if the panels were removed, false otherwise.
 *  @see ContentFactory, Content
 */
void Manager::removePanels()
{
	BW_GUARD;

	if ( !dock_ )
		return;
	CFrameWnd* mainFrame = dock_->getMainFrame();
	CWnd* mainView = dock_->getMainView();
	dock_ = 0;
	dock_ = new Dock( mainFrame, mainView );
}


/**
 *  Show or Hide a panel by it's Panel Handle.
 *
 *  @param panel the panel handle of the panel to show/hide
 *  @param show true to show the panel, false to hide it
 *  @see Manager::insertPanel(), Manager::removePanel()
 */
void Manager::showPanel( PanelHandle panel, bool show )
{
	BW_GUARD;

	if ( !dock_ )
		return;

	ContentPtr content = panel;
	dock_->showPanel( content, show );
}


/**
 *  Show or Hide a panel/tab by it's contentID
 *  IMPORTANT NOTE: If there are several content objects with the same
 *  contentID (for instance, panels that are cloned), this method will
 *	show/hide them all.
 *
 *  @param panel the panel handle of the panel to show/hide
 *  @param show true to show the panel, false to hide it
 *  @see Manager::insertPanel()
 */
void Manager::showPanel( const BW::wstring & contentID, bool show )
{
	BW_GUARD;

	if ( !dock_ )
		return;

	dock_->showPanel( contentID, show );
}


/**
 *  Query if a content is visible in one or more panels.
 *  IMPORTANT NOTE: There can bee several content objects with the same
 *  contentID (for instance, panels that are cloned).
 *
 *  @return true to show the panel, false to hide it
 *  @see Manager::insertPanel()
 */
bool Manager::isContentVisible( const BW::wstring & contentID )
{
	BW_GUARD;

	if ( !dock_ )
		return false;

	return dock_->isContentVisible( contentID );
}


/**
 *  Return's a Content object pointer by it's contentID, or NULL if no
 *	Panel/Tab has been inserted with that contentID. The application can
 *	then cast the pointer back to the original type declared in the
 *	corresponding ContentFactory derived class.
 *  IMPORTANT NOTE: this method returns the first instance of the Content,
 *	so in the case of multiple instances of the same content you should use
 *	the getContents method.
 *
 *  @param contentID identifier of the desired Content.
 *  @param index index, when there's more than one 'contentID' panel.
 *  @return a pointer to the Content, or NULL if no contentID panel exists.
 *  @see Manager::insertPanel()
 */
Content* Manager::getContent( const BW::wstring & contentID, int index /*=0*/ )
{
	BW_GUARD;

	if ( !dock_ )
		return 0;

	return dock_->getContent( contentID, index ).getObject();
}


/**
 *  Finds if a Panel Handle is still valid or not. For more info, read the
 *  insertPanel important notes section.
 *
 *  @param panel the panel handle of the panel to test
 *  @see Manager::insertPanel()
 */
bool Manager::isValid( PanelHandle panel )
{
	BW_GUARD;

	if ( !dock_ )
		return false;

	if ( dock_->getPanelByHandle( panel ) )
		return true;
	else
		return false;
}


/**
 *  Utility method to see if the dock is visible.
 *
 *  @param show true if the dock is visible, false otherwise.
 */
bool Manager::isDockVisible()
{
	BW_GUARD;

	if ( !dock_ )
		return false;

	return dock_->isDockVisible();
}


/**
 *  Utility method to show or hide all docked panels.
 *
 *  @param show true to show the dock and all panels, false to hide.
 */
void Manager::showDock( bool show )
{
	BW_GUARD;

	if ( !dock_ )
		return;

	dock_->showDock( show );
}


/**
 *  Utility method to show or hide all floating panel windows.
 *
 *  @param show true to show all panels, false to hide all panels.
 */
void Manager::showFloaters( bool show )
{
	BW_GUARD;

	if ( !dock_ )
		return;

	dock_->showFloaters( show );
}


/**
 *  Utility method to send a message to all content windows.
 *
 *  @param msg message id.
 *  @param WPARAM additional message-dependent information.
 *  @param LPARAM additional message-dependent information.
 */
void Manager::broadcastMessage( UINT msg, WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	if ( !dock_ )
		return;

	dock_->broadcastMessage( msg, wParam, lParam );
}


/**
 *  Utility method to send a message to content windows with ID contentID.
 *
 *  @param contentID content identifier.
 *  @param msg message id.
 *  @param WPARAM additional message-dependent information.
 *  @param LPARAM additional message-dependent information.
 */
void Manager::sendMessage( const BW::wstring & contentID, UINT msg, WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	if ( !dock_ )
		return;

	dock_->sendMessage( contentID, msg, wParam, lParam );
}


/**
 *  Load all panels previously saved. This will reload the last saved panels
 *  with their corresponding insert position, floating state, visibility, etc.
 *  This function is tipically called on app's startup, after creating the
 *  mainframe window and the main 3D view window.
 *
 *  @param fname file to load layout from.
 *  @return true if successful, false otherwise 
 */
bool Manager::load( const wchar_t* defaultFilename, const  BW::wstring & fname )
{
	BW_GUARD;

	if ( !dock_ )
		return false;

	BW::wstring loadName = fname;
	if ( fname.empty() )
	{
		wchar_t buffer[MAX_PATH+1];
		GetCurrentDirectory( ARRAY_SIZE( buffer ), buffer );
		wcscat( buffer, L"\\" );
		wcscat( buffer, defaultFilename );
		loadName = buffer;
		std::replace( loadName.begin(), loadName.end(), L'\\', L'/' );
	}
	lastLayoutFile_ = loadName;

	BW::string nloadName;
	bw_wtoutf8( loadName, nloadName );
	DataResource file( nloadName, RESOURCE_TYPE_XML );
	DataSectionPtr section = file.getRootSection();

	if ( !section )
		return false;

	if ( !dock_->empty() )
		removePanels();

	if ( !dock_->load( section->openSection( "Dock" ) ) )
	{
		removePanels();
		return false;
	}

	return true;
}


/**
 *  Load all panels previously saved. This will reload the las panels
 *  with their corresponding insert position, floating state, etc.
 *  This function is tipically called on app's exit, before destroying the
 *  mainframe window and the main 3D view window.
 *
 *  @param fname file to save into, or "" to save to the last loaded file.
 *  @return true if successful, false otherwise 
 */
bool Manager::save( const BW::wstring & fname )
{
	BW_GUARD;

	if ( !dock_ )
		return false;

	BW::wstring saveName = fname;
	if ( fname.empty() )
		saveName = lastLayoutFile_;
	if ( saveName.empty() )
		return false;

	BW::string nsaveName;
	bw_wtoutf8( saveName, nsaveName );
	DataResource file( nsaveName, RESOURCE_TYPE_XML );
	DataSectionPtr section = file.getRootSection();

	if ( !section )
		return false;

	if ( !dock_->save( section->openSection( "Dock", true ) ) )
		return false;

	if ( file.save( nsaveName ) != DataHandle::DHE_NoError )
		return false;
	return true;
}


/**
 *  Clone a tab ( a content/panelhandle ) to a new dialog.
 *
 *  @param content pointer to the panel handle (content) to be cloned
 *  @param x desired X position of the newly created panel
 *  @param y desired Y position of the newly created panel
 *  @return panel handle of the new panel, 0 if no panel could be created.
 */
PanelHandle Manager::clone( PanelHandle content, int x, int y  )
{
	BW_GUARD;

	if ( !dock_ )
		return 0;

	PanelPtr panel = dock_->getPanelByHandle( content );
	if ( !panel )
		return 0;

	return panel->cloneTab( content, x, y  ).getObject();
}


// Utility methods used internally

/**
 *  Create a Content derived-object from a previously registered
 *  ContentFactory-derived object matching the contentID.
 *
 *  @see Tab
 */
ContentPtr Manager::createContent( const BW::wstring & contentID )
{
	BW_GUARD;

	for( ContentFactoryItr i = factoryList_.begin(); i != factoryList_.end(); ++i )
	{
		if ( contentID.compare( (*i)->getContentID() ) == 0 )
			return (*i)->create();
	}

	return 0;
}


/**
 *  Get the main Dock object of the Manager.
 *
 *  @see Dock
 */
DockPtr Manager::dock()
{
	BW_GUARD;

	return dock_;
}


/**
 *  Get the DragManager object.
 *
 *  @see DragManager
 */
DragManagerPtr Manager::dragManager()
{
	BW_GUARD;

	return dragMgr_;
}


} // namespace
BW_END_NAMESPACE

