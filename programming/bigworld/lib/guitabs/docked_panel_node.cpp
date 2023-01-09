
#include "pch.hpp"
#include "guitabs.hpp"
#include "cstdmf/guard.hpp"


BW_BEGIN_NAMESPACE

namespace GUITABS
{

/**
 *	Constructor.
 */
DockedPanelNode::DockedPanelNode()
{
	BW_GUARD;
}


/**
 *	Destructor.
 */
DockedPanelNode::DockedPanelNode( PanelPtr dockedPanel )
{
	BW_GUARD;

	init( dockedPanel );
}


/**
 *	This method initialises the node with its corresponding panel.
 *
 *	@param dockedPanel	Panel associated with this node.
 */
void DockedPanelNode::init( PanelPtr dockedPanel )
{
	BW_GUARD;

	dockedPanel_ = dockedPanel;
}


/**
 *	This method returns the panel's CWnd.
 *
 *	@return		The panel's CWnd.
 */
CWnd* DockedPanelNode::getCWnd()
{
	BW_GUARD;

	return dockedPanel_->getCWnd();
}


/**
 *	This method loades the layout information for a panel and associates it
 *	with an already created panel by index (this index corresponds to the index
 *	of creation of the panel in Dock::load).
 *
 *	@param section	Datasection containing the node info.
 *	@param parent	Parent window for this node's panel.
 *	@param wndID	Window's dialog control ID for this node's panel.
 *	@return		True if successful, false if not.
 */
bool DockedPanelNode::load( DataSectionPtr section, CWnd* parent, int wndID )
{
	BW_GUARD;

	if ( !section )
		return false;

	DataSectionPtr nodeSec = section->openSection( "DockedPanel", true );
	if ( !nodeSec )
		return false;

	int index = nodeSec->readInt( "index", -1 );
	if ( index < 0 )
		return false;

	PanelPtr panel = Manager::instance().dock()->getPanelByIndex( index );
	if ( !panel )
		return false;

	init( panel );
	panel->SetDlgCtrlID( wndID );
	panel->SetParent( parent );
	panel->ShowWindow( SW_SHOW );

	return true;
}


/**
 *	This method saves the layout information for the associated panel (by this
 *	panel's index of creation).
 *
 *	@param section	Datasection containing the node info.
 *	@return		True if successful, false if not.
 */
bool DockedPanelNode::save( DataSectionPtr section )
{
	BW_GUARD;

	if ( !section )
		return false;

	DataSectionPtr nodeSec = section->openSection( "DockedPanel", true );
	if ( !nodeSec )
		return false;

	nodeSec->writeInt( "index", dockedPanel_->getIndex() );
    return true;
}


/**
 *	This method is used to ask the node what its preferred default size is.
 *
 *	@param w	Return param, the default preferred width for the node.
 *	@param h	Return param, the default preferred height for the node.
 */
void DockedPanelNode::getPreferredSize( int& w, int& h )
{
	BW_GUARD;

	dockedPanel_->getPreferredSize( w, h );
}


/**
 *	This method returns whether or not the panel is expanded, as opposed to 
 *	rolled up.
 *
 *	@return	True if the node is expanded, false if not.
 */
bool DockedPanelNode::isExpanded()
{
	BW_GUARD;

	return dockedPanel_->isExpanded();
}


} // namespace
BW_END_NAMESPACE

