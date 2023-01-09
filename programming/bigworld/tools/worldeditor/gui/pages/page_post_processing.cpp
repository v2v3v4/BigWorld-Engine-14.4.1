
#include "pch.hpp"
#include "gui/pages/page_post_processing.hpp"
#include "gui/post_processing/view_skin.hpp"
#include "gui/post_processing/effect_node_view.hpp"
#include "gui/post_processing/phase_node_view.hpp"
#include "gui/post_processing/effect_edge.hpp"
#include "gui/post_processing/effect_edge_view.hpp"
#include "gui/post_processing/phase_edge.hpp"
#include "gui/post_processing/phase_edge_view.hpp"
#include "gui/post_processing/chain_editors.hpp"
#include "gui/post_processing/post_proc_undo.hpp"
#include "gui/post_processing/post_proc_property_editor.hpp"
#include "guitabs/nice_splitter_wnd.hpp"
#include "post_processing/effect.hpp"
#include "post_processing/phase.hpp"
#include "post_processing/manager.hpp"
#include "appmgr/options.hpp"
#include "common/user_messages.hpp"
#include "graph/graph.hpp"
#include "graph/node.hpp"
#include "graph/edge.hpp"
#include "ual/ual_manager.hpp"

DECLARE_DEBUG_COMPONENT2( "WorldEditor", 0 )

BW_BEGIN_NAMESPACE

// Including the Asset Browser self-registering providers so they get linked
// in.
extern int UalEffectProv_token;
extern int UalPhaseProv_token;
static int s_ualPostProcessingProviders = UalEffectProv_token | UalPhaseProv_token;


namespace
{
	// Some Graph View drawing constants.
	const int ELEMENT_PADDING = 7;

	const int DROP_AUTOSCROLL_EDGE = 10;

	const int DROP_AUTOSCROLL_STEP = 20;

	const int MIN_ZOOM_STEP = -4;
	const int MAX_ZOOM_STEP = 40;

	const int BUTTON_ZOOM_START = 2;
	const float BUTTON_ZOOM_FACTOR = 1.5f;

	const int SCROLLBAR_SIZE = 13;

	// Constants for the internal sub panel layout.
	const int DEFAULT_SPLITTER_WIDTH = 210;
	const int DEFAULT_SPLITTER_HEIGHT = 180;
	const int MIN_GRAPH_SIZE = 60;
	const int MIN_CHAINS_SIZE = 10;

	// Drag and drop popup constants.
	const int PHASE_POPUP_ALPHA = 200;

	#define DROP_ON_EDGE_OFFSET( rect, dir ) (rect.##dir##() * 2 / 3)


	/**
	 *	This function returns whether or not a file corresponds to a texture.
	 *
	 *	@param filePath	File path we want to test.
	 *	@return	True if the file is a texture, false if not.
	 */
	bool isTexture( const BW::wstring & filePath )
	{
		BW_GUARD;

		BW::wstring ext = BWResource::getExtensionW( filePath );
		return	ext == L"tga" ||
				ext == L"bmp" ||
				ext == L"jpg" ||
				ext == L"png" ||
				ext == L"dds" ||
				ext == L"texanim";
	}


	/**
	 *	This utility function tests if a string starts with a prefix.
	 *
	 *	@param str	String to test.
	 *	@param prefix	Prefix to test.
	 *	@return	True if str starts with prefix, false if not.
	 */
	bool startsWith( const BW::wstring & str, const BW::wstring & prefix )
	{
		size_t len = prefix.length();
		return str.length() >= len && str.substr( 0, len ) == prefix;
	}

} // anonymous namespace


///////////////////////////////////////////////////////////////////////////////
// Section: GraphContainer
///////////////////////////////////////////////////////////////////////////////

/**
 *	Constructor.
 */
PagePostProcessing::GraphContainer::GraphContainer() :
	ignoreScrolling_( false ),
	graphView_( NULL ),
	captionBar_( NULL ),
	hScroll_( NULL ),
	vScroll_( NULL ),
	captionBarHeight_( 0 )
{
}


/**
 *	This method initialises the container with all its elements, the graph
 *	view, the caption bar, the horizontal scroll bar and the vertical scroll
 *	bar.
 *
 *	@param graphView	Graph View that displays the nodes.
 *	@param captionBar	Small caption bar for the graph view.
 *	@param hScroll		Horizontal scroll bar.
 *	@param vScroll		Vertical scroll bar.
 */
void PagePostProcessing::GraphContainer::setChildren( Graph::GraphView * graphView, CWnd * captionBar, CScrollBar * hScroll, CScrollBar * vScroll )
{
	BW_GUARD;

	graphView_ = graphView;
	captionBar_ = captionBar;
	hScroll_ = hScroll;
	vScroll_ = vScroll;

	if (captionBar_)
	{
		CRect rect;
		captionBar_->GetWindowRect( rect );
		captionBarHeight_ = rect.Height();
	}
}


// MFC message map
BEGIN_MESSAGE_MAP( PagePostProcessing::GraphContainer, CWnd )
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()


/**
 *	This MFC method is overriden to rearrange all the elements in their correct
 *	positions.
 *
 *	@param nType	MFC resize type.
 *	@param cx	MFC new width.
 *	@param cy	MFC new height.
 */
void PagePostProcessing::GraphContainer::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	CWnd::OnSize( nType, cx, cy );

	if (graphView_)
	{
		graphView_->SetWindowPos( NULL, 0, captionBarHeight_,
			cx - SCROLLBAR_SIZE, cy - SCROLLBAR_SIZE - captionBarHeight_,
			SWP_NOZORDER );
	}

	if (captionBar_)
	{
		captionBar_->SetWindowPos( 0, 0, 0, cx, captionBarHeight_, SWP_NOZORDER );
	}

	if (hScroll_)
	{
		hScroll_->SetWindowPos( NULL, 0, cy - SCROLLBAR_SIZE,
							cx - SCROLLBAR_SIZE, SCROLLBAR_SIZE, SWP_NOZORDER );
	}

	if (vScroll_)
	{
		vScroll_->SetWindowPos( NULL, cx - SCROLLBAR_SIZE, captionBarHeight_,
			SCROLLBAR_SIZE, cy - SCROLLBAR_SIZE - captionBarHeight_, SWP_NOZORDER );
	}
}


/**
 *	This MFC method is called when the horizontal scroll bar is used.
 *
 *	@param nSBCode	MFC	scroll event type.
 *	@param nPos		MFC new scroll bar position.
 *	@param pScrollBar	MFC scroll bar being moved.
 */
void PagePostProcessing::GraphContainer::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar * pScrollBar )
{
	BW_GUARD;

	CWnd::OnHScroll( nSBCode, nPos, pScrollBar );

	if (!ignoreScrolling_)
	{
		int lineScroll = ViewSkin::phaseNodeSize().cx / 4;
		int oldPos = hScroll_->GetScrollPos();
		if (nSBCode == SB_THUMBTRACK  || nSBCode == SB_THUMBPOSITION)
		{
			graphView_->pan( CPoint( -((int)nPos - oldPos), 0 ) );
		}
		else if (nSBCode == SB_LINELEFT)
		{
			nPos = oldPos - lineScroll;
			graphView_->pan( CPoint( -((int)nPos - oldPos), 0 ) );
		}
		else if (nSBCode == SB_LINERIGHT)
		{
			nPos = oldPos + lineScroll;
			graphView_->pan( CPoint( -((int)nPos - oldPos), 0 ) );
		}
		else if (nSBCode == SB_PAGELEFT)
		{
			nPos = oldPos - lineScroll * 5;
			graphView_->pan( CPoint( -((int)nPos - oldPos), 0 ) );
		}
		else if (nSBCode == SB_PAGERIGHT)
		{
			nPos = oldPos + lineScroll * 5;
			graphView_->pan( CPoint( -((int)nPos - oldPos), 0 ) );
		}
		hScroll_->SetScrollPos( nPos );
	}
}


/**
 *	This MFC method is called when the vertical scroll bar is used.
 *
 *	@param nSBCode	MFC	scroll event type.
 *	@param nPos		MFC new scroll bar position.
 *	@param pScrollBar	MFC scroll bar being moved.
 */
void PagePostProcessing::GraphContainer::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar * pScrollBar )
{
	BW_GUARD;

	CWnd::OnVScroll( nSBCode, nPos, pScrollBar );

	if (!ignoreScrolling_)
	{
		int lineScroll = ViewSkin::phaseNodeSize().cy / 4;
		int oldPos = vScroll_->GetScrollPos();
		if (nSBCode == SB_THUMBTRACK  || nSBCode == SB_THUMBPOSITION)
		{
			graphView_->pan( CPoint( 0, -((int)nPos - oldPos) ) );
		}
		else if (nSBCode == SB_LINELEFT)
		{
			nPos = oldPos - lineScroll;
			graphView_->pan( CPoint( 0, -((int)nPos - oldPos) ) );
		}
		else if (nSBCode == SB_LINERIGHT)
		{
			nPos = oldPos + lineScroll;
			graphView_->pan( CPoint( 0, -((int)nPos - oldPos) ) );
		}
		else if (nSBCode == SB_PAGELEFT)
		{
			nPos = oldPos - lineScroll * 5;
			graphView_->pan( CPoint( 0, -((int)nPos - oldPos) ) );
		}
		else if (nSBCode == SB_PAGERIGHT)
		{
			nPos = oldPos + lineScroll * 5;
			graphView_->pan( CPoint( 0, -((int)nPos - oldPos) ) );
		}
		vScroll_->SetScrollPos( nPos );
	}
}



///////////////////////////////////////////////////////////////////////////////
// Section: PagePostProcessing
///////////////////////////////////////////////////////////////////////////////


// GUITABS content ID ( declared by the IMPLEMENT_BASIC_CONTENT macro )
const BW::wstring PagePostProcessing::contentID = L"PagePostProcessing";


// Static flags raised by a phase's properties when the user changes them.
/*static*/ bool PagePostProcessing::s_phaseChanged_ = false;
/*static*/ bool PagePostProcessing::s_phaseChangedReload_ = false;


/**
 *	Constructor.
 */
PagePostProcessing::PagePostProcessing() :
	CDialog( PagePostProcessing::IDD ),
	pChainsSplitter_( NULL ),
	pGraphSplitter_( NULL ),
	layout_( NORMAL ),
	chainsSplitterSize_( 0 ),
	graphSplitterSize_( 0 ),
	updateGraph_( true ),
	curZoom_( 0 ),
	zoomToCentre_( false ),
	zoomToTopRight_( false ),
	graphWidth_( 0 ),
	graphHeight_( 0 ),
	renderTargetMBs_( 0.0 ),
	lastGPUTime_( 0.0 ),
	lastDropTargetPhase_( NULL ),
	dragging_( false ),
	cloning_( false ),
	dragNodeOldPos_( 0, 0 )
{
	BW_GUARD;

	preview_ = SmartPointer<PPPreview>( new PPPreview, PyObjectPtr::STEAL_REFERENCE );
	PostProcessing::Manager::instance().debug( preview_ );
}


/**
 *	Destructor.
 */
PagePostProcessing::~PagePostProcessing()
{
	BW_GUARD;

	bw_safe_delete(pChainsSplitter_);
	bw_safe_delete(pGraphSplitter_);
	PostProcessing::Manager::instance().debug( NULL );
	preview_ = NULL;
}


/**
 *	This method loads the panel from the layout xml file.
 *
 *	@param section	Data section containing the panel's internal layout.
 *	@return True if successful.
 */
