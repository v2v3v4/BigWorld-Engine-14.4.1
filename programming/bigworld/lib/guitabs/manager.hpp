/**
 *	GUI Tearoff panel framework - Manager class
 */

#ifndef GUITABS_MANAGER_HPP
#define GUITABS_MANAGER_HPP


#include "datatypes.hpp"
#include "cstdmf/singleton.hpp"
#include "cstdmf/bw_list.hpp"

class CFrameWnd;

BW_BEGIN_NAMESPACE

namespace GUITABS
{


/**
 *  This singleton class is the only class accesible by the user of the tearoff
 *  panel framework. The programmer must first register it's content factories
 *  and insert a dock into his mainFrame (using registerFactory and insertDock)
 *  in any order, and the the user can load a configuration file or insert
 *  panels manually to actually create and display the desired panels on the
 *  screen (using load or insertPanel). It is required that the programmer
 *  calls removeDock() on exit, before destroying it's main frame and view
 *  windows. It is recommended that the mainView window doesn't have a border.
 *  For more information, please see content.hpp and content_factory.hpp
 */
class Manager : public Singleton<Manager>, public ReferenceCount
{
public:
	Manager();
	~Manager();

	bool registerFactory( ContentFactoryPtr factory );

	bool insertDock( CFrameWnd* mainFrame, CWnd* mainView );

	void removeDock();

	PanelHandle insertPanel( const BW::wstring & contentID, InsertAt insertAt, PanelHandle destPanel = 0 );

	bool removePanel( PanelHandle panel );

	bool removePanel( const BW::wstring & contentID );

	void removePanels();

	void showPanel( PanelHandle panel, bool show );

	void showPanel( const BW::wstring & contentID, bool show );

	bool isContentVisible( const BW::wstring & contentID );

	Content* getContent( const BW::wstring & contentID, int index = 0 );

	bool isValid( PanelHandle panel );

	bool isDockVisible();

	void showDock( bool show );

	void showFloaters( bool show );

	void broadcastMessage( UINT msg, WPARAM wParam, LPARAM lParam );

	void sendMessage( const BW::wstring & contentID, UINT msg, WPARAM wParam, LPARAM lParam );

	bool load( const wchar_t* defaultFilename, const  BW::wstring & fname = L"" );

	bool save( const BW::wstring & fname = L"" );

	PanelHandle clone( PanelHandle content, int x, int y );

	DockPtr dock();

private:
	DockPtr dock_;
	DragManagerPtr dragMgr_;
	BW::list<ContentFactoryPtr> factoryList_;
	typedef BW::list<ContentFactoryPtr>::iterator ContentFactoryItr;
	BW::wstring lastLayoutFile_;

	// Utility methods, for friend classes internal use only.
	// In the case of the dock() method, this was done in order to avoid
	// having a pointer to the Dock inside each of the friend classes.
	friend Tab;
	friend DragManager;
	friend Panel;
	friend SplitterNode;
	friend Floater;
	friend DockedPanelNode;
	friend ContentContainer;

	ContentPtr createContent( const BW::wstring & contentID );

	DragManagerPtr dragManager();
};


} // namespace
 BW_END_NAMESPACE

#endif // GUITABS_MANAGER_HPP
