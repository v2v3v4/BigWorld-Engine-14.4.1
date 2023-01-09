
#include "pch.hpp"
#include "guitabs.hpp"
#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

namespace GUITABS
{


/**
 *	Constructor.
 *
 *	@param mainView	Main view window to associate with this node.
 */
MainViewNode::MainViewNode( CWnd* mainView ) :
	mainView_( mainView )
{
	BW_GUARD;
}


/**
 *	This method returns the main view's CWnd.
 *
 *	@return		The panel's CWnd.
 */
CWnd* MainViewNode::getCWnd()
{
	BW_GUARD;

	return mainView_;
}


/**
 *	This method initialises the main view node and sets its parent so it's
 *	put in the dock.
 *
 *	@param section	Datasection containing the node info.
 *	@param parent	Parent window for the main view.
 *	@param wndID	Window's dialog control ID for the main view.
 *	@return		True if successful, false if not.
 */
bool MainViewNode::load( DataSectionPtr section, CWnd* parent, int wndID )
{
	BW_GUARD;

	if ( !section )
		return false;

	DataSectionPtr nodeSec = section->openSection( "MainView" );
	if ( !nodeSec )
		return false;

	mainView_->SetDlgCtrlID( wndID );
	mainView_->SetParent( parent );

	return true;
}


/**
 *	This method saves the layout information for the associated panel (by this
 *	panel's index of creation).
 *
 *	@param section	Datasection to save the node info to.
 *	@return		True if successful, false if not.
 */
bool MainViewNode::save( DataSectionPtr section )
{
	BW_GUARD;

	if ( !section )
		return false;

	DataSectionPtr nodeSec = section->openSection( "MainView", true );
	if ( !nodeSec )
		return false;

	nodeSec->setString( "Main Application View Window" );
	return true;
}


} // namespace
BW_END_NAMESPACE