bool PagePostProcessing::load( DataSectionPtr section )
{
	BW_GUARD;

	BW::string layoutStr = section->readString( "layout", "NORMAL" );
	if (layoutStr == "NORMAL")
	{
		layout_ = NORMAL;
	}
	else if (layoutStr == "WIDE")
	{
		layout_ = WIDE;
	}
	else if (layoutStr == "TALL")
	{
		layout_ = TALL;
	}
	else
	{
		layout_ = NORMAL;
		WARNING_MSG( "PagePostProcessing::load: Unrecognised layout.\n" );
	}

	chainsSplitterSize_ = section->readInt( "chainSplitterSize", 0 );
	graphSplitterSize_ = section->readInt( "graphSplitterSize", 0 );

	CRect rect;
	GetClientRect( rect );
	setLayout( layout_, rect.Width(), rect.Height(), false /* dont clear sizes*/ );

	return true;
}


/**
 *	This method saves the panel to the layout xml file.
 *
 *	@param section	Data section to save the panel's internal layout to.
 *	@return True if successful.
 */
bool PagePostProcessing::save( DataSectionPtr section )
{
	BW_GUARD;

	// This is called on exit. Check if we need to save the chain.
	chainsDlg_.onExit();

	BW::string layoutStr;
	if (layout_ == NORMAL)
	{
		layoutStr = "NORMAL";
	}
	else if (layout_ == WIDE)
	{
		layoutStr = "WIDE";
	}
	else // layout_ == TALL
	{
		layoutStr = "TALL";
	}
	section->writeString( "layout", layoutStr );

	if (pChainsSplitter_->GetSafeHwnd() &&
		pGraphSplitter_->GetSafeHwnd())
	{
		int min;

		if (layout_ == WIDE)
		{
			pGraphSplitter_->GetColumnInfo( 0, graphSplitterSize_, min );
			pChainsSplitter_->GetColumnInfo( 0, chainsSplitterSize_, min );
		}
		else if (layout_ == TALL)
		{
			pGraphSplitter_->GetRowInfo( 0, graphSplitterSize_, min );
			pChainsSplitter_->GetRowInfo( 0, chainsSplitterSize_, min );
		}
		else // layout_ == NORMAL
		{
			pChainsSplitter_->GetRowInfo( 0, chainsSplitterSize_, min );
			pGraphSplitter_->GetColumnInfo( 0, graphSplitterSize_, min );
		}
	}

	section->writeInt( "chainSplitterSize", chainsSplitterSize_ );
	section->writeInt( "graphSplitterSize", graphSplitterSize_ );
	return true;
}


/**
 *	This method gets called from a graph node when it's clicked.
 *
 *	@param node	Node just clicked.
 */
void PagePostProcessing::nodeClicked( const BasePostProcessingNodePtr & node )
{
	BW_GUARD;

	currentNode_ = node;
	propertiesDlg_.editNode( node );
	if (!dragging_)
	{
		graphView_.RedrawWindow();
	}
}


/**
 *	This method gets called from a graph node when its active state changes.
 *
 *	@param node	Node whose active status just changed.
 *	@param active	New active state of the node.
 */
void PagePostProcessing::nodeActive( const BasePostProcessingNodePtr & node, bool active )
{
	BW_GUARD;

	PhaseNodeSet phases;

	if (getPhaseNodes( node, phases ))
	{
		// It's an EffectNode, so propagate activating/deactivating to its phases.
		Graph::GraphPtr graph = graphView_.graph();
		
		for (PhaseNodeSet::iterator it = phases.begin();
			it != phases.end(); ++it)
		{
			(*it)->active( active );
		}
	}
}


/**
 *	This method gets called from a graph node when it's delete.
 *
 *	@param node	Node just deleted.
 */
