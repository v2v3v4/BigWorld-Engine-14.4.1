
#include "pch.hpp"
#include "guitabs.hpp"
#include "cstdmf/guard.hpp"


BW_BEGIN_NAMESPACE

namespace GUITABS
{


/**
 *	Constructor.
 *
 *	@param mainFrame	Window that will be the parent for the dock's root
 *						splitter.
 *	@param mainView		Main app view window, that will be a child of the
 *						dock's root splitter. This is the 3D view in WE/PE/ME.
 */
Dock::Dock( CFrameWnd* mainFrame, CWnd* mainView ) :
	dockVisible_( true ),
	dockTreeRoot_( 0 ),
	mainFrame_( mainFrame ),
	mainView_( mainView )
{
	BW_GUARD;

	originalMainViewID_ = mainView_->GetDlgCtrlID();
	dockTreeRoot_ = new MainViewNode( mainView );
}


/**
 *	Destructor.
 */
Dock::~Dock()
{
	BW_GUARD;

	showDock( false );

	// dettach the mainView_ window from the dock tree, and put it as a child
	// of mainFrame_. Also sets the active view so we don't get any strange MFC
	// asserts on exit.
	if ( IsWindow( mainView_->GetSafeHwnd() ) && IsWindow( mainFrame_->GetSafeHwnd() ) )
	{
		mainView_->SetDlgCtrlID( originalMainViewID_ );
		mainView_->SetParent( mainFrame_ );
		mainView_->SetFocus();
	}
	if ( IsWindow( mainFrame_->GetSafeHwnd() ) )
	{
		mainFrame_->SetActiveView( 0 );
		mainFrame_->RecalcLayout();
	}

	// destroy tree in leaf to parent order, to avoid MFC destroy calls on
	// children when the parent is destroyed.
	dockTreeRoot_->destroy();
	for(FloaterItr i = floaterList_.begin(); i != floaterList_.end(); ++i )
	{
		(*i)->getRootNode()->destroy();
	}
}


/**
 *	This method returns whether or not the list of panels is empty.
 *
 *	@return		True if there are no panels in this dock, false otherwise.
 */
bool Dock::empty()
{
	BW_GUARD;

	return panelList_.empty();
}


/**
 *	This method returns the main frame window used during construction.
 *
 *	@return		The main frame window used during construction.
 */
CFrameWnd* Dock::getMainFrame()
{
	BW_GUARD;

	return mainFrame_;
}


/**
 *	This method returns the main view window used during construction.
 *
 *	@return		The main view window used during construction.
 */
CWnd* Dock::getMainView()
{
	return mainView_;
}


/**
 *	This method returns a dock node (panel or splitter) from a point on the
 *	screen.
 *
 *	@param x	Screen X coordinate.
 *	@param y	Screen Y coordinate.
 *	@return		The dock node under the screen position x,y.
 */
DockNodePtr Dock::getNodeByPoint( int x, int y )
{
	BW_GUARD;

	DockNodePtr ret = 0;
	
	for( FloaterItr i = floaterList_.begin(); i != floaterList_.end() && !ret; ++i )
	{
		if ( (*i)->IsWindowVisible() &&
			::IsChild( (*i)->getCWnd()->GetSafeHwnd(), ::WindowFromPoint( CPoint( x, y ) ) ) )
		{
			ret = (*i)->getRootNode()->getNodeByPoint( x, y );
			if ( ret )
				break;
		}
	}

	if ( ! ret )
		ret = dockTreeRoot_->getNodeByPoint( x, y );

	return ret;
}


/**
 *	This method moves a tab from a panel and docks it according to the insertAt
 *	param.  For example, if insertAt is LEFT, then the tab is inserted as a
 *	panel to the left of destPanel.
 *
 *	@param panel	Panel where the tab currently is.
 *	@param tab		Tab to be moved.
 *	@param destPanel	Panel to use for the insertAt operation.
 *	@param insertAt		How to insert the tab.
 *	@param srcX		Source X coordinate, used for when floating the panel.
 *	@param srcY		Source Y coordinate, used for when floating the panel.
 *	@param dstX		Destination X coordinate, used for when floating the panel.
 *	@param dstY		Destination Y coordinate, used for when floating the panel.
 */
void Dock::dockTab( PanelPtr panel, TabPtr tab, CWnd* destPanel, InsertAt insertAt, int srcX, int srcY, int dstX, int dstY )
{
	BW_GUARD;

	CRect rect;
	panel->GetWindowRect( &rect );

	panel->detachTab( tab );

	PanelPtr newPanel = new Panel( mainFrame_ );
	panelList_.push_back( newPanel );

	newPanel->addTab( tab );

	newPanel->setLastPos( rect.left, rect.top );

	CRect mainRect;
	mainFrame_->GetWindowRect( &mainRect );
	rect.OffsetRect( -mainRect.left, -mainRect.top );
	newPanel->SetWindowPos( 0, rect.left, rect.top, 0, 0, SWP_NOSIZE|SWP_NOZORDER );

	copyPanelRestorePosToTab( panel, newPanel );

	dockPanel( newPanel, destPanel, insertAt, srcX, srcY, dstX, dstY );

	newPanel->activate();
}


/**
 *	This method docks a panel into another panel, or floats it if insertAt is
 *	FLOATING, or inserts it as a tab if insertAt is TAB.
 *
 *	@param panel	Panel to me docked or floated.
 *	@param destPanel	Panel to use for the insertAt operation.
 *	@param insertAt		How to insert the tab.
 *	@param srcX		Source X coordinate, used for when floating the panel.
 *	@param srcY		Source Y coordinate, used for when floating the panel.
 *	@param dstX		Destination X coordinate, used for when floating the panel.
 *	@param dstY		Destination Y coordinate, used for when floating the panel.
 */
void Dock::dockPanel( PanelPtr panel, CWnd* destPanel, InsertAt insertAt, int srcX, int srcY, int dstX, int dstY )
{
	BW_GUARD;

	if ( !panel ) 
		return;

	if ( insertAt == FLOATING )
	{
		floatPanel( panel, srcX, srcY, dstX, dstY );
	}
	else if ( destPanel )
	{
		insertPanelIntoPanel( panel, destPanel, insertAt );
	}
}


/**
 *	This method does all the hard work to make a panel float (a popup panel as
 *	opposed to a docked panel).
 *
 *	@param panel	Panel to float.
 *	@param srcX		Source X coordinate, used for when floating the panel.
 *	@param srcY		Source Y coordinate, used for when floating the panel.
 *	@param dstX		Destination X coordinate, used for when floating the panel.
 *	@param dstY		Destination Y coordinate, used for when floating the panel.
 */
void Dock::floatPanel( PanelPtr panel, int srcX, int srcY, int dstX, int dstY )
{
	BW_GUARD;

	CRect rect;
	panel->GetWindowRect( &rect );
	dstX = dstX - ( srcX - rect.left );
	dstY = dstY - ( srcY - rect.top );

	int w = rect.Width() + GetSystemMetrics( SM_CXFIXEDFRAME  ) * 2;
	int h = rect.Height() + GetSystemMetrics( SM_CYFIXEDFRAME  ) * 2;

	if ( !panel->isFloating() )
	{
		showDock( true );
		panel->getPreferredSize( w, h );
		w += GetSystemMetrics( SM_CXFIXEDFRAME  ) * 2;
		h += GetSystemMetrics( SM_CYFIXEDFRAME  ) * 2;
	}

	Floater::validatePos( dstX, dstY, w, h );

	if ( panel->isFloating() )
	{
		FloaterPtr floater = getFloaterByWnd( panel->getCWnd() );

		if ( floater && floater->getRootNode()->isLeaf() )
		{
			floater->SetWindowPos( 0, dstX, dstY, 0, 0, SWP_NOZORDER|SWP_NOSIZE );
			floater->ShowWindow( SW_SHOW );
			return;
		}
	}

	panel->setLastPos( dstX, dstY );

	removeNodeByWnd( panel->getCWnd() );

	FloaterPtr floater = new Floater( mainFrame_ );
	floater->SetWindowPos( 0, dstX, dstY, w, h, SWP_NOZORDER );
	floaterList_.push_back( floater );

	DockedPanelNodePtr panelNode = new DockedPanelNode( panel );

	floater->setRootNode( panelNode );

	panelNode->setParentWnd( floater->getCWnd() );
	panelNode->getCWnd()->SetDlgCtrlID( AFX_IDW_PANE_FIRST );
	floater->RecalcLayout();

	panel->setFloating( true );

	floater->ShowWindow( SW_SHOW );

	// update current docking positions
	for( PanelItr i = panelList_.begin(); i != panelList_.end(); ++i )
		savePanelDockPos( *i );
}


/**
 *	This method does the necessary work for inserting a panel as a tab into
 *	another panel.
 *
 *	@param panel		Panel to me docked as a tab.
 *	@param destPanel	Panel where the tab will be inserted.
 */
void Dock::attachAsTab( PanelPtr panel, CWnd* destPanel )
{
	BW_GUARD;

	TabPtr tab;
	PanelPtr dest = 0;

	for( PanelItr i = panelList_.begin(); i != panelList_.end(); ++i )
	{
		if ( (*i)->getCWnd() == destPanel )
		{
			dest = (*i);
			break;
		}
	}

	if ( !dest )
		return;

	while ( tab = panel->detachFirstTab() )
		dest->addTab( tab );

	dest->activate();

	removePanel( panel );
}


/**
 *	This method returns whether or not the dock is visible.
 *
 *	@return True if the dock is visible, false if not.
 */
bool Dock::isDockVisible()
{
	BW_GUARD;

	return dockVisible_;
}


/**
 *	This method shows or hides the dock and all its panels.
 *
 *	@param show	True to show the dock and all its panels, false to hide it.
 */
void Dock::showDock( bool show )
{
	BW_GUARD;

	if ( dockVisible_ == show )
		return;

	dockVisible_ = show;

	if ( dockTreeRoot_->getCWnd() == mainView_ )
		return;

	// show/hide mainFrame docked panels
	CWnd* wnd = dockTreeRoot_->getCWnd();

	if ( show )
	{
		DockNodePtr node;
		DockNodePtr parent;
		getNodeByWnd( mainView_, node, parent );

		int id = 0;
		int side = 0;
		if ( parent->getRightChild()->getCWnd() == mainView_ )
			side = 1;

		if ( parent->getSplitOrientation() == HORIZONTAL )
			id = ((CSplitterWnd*)parent->getCWnd())->IdFromRowCol( 0, side );
		else
			id = ((CSplitterWnd*)parent->getCWnd())->IdFromRowCol( side, 0 );

		mainView_->SetDlgCtrlID( id );

		mainView_->SetParent( parent->getCWnd() );

		wnd->SetDlgCtrlID( originalMainViewID_ );
		dockTreeRoot_->setParentWnd( mainFrame_ );
		dockTreeRoot_->recalcLayout();
		wnd->ShowWindow( SW_SHOW );
	}
	else
	{
		wnd->SetDlgCtrlID( 0 );
		dockTreeRoot_->setParentWnd( 0 );
		wnd->ShowWindow( SW_HIDE );

		mainView_->SetDlgCtrlID( originalMainViewID_ );
		mainView_->SetParent( mainFrame_ );
		mainView_->SetFocus();
	}
	mainFrame_->RecalcLayout();
}


/**
 *	This method shows or hides all the floating panels.
 *
 *	@param show	True to show the floating panels, false to hide them.
 */
void Dock::showFloaters( bool show )
{
	BW_GUARD;

	for( FloaterItr i = floaterList_.begin(); i != floaterList_.end(); ++i )
		(*i)->ShowWindow( show?SW_SHOW:SW_HIDE );
}


/**
 *	This method does all the hard for for docking a panel.
 *
 *	@param panel	Panel to me docked.
 *	@param destPanel	Panel to use for the insertAt operation.  Should never
 *						be FLOATING for this method.
 *	@param insertAt		How to insert the tab.
 */
void Dock::insertPanelIntoPanel( PanelPtr panel, CWnd* destPanel, InsertAt insertAt )
{
	BW_GUARD;

	if ( !panel || !destPanel )
		return;

	showDock( true );

	// remove panel from it's old docking position
	removeNodeByWnd( panel->getCWnd() );

	// find the destination panel's node and parent node
	DockNodePtr childNode = 0;
	DockNodePtr parentNode = 0;

	getNodeByWnd( destPanel, childNode, parentNode );

	if ( !childNode )
		return;

	// try to find out if it's in a floating window, and return the floater
	FloaterPtr floater = getFloaterByWnd( destPanel );

	// we now know destPanel is valid, so if it's tab, insert it in
	if ( insertAt == TAB )
	{
		attachAsTab( panel, destPanel );
		getNodeByWnd( destPanel, childNode, parentNode );
		if ( floater )
		{
			if ( !floater->getRootNode()->isLeaf() && childNode )
			{
				floater->RecalcLayout();
				floater->getRootNode()->recalcLayout();
				floater->RecalcLayout();
			}
		}
		else
		{
			if ( !dockTreeRoot_->isLeaf() && childNode )
			{
				mainFrame_->RecalcLayout();
				dockTreeRoot_->recalcLayout();
				mainFrame_->RecalcLayout();
			}
		}
		return;
	}

	// create a new splitter to insert the panel in
	Orientation dir;

	if ( insertAt == TOP || insertAt == BOTTOM )
		dir = VERTICAL;
	else
		dir = HORIZONTAL;

	CWnd* parentWnd;

	if ( parentNode )
		parentWnd = parentNode->getCWnd();
	else
	{
		if ( floater )
			parentWnd = floater->getCWnd();
		else
			parentWnd = mainFrame_;
	}

	CRect destRect;
	int destID;

	destPanel->GetWindowRect( &destRect );
	destID = destPanel->GetDlgCtrlID();

	SplitterNodePtr newSplitter = new SplitterNode( dir, parentWnd, destID );
	DockedPanelNodePtr newNode = new DockedPanelNode( panel );

	int w;
	int h;

	panel->getPreferredSize( w, h );

	// set the splitter's childs
	int leftChildSize = 0;
	int rightChildSize = 0;

	if ( insertAt == LEFT || insertAt == TOP ) 
	{
		newSplitter->setLeftChild( newNode );
		newSplitter->setRightChild( childNode );
		if ( dir == VERTICAL )
			leftChildSize = h;
		else
			leftChildSize = w;
	}
	else
	{
		newSplitter->setLeftChild( childNode );
		newSplitter->setRightChild( newNode );
		if ( dir == VERTICAL )
			rightChildSize = h;
		else
			rightChildSize = w;
	}

	// set the splitter's parent
	if ( parentNode )
	{
		if ( parentNode->getLeftChild() == childNode )
		{
			parentNode->setLeftChild( newSplitter );
		}
		else
		{
			parentNode->setRightChild( newSplitter );
		}
	}
	else
	{
		if ( floater )
		{
			floater->setRootNode( newSplitter );
		}
		else
			dockTreeRoot_ = newSplitter;
	}

	// finish splitter window required operations
	newSplitter->finishInsert( &destRect, leftChildSize, rightChildSize );

	// recalc layout, needed by splitter, miniframe and frame windows
	if ( floater )
	{
		floater->ShowWindow( SW_SHOW );

		panel->setFloating( true );
		floater->adjustSize();
		floater->getRootNode()->recalcLayout();
		floater->RecalcLayout();
	}
	else
	{
		panel->setFloating( false );
		dockTreeRoot_->recalcLayout();
		mainFrame_->RecalcLayout();
		dockTreeRoot_->adjustSizeToNode( newNode );
		dockTreeRoot_->recalcLayout();
		mainFrame_->RecalcLayout();
	}

	// update current docking positions
	for( PanelItr i = panelList_.begin(); i != panelList_.end(); ++i )
		savePanelDockPos( *i );
}


/**
 *	This method returns a node and its parent by the node's CWnd, no matter if
 *	it's docked or floating.
 *
 *	@param ptr	CWnd corresponding to the desired node.
 *	@param childNode	Return param, the node (panel or splitter) matching the
 *						CWnd pointer.
 *	@param parentNode	Return param, the parent of the node.
 *	@return		True if found, false if not.
 */
bool Dock::getNodeByWnd( CWnd* ptr, DockNodePtr& childNode, DockNodePtr& parentNode )
{
	BW_GUARD;

	childNode = 0;
	parentNode = 0;

	dockTreeRoot_->getNodeByWnd( ptr, childNode, parentNode );

	FloaterItr i = floaterList_.begin();
	while( i != floaterList_.end() && !childNode )
		(*i++)->getRootNode()->getNodeByWnd( ptr, childNode, parentNode );

	return !!childNode;
}


/**
 *	This method returns the floater window where the node matching the
 *	specified CWnd is in.
 *
 *	@param ptr	CWnd corresponding to the desired node.
 *	@return		Floater window where the node is, or NULL if not found.
 */
FloaterPtr Dock::getFloaterByWnd( CWnd* ptr )
{
	BW_GUARD;

	DockNodePtr childNode = 0;
	DockNodePtr parentNode = 0;

	for( FloaterItr i = floaterList_.begin(); i != floaterList_.end() && !childNode; ++i )
	{
		(*i)->getRootNode()->getNodeByWnd( ptr, childNode, parentNode );
		if ( childNode )
			return *i;
	}

	return 0;
}


/**
 *	This method does the hard work of removing a node from the dock tree, by
 *	CWnd.
 *
 *	@param ptr	CWnd corresponding to the desired node.
 */
void Dock::removeNodeByWnd( CWnd* ptr )
{
	BW_GUARD;

	DockNodePtr childNode = 0;
	DockNodePtr parentNode = 0;
	FloaterPtr floater;

	getNodeByWnd( ptr, childNode, parentNode );
	floater = getFloaterByWnd( ptr );

	if ( floater )
	{
		// Hack: To avoid that the CFrameWnd floater still points to a deleted
		// panel as its view, which later results in assertion failed/crash.
		floater->SetActiveView( 0 );
	}

	if ( !childNode )
		return; // node not found, it's already removed from trees

	childNode->getCWnd()->ShowWindow( SW_HIDE );
	childNode->setParentWnd( 0 );
	childNode->getCWnd()->SetWindowPos( 0, 0, 0, 0, 0, SWP_NOZORDER|SWP_NOSIZE );

	// set last floating position
	if ( floater )
	{
		CRect rect;
		floater->GetWindowRect( &rect );
		PanelPtr panel = getPanelByWnd( ptr );
		if ( panel )
			panel->setLastPos( rect.left, rect.top );
	}

	if ( !parentNode )
	{
		if ( floater )
		{
			for( FloaterItr i = floaterList_.begin(); i != floaterList_.end(); ++i )
			{
				if ( (*i) == floater )
				{
					floaterList_.erase( i );
					break;
				}
			}
		}
		return;
	}

	DockNodePtr grandParentNode = 0;

	getNodeByWnd( parentNode->getCWnd(), parentNode, grandParentNode );

	if ( !parentNode )
		return; // at this point, it should always find the parentNode

	DockNodePtr otherChildNode;

	parentNode->setParentWnd( 0 );

	if ( parentNode->getLeftChild() == childNode )
		otherChildNode = parentNode->getRightChild();
	else
		otherChildNode = parentNode->getLeftChild();

	if ( grandParentNode )
	{
		if ( grandParentNode->getLeftChild() == parentNode )
			grandParentNode->setLeftChild( otherChildNode );
		else
			grandParentNode->setRightChild( otherChildNode );
	}
	else
	{
		int id = parentNode->getCWnd()->GetDlgCtrlID();
		otherChildNode->getCWnd()->SetDlgCtrlID( id );
		otherChildNode->setParentWnd( mainFrame_ );

		if ( floater )
			floater->setRootNode( otherChildNode );
		else
			dockTreeRoot_ = otherChildNode;
	}

	if ( floater )
	{
		floater->getRootNode()->recalcLayout();
		floater->adjustSize();
		floater->RecalcLayout();
	}
	else
	{
		dockTreeRoot_->recalcLayout();
		dockTreeRoot_->adjustSizeToNode( otherChildNode );
		mainFrame_->RecalcLayout();
	}
}


/**
 *	This method creates and inserts a panel into the dock from the panel's
 *	contentID.
 *
 *	@param contentID	Desired panel's contentID.
 *	@param destPanel	Panel to use for the insertAt operation.
 *	@param insertAt		How to insert the panel.
 *	@return		Newly created and inserted panel.
 */
PanelPtr Dock::insertPanel( const BW::wstring & contentID, PanelHandle destPanel, InsertAt insertAt )
{
	BW_GUARD;

	DockNodePtr childNode = 0;
	DockNodePtr parentNode = 0;

	if ( insertAt == FLOATING )
		destPanel = 0;

	PanelPtr dest = getPanelByHandle( destPanel );

	if ( destPanel )
	{
		if ( !dest )
			destPanel = 0;
		else
			showPanel( dest, true );
	}

	if ( !destPanel )
	{
		if ( insertAt == TAB )
		{
			insertAt = FLOATING;
		}
		else if ( insertAt != FLOATING )
		{
			getNodeByWnd( mainView_, childNode, parentNode );

			if ( !childNode )
				insertAt = FLOATING;
		}
	}

	if ( insertAt == SUBCONTENT && !!destPanel && !!dest )
	{
		ContentContainerPtr cc = (ContentContainer*)destPanel;
		cc->addContent( contentID );

		FloaterPtr floater = getFloaterByWnd( dest->getCWnd() );
		getNodeByWnd( dest->getCWnd(), childNode, parentNode );

		if ( floater )
		{
			floater->getRootNode()->adjustSizeToNode( childNode );
			floater->getRootNode()->recalcLayout();
			floater->RecalcLayout();
			floater->adjustSize( true );
		}
		else
		{
			dockTreeRoot_->adjustSizeToNode( childNode );
			dockTreeRoot_->recalcLayout();
			mainFrame_->RecalcLayout();
		}
		return dest;
	}

	PanelPtr panel = new Panel( mainFrame_ );
	panelList_.push_back( panel );

	panel->addTab( contentID );

	if ( dest )
	{
		insertPanelIntoPanel( panel, dest->getCWnd(), insertAt );

		if ( insertAt == TAB )
			return getPanelByHandle( destPanel );
		else
			return panel;
	}
	else if ( insertAt == FLOATING )
	{
		floatPanel( panel, 0, 0, 300, 200 );
		return panel;
	}

	CWnd* destWnd = 0;

	DockNodePtr node = dockTreeRoot_;

	if ( insertAt == LEFT )
	{
		while( !node->isLeaf() ) node = node->getLeftChild();

		if ( node != childNode )
			insertAt = BOTTOM;

		destWnd = node->getCWnd();
	}
	else if ( insertAt == RIGHT )
	{
		while( !node->isLeaf() ) node = node->getRightChild();

		if ( node != childNode )
			insertAt = TOP;

		destWnd = node->getCWnd();
	}
	else if ( childNode )
	{
		destWnd = childNode->getCWnd();
	}

	dockPanel( panel, destWnd, insertAt, 0, 0, 0, 0 );

	return panel;
}

const PanelItr Dock::removePanel( PanelPtr panel )
{
	BW_GUARD;

	removeNodeByWnd( panel->getCWnd() );

	for( PanelItr i = panelList_.begin(); i != panelList_.end(); ++i )
		if ( (*i) == panel )
			return panelList_.erase( i );

	return panelList_.end();
}


/**
 *	This method removes a panel by its contentID.
 *
 *	@param contentID	Desired panel's contentID.
 */
void Dock::removePanel( const BW::wstring & contentID )
{
	BW_GUARD;

	for( PanelItr i = panelList_.begin(); i != panelList_.end(); )
	{
		(*i)->detachTab( contentID );
		if ( (*i)->tabCount() == 0 ) 
			i = removePanel( *i );
		else
			++i;
	}
}


/**
 *	This method returns a panel by its CWnd.
 *
 *	@param ptr	CWnd of the desired panel.
 *	@return		Panel that matches the CWnd ptr.
 */
PanelPtr Dock::getPanelByWnd( CWnd* ptr )
{
	BW_GUARD;

	for( PanelItr i = panelList_.begin(); i != panelList_.end(); ++i )
	{
		if ( (*i)->getCWnd() == ptr )
		{
			return *i;
		}
	}
	return 0;
}


/**
 *	This method returns a panel by its Content pointer.
 *
 *	@param handle	Content pointer of the desired panel.
 *	@return		Panel that matches the Content ptr.
 */
PanelPtr Dock::getPanelByHandle( PanelHandle handle )
{
	BW_GUARD;

	for( PanelItr i = panelList_.begin(); i != panelList_.end(); ++i )
	{
		if ( (*i)->contains( handle ) )
		{
			return *i;
		}
	}
	return 0;
}


/**
 *	This method creates the appropriate dock node type for the specified data
 *	section.
 *
 *	@param section	Data section containing the node to create.
 *	@return		The appropriate dock node type for the specified data section.
 */
DockNodePtr Dock::nodeFactory( DataSectionPtr section )
{
	BW_GUARD;

	if ( !section )
		return 0;

	if ( !!section->openSection( "MainView" ) )
		return new MainViewNode( mainView_ );
	else if ( !!section->openSection( "DockedPanel" ) )
		return new DockedPanelNode();
	else if ( !!section->openSection( "Splitter" ) )
		return new SplitterNode();

	return false;
}


/**
 *	This method recreates the tree of docked and floating panels from a data
 *	section.
 *
 *	@param section	Data section containing the previously saved dock nodes.
 *	@return		True if sucessfull.
 */
bool Dock::load( DataSectionPtr section )
{
	BW_GUARD;

	if ( !section )
		return false;

	BW::vector<DataSectionPtr> sections;
	section->openSections( "Panel", sections );
	if ( sections.empty() )
		return false;
	for( BW::vector<DataSectionPtr>::iterator i = sections.begin(); i != sections.end(); ++i )
	{
		PanelPtr newPanel = new Panel( mainFrame_ );
		if ( !newPanel->load( *i ) )
			return false;
		panelList_.push_back( newPanel );
	}

	DataSectionPtr treeSec = section->openSection( "Tree" );
	if ( !treeSec )
		return false;

	DockNodePtr node = nodeFactory( treeSec );
	if ( !node )
		return false;

	if ( !node->load( treeSec, mainFrame_, originalMainViewID_ ) )
		return false;

	dockTreeRoot_ = node;
	dockTreeRoot_->recalcLayout();
	mainFrame_->RecalcLayout();

	sections.clear();
	section->openSections( "Floater", sections );
	for( BW::vector<DataSectionPtr>::iterator i = sections.begin(); i != sections.end(); ++i )
	{
		FloaterPtr newFloater = new Floater( mainFrame_ );
		if ( !newFloater->load( *i ) )
			return false;
		floaterList_.push_back( newFloater );
	}

	for( PanelItr i = panelList_.begin(); i != panelList_.end(); ++i )
	{
		savePanelDockPos( *i );
		if ( getFloaterByWnd( (*i)->getCWnd() ) )
			(*i)->setFloating( true );
	}

	showDock( section->readBool( "visible", dockVisible_ ) );
	return true;
}


/**
 *	This method saves the layout tree of docked and floating panels to a data
 *	section.
 *
 *	@param section	Data section where to save the dock node layout tree.
 *	@return		True if sucessfull.
 */
bool Dock::save( DataSectionPtr section )
{
	BW_GUARD;

	if ( !section )
		return false;

	section->writeBool( "visible", dockVisible_ );

	section->deleteSections( "Panel" );
	section->deleteSections( "Tree" );
	section->deleteSections( "Floater" );

	for( PanelItr i = panelList_.begin(); i != panelList_.end(); ++i )
		if ( !(*i)->save( section->newSection( "Panel" ) ) )
			return false;

	if ( !dockTreeRoot_->save( section->newSection( "Tree" ) ) )
		return false;

	for( FloaterItr i = floaterList_.begin(); i != floaterList_.end(); ++i )
		if ( !(*i)->save( section->newSection( "Floater" ) ) )
			return false;

	return true;
}


/**
 *	This method sets the current active panel.
 *
 *	@param panel	The new current active panel.
 */
void Dock::setActivePanel( PanelPtr panel )
{
	BW_GUARD;

	for( PanelItr i = panelList_.begin(); i != panelList_.end(); ++i )
	{
		if ( panel != (*i) )
		{
			(*i)->deactivate();
		}
	}
}


/**
 *	This method shows or hides a panel by it's Panel pointer..
 *
 *	@param panel	Panel to show or hide.
 *	@param show		True to show the panel, false to hide it.
 */
void Dock::showPanel( PanelPtr panel, bool show )
{
	BW_GUARD;

	if ( show == false )
	{
		removeNodeByWnd( panel->getCWnd() );
	}
	else
	{
		restorePanelDockPos( panel );
		panel->activate();
	}
}


/**
 *	This method shows or hides a panel by its Content pointer.
 *
 *	@param content	Content pointer of the panel to show or hide.
 *	@param show		True to show the panel, false to hide it.
 */
void Dock::showPanel( ContentPtr content, bool show )
{
	BW_GUARD;

	for( PanelItr i = panelList_.begin(); i != panelList_.end(); ++i )
	{
		if ( (*i)->contains( content ) )
		{
			if ( show )
				showPanel( *i, show );
			(*i)->showTab( content, show );
			if ( show )
				(*i)->activate();
			break;
		}
	}
}


/**
 *	This method shows or hides a panel by its contentID.
 *
 *	@param contentID	Content id of the panel to show or hide.
 *	@param show		True to show the panel, false to hide it.
 */
void Dock::showPanel( const BW::wstring & contentID, bool show )
{
	BW_GUARD;

	for( PanelItr i = panelList_.begin(); i != panelList_.end(); ++i )
	{
		if ( (*i)->contains( contentID ) )
		{
			if ( show )
				showPanel( *i, show );
			(*i)->showTab( contentID, show );
			if ( show )
				(*i)->activate();
		}
	}
}


/**
 *	This method returns the Content object corresponding to the specified
 *	contentID.
 *
 *	@param contentID	Desired Content's contentID.
 *	@param index	If multiple contents of this type exist, get the occurrence
 *					number "index".
 *	@return		The Content object corresponding to the specified contentID.
 */
ContentPtr Dock::getContent( const BW::wstring & contentID, int index )
{
	BW_GUARD;

	for( PanelItr i = panelList_.begin(); i != panelList_.end(); ++i )
		if ( (*i)->contains( contentID ) )
		{
			ContentPtr content = (*i)->getContent( contentID, index );
			if ( content )
				return content;
		}

	return 0;
}


/**
 *	This method returns whether or not a content is visible.
 *
 *	@param contentID	Desired Content's contentID.
 *	@return		True if the content is visible, false if not.
 */
bool Dock::isContentVisible( const BW::wstring & contentID )
{
	BW_GUARD;

	for( PanelItr i = panelList_.begin(); i != panelList_.end(); ++i )
	{
		if ( (*i)->contains( contentID ) )
		{
			DockNodePtr node;
			DockNodePtr parent;
			getNodeByWnd( (*i)->getCWnd(), node, parent );

			if ( node )
			{
				if ( (*i)->isFloating() )
				{
					FloaterPtr floater = getFloaterByWnd( (*i)->getCWnd() );

					if ( !floater || !floater->IsWindowVisible() )
						return false;
				}

				if ( (*i)->isTabVisible( contentID ) )
					return true;
			}
		}
	}
	return false;
}


/**
 *	This method returns the leaf nodes under a node's subtree, or the same node
 *	if the node is itself a leaf.
 *
 *	@param node		Node to start looking for leaves from.
 *	@param leaves	Return param, list of leaves in the subtree.
 */
void Dock::getLeaves( DockNodePtr node, BW::vector<DockNodePtr>& leaves )
{
	BW_GUARD;

	if ( node->isLeaf() )
	{
		leaves.push_back( node );
		return;
	}

	getLeaves( node->getLeftChild(), leaves );
	getLeaves( node->getRightChild(), leaves );
}


/**
 *	This method stores information about the position and layout of panels for
 *	later use.  This is useful when hidding and showing panels, keeping track
 *	of where the panel was before hiding so when showing it, it shows up at the
 *	same position it was.
 *
 *	@param docked	Whether the panel is docked or floating.
 *	@param node		Recursion param, current node being traversed.
 *	@param panel	Panel object we are storing the position info for.
 *	@return		True if the panel is found (should be), false if not.
 */
bool Dock::buildPanelPosList( bool docked, DockNodePtr node, PanelPtr panel )
{
	BW_GUARD;

	if ( node->isLeaf() )
	{
		if ( node->getCWnd() == panel->getCWnd() )
			return true;
		else 
			return false;
	}

	bool inLeft = buildPanelPosList( docked, node->getLeftChild(), panel );
	bool inRight = buildPanelPosList( docked, node->getRightChild(), panel );

	if ( inLeft )
	{
		BW::vector<DockNodePtr> leaves;
		getLeaves( node, leaves );

		for( BW::vector<DockNodePtr>::iterator i = leaves.begin(); i != leaves.end(); ++i )
		{
			InsertAt ins;

			if ( node->getSplitOrientation() == HORIZONTAL )
				ins = LEFT;
			else
				ins = TOP;

			panel->insertPos( docked, PanelPos( ins, (*i)->getCWnd() ) );
		}
	}
	else if ( inRight )
	{
		BW::vector<DockNodePtr> leaves;
		getLeaves( node, leaves );

		for( BW::vector<DockNodePtr>::reverse_iterator i = leaves.rbegin(); i != leaves.rend(); ++i )
		{
			InsertAt ins;

			if ( node->getSplitOrientation() == HORIZONTAL )
				ins = RIGHT;
			else
				ins = BOTTOM;

			panel->insertPos( docked, PanelPos( ins, (*i)->getCWnd() ) );
		}
	}

	return inLeft || inRight;
}


/**
 *	This method stores information about the position and layout of panels for
 *	later use.  This is useful when hidding and showing panels, keeping track
 *	of where the panel was before hiding so when showing it, it shows up at the
 *	same position it was.
 *
 *	@param panel	Panel object we are storing the position info for.
 */
void Dock::savePanelDockPos( PanelPtr panel )
{
	BW_GUARD;

	FloaterPtr floater = getFloaterByWnd( panel->getCWnd() );
	if ( floater )
	{
		panel->clearPosList( false );
		buildPanelPosList( false, floater->getRootNode(), panel );
	}
	else
	{
		panel->clearPosList( true );
		buildPanelPosList( true, dockTreeRoot_, panel );
	}
}


/**
 *	This method restores a panel's into its previous dock insertion from the
 *	position information previously saved in savePanelDockPos.
 *
 *	@param panel	Panel object we are storing the position info for.
 */
void Dock::restorePanelDockPos( PanelPtr panel )
{
	BW_GUARD;

	DockNodePtr node;
	DockNodePtr parent;
	getNodeByWnd( panel->getCWnd(), node, parent );

	if ( node )
		return;	// already visible

	bool docked;

	docked = !panel->isFloating();

	panel->resetPosList( docked );

	PanelPos pos;

	while( panel->getNextPos( docked, pos ) )
	{
		if ( panel->isFloating() )
		{
			for( FloaterItr i = floaterList_.begin(); i != floaterList_.end(); ++i )
			{
				DockNodePtr node;
				DockNodePtr parent;
				(*i)->getRootNode()->getNodeByWnd( pos.destPanel, node, parent );
				if ( node )
				{
					insertPanelIntoPanel( panel, pos.destPanel, pos.insertAt );
					(*i)->ShowWindow( SW_SHOW );
					return;
				}
			}
		}
		else
		{
			DockNodePtr node;
			DockNodePtr parent;
			dockTreeRoot_->getNodeByWnd( pos.destPanel, node, parent );
			if ( node )
			{
				insertPanelIntoPanel( panel, pos.destPanel, pos.insertAt );
				return;
			}
		}
	}
	
	if ( panel->isFloating() )
	{
		int x;
		int y;
		panel->getLastPos( x, y );
		floatPanel( panel, 0, 0, x, y );
	}
	else
		insertPanelIntoPanel( panel, mainView_, RIGHT );

}


/**
 *	This method toggles a panel between docked and floating.
 *
 *	@param panel	Panel object whose position we are toggling.
 */
void Dock::togglePanelPos( PanelPtr panel )
{
	BW_GUARD;

	removeNodeByWnd( panel->getCWnd() );
	
	if ( !panel->isFloating() )
	{
		int w;
		int h;
		panel->getPreferredSize( w, h );
		panel->SetWindowPos( 0, 0, 0, w, h, SWP_NOZORDER );
	}

	panel->setFloating( !panel->isFloating() );
	restorePanelDockPos( panel );
}


/**
 *	This method toggles a tab between docked and floating.
 *
 *	@param panel	Panel object containing the specified tab.
 *	@param tab		Tab object whose position we are toggling.
 */
void Dock::toggleTabPos( PanelPtr panel, TabPtr tab )
{
	BW_GUARD;

	// create a new panel from the tab
	PanelPtr newPanel = detachTabToPanel( panel, tab );

	// call restore to put this panel in the previous state of the original panel
	newPanel->setFloating( !panel->isFloating() );
	int w;
	int h;
	newPanel->getPreferredSize( w, h );
	newPanel->SetWindowPos( 0, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER );
	restorePanelDockPos( newPanel );
}


/**
 *	This method destroys a floater window.
 *
 *	@param floater	Floating window to be destroyed.
 */
void Dock::destroyFloater( FloaterPtr floater )
{
	BW_GUARD;

	// remove all panels from floater
	if ( IsWindow( floater->GetSafeHwnd() ) )
	{
		// still a window, remove the rest of the panels
		CRect rect;
		floater->GetWindowRect( &rect );

		for( PanelItr i = panelList_.begin(); i != panelList_.end(); ++i )
		{
			DockNodePtr node;
			DockNodePtr parent;
			floater->getRootNode()->getNodeByWnd( (*i)->getCWnd(), node, parent );
			if ( node )
			{
				(*i)->setLastPos( rect.left, rect.top );
				removeNodeByWnd( node->getCWnd() );
			}
		}
	}

	// remove all panels from floater
	for( FloaterItr i = floaterList_.begin(); i != floaterList_.end(); ++i )
	{
		if ( (*i) == floater )
		{
			floaterList_.erase( i );
			break;
		}
	}
}


/**
 *	This method forwards a windows message to all the nodes in the dock.
 *
 *	@param msg	Windows message.
 *	@param wParam	Windows message WPARAM.
 *	@param lParam	Windows message LPARAM.
 */
void Dock::broadcastMessage( UINT msg, WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	for( PanelItr i = panelList_.begin(); i != panelList_.end(); ++i )
		(*i)->broadcastMessage( msg, wParam, lParam );
}


/**
 *	This method forwards a windows message to a particular kind of panel
 *	wherever it is in the dock.
 *
 *	@param contentID	Content id of the desired panel.
 *	@param msg	Windows message.
 *	@param wParam	Windows message WPARAM.
 *	@param lParam	Windows message LPARAM.
 */
void Dock::sendMessage( const BW::wstring & contentID, UINT msg, WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	int index = 0;
	while( true )
	{
		ContentPtr content = getContent( contentID, index++ );

		if (!content)
		{
			break;
		}

		CWnd* window = content->getCWnd();

		if (window->GetSafeHwnd())
		{
			window->SendMessage( msg, wParam, lParam );
		}
	}
}


/**
 *	This method returns the number of instances of the content type contentID
 *	in the dock.
 *
 *	@param contentID	Content id of the desired panel.
 *	@return		Number of contentID panels in the dock.
 */
int Dock::getContentCount( const BW::wstring & contentID )
{
	BW_GUARD;

	int cnt = 0;

	for( PanelItr i = panelList_.begin(); i != panelList_.end(); ++i )
		cnt += (*i)->contains( contentID );

	return cnt;
}


/**
 *	This method detaches a tab from a panel and puts it into its own new panel.
 *
 *	@param panel	Panel where the tab currently is.
 *	@param tab		Tab we want to detach.
 *	@return		New panel created to contain the detached tab.
 */
PanelPtr Dock::detachTabToPanel( PanelPtr panel, TabPtr tab )
{
	BW_GUARD;

	CRect rect;
	panel->GetWindowRect( &rect );

	int w;
	int h;
	panel->getPreferredSize( w, h );

	panel->detachTab( tab );

	PanelPtr newPanel = new Panel( mainFrame_ );
	panelList_.push_back( newPanel );

	newPanel->addTab( tab );

	if ( panel->isFloating() )
	{
		newPanel->SetWindowPos( 0, rect.left, rect.top, 0, 0, SWP_NOSIZE|SWP_NOZORDER );
		newPanel->setLastPos( rect.left, rect.top );
	}
	else
	{
		newPanel->SetWindowPos( 0, 0, 0, w, h, SWP_NOZORDER );
	}

	copyPanelRestorePosToTab( panel, newPanel );

	return newPanel;
}


/**
 *	This method copies the restore position from one panel to another.
 *
 *	@param src	Panel containing the restore position info.
 *	@param dstTab	Panel that will receive the other panels position info.
 */
void Dock::copyPanelRestorePosToTab( PanelPtr src, PanelPtr dstTab )
{
	BW_GUARD;

	// copy panel's restore positions
	PanelPos pos;
	
	dstTab->clearPosList( false );
	src->resetPosList( false );
	if ( src->isFloating() )
		dstTab->insertPos( false, PanelPos( TAB, src->getCWnd() ) );
	while ( src->getNextPos( false, pos ) )
		dstTab->insertPos( false, pos );

	dstTab->clearPosList( true );
	src->resetPosList( true );
	if ( !src->isFloating() )
		dstTab->insertPos( true, PanelPos( TAB, src->getCWnd() ) );
	while ( src->getNextPos( true, pos ) )
		dstTab->insertPos( true, pos );
}


/**
 *	This method rolls up a panel (leaving it as high as its caption bar).
 *
 *	@param panel	Panel to roll up.
 */
void Dock::rollupPanel( PanelPtr panel )
{
	BW_GUARD;

	DockNodePtr node;
	DockNodePtr parent;

	getNodeByWnd( panel->getCWnd(), node, parent );

	if ( !node )
		return;

	FloaterPtr floater = getFloaterByWnd( panel->getCWnd() );

	if ( floater )
	{
		floater->getRootNode()->adjustSizeToNode( node );
		floater->getRootNode()->recalcLayout();
		floater->RecalcLayout();
		floater->adjustSize( true );
	}
	else
	{
		dockTreeRoot_->adjustSizeToNode( node );
		dockTreeRoot_->recalcLayout();
		mainFrame_->RecalcLayout();
	}
}


/**
 *	This method returns the index of a panel in the internal panel list.
 *
 *	@param panel	Panel we want its index.
 *	@return		Index of the panel on the list.
 */
int Dock::getPanelIndex( PanelPtr panel )
{	
	int idx = 0;
	for( PanelItr i = panelList_.begin(); i != panelList_.end(); ++i, ++idx )
		if ( (*i) == panel )
			return idx;

	return -1;
}


/**
 *	This method returns a panel by its index in the internal panel list.
 *
 *	@param index	Index of the desired panel.
 *	@return		Panel at the position specified by "index".
 */
PanelPtr Dock::getPanelByIndex( int index )
{
	BW_GUARD;

	int idx = 0;
	for( PanelItr i = panelList_.begin(); i != panelList_.end(); ++i, ++idx )
		if ( index == idx )
			return (*i);

	return 0;
}


} // namespace
BW_END_NAMESPACE