void PagePostProcessing::nodeDeleted( const BasePostProcessingNodePtr & node )
{
	BW_GUARD;

	EffectNodePtr effectNode = node->effectNode();
	PhaseNodePtr phaseNode = node->phaseNode();

	BW::wstring text = effectNode ?
		Localise( L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/DELETE_EFFECT_TEXT" ) :
		Localise( L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/DELETE_PHASE_TEXT" );
	BW::wstring title = effectNode ?
		Localise( L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/DELETE_EFFECT_CAPTION" ) :
		Localise( L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/DELETE_PHASE_CAPTION" );

	if (MessageBox( text.c_str(), title.c_str(), MB_ICONWARNING | MB_YESNO ) == IDYES)
	{
		if (currentNode_ == node)
		{
			currentNode_ = NULL;
			propertiesDlg_.editNode( NULL );
		}

		if (effectNode)
		{
			editEffectChain( new DeleteEffectEditor( effectNode ) );
		}
		else if (phaseNode)
		{
			// Find parent effect node for the phase, and delete the phase from it.
			EffectNodePtr phaseEffectNode = getPhaseEffect( phaseNode );
			if (phaseEffectNode)
			{
				editEffectPhases( phaseEffectNode, new DeletePhaseEditor( phaseNode ) );
			}
		}
	}
}


/**
 *	This method gets called from a graph node when it's being dragged around.
 *
 *	@param pt	Mouse position.
 *	@param node	Node being dragged.
 */
void PagePostProcessing::doDrag( const CPoint & pt, const BasePostProcessingNodePtr & node )
{
	BW_GUARD;

	Graph::NodeViewPtr nodeView = graphView_.nodeView( node.get() );
	if (nodeView)
	{
		bool showPhasePopup = false;
		draggingRT_ = false;

		PhaseNodePtr srcPhase = node->phaseNode();
		PhaseNodeView * pPhaseView = dynamic_cast<PhaseNodeView*>( nodeView.get() );
		if (srcPhase && pPhaseView && pPhaseView->isRenderTargetDragging())
		{
			// Dragging a phase's render target
			draggingRT_ = true;

			CPoint graphViewPt = pt;
			graphViewPt.Offset( graphView_.panOffset() ); // pt doesn't include the offset, so add it.

			CPoint mousePt = graphViewPt;
			graphView_.ClientToScreen( &mousePt );

			CDC * pDC = beginDrawDrag();
			pPhaseView->drawRenderTargetName( *pDC, mousePt );
			endDrawDrag( pDC );

			CRect popupRect;
			phasePopup_.GetWindowRect( popupRect );
			bool mouseOnPhasePopup = (phasePopup_.GetSafeHwnd() != NULL && popupRect.PtInRect( mousePt ));

			PhaseNodePtr phase;

			if (mouseOnPhasePopup)
			{	
				phase = lastDropTargetPhase_;
			}

			if (!phase)
			{
				phase = getPhaseByPt( graphViewPt );
			}

			if (phase && phase != srcPhase)
			{
				BW::wstring rtName = L"RT:" + bw_utf8tow( srcPhase->output() );
				showPhasePopup = showHidePhasePopup( rtName, phase );
			}

			lastDropTargetPhase_ = phase.get();
		}
		else
		{
			// Dragging a node (phase or effect).
			CRect rect = nodeView->rect();

			if (!dragging_)
			{
				dragNodeOldPos_ = rect.TopLeft();
				Graph::EdgesSet adjEdges;
				graphView_.graph()->edges( node.get(), adjEdges );
				graphView_.graph()->backEdges( node.get(), adjEdges );
				
				dragNodeEdges_.clear();
				for (Graph::EdgesSet::iterator it = adjEdges.begin();
					it != adjEdges.end(); ++it)
				{
					dragNodeEdges_.insert( std::make_pair( (*it).get(), graphView_.edgeView( *it ) ) );
					graphView_.unregisterEdgeView( (*it).get() );
				}

				dragging_ = true;
				cloning_ = false;
			}

			CPoint graphViewPt = pt;
			graphViewPt.Offset( graphView_.panOffset() ); // pt doesn't include the offset, so add it.

			CRect dropRect;

			if (graphView_.cloning() && !cloning_)
			{
				// started cloning, add temp node view
				cloning_ = true;
				createExtraNodesForCloning( node );
			}
			else if (!graphView_.cloning() && cloning_)
			{
				// stoped cloning, remove temp node view
				cloning_ = false;
				removeExtraNodesForCloning( node );
			}

			if (node->effectNode())
			{
				nodeView->position( CPoint( pt.x - rect.Width() / 2, rect.top ) );

				PhaseNodeSet phasesSet;
				getPhaseNodes( node, phasesSet );
				for(PhaseNodeSet::iterator it = phasesSet.begin(); it != phasesSet.end(); ++it)
				{
					Graph::NodeViewPtr phaseView = graphView_.nodeView( (*it).get() );
					CRect rect = phaseView->rect();
					phaseView->position( CPoint( pt.x - rect.Width() / 2, rect.top ) );
					(*it)->dragging( true );
				}

				// Check drop target for effects.
				dropBeforeEffect_ = NULL;
				dropRect = findEffectDropPoint( graphViewPt, node );
			}
			else if (node->phaseNode())
			{
				nodeView->position( CPoint( pt.x - rect.Width() / 2, pt.y - rect.Height() / 2 ) );

				// Check drop target for phases.
				dropBeforeEffect_ = NULL;
				dropBeforePhase_ = NULL;
				dropRect = findPhaseDropPoint( graphViewPt, node );
			}
			else
			{
				ERROR_MSG( "Dragging unknown node.\n" );
			}

			// draw the drop rect, if any
			if (!dropRect.IsRectEmpty())
			{
				// draw a drag rect.
				graphView_.ClientToScreen( dropRect );
				drawDragRect( dropRect );
			}
		}

		// pan, if needed
		CPoint scrnPt;
		GetCursorPos( &scrnPt );
		graphView_.ScreenToClient( &scrnPt );
		if (graphViewAutoPan( scrnPt ))
		{
			lastDropTargetPhase_ = NULL;
		}

		if (showPhasePopup)
		{
			phasePopup_.update( PHASE_POPUP_ALPHA );
		}
		else
		{
			phasePopup_.close();
		}
	}
}


/**
 *	This method gets called from a graph node when the user has stopped
 *	dragging it.
 *
 *	@param pt	Mouse position.
 *	@param node	Node being dragged.
 *	@param canceled	True if the drag and drop was canceled, false if it was
 *					ended normally and successfully.
 */
void PagePostProcessing::endDrag( const CPoint & pt, const BasePostProcessingNodePtr & node, bool canceled /*= false*/ )
{
	BW_GUARD;

	if (dragging_)
	{
		removeExtraNodesForCloning( node );

		Graph::NodeViewPtr nodeView = graphView_.nodeView( node.get() );
		
		if (nodeView)
		{
			nodeView->position( dragNodeOldPos_ );

			for (DragEdgesMap::iterator it = dragNodeEdges_.begin();
				it != dragNodeEdges_.end(); ++it)
			{
				graphView_.registerEdgeView( (*it).first, (*it).second );
			}

			bool needsRedraw = true;

			EffectNodePtr effectNode = node->effectNode();
			PhaseNodePtr phaseNode = node->phaseNode();
			if (effectNode)
			{
				PhaseNodeSet phasesSet;
				getPhaseNodes( node, phasesSet );
				for(PhaseNodeSet::iterator it = phasesSet.begin(); it != phasesSet.end(); ++it)
				{
					Graph::NodeViewPtr phaseView = graphView_.nodeView( (*it).get() );
					CRect rect = phaseView->rect();
					phaseView->position( CPoint( dragNodeOldPos_.x, rect.top ) );
					(*it)->dragging( false );
				}

				if (!canceled)
				{
					int chainPos = dropBeforeEffect_ ? dropBeforeEffect_->chainPos() : BasePostProcessingNode::chainPosCounter();
					if (!cloning_)
					{
						if (chainPos > effectNode->chainPos())
						{
							chainPos -= static_cast<int>(phasesSet.size() + 1);
						}
						effectNode->chainPos( chainPos );
						editEffectChain( new MoveEffectEditor( effectNode, dropBeforeEffect_ ) );
					}
					else
					{
						EffectNodePtr newEffectNode = effectNode->clone();
						if (newEffectNode)
						{
							newEffectNode->chainPos( chainPos );
							editEffectChain( new AddEffectEditor( newEffectNode->pyEffect(), dropBeforeEffect_ ) );
							currentNode_ = newEffectNode;
						}
						else
						{
							ERROR_MSG( "PagePostProcessing::endDrag: Failed to clone effect.\n" );
						}
					}
					needsRedraw = false;
				}
			}
			else if (phaseNode)
			{
				if (!canceled && dropBeforeEffect_)
				{
					EffectNodePtr oldEffectNode = getPhaseEffect( phaseNode );
					if (oldEffectNode)
					{
						int chainPos = 0;
						if (dropBeforePhase_)
						{
							chainPos = dropBeforePhase_->chainPos();
						}
						else
						{
							PhaseNodeSet phases;
							getPhaseNodes( dropBeforeEffect_, phases );
							chainPos = static_cast<int>(dropBeforeEffect_->chainPos() + phases.size() + 1);
						}
						if (!cloning_)
						{
							if (chainPos > phaseNode->chainPos())
							{
								chainPos--;
							}
							phaseNode->chainPos( chainPos );
							editEffectPhases( oldEffectNode, new DeletePhaseEditor( phaseNode ), false );
							editEffectPhases( dropBeforeEffect_, new AddPhaseEditor( phaseNode->pyPhase(), dropBeforePhase_ ) );
						}
						else
						{
							PhaseNodePtr newPhaseNode = phaseNode->clone();
							if (newPhaseNode)
							{
								newPhaseNode->chainPos( chainPos );
								editEffectPhases( dropBeforeEffect_, new AddPhaseEditor( newPhaseNode->pyPhase(), dropBeforePhase_ ) );
								currentNode_ = newPhaseNode;
							}
							else
							{
								ERROR_MSG( "PagePostProcessing::endDrag: Failed to clone phase.\n" );
							}
						}
						needsRedraw = false;
					}
					else
					{
						// Should never get here.
						ERROR_MSG( "PagePostProcessing::endDrag: Draggging orphan phase.\n" );
					}
				}
			}

			if (needsRedraw)
			{
				graphView_.RedrawWindow();
			}
		}

		dropBeforeEffect_ = NULL;
		dropBeforePhase_ = NULL;
		dragNodeEdges_.clear();
		dragging_ = false;
	}
	else if (draggingRT_)
	{
		PhaseNodePtr srcPhase = node->phaseNode();
		if (srcPhase)
		{
			BW::wstring rtName = L"RT:" + bw_utf8tow( srcPhase->output() );
			onPhasePopupDrop( rtName );
		}
		graphView_.RedrawWindow();
		dropBeforePhase_ = NULL;
		draggingRT_ = false;
	}
}


/**
 *	This MFC method initialises the panel's controls with their descriptions in
 *	the Windows resources file.
 *
 *	@param pDX	MFC data exchange struct.
 */
void PagePostProcessing::DoDataExchange( CDataExchange * pDX )
{
	CDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_GRAPH_VIEW, graphView_ );
	DDX_Control( pDX, IDC_PPGRAPH_VERT_SB, vertScroll_ );
	DDX_Control( pDX, IDC_PPGRAPH_HORZ_SB, horzScroll_ );
}


/**
 *	This MFC method is called when the panel is initialised, which allows the
 *	panel to initialise all its internals.
 *
 *	@return	TRUE if initialised successfully.
 */
BOOL PagePostProcessing::OnInitDialog()
{
	BW_GUARD;

	BOOL ok = CDialog::OnInitDialog();

	if (ok == FALSE)
	{
		return ok;
	}

	// Init scripting modules
	pEditorPhases_ = PyObjectPtr( PyImport_ImportModule( "PostProcessing.Phases" ), PyObjectPtr::STEAL_REFERENCE );
	if (!pEditorPhases_)
	{
		ERROR_MSG( "Couldn't initialise the PostProcessing.Phases python module.\n" );
	}

	// Init chains dialog
	chainsDlg_.Create( PostProcessingChains::IDD, this );
	chainsDlg_.ShowWindow( SW_SHOW );
	chainsDlg_.setEventHandler( this );

	// Init properties dialog
	propertiesDlg_.Create( PostProcessingProperties::IDD, this );
	propertiesDlg_.ShowWindow( SW_SHOW );
	CRect propsRect;
	propertiesDlg_.GetWindowRect( propsRect );
	ScreenToClient( propsRect );

	// Init graph view
	graphContainer_.Create( _T( "STATIC" ), L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
							CRect( 0, 0, 10, 10 ), this, 0 );

	Graph::GraphPtr graph = new Graph::Graph();
	graphView_.init( graph, ViewSkin::bkColour() );
	graphView_.SetParent( &graphContainer_ );
	NONCLIENTMETRICS metrics;
	metrics.cbSize = sizeof( metrics );
	SystemParametersInfo( SPI_GETNONCLIENTMETRICS, metrics.cbSize, (void*)&metrics, 0 );
	metrics.lfSmCaptionFont.lfItalic = TRUE;
	graphView_.footer( Localise( L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/FOOTER" ), &metrics.lfSmCaptionFont, ViewSkin::footerColour() );

	CRect captionRect;
	captionDlg_.Create( PostProcCaptionBar::IDD, &graphContainer_ );
	captionDlg_.ShowWindow( SW_SHOW );
	captionDlg_.GetWindowRect( captionRect );
	captionDlg_.setEventHandler( this );

	vertScroll_.SetParent( &graphContainer_ );
	horzScroll_.SetParent( &graphContainer_ );

	graphContainer_.setChildren( &graphView_, &captionDlg_, &horzScroll_, &vertScroll_ );

	// Splitter bars
	layout_ = TALL; // By default be TALL, since we get chucked this way on the default layout
	setLayout( layout_, 100, 100 );

	updateGraph_ = true;

	UalManager::instance().dropManager().add( new UalDropFunctor< PagePostProcessing >(
		&graphView_, "",  this, &PagePostProcessing::onGraphViewDrop, false, &PagePostProcessing::onGraphViewDropTest ) );

	UalManager::instance().dropManager().add( new UalDropFunctor< PagePostProcessing >(
		propertiesDlg_.propertyList(), "" /*render targets*/,  this, &PagePostProcessing::onPropertiesDrop, false, &PagePostProcessing::onPropertiesDropTest ) );

	return TRUE;
}


// MFC message map
BEGIN_MESSAGE_MAP( PagePostProcessing, CDialog )
	ON_WM_SIZE()
	ON_MESSAGE( WM_UPDATE_CONTROLS, OnUpdateControls )
	ON_MESSAGE( WM_PP_ZOOM_IN, OnZoomIn )
	ON_MESSAGE( WM_PP_ZOOM_OUT, OnZoomOut )
	ON_MESSAGE( WM_PP_NO_ZOOM, OnNoZoom )
	ON_MESSAGE( WM_PP_3D_PREVIEW, On3dPreview )
	ON_MESSAGE( WM_PP_PROFILE, OnProfile )
	ON_MESSAGE( WM_PP_LAYOUT, OnLayout )
	ON_MESSAGE( WM_PP_DELETE_ALL, OnDeleteAll )
	ON_MESSAGE( WM_PP_CHAIN_SELECTED, OnChainSelected )
END_MESSAGE_MAP()


/**
 *	This MFC method is overriden to resize the appropriate internal sub panels
 *	according to the new panel size.
 *
 *	@param nType	MFC resize type.
 *	@param cx	MFC new width.
 *	@param cy	MFC new height.
 */
void PagePostProcessing::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	CDialog::OnSize( nType, cx, cy );

	if (pGraphSplitter_)
	{
		if (layout_ == NORMAL)
		{
			pGraphSplitter_->SetWindowPos( NULL, 0, 0, cx, cy, SWP_NOZORDER );
		}
		else
		{
			pChainsSplitter_->SetWindowPos( NULL, 0, 0, cx, cy, SWP_NOZORDER );
		}
	}

	RedrawWindow();
}


/**
 *	This MFC method is called each frame to allow for the panel to perform
 *	regular updates.
 *
 *	@param wParam	MFC WPARAM, ignored.
 *	@param lParam	MFC LPARAM, ignored.
 *	@result	MFC message result, 0.
 */
LRESULT PagePostProcessing::OnUpdateControls( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	static DogWatch s_dw( "PostProcessingPanel" );
	ScopedDogWatch sdw( s_dw );

	if (!IsWindowVisible())
	{
		return 0;
	}

	// Detect when we need to update the graph
	if (WorldManager::instance().changedPostProcessing())
	{
		// Somebody else (Weather, Graphics Settings) changed the chain, so
		// update the graph, and tell the "chains" list.
		currentNode_ = NULL;
		propertiesDlg_.editNode( NULL );
		WorldManager::instance().changedPostProcessing( false );
		updateGraph_ = true;
		chainsDlg_.chainChanged();
	}
	
	if (PostProcUndo::undoRedoDone())
	{
		PostProcUndo::undoRedoDone( false );
		updateGraph_ = true;
	}

	if (!draggingRT_)
	{
		// Just in case our drag&drop was canceled abruptly, hide any windows or
		// visuals displayed during the drag operation.
		phasePopup_.close();
	}

	// Force rebuild if zoom changed
	if (graphView_.zoom() < MIN_ZOOM_STEP)
	{
		graphView_.zoom( MIN_ZOOM_STEP );
	}
	else if (graphView_.zoom() > MAX_ZOOM_STEP)
	{
		graphView_.zoom( MAX_ZOOM_STEP );
	}

	if (graphView_.zoom() != curZoom_)
	{
		curZoom_ = graphView_.zoom();
		
		ViewSkin::nodeZoom( curZoom_ );

		CPoint zoomPos;
		if (zoomToCentre_)
		{
			CRect rect;
			graphView_.GetClientRect( rect );
			zoomPos = rect.CenterPoint();
		}
		else if (zoomToTopRight_)
		{
			zoomPos = CPoint( 0, 0 );
		}
		else
		{
			GetCursorPos( &zoomPos );
			graphView_.ScreenToClient( &zoomPos );
		}

		CPoint oldPan = graphView_.panOffset();
		float relPanX = float( oldPan.x - zoomPos.x ) / graphWidth_;
		float relPanY = float( oldPan.y - zoomPos.y ) / graphHeight_;

		if (!buildGraph( false ))
		{
			ERROR_MSG( "Failed to create the Post Processing graph in the panel.\n" );
		}

		int absPanX = int( relPanX * graphWidth_ ) + zoomPos.x;
		int absPanY = int( relPanY * graphHeight_ ) + zoomPos.y;

		CPoint newPan;
		if (zoomToTopRight_)
		{
			newPan = -graphView_.panOffset();
		}
		else
		{
			newPan = CPoint( absPanX - oldPan.x, absPanY - oldPan.y );
		}

		graphView_.pan( newPan );

		updateGraph_ = false;
		zoomToCentre_ = false;
		zoomToTopRight_ = false;
	}
	
	if (updateGraph_)
	{
		updateGraph_ = false;
		if (!buildGraph())
		{
			ERROR_MSG( "Failed to create the Post Processing graph in the panel.\n" );
		}
	}

	graphView_.update();

	chainsDlg_.update();

	propertiesDlg_.update();

	// Update scrollbars
	graphContainer_.ignoreScrolling( true );
	CRect rect;
	graphView_.GetClientRect( rect );
	vertScroll_.SetScrollRange( 0, graphHeight_ - rect.Height() );
	horzScroll_.SetScrollRange( 0, graphWidth_ - rect.Width() );
	vertScroll_.SetScrollPos( -graphView_.panOffset().y );
	horzScroll_.SetScrollPos( -graphView_.panOffset().x );
	graphContainer_.ignoreScrolling( false );

	bool renderPreviews = Options::getOptionInt(
					PostProcCaptionBar::OPTION_PREVIEW3D, 0 ) ? true : false;

	bool needsRedraw =
		((renderPreviews && !dragging_) ||		// if preview and not dragging, update to get realtime preview.
		 (!renderPreviews && BasePostProcessingNode::previewMode()));	// if not preview, but was preview, draw once more to refresh

	BasePostProcessingNode::previewMode( renderPreviews );

	if (BasePostProcessingNode::hasChanged() || s_phaseChanged_)
	{
		BasePostProcessingNode::changed( false );

		if (s_phaseChanged_)
		{
			if (currentNode_ && currentNode_->phaseNode())
			{
				if (s_phaseChangedReload_)
				{
					propertiesDlg_.editNode( currentNode_ );
				}
				propertiesDlg_.propertyList()->RedrawWindow();
			}

			s_phaseChanged_ = false;
			s_phaseChangedReload_ = false;
		}

		needsRedraw = true;
	}
	
	if (needsRedraw)
	{
		RECT clientRect;
		this->graphView_.GetClientRect(&clientRect);
		BW::vector<Graph::NodeView*> nodeViews;
		BW::vector<PhaseNodeView*> phaseNodeViews;
		graphView_.collectNodeViews( nodeViews );
		for ( size_t i=0; i<nodeViews.size(); i++ )
		{
			PhaseNodeView* pnv = dynamic_cast<PhaseNodeView*>(nodeViews[i]);
			if ( pnv )
			{
				phaseNodeViews.push_back(pnv);
			}
		}
		preview_->edLayout( clientRect, this->graphView_.panOffset(), phaseNodeViews );
		graphView_.RedrawWindow();
	}

	// Profile
	if (Options::getOptionInt( PostProcCaptionBar::OPTION_PROFILE, 0 ))
	{
		PyObject * pPP = PyImport_AddModule( "PostProcessing" );
		if (pPP)
		{
			PyObject * pFunction = PyObject_GetAttrString( pPP, "profile" );
			if (pFunction)
			{
				int numIterations = Options::getOptionInt( "post_processing/profileIterations", 10 );
				PyObjectPtr pResult( Script::ask( pFunction, Py_BuildValue( "(i)", numIterations ) ),
										PyObjectPtr::STEAL_REFERENCE );
				if (pResult && PyFloat_Check( pResult.get() ))
				{
					double gpuTime;
					if (Script::setData( pResult.get(), gpuTime, "PostProcGPUtime" ) == 0)
					{
						// round to milliseconds with three digits
						double milliseconds = gpuTime * 1000.0f;
						lastGPUTime_ = float( BW_ROUND( milliseconds * 1000 ) / 1000.0f );
						updateCaptionBar();
					}
				}
				else
				{
					ERROR_MSG( "Could not profile the post-processing GPU time. "
								"Please update your video drivers.\n" );
					captionDlg_.setProfile( false );
				}
			}

			if (PyErr_Occurred())
			{
				PyErr_Print();
			}
		}
	}

	return 0;
}


/**
 *	This method is called from the caption bar when the zoom in button is
 *	pressed.
 *
 *	@param wParam	MFC WPARAM, ignored.
 *	@param lParam	MFC LPARAM, ignored.
 *	@result	MFC message result, 0.
 */
LRESULT PagePostProcessing::OnZoomIn( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	int zoom = graphView_.zoom();

	if (zoom < MAX_ZOOM_STEP)
	{
		if (zoom >= BUTTON_ZOOM_START)
		{
			graphView_.zoom( int( zoom * BUTTON_ZOOM_FACTOR ) );
		}
		else
		{
			graphView_.zoom( zoom + 1 );
		}
		zoomToCentre_ = true;
	}

	return 0;
}


/**
 *	This method is called from the caption bar when the zoom out button is
 *	pressed.
 *
 *	@param wParam	MFC WPARAM, ignored.
 *	@param lParam	MFC LPARAM, ignored.
 *	@result	MFC message result, 0.
 */
LRESULT PagePostProcessing::OnZoomOut( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	int zoom = graphView_.zoom();

	if (zoom > MIN_ZOOM_STEP)
	{
		if (zoom >= BUTTON_ZOOM_START)
		{
			graphView_.zoom( int( zoom / BUTTON_ZOOM_FACTOR ) );
		}
		else
		{
			graphView_.zoom( zoom - 1 );
		}
		zoomToCentre_ = true;
	}

	return 0;
}


/**
 *	This method is called from the caption bar when the zoom reset button
 *	is pressed.
 *
 *	@param wParam	MFC WPARAM, ignored.
 *	@param lParam	MFC LPARAM, ignored.
 *	@result	MFC message result, 0.
 */
LRESULT PagePostProcessing::OnNoZoom( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	graphView_.zoom( 0 );
	zoomToTopRight_ = true;

	return 0;
}


/**
 *	This method is called from the caption bar when the preview button
 *	is pressed.
 *
 *	@param wParam	MFC WPARAM, ignored.
 *	@param lParam	MFC LPARAM, ignored.
 *	@result	MFC message result, 0.
 */
LRESULT PagePostProcessing::On3dPreview( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	// Stop profiling if using the 3d preview.
	captionDlg_.setProfile( false );
	updateCaptionBar();
	return 0;
}


/**
 *	This method is called from the caption bar when the profiling button
 *	is pressed.
 *
 *	@param wParam	MFC WPARAM, ignored.
 *	@param lParam	MFC LPARAM, ignored.
 *	@result	MFC message result, 0.
 */
LRESULT PagePostProcessing::OnProfile( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	// Stop preview if profiling.
	captionDlg_.setPreview3D( false );
	updateCaptionBar();
	return 0;
}


/**
 *	This method is called from the caption bar when the layout button
 *	is pressed.
 *
 *	@param wParam	MFC WPARAM, ignored.
 *	@param lParam	MFC LPARAM, ignored.
 *	@result	MFC message result, 0.
 */
LRESULT PagePostProcessing::OnLayout( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	CRect rect;
	GetClientRect( rect );
	if (layout_ == NORMAL)
	{
		layout_ = WIDE;
	}
	else if (layout_ == WIDE)
	{
		layout_ = TALL;
	}
	else // layout == TALL
	{
		layout_ = NORMAL;
	}
	setLayout( layout_, rect.Width(), rect.Height() );

	return 0;
}


/**
 *	This method is called from the caption bar when the delete all button
 *	is pressed.
 *
 *	@param wParam	MFC WPARAM, ignored.
 *	@param lParam	MFC LPARAM, ignored.
 *	@result	MFC message result, 0.
 */
LRESULT PagePostProcessing::OnDeleteAll( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	if (MessageBox( Localise( L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/DELETE_ALL_TEXT" ),
					Localise( L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/DELETE_ALL_CAPTION" ),
					MB_ICONWARNING | MB_YESNO ) == IDYES )
	{
		editEffectChain( new DeleteEffectEditor( NULL /* delete all */ ) );
	}

	return 0;
}


/**
 *	This method is called from the chains dialog when a chain is selected.
 *
 *	@param wParam	MFC WPARAM, ignored.
 *	@param lParam	MFC LPARAM, ignored.
 *	@result	MFC message result, 0.
 */
LRESULT PagePostProcessing::OnChainSelected( WPARAM wParam, LPARAM lParam )
{
	updateGraph_ = true;
	return 0;
}


/**
 *	This method is called when the user drops an asset from the Asset Browser
 *	onto this panel.
 *
 *	@param ii	Asset Browser asset info.
 *	@result	Always returns true.
 */
bool PagePostProcessing::onGraphViewDrop( UalItemInfo * ii )
{
	BW_GUARD;

	if (startsWith( ii->longText(), L"effects:" ))
	{
		if (!dropBeforeEffect_)
		{
			// Drop to the right-most end of the chain.
			editEffectChain( new AddEffectEditor( ii->text(), NULL ) ); // NULL = at the end
		}
		else if (dropBeforeEffect_)
		{
			// Drop to the effect before the dropBeforeEffect_.
			editEffectChain( new AddEffectEditor( ii->text(), dropBeforeEffect_ ) );
		}
	}
	else if (startsWith( ii->longText(), L"phases:" ))
	{
		if (dropBeforeEffect_)
		{
			if (!dropBeforePhase_)
			{
				// Drop to the bottom of the effect's phases
				editEffectPhases( dropBeforeEffect_, new AddPhaseEditor( ii->text(), NULL, pEditorPhases_ ) ); // NULL = at the end
			}
			else if (dropBeforePhase_)
			{
				// Drop to the phase before the dropBeforePhase_.
				editEffectPhases( dropBeforeEffect_, new AddPhaseEditor( ii->text(), dropBeforePhase_, pEditorPhases_ ) );
			}
		}
	}
	else if (startsWith( ii->longText(), L"RT:" ) ||
			isTexture( ii->longText() ) ||
			BWResource::getExtensionW( ii->longText() ) == L"fx")
	{
		onPhasePopupDrop( ii->longText() );
	}

	dropBeforeEffect_ = NULL;
	dropBeforePhase_ = NULL;

	return true;
}


/**
 *	This method is called when the user is dragging an asset from the Asset
 *	Browser on top if this panel, to test if this asset type is handled by the
 *	area of the panel the mouse is over.
 *
 *	@param ii	Asset Browser asset info.
 *	@result	Rectangle of the area of the panel that would accept the asset, or
 *			HIT_TEST_MISS if the area or the asset are invalid.
 */
RectInt PagePostProcessing::onGraphViewDropTest( UalItemInfo *ii )
{
	BW_GUARD;

	CPoint mousePt;
	GetCursorPos( &mousePt );
	CPoint pt( mousePt );
	graphView_.ScreenToClient( &pt );

	RectInt ret( UalDropManager::HIT_TEST_MISS );

	dropBeforeEffect_ = NULL;
	dropBeforePhase_ = NULL;

	bool showPhasePopup = false;

	CRect popupRect;
	phasePopup_.GetWindowRect( popupRect );
	bool mouseOnPhasePopup = (phasePopup_.GetSafeHwnd() != NULL && popupRect.PtInRect( mousePt ));

	if (mouseOnPhasePopup || !graphViewAutoPan( pt ))
	{
		if (startsWith( ii->longText(), L"effects:" ))
		{
			// Not panning, check drop target for effects.
			CRect rect = findEffectDropPoint( pt, NULL );
			ret = RectInt( rect.left, rect.bottom, rect.right, rect.top );
		}
		else if (startsWith( ii->longText(), L"phases:" ))
		{
			// Not panning and not an effect, check drop target for phases.
			CRect rect = findPhaseDropPoint( pt, NULL );
			ret = RectInt( rect.left, rect.bottom, rect.right, rect.top );
		}
		else if (startsWith( ii->longText(), L"RT:" ) ||
				isTexture( ii->longText() ) ||
				BWResource::getExtensionW( ii->longText() ) == L"fx")
		{
			// It's a render target, texture, or shader file. We need to popup a menu with the
			// properties that can take files of this type.
			PhaseNodePtr phase;

			if (mouseOnPhasePopup)
			{	
				phase = lastDropTargetPhase_;
			}

			if (!phase)
			{
				phase = getPhaseByPt( pt );
			}
			
			if (phase)
			{
				ret = UalDropManager::HIT_TEST_OK_NO_RECT;

				showPhasePopup = showHidePhasePopup( ii->longText(), phase );
			}

			lastDropTargetPhase_ = phase.get();
		}
	}

	if (showPhasePopup)
	{
		phasePopup_.update( PHASE_POPUP_ALPHA );
	}
	else
	{
		phasePopup_.close();
	}

	return ret;
}


/**
 *	This method is called when the user drops an asset from the Asset Browser
 *	onto this panel's property list.
 *
 *	@param ii	Asset Browser asset info.
 *	@result	Always returns true.
 */
bool PagePostProcessing::onPropertiesDrop( UalItemInfo * ii )
{
	BW_GUARD;

	if (startsWith( ii->longText(), L"RT:" ))
	{
		// We add a ".rt" extension to the render target name so the properties list accepts it.
		return propertiesDlg_.propertyList()->doDrop( CPoint( ii->x(), ii->y() ), ii->text() + L".rt" );
	}
	else
	{
		// We assume it's a texture or an fx file.
		return propertiesDlg_.propertyList()->doDrop( CPoint( ii->x(), ii->y() ),
			BWResource::dissolveFilenameW( ii->longText() ) );
	}
}


/**
 *	This method is called when the user is dragging an asset from the Asset
 *	Browser on top if this panel's property list, to test if this asset type is
 *	handled by the property the mouse is over.
 *
 *	@param ii	Asset Browser asset info.
 *	@result	Rectangle of the property that would accept the asset, or
 *			HIT_TEST_MISS if the asset is not handled by the property.
 */
RectInt PagePostProcessing::onPropertiesDropTest( UalItemInfo *ii )
{
	BW_GUARD;

	RectInt ret( UalDropManager::HIT_TEST_MISS );

	if (startsWith( ii->longText(), L"RT:" ))
	{
		// Here we just add test which properties support textures.
		CRect rect = propertiesDlg_.propertyList()->dropTest( CPoint( ii->x(), ii->y() ), ii->text() + L".rt" );
		ret = RectInt( rect.left, rect.bottom, rect.right, rect.top );
	}
	else if (isTexture( ii->longText() ) || BWResource::getExtensionW( ii->longText() ) == L"fx")
	{
		// Also handle textures.
		CRect rect = propertiesDlg_.propertyList()->dropTest( CPoint( ii->x(), ii->y() ),
									BWResource::dissolveFilenameW( ii->longText() ) );
		ret = RectInt( rect.left, rect.bottom, rect.right, rect.top );
	}

	return ret;
}


/**
 *	This method checks to see if the specified point is near the edges of the
 *	graph view, and if so, automatically pans/scrolls the graph view.  Useful
 *	when the user is dragging a, effect or phase.
 *
 *	@param pt	Mouse position.
 *	@result	True if the graph view was panned, false if not.
 */
bool PagePostProcessing::graphViewAutoPan( const CPoint & pt )
{
	BW_GUARD;

	bool ret = false;

	CRect gvRect;
	graphView_.GetClientRect( gvRect );

	CRect gvLeftRect = gvRect;
	gvLeftRect.right = gvLeftRect.left + DROP_AUTOSCROLL_EDGE;
	CRect gvRightRect = gvRect;
	gvRightRect.left = gvRightRect.right - DROP_AUTOSCROLL_EDGE;

	CRect gvTopRect = gvRect;
	gvTopRect.bottom = gvTopRect.top + DROP_AUTOSCROLL_EDGE;
	CRect gvBottomRect = gvRect;
	gvBottomRect.top = gvBottomRect.bottom - DROP_AUTOSCROLL_EDGE;

	CSize panOffset( 0, 0 );

	if (gvLeftRect.PtInRect( pt ))
	{
		// pan left
		panOffset.cx = DROP_AUTOSCROLL_STEP;
	}
	else if (gvRightRect.PtInRect( pt ))
	{
		// pan right
		panOffset.cx = -DROP_AUTOSCROLL_STEP;
	}

	if (gvTopRect.PtInRect( pt ))
	{
		// pan left
		panOffset.cy = DROP_AUTOSCROLL_STEP;
	}
	else if (gvBottomRect.PtInRect( pt ))
	{
		// pan right
		panOffset.cy = -DROP_AUTOSCROLL_STEP;
	}

	if (panOffset != CSize( 0, 0 ))
	{
		// Panning
		graphView_.pan( panOffset );
		ret = true;
	}

	return ret;
}


/**
 *	This static method is called by phase properties when they change.
 *
 *	@param needsReload	True if the properties need to be reloaded.
 */
/*static*/ void PagePostProcessing::phaseChanged( bool needsReload )
{
	BW_GUARD;

	// User is editing the panel... don't allow changes underneath it
	WorldManager::instance().userEditingPostProcessing( true );

	s_phaseChanged_ = true;
	s_phaseChangedReload_ = needsReload;
}


/**
 *	This method goes through the post processing chain in python and rebuilds
 *	the node graph to match it.
 *
 *	@param redraw	True to redraw the graph view.
 *	@result	True if the graph was rebuilt successfully.
 */
bool PagePostProcessing::buildGraph( bool redraw /* = true */)
{
	BW_GUARD;

	bool ok = false;

	renderTargetMBs_ = 0.0;

	BasePostProcessingNodePtr oldSelection = currentNode_;
	propertiesDlg_.editNode( NULL );

	PyObject * pPP = PyImport_AddModule( "PostProcessing" );
	
	PyObject * pChainAttr = NULL;
	if (pPP)
	{
		pChainAttr = PyObject_GetAttrString( pPP, "chain" );
	}

	if (pChainAttr)
	{
		PyObjectPtr pChain( Script::ask( pChainAttr, PyTuple_New(0) ),
							PyObjectPtr::STEAL_REFERENCE );

		if (pChain && PySequence_Check( pChain.get() ))
		{
			// Reset states
			graphView_.graph()->clear();
			graphView_.graph( graphView_.graph() );
			basePostProcessingNodes_.clear();
			BasePostProcessingNode::resetChainPosCounter();
			currentNode_ = NULL;
			graphWidth_ = 0;
			graphHeight_ = 0;

			// Get effect nodes from python.
			EffectNodePtr prevEffect;
			for (int i = 0; i < PySequence_Size( pChain.get() ); ++i)
			{
				PyObjectPtr pItem( PySequence_GetItem( pChain.get(), i ), PyObjectPtr::STEAL_REFERENCE );
				if (PostProcessing::Effect::Check( pItem.get() ))
				{
					PostProcessing::EffectPtr pyEffect;
					if (Script::setData( pItem.get(), pyEffect, "PagePostProcessing::buildGraph" ) == 0)
					{
						// Build effect
						EffectNodePtr effect = buildEffect( i, pyEffect.get(), prevEffect );
						prevEffect = effect;

						// Build its phases.
						buildPhases( i, effect, pyEffect.get() );

						// Make sure it's active state is updated in the gui
						nodeActive( effect, effect->active() );
					}
				}
			}

			graphWidth_ += Graph::GraphView::CANVAS_WIDTH_MARGIN;
			graphHeight_ += Graph::GraphView::CANVAS_HEIGHT_MARGIN;

			// Restore the selection
			if (oldSelection)
			{
				PyObject * oldObj = oldSelection->pyObject();

				for (BasePostProcessingNodesMap::iterator it = basePostProcessingNodes_.begin();
					it != basePostProcessingNodes_.end(); ++it)
				{
					PyObject * newObj = (*it).second->pyObject();

					if (oldObj == newObj && oldSelection->chainPos() == (*it).second->chainPos())
					{
						// the selected node is still here, so re-selected it
						currentNode_ = (*it).second;
						graphView_.selectedNode( graphView_.nodeView( currentNode_ ) );
						propertiesDlg_.editNode( currentNode_ );
						break;
					}
				}
			}

			PyObject * pFunction = PyObject_GetAttrString( pPP, "calcMemoryUsage" );
			if (pFunction)
			{
				PyObjectPtr pResult( Script::ask( pFunction, Py_BuildValue( "(O)", Py_True ) ),
										PyObjectPtr::STEAL_REFERENCE );

				if (pResult && PyFloat_Check( pResult.get() ))
				{
					double megabytes = 0;
					if (Script::setData( pResult.get(), megabytes, "RenderTargetMBs" ) == 0)
					{
						// round to two digits
						renderTargetMBs_ = float( BW_ROUND( megabytes * 100 ) / 100.0f );
					}
				}
			}

			updateCaptionBar();
			ok = true;
		}
	}

	if (PyErr_Occurred())
	{
		PyErr_Print();
	}

	graphView_.resizeToNodes();
	
	if (redraw)
	{
		graphView_.RedrawWindow();
	}

	return ok;
}


/**
 *	This method takes a python post processing effect and builds its
 *	corresponding Effect node in the graph.
 *
 *	@param col	Column or effect index, from left to right.
 *	@param pyEffect	Python post processing effect.
 *	@param prevEffect	Previous Effect node.
 *	@result	New Effect node just built.
 */
EffectNodePtr PagePostProcessing::buildEffect( int col, PostProcessing::Effect * pyEffect, const EffectNodePtr & prevEffect )
{
	BW_GUARD;

	EffectNodePtr effect;

	if (pyEffect)
	{
		// Create effect node for the effect.
		effect = new EffectNode( pyEffect, this );
		graphView_.graph()->addNode( effect );
		Graph::NodeViewPtr effectNodeView = new EffectNodeView( graphView_, effect );
		CPoint pt( ViewSkin::nodeMargin().cx + col * ViewSkin::effectNodeSeparation().cx,
					ViewSkin::nodeMargin().cy );
		effectNodeView->position( pt );
		if (pt.x + effectNodeView->rect().Width() > graphWidth_)
		{
			graphWidth_ = pt.x + effectNodeView->rect().Width();
		}
		if (pt.y + effectNodeView->rect().Height() > graphHeight_)
		{
			graphHeight_ = pt.y + effectNodeView->rect().Height();
		}
		
		// Link to previous effect if necessary.
		if (prevEffect)
		{
			EffectEdgePtr effectEdge = new EffectEdge( prevEffect.get(), effect );
			graphView_.graph()->addEdge( effectEdge );
			new EffectEdgeView( graphView_, effectEdge );
		}

		// Add to our registry of BasePostProcessingNode-derived nodes.
		basePostProcessingNodes_.insert( std::make_pair( effect.get(), effect.get() ) );
	}

	return effect;
}


/**
 *	This method builds a series of connected Phase nodes from the list of
 *	python post processing phases of a python effect.
 *
 *	@param col	Column or effect index, from left to right.
 *	@param effect	Effect node in the graph.
 *	@param pyEffect	Python post processing effect.
 *	@result	True if successful.
 */
bool PagePostProcessing::buildPhases( int col, const EffectNodePtr & effect, PostProcessing::Effect * pyEffect )
{
	BW_GUARD;

	bool ok = false;

	PyObjectPtr pyPhases( PyObject_GetAttrString( pyEffect, "phases" ), PyObjectPtr::STEAL_REFERENCE );

	if (pyPhases && PySequence_Check( pyPhases.get() ))
	{
		Graph::NodePtr prevNode = effect.get();

		CSize offset;
		offset.cx = ViewSkin::phaseNodeSeparation().cx * col;
		offset.cy = ViewSkin::effectNodeSeparation().cy;
		
		for (int i = 0; i < PySequence_Size( pyPhases.get() ); ++i)
		{
			PyObjectPtr pItem( PySequence_GetItem( pyPhases.get(), i ), PyObjectPtr::STEAL_REFERENCE );
			if (PostProcessing::Phase::Check( pItem.get() ))
			{
				PostProcessing::PhasePtr pyPhase;
				if (Script::setData( pItem.get(), pyPhase, "PagePostProcessing::buildPhases" ) == 0)
				{
					// Create phase node for the phase.
					PhaseNodePtr phase = new PhaseNode( pyPhase.get(), this );
					graphView_.graph()->addNode( phase );
					Graph::NodeViewPtr phaseNodeView = new PhaseNodeView( graphView_, phase, preview_ );

					CPoint pt( ViewSkin::nodeMargin().cx + offset.cx,
								ViewSkin::nodeMargin().cy + offset.cy );
					phaseNodeView->position( pt );
					if (pt.x + phaseNodeView->rect().Width() > graphWidth_)
					{
						graphWidth_ = pt.x + phaseNodeView->rect().Width();
					}
					if (pt.y + phaseNodeView->rect().Height() > graphHeight_)
					{
						graphHeight_ = pt.y + phaseNodeView->rect().Height();
					}

					pyPhase->edChangeCallback( &PagePostProcessing::phaseChanged );
					
					// Link to previous effect if necessary.
					if (prevNode)
					{
						PhaseEdgePtr phaseEdge = new PhaseEdge( prevNode, phase );
						graphView_.graph()->addEdge( phaseEdge );
						new PhaseEdgeView( graphView_, phaseEdge );
					}
					prevNode = phase;

					// Add to our registry of BasePostProcessingNode-derived nodes.
					basePostProcessingNodes_.insert( std::make_pair( phase.get(), phase.get() ) );
				}
				offset.cy += ViewSkin::phaseNodeSeparation().cy;
			}
		}

		ok = true;
	}

	return ok;
}


/**
 *	This method returns the Phase nodes of an Effect node.
 *
 *	@param effectNode	Effect node to get the phases from.
 *	@param retPhases	Return param, list of phase nodes.
 *	@result	True if successful.
 */
bool PagePostProcessing::getPhaseNodes( const BasePostProcessingNodePtr & effectNode, PhaseNodeSet & retPhases ) const
{
	BW_GUARD;

	bool isEffectNode = (effectNode->effectNode() != NULL);

	if (isEffectNode)
	{
		// It's an EffectNode, find its phases
		Graph::GraphPtr graph = graphView_.graph();
		
		Graph::NodePtr startNode = effectNode.get();
		while (startNode)
		{
			Graph::NodesSet adjacentNodes;
			graph->adjacentNodes( startNode, adjacentNodes );

			startNode = NULL;

			for (Graph::NodesSet::iterator it = adjacentNodes.begin();
				it != adjacentNodes.end(); ++it)
			{
				BasePostProcessingNodesMap::const_iterator itPPNode = basePostProcessingNodes_.find( (*it).get() );
				if (itPPNode != basePostProcessingNodes_.end())
				{
					PhaseNodePtr phaseNode = (*itPPNode).second->phaseNode();
					if (phaseNode)
					{
						// Found a phase connected to it. Assuming a node can
						// connect to at most one phase.
						startNode = *it;
						retPhases.insert( phaseNode );
						break;
					}
				}
				else
				{
					INFO_MSG( "PagePostProcessing::getPhaseNodes: Effect node adjacent to unregistered node.\n" );
				}
			}
		}
	}

	return isEffectNode;
}


/**
 *	This method applies an Effect editor to the chain.  An effect editor could
 *	be adding an effect, moving an effect, or even deleting an effect.
 *
 *	@param effectEditor	Object that knows what and how to edit the chain.
 *	@result	True if successful.
 */
bool PagePostProcessing::editEffectChain( SequenceEditorPtr effectEditor )
{
	BW_GUARD;

	bool ok = false;

	if (effectEditor && effectEditor->isOK())
	{
		PyObject * pPP = PyImport_AddModule( "PostProcessing" );

		PyObject * pChainAttr = NULL;
		if (pPP)
		{
			pChainAttr = PyObject_GetAttrString( pPP, "chain" );
		}

		if (pChainAttr)
		{
			PyObjectPtr pChain( Script::ask( pChainAttr, PyTuple_New(0) ),
								PyObjectPtr::STEAL_REFERENCE );

			if (pChain && PySequence_Check( pChain.get() ))
			{
				UndoRedo::instance().add( new ChainUndoOp() );

				Py_ssize_t seqLen = PySequence_Size( pChain.get() );

				PyObjectPtr pNewChain( PyList_New( 0 ), PyObjectPtr::STEAL_REFERENCE );

				for (int i = 0; i < seqLen; ++i)
				{
					effectEditor->modify( pChain, i, pNewChain );
				}

				effectEditor->modify( pChain, -1 /*signal it's the last one*/, pNewChain );

				// Set this to false so we can change the chain.
				WorldManager::instance().userEditingPostProcessing( false );

				Script::call( PyObject_GetAttrString( pPP, "chain" ), Py_BuildValue( "(O)", pNewChain.get() ) );
				
				// User is editing the panel... don't allow changes underneath it
				WorldManager::instance().userEditingPostProcessing( true );

				// Restore the changed state of the chain. This state is used for
				// when someone else (Weather, Graphic Settings) changes it, but
				// here we changed it ourselves.
				WorldManager::instance().changedPostProcessing( false );
				
				UndoRedo::instance().barrier( LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/UNDO_CHAIN"), false );

				updateGraph_ = true;

				ok = true;
			}
		}
	}
	
	if (PyErr_Occurred())
	{
		PyErr_Print();
	}
	
	if (!ok)
	{
		ERROR_MSG( "PagePostProcessing::editEffectChain: failed to set a new post-processing chain.\n" );
	}

	return ok;
}


/**
 *	This method applies a Phase editor to the list of phases of an Effect.  A
 *	Phase editor could be adding a phase, moving a phase, or deleting a phase.
 *
 *	@param effectNode	Effect node that we want to edit its phases list.
 *	@param phaseEditor	Object that knows how to edit the list of phases.
 *	@param addBarrier	Whether or not it should add an undo/redo barrier.
 *	@result	True if successful.
 */
bool PagePostProcessing::editEffectPhases( EffectNodePtr effectNode, SequenceEditorPtr phaseEditor, bool addBarrier /*= true*/ )
{
	BW_GUARD;

	bool ok = false;

	if (effectNode && phaseEditor && phaseEditor->isOK())
	{
		PyObjectPtr pPhases( PyObject_GetAttrString( effectNode->pyEffect().get(), "phases" ),
								PyObjectPtr::STEAL_REFERENCE );

		if (pPhases && PySequence_Check( pPhases.get() ))
		{
			UndoRedo::instance().add( new PhasesUndoOp( effectNode->pyEffect().get() ) );

			Py_ssize_t seqLen = PySequence_Size( pPhases.get() );

			PyObjectPtr pNewPhases( PyList_New( 0 ), PyObjectPtr::STEAL_REFERENCE );

			MF_ASSERT( seqLen <= INT_MAX );
			for (int i = 0; i < ( int ) seqLen; ++i)
			{
				phaseEditor->modify( pPhases, i, pNewPhases );
			}

			phaseEditor->modify( pPhases, -1 /*signal it's the last one*/, pNewPhases );

			PyObject_SetAttrString( effectNode->pyEffect().get(), "phases" , pNewPhases.get() );
			
			updateGraph_ = true;
			
			// User is editing the panel... don't allow changes underneath it
			WorldManager::instance().userEditingPostProcessing( true );

			if (addBarrier)
			{
				UndoRedo::instance().barrier( LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/UNDO_PHASES"), false );
			}

			ok = true;
		}
	}
	return ok;
}


/**
 *	This method initialises and returns a device context to be used for drawing
 *	a feedback rectangle or line to show the user where an Effect or Phase will
 *	fall in the current node graph.
 *
 *	@result	Device context to draw the feedback rectangles in.
 */
CDC * PagePostProcessing::beginDrawDrag()
{
	BW_GUARD;

	CWnd * pScreenWnd = CWnd::GetDesktopWindow();
	
	CRgn clipRgn;
	CRect graphViewRect;
	graphView_.GetWindowRect( graphViewRect );
	clipRgn.CreateRectRgn( graphViewRect.left, graphViewRect.top, graphViewRect.right, graphViewRect.bottom );

	CDC * pScreenDC = pScreenWnd->GetDCEx( &clipRgn, DCX_WINDOW | DCX_CACHE | DCX_INTERSECTRGN );
	
	return pScreenDC;
}


/**
 *	This method releases the device context created in beginDrawDrag.
 *
 *	@param pScreenDC	Device context created in beginDrawDrag.
 */
void PagePostProcessing::endDrawDrag( CDC * pScreenDC )
{
	BW_GUARD;

	CWnd * pScreenWnd = CWnd::GetDesktopWindow();

	pScreenWnd->ReleaseDC( pScreenDC );
}


/**
 *	This method draws a feedback rectangle on the temporary device context.
 *
 *	@param rect	Feedback rectangle position and size.
 */
void PagePostProcessing::drawDragRect( const CRect & rect )
{
	BW_GUARD;

	CDC * pScreenDC = beginDrawDrag();
	if (pScreenDC)
	{
		CPen pen( PS_SOLID, 3, RGB( 96, 96, 96 ) );
		CPen * oldPen = pScreenDC->SelectObject( &pen );
		int oldROP = pScreenDC->SetROP2( R2_NOTXORPEN );

		pScreenDC->Rectangle( rect );

		pScreenDC->SetROP2( oldROP );
		pScreenDC->SelectObject( oldPen );
		endDrawDrag( pScreenDC );
	}
}


/**
 *	This method returns a Phase's Effect node.
 *
 *	@param phaseNode	Phase node for which we want its Effect node.
 *	@result	Effect node under which the Phase node is.
 */
EffectNodePtr PagePostProcessing::getPhaseEffect( PhaseNodePtr phaseNode )
{
	BW_GUARD;

	EffectNodePtr ret;

	BasePostProcessingNodePtr curNode = phaseNode;
	while (curNode && !curNode->effectNode())
	{
		Graph::NodesSet adjNodes;
		graphView_.graph()->backAdjacentNodes( curNode, adjNodes );
		if (adjNodes.size() == 1)
		{
			BasePostProcessingNodesMap::iterator it =
				basePostProcessingNodes_.find( (*adjNodes.begin()).get() );
			curNode = (*it).second;
		}
		else
		{
			curNode = NULL;
			ERROR_MSG( "A phase node can only have one parent.\n" );
		}
	}

	if (curNode && curNode->effectNode())
	{
		ret = curNode->effectNode();
	}

	return ret;
}


/**
 *	This method returns the Phase node that is under a point.
 *
 *	@param x	Screen X position.
 *	@param y	Screen Y position.
 *	@result	Phase node under the point, or NULL if not over any Phase.
 */
PhaseNodePtr PagePostProcessing::getPhaseByPt( const CPoint & pt ) const
{
	BW_GUARD;

	CRect graphRect;
	graphView_.GetClientRect( graphRect );

	PhaseNodePtr foundPhase;

	for (BasePostProcessingNodesMap::const_iterator it = basePostProcessingNodes_.begin();
		it != basePostProcessingNodes_.end(); ++it)
	{
		PhaseNodePtr phaseNode = (*it).second->phaseNode();
		if (phaseNode)
		{
			Graph::NodeViewPtr nodeView = graphView_.nodeView( phaseNode );

			CRect rect = nodeView->rect();
			rect.OffsetRect( graphView_.panOffset() );

			if (rect.PtInRect( pt ))
			{
				foundPhase = phaseNode;
				break;
			}
		}
	}

	return foundPhase;
}


/**
 *	This method finds the point in the post processing chain where an Effect
 *	node can be dropped.
 *
 *	@param pt	Mouse position.
 *	@param skipNode	If not NULL, it igores this node.
 *	@result	Feedback rectangle.
 */
CRect PagePostProcessing::findEffectDropPoint( const CPoint & pt, const BasePostProcessingNodePtr & skipNode )
{
	BW_GUARD;

	CRect ret( 0, 0, 0, 0 );

	CRect graphRect;
	graphView_.GetClientRect( graphRect );
	int rectHeight = graphRect.Height();

	Graph::NodeViewPtr leftNode;
	Graph::NodeViewPtr rightNode;
	for (BasePostProcessingNodesMap::iterator it = basePostProcessingNodes_.begin();
		it != basePostProcessingNodes_.end(); ++it)
	{
		EffectNodePtr effectNode = (*it).second->effectNode();
		if ((*it).second != skipNode && effectNode)
		{
			Graph::NodeViewPtr nodeView = graphView_.nodeView( effectNode );

			CRect rect = nodeView->rect();
			rect.OffsetRect( graphView_.panOffset() );
			if (rect.CenterPoint().x < pt.x &&
				(leftNode == NULL || leftNode->rect().CenterPoint().x < nodeView->rect().CenterPoint().x))
			{
				leftNode = nodeView;
			}
			else if (rect.CenterPoint().x > pt.x &&
				(rightNode == NULL || rightNode->rect().CenterPoint().x > nodeView->rect().CenterPoint().x))
			{
				rightNode = nodeView;
				dropBeforeEffect_ = effectNode;
			}
		}
	}

	if (leftNode && !rightNode)
	{
		ret = leftNode->rect();
		ret.OffsetRect( graphView_.panOffset() );

		ret.OffsetRect( DROP_ON_EDGE_OFFSET( ret, Width ), 0 );
		ret.bottom = ret.top + rectHeight;
	}
	else if (!leftNode && rightNode)
	{
		ret = rightNode->rect();
		ret.OffsetRect( graphView_.panOffset() );

		ret.OffsetRect( -DROP_ON_EDGE_OFFSET( ret, Width ), 0 );
		ret.bottom = ret.top + rectHeight;
	}
	else if (leftNode && rightNode)
	{
		ret = leftNode->rect();
		ret.OffsetRect( graphView_.panOffset() );

		CRect rightRect = rightNode->rect();
		rightRect.OffsetRect( graphView_.panOffset() );

		ret.OffsetRect( (rightRect.CenterPoint().x - ret.CenterPoint().x) / 2, 0 );
		ret.bottom = ret.top + rectHeight;
	}
	else
	{
		ret = CRect( ViewSkin::nodeMargin().cx, ViewSkin::nodeMargin().cy,
			ViewSkin::nodeMargin().cx + 1, ViewSkin::nodeMargin().cy + rectHeight );
	}

	if (!ret.IsRectEmpty())
	{
		// Make a line
		int cx = ret.CenterPoint().x;
		ret.left = cx;
		ret.right = cx + 1;
	}

	return ret;
}


/**
 *	This method finds the point in the post processing chain where a Phase node
 *	can be dropped.
 *
 *	@param pt	Mouse position.
 *	@param skipNode	If not NULL, it igores this node.
 *	@result	Feedback rectangle.
 */
CRect PagePostProcessing::findPhaseDropPoint( const CPoint & pt, const BasePostProcessingNodePtr & skipNode )
{
	BW_GUARD;

	CRect ret( 0, 0, 0, 0 );

	CRect effectRect;
	for (BasePostProcessingNodesMap::iterator it = basePostProcessingNodes_.begin();
		it != basePostProcessingNodes_.end(); ++it)
	{
		EffectNodePtr effectNode = (*it).second->effectNode();
		if (effectNode)
		{
			Graph::NodeViewPtr nodeView = graphView_.nodeView( effectNode );

			CRect rect = nodeView->rect();
			rect.OffsetRect( graphView_.panOffset() );
			if (rect.left < pt.x && rect.right > pt.x)
			{
				dropBeforeEffect_ = effectNode;
				effectRect = rect;
				break;
			}
		}
	}

	if (dropBeforeEffect_)
	{
		Graph::NodeViewPtr topNode;
		Graph::NodeViewPtr bottomNode;

		PhaseNodeSet phasesSet;
		getPhaseNodes( dropBeforeEffect_, phasesSet );

		for (PhaseNodeSet::iterator it = phasesSet.begin(); it != phasesSet.end(); ++it)
		{
			PhaseNodePtr phaseNode = (*it);
			Graph::NodeViewPtr nodeView = graphView_.nodeView( phaseNode );

			CRect rect = nodeView->rect();
			rect.OffsetRect( graphView_.panOffset() );
			if (rect.CenterPoint().y < pt.y &&
				(topNode == NULL || topNode->rect().CenterPoint().y < nodeView->rect().CenterPoint().y))
			{
				topNode = nodeView;
			}
			else if (rect.CenterPoint().y > pt.y &&
				(bottomNode == NULL || bottomNode->rect().CenterPoint().y > nodeView->rect().CenterPoint().y))
			{
				bottomNode = nodeView;
				dropBeforePhase_ = phaseNode;
			}
		}

		if (topNode && !bottomNode)
		{
			ret = topNode->rect();
			ret.OffsetRect( graphView_.panOffset() );

			ret.OffsetRect( 0, DROP_ON_EDGE_OFFSET( ret, Height ) );
		}
		else if (!topNode && bottomNode)
		{
			ret = bottomNode->rect();
			ret.OffsetRect( graphView_.panOffset() );

			ret.OffsetRect( 0, -DROP_ON_EDGE_OFFSET( ret, Height ) );
		}
		else if (topNode && bottomNode)
		{
			ret = topNode->rect();
			ret.OffsetRect( graphView_.panOffset() );

			CRect bottomRect = bottomNode->rect();
			bottomRect.OffsetRect( graphView_.panOffset() );

			ret.OffsetRect( 0, (bottomRect.CenterPoint().y - ret.CenterPoint().y) / 2 );
		}
		else
		{
			ret = effectRect;
			ret.OffsetRect( 0, DROP_ON_EDGE_OFFSET( ret, Height ) );
		}

		// Make a line
		int cy = ret.CenterPoint().y;
		ret.top = cy;
		ret.bottom = cy + 1;
	}

	return ret;
}


/**
 *	This method sets the layout of the panel's internal areas as specified.
 *
 *	@param layout	Either NORMAL, WIDE or TALL (@see PanelsLayout).
 *	@param cx	Panel width.
 *	@param cy	Panel height.
 *	@param clearSizes	If true, it clears the stored sizes for the chains and
 *						graph sub panels, making it use their default sizes.
 */
void PagePostProcessing::setLayout( PanelsLayout layout, int cx, int cy, bool clearSizes /* = true */ )
{
	BW_GUARD;

	if (clearSizes)
	{
		chainsSplitterSize_ = 0;
		graphSplitterSize_ = 0;
	}

	graphContainer_.SetParent( this );
	propertiesDlg_.SetParent( this );
	chainsDlg_.SetParent( this );

	bw_safe_delete(pChainsSplitter_);
	bw_safe_delete(pGraphSplitter_);

	pChainsSplitter_ = new NiceSplitterWnd();
	pChainsSplitter_->setMinRowSize( MIN_CHAINS_SIZE );
	pChainsSplitter_->setMinColSize( MIN_CHAINS_SIZE );

	pGraphSplitter_ = new NiceSplitterWnd();
	pGraphSplitter_->setMinRowSize( MIN_GRAPH_SIZE );
	pGraphSplitter_->setMinColSize( MIN_GRAPH_SIZE );

	if (layout == WIDE)
	{
		// [C_G_P]
		pChainsSplitter_->CreateStatic( this, 1, 2, WS_CHILD );

		pGraphSplitter_->CreateStatic( this, 1, 2, WS_CHILD );

		int chainsId = pChainsSplitter_->IdFromRowCol( 0, 0 );
		int innerSplitterId = pChainsSplitter_->IdFromRowCol( 0, 1 );
		int graphId = pGraphSplitter_->IdFromRowCol( 0, 0 );
		int propsId = pGraphSplitter_->IdFromRowCol( 0, 1 );

		graphContainer_.SetDlgCtrlID( graphId );
		graphContainer_.SetParent( pGraphSplitter_ );

		propertiesDlg_.SetDlgCtrlID( propsId );
		propertiesDlg_.SetParent( pGraphSplitter_ );

		chainsDlg_.SetDlgCtrlID( chainsId );
		chainsDlg_.SetParent( pChainsSplitter_ );

		pGraphSplitter_->SetDlgCtrlID( innerSplitterId );
		pGraphSplitter_->SetParent( pChainsSplitter_ );

		int graphSplitterSize = graphSplitterSize_ > 0 ? graphSplitterSize_ :
												std::max( MIN_GRAPH_SIZE, cx - DEFAULT_SPLITTER_WIDTH * 2 );
		pGraphSplitter_->SetColumnInfo( 0, graphSplitterSize, 1 );
		pGraphSplitter_->SetColumnInfo( 1, 10, 1 );

		int chainsSplitterSize = chainsSplitterSize_ > 0 ? chainsSplitterSize_ :
														DEFAULT_SPLITTER_WIDTH;
		pChainsSplitter_->SetColumnInfo( 0, chainsSplitterSize, 1 );
		pChainsSplitter_->SetColumnInfo( 1, 10, 1 );
	}
	else if (layout == TALL)
	{
		// [C]
		// [G]
		// [P]
		pChainsSplitter_->CreateStatic( this, 2, 1, WS_CHILD );

		pGraphSplitter_->CreateStatic( this, 2, 1, WS_CHILD );

		int chainsId = pChainsSplitter_->IdFromRowCol( 0, 0 );
		int innerSplitterId = pChainsSplitter_->IdFromRowCol( 1, 0 );
		int graphId = pGraphSplitter_->IdFromRowCol( 0, 0 );
		int propsId = pGraphSplitter_->IdFromRowCol( 1, 0 );

		graphContainer_.SetDlgCtrlID( graphId );
		graphContainer_.SetParent( pGraphSplitter_ );

		propertiesDlg_.SetDlgCtrlID( propsId );
		propertiesDlg_.SetParent( pGraphSplitter_ );

		chainsDlg_.SetDlgCtrlID( chainsId );
		chainsDlg_.SetParent( pChainsSplitter_ );

		pGraphSplitter_->SetDlgCtrlID( innerSplitterId );
		pGraphSplitter_->SetParent( pChainsSplitter_ );

		int graphSplitterSize = graphSplitterSize_ > 0 ? graphSplitterSize_ :
												std::max( MIN_GRAPH_SIZE, cy - DEFAULT_SPLITTER_HEIGHT * 2 );
		pGraphSplitter_->SetRowInfo( 0, graphSplitterSize, 1 );
		pGraphSplitter_->SetRowInfo( 1, 10, 1 );

		int chainsSplitterSize = chainsSplitterSize_ > 0 ? chainsSplitterSize_ :
														DEFAULT_SPLITTER_HEIGHT;
		pChainsSplitter_->SetRowInfo( 0, chainsSplitterSize, 1 );
		pChainsSplitter_->SetRowInfo( 1, 10, 1 );
	}
	else // layout == NORMAL
	{
		// [C GGG ]
		// [P_GGG_]
		pChainsSplitter_->CreateStatic( this, 2, 1, WS_CHILD );

		pGraphSplitter_->CreateStatic( this, 1, 2, WS_CHILD );

		int chainsId = pChainsSplitter_->IdFromRowCol( 0, 0 );
		int propsId = pChainsSplitter_->IdFromRowCol( 1, 0 );
		int innerSplitterId = pGraphSplitter_->IdFromRowCol( 0, 0 );
		int graphId = pGraphSplitter_->IdFromRowCol( 0, 1 );

		graphContainer_.SetDlgCtrlID( graphId );
		graphContainer_.SetParent( pGraphSplitter_ );

		chainsDlg_.SetDlgCtrlID( chainsId );
		chainsDlg_.SetParent( pChainsSplitter_ );

		propertiesDlg_.SetDlgCtrlID( propsId );
		propertiesDlg_.SetParent( pChainsSplitter_ );

		pChainsSplitter_->SetDlgCtrlID( innerSplitterId );
		pChainsSplitter_->SetParent( pGraphSplitter_ );

		int chainsSplitterSize = chainsSplitterSize_ > 0 ? chainsSplitterSize_ :
														DEFAULT_SPLITTER_HEIGHT;
		pChainsSplitter_->SetRowInfo( 0, chainsSplitterSize, 1 );
		pChainsSplitter_->SetRowInfo( 1, 10, 1 );

		int graphSplitterSize = graphSplitterSize_ > 0 ? graphSplitterSize_ :
												DEFAULT_SPLITTER_WIDTH;
		pGraphSplitter_->SetColumnInfo( 0, graphSplitterSize, 1 );
		pGraphSplitter_->SetColumnInfo( 1, 10, 1 );
	}

	pChainsSplitter_->ShowWindow( SW_SHOW );
	pChainsSplitter_->RecalcLayout();

	pGraphSplitter_->ShowWindow( SW_SHOW );
	pGraphSplitter_->RecalcLayout();

	if (layout_ == NORMAL)
	{
		pGraphSplitter_->SetWindowPos( NULL, 0, 0, cx, cy, SWP_NOZORDER );
	}
	else
	{
		pChainsSplitter_->SetWindowPos( NULL, 0, 0, cx, cy, SWP_NOZORDER );
	}

	RedrawWindow();
}


/**
 *	This method creates extra temporary nodes that the user will drag while
 *	cloning.  If the specified node is an Effect node, it will clone the Effect
 *	node and all its Phase nodes.  If it's a Phase, only the Phase node is
 *	cloned.
 *
 *	@param node	Node to clone.
 */
void PagePostProcessing::createExtraNodesForCloning( const BasePostProcessingNodePtr & node )
{
	BW_GUARD;

	if (node->effectNode())
	{
		EffectNode * origEffect = node->effectNode();
		EffectNodePtr effect = new EffectNode( origEffect->pyEffect().get(), this );
		effect->active( origEffect->active() );
		effect->chainPos( origEffect->chainPos() );
		graphView_.graph()->addNode( effect );
		Graph::NodeViewPtr effectNodeView = new EffectNodeView( graphView_, effect );
		effectNodeView->position( dragNodeOldPos_ );
		basePostProcessingNodes_.insert( std::make_pair( effect.get(), effect.get() ) );
		dragClonedNodes_.insert( effect );

		CSize offset;
		offset.cx = 0;
		offset.cy = ViewSkin::effectNodeSeparation().cy;

		PhaseNodeSet phasesSet;
		getPhaseNodes( origEffect, phasesSet );
		for(PhaseNodeSet::iterator it = phasesSet.begin(); it != phasesSet.end(); ++it)
		{
			PhaseNode * origPhase = (*it).get();
			PhaseNodePtr phase = new PhaseNode( origPhase->pyPhase().get(), this );
			phase->active( origPhase->active() );
			phase->chainPos( origPhase->chainPos() );
			graphView_.graph()->addNode( phase );
			Graph::NodeViewPtr phaseNodeView = new PhaseNodeView( graphView_, phase, preview_ );
			phaseNodeView->position( dragNodeOldPos_ + offset );
			basePostProcessingNodes_.insert( std::make_pair( phase.get(), phase.get() ) );
			dragClonedNodes_.insert( phase );
			offset.cy += ViewSkin::phaseNodeSeparation().cy;
		}
	}
	else if (node->phaseNode())
	{
		PhaseNode * origPhase = node->phaseNode();
		PhaseNodePtr phase = new PhaseNode( origPhase->pyPhase().get(), this );
		phase->active( origPhase->active() );
		phase->chainPos( origPhase->chainPos() );
		graphView_.graph()->addNode( phase );
		Graph::NodeViewPtr phaseNodeView = new PhaseNodeView( graphView_, phase, preview_ );
		phaseNodeView->position( dragNodeOldPos_ );
		basePostProcessingNodes_.insert( std::make_pair( phase.get(), phase.get() ) );
		dragClonedNodes_.insert( phase );

		// we need to remap the edges
		remapClonedEdges( node.get(), phase.get(), true );
	}
}


/**
 *	This method removes the temporary nodes created for cloning in a previous
 *	call to createExtraNodesForCloning. If the specified node is an Effect
 *	node, it will remove the temporary Effect node and all its Phase nodes.  If
 *	it's a Phase, only the temporary Phase node is removed.
 *
 *	@param node	Node to remove.
 */
void PagePostProcessing::removeExtraNodesForCloning( const BasePostProcessingNodePtr & node )
{
	BW_GUARD;

	if (!dragClonedNodes_.empty())
	{
		PhaseNodePtr phase;
		for (BasePostProcessingNodes::iterator it = dragClonedNodes_.begin();
			it != dragClonedNodes_.end(); ++it)
		{
			graphView_.unregisterNodeView( (*it).get() );
			graphView_.graph()->removeNode( (*it).get() );
			basePostProcessingNodes_.erase( (*it).get() );
			if (dragClonedNodes_.size() == 1 && (*it)->phaseNode())
			{
				phase = (*it)->phaseNode();
			}
		}
		dragClonedNodes_.clear();

		if (phase)
		{
			// we were cloning a phase, so restore the edges
			remapClonedEdges( phase.get(), node.get(), false );
		}
	}
}


/**
 *	This method updates edges between cloned nodes.
 *
 *	@param fromNode	Starting node.
 *	@param toNode	Ending node.
 *	@param addingEdges	Whether it needs to add or remove edges.
 */
void PagePostProcessing::remapClonedEdges( const Graph::NodePtr fromNode, const Graph::NodePtr toNode, bool addingEdges )
{
	BW_GUARD;

	for (DragEdgesMap::iterator it = dragNodeEdges_.begin();
		it != dragNodeEdges_.end(); ++it)
	{
		Graph::EdgePtr curEdge = (*it).first;
		Graph::NodePtr startNode = curEdge->start();
		Graph::NodePtr endNode = curEdge->end();
		if (startNode == fromNode.get() || endNode == fromNode.get())
		{
			if (startNode == fromNode.get())
			{
				graphView_.graph()->updateEdge( curEdge, toNode.get(), curEdge->end() );
			}
			else if (endNode == fromNode.get())
			{
				graphView_.graph()->updateEdge( curEdge, curEdge->start(), toNode.get() );
			}
		}

		if (addingEdges)
		{
			graphView_.registerEdgeView( curEdge.get(), (*it).second );
		}
		else
		{
			graphView_.unregisterEdgeView( curEdge.get() );
		}
	}
}


/**
 *	This method updates the caption bar's info.
 */
void PagePostProcessing::updateCaptionBar()
{
	BW_GUARD;

	if (Options::getOptionInt( PostProcCaptionBar::OPTION_PROFILE, 0 ) == 1)
	{
		captionDlg_.captionText(
			Localise( L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/CAPTIONBAR_TEXT_PROFILE",
						lastGPUTime_,
						renderTargetMBs_ ) );
	}
	else
	{
		captionDlg_.captionText(
			Localise( L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/CAPTIONBAR_TEXT",
						renderTargetMBs_ ) );
	}
}


/**
 *	This method updates the Phase popup when dragging a texture, shader or
 *	render target over the graph view, showing it if the mouse is over a phase
 *	that supports the item being dragged and hidding it if the mouse is over
 *	the background or over a phase that doesn't support it.
 *
 *	@param dropItemName	Name of the item being dragged.
 *	@param curPhase		Phase node the mouse is currenty over.
 *	@return True if the popup is being shown, false if not.
 */
bool PagePostProcessing::showHidePhasePopup( const BW::wstring & dropItemName, PhaseNodePtr curPhase )
{
	bool showPhasePopup = false;

	Graph::NodeViewPtr nodeView = graphView_.nodeView( curPhase );

	CRect nodeRect = nodeView->rect();
	nodeRect.OffsetRect( graphView_.panOffset() );

	dropBeforePhase_ = curPhase;

	int propsType = 0;

	if (startsWith( dropItemName, L"RT:" ))
	{
		propsType = PostProcPropertyEditor::RENDER_TARGETS;
		// Don't allow using the special "backBuffer" RT in textures.
		if (dropItemName.substr( 3 ) != bw_utf8tow( PhaseNode::BACK_BUFFER_STR ))
		{
			propsType |= PostProcPropertyEditor::TEXTURES;
		}
	}
	else if (isTexture( dropItemName ))
	{
		propsType = PostProcPropertyEditor::TEXTURES;
	}
	else // must be an fx file
	{
		propsType = PostProcPropertyEditor::SHADERS;
	}

	BW::vector< BW::string > validProperties;

	if (propsType > 0)
	{
		PostProcPropertyEditorPtr nodeEditor(
			new PostProcPropertyEditor( curPhase ), true /* steal the reference*/ );

		nodeEditor->getProperties( propsType, validProperties );
	}

	if (curPhase && !validProperties.empty())
	{
		if (curPhase != lastDropTargetPhase_)
		{
			phasePopup_.close();
		}

		if (phasePopup_.GetSafeHwnd() == NULL)
		{
			CRect screenRect( nodeRect );
			graphView_.ClientToScreen( screenRect );

			CPoint popupPos( (screenRect.left + screenRect.right) / 2, screenRect.bottom - 13 );

			CSize popupSize( phasePopup_.calcSize( validProperties ) );

			CRect popupRect( popupPos, popupSize );

			CRect graphViewRect;
			graphView_.GetWindowRect( graphViewRect );

			bool arrowUp = true;

			if (popupRect.bottom > graphViewRect.bottom)
			{
				arrowUp = false;
				popupPos.y = screenRect.top + 13 - popupSize.cy;
			}

			phasePopup_.open( popupPos, validProperties, arrowUp );
		}
		showPhasePopup = true;
	}
	return showPhasePopup;
}


/**
 *	This method is called when an shader, texture or render target is dropped
 *	on top of the Phase popup menu, in which case it will try to assign the
 *	dragged item to the popup menu item (property) under the mouse, if any.
 *
 *	@param dropItemName	Name of the item being dropped.
 */
void PagePostProcessing::onPhasePopupDrop( const BW::wstring & dropItemName )
{
	if (phasePopup_.GetSafeHwnd() != NULL && dropBeforePhase_ != NULL)
	{
		BW::string curProp = phasePopup_.update( PHASE_POPUP_ALPHA );
		
		for (int i = PHASE_POPUP_ALPHA; i > 0; i -= 10)
		{
			phasePopup_.update( i );
			Sleep( 10 );
		}

		phasePopup_.close();

		PostProcPropertyEditorPtr nodeEditor(
			new PostProcPropertyEditor( dropBeforePhase_ ), true /* steal the reference*/ );

		BW::string value;
		if (startsWith( dropItemName, L"RT:" ))
		{
			value = bw_wtoutf8( dropItemName.substr( 3 ) );
		}
		else
		{
			value = bw_wtoutf8( BWResource::dissolveFilenameW( dropItemName ) );
		}

		nodeEditor->setProperty( curProp, value );
	}
}
BW_END_NAMESPACE

