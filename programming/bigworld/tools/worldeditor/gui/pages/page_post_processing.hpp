#ifndef PAGE_POST_PROCESSING_HPP
#define PAGE_POST_PROCESSING_HPP

#include "math/rectt.hpp"
#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "post_processing_chains.hpp"
#include "post_processing_properties.hpp"
#include "post_proc_caption_bar.hpp"
#include "guitabs/guitabs_content.hpp"
#include "graph/graph_view.hpp"
#include "gui/post_processing/node_callback.hpp"
#include "gui/post_processing/popup_drag_target.hpp"
#include "gui/post_processing/pp_preview.hpp"
#include <afxwin.h>
#include <afxcmn.h>

BW_BEGIN_NAMESPACE

// Forward declarations
namespace PostProcessing
{
	class Effect;
}

namespace Graph
{
	class Edge;
	class EdgeView;
	typedef SmartPointer< EdgeView > EdgeViewPtr;
}

class EffectNode;
typedef SmartPointer< EffectNode > EffectNodePtr;

class PhaseNode;
typedef SmartPointer< PhaseNode > PhaseNodePtr;


class BasePostProcessingNode;
typedef SmartPointer< BasePostProcessingNode > BasePostProcessingNodePtr;


class SequenceEditor;
typedef SmartPointer< SequenceEditor > SequenceEditorPtr;


class NiceSplitterWnd;


/**
 *	This class implements the Post-Processing panel.
 */
class PagePostProcessing : public CDialog, public GUITABS::Content, public NodeCallback
{
	IMPLEMENT_LOADABLE_CONTENT( Localise(L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/SHORT_NAME"),
		Localise(L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/LONG_NAME"), 280, 400, NULL )

public:
	enum { IDD = IDD_PAGE_POST_PROCESSING };

	PagePostProcessing();
	virtual ~PagePostProcessing();

	bool load( DataSectionPtr section );
	bool save( DataSectionPtr section );

	// NodeCallback interface
	void nodeClicked( const BasePostProcessingNodePtr & node );
	void nodeActive( const BasePostProcessingNodePtr & node, bool active );
	void nodeDeleted( const BasePostProcessingNodePtr & node );
	void doDrag( const CPoint & pt, const BasePostProcessingNodePtr & node );
	void endDrag( const CPoint & pt, const BasePostProcessingNodePtr & node, bool canceled = false );

protected:
	virtual void DoDataExchange( CDataExchange * pDX );    // DDX/DDV support

	virtual BOOL OnInitDialog();

	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg LRESULT OnUpdateControls( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnZoomIn( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnZoomOut( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnNoZoom( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT On3dPreview( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnProfile( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnLayout( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnDeleteAll( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnChainSelected( WPARAM wParam, LPARAM lParam );
	DECLARE_MESSAGE_MAP()

	bool onGraphViewDrop( UalItemInfo * ii );
	RectInt onGraphViewDropTest( UalItemInfo *ii );

	bool onPropertiesDrop( UalItemInfo * ii );
	RectInt onPropertiesDropTest( UalItemInfo *ii );

	bool graphViewAutoPan( const CPoint & pt );

private:
	typedef BW::set< PhaseNodePtr > PhaseNodeSet;

	typedef BW::set< BasePostProcessingNodePtr > BasePostProcessingNodes;

	typedef BW::map< const Graph::Node *, BasePostProcessingNodePtr > BasePostProcessingNodesMap;

	typedef BW::map< Graph::Edge *, Graph::EdgeViewPtr > DragEdgesMap;


	// Different layout styles for arranging the internal sub panels.
	enum PanelsLayout
	{
	 NORMAL, // Chains and properties to the left, graph to the right.
	 WIDE, // Chains to the left, graph in the middle, properties to the right.
	 TALL  // Chains at the top, graph in the middle, properties to the bottom.
	};


	/**
	 *	This internal class serves as a parent for the graph view, working as 
	 *	a scrollable viewport into a potentially much bigger graph view, and
	 *	also manages a caption bar and vertical and horizontal scroll bars.
	 */
	class GraphContainer : public CWnd
	{
	public:
		GraphContainer();

		void setChildren( Graph::GraphView * graphView, CWnd * captionBar, CScrollBar * hScroll, CScrollBar * vScroll );

		void ignoreScrolling( bool ignore ) { ignoreScrolling_ = ignore; }

		afx_msg void OnSize( UINT nType, int cx, int cy );
		afx_msg void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar * pScrollBar );
		afx_msg void OnVScroll( UINT nSBCode, UINT nPos, CScrollBar * pScrollBar );
		DECLARE_MESSAGE_MAP()

	private:
		bool ignoreScrolling_;
		Graph::GraphView * graphView_;
		CWnd * captionBar_;
		CScrollBar * hScroll_;
		CScrollBar * vScroll_;
		int captionBarHeight_;
	};


	PostProcessingChains chainsDlg_;
	PostProcessingProperties propertiesDlg_;
	PostProcCaptionBar captionDlg_;
	Graph::GraphView graphView_;
	GraphContainer graphContainer_;
	CScrollBar vertScroll_;
	CScrollBar horzScroll_;
	NiceSplitterWnd * pChainsSplitter_;
	NiceSplitterWnd * pGraphSplitter_;
	PanelsLayout layout_;
	int chainsSplitterSize_;
	int graphSplitterSize_;

	PyObjectPtr pEditorPhases_;

	bool updateGraph_;

	int curZoom_;
	bool zoomToCentre_;
	bool zoomToTopRight_;

	int graphWidth_;
	int graphHeight_;
	float renderTargetMBs_;
	double lastGPUTime_;

	static bool s_phaseChanged_;
	static bool s_phaseChangedReload_;

	BasePostProcessingNodesMap basePostProcessingNodes_;

	BasePostProcessingNodePtr currentNode_;

	EffectNodePtr dropBeforeEffect_;
	PhaseNodePtr dropBeforePhase_;
	PhaseNode * lastDropTargetPhase_;

	PopupDragTarget phasePopup_;

	bool dragging_;
	bool cloning_;
	bool draggingRT_;
	CPoint dragNodeOldPos_;
	BasePostProcessingNodes dragClonedNodes_;
	DragEdgesMap dragNodeEdges_;

	PPPreviewPtr preview() const	{ return preview_; }
	PPPreviewPtr preview_;

	static void phaseChanged( bool needsReload );

	bool buildGraph( bool redraw = true );
	EffectNodePtr buildEffect( int col, PostProcessing::Effect * pyEffect, const EffectNodePtr & prevEffect );
	bool buildPhases( int col, const EffectNodePtr & effect, PostProcessing::Effect * pyEffect );

	bool getPhaseNodes( const BasePostProcessingNodePtr & effectNode, PhaseNodeSet & retPhases ) const;

	bool editEffectChain( SequenceEditorPtr effectEditor );
	bool editEffectPhases( EffectNodePtr effectNode, SequenceEditorPtr phaseEditor, bool addBarrier = true );

	CDC * beginDrawDrag();
	void endDrawDrag( CDC * pScreenDC );
	void drawDragRect( const CRect & rect );

	EffectNodePtr getPhaseEffect( PhaseNodePtr phaseNode );
	
	PhaseNodePtr getPhaseByPt( const CPoint & pt ) const;

	CRect findEffectDropPoint( const CPoint & pt, const BasePostProcessingNodePtr & skipNode );
	CRect findPhaseDropPoint( const CPoint & pt, const BasePostProcessingNodePtr & skipNode );

	void setLayout( PanelsLayout layout, int cx, int cy, bool clearSizes = true );

	void createExtraNodesForCloning( const BasePostProcessingNodePtr & node );
	void removeExtraNodesForCloning( const BasePostProcessingNodePtr & node );

	void remapClonedEdges( const Graph::NodePtr fromNode, const Graph::NodePtr toNode, bool addingEdges );

	void updateCaptionBar();

	bool showHidePhasePopup( const BW::wstring & dropItemName, PhaseNodePtr curPhase );
	void onPhasePopupDrop( const BW::wstring & dropItemName );
};


IMPLEMENT_CDIALOG_CONTENT_FACTORY( PagePostProcessing, PagePostProcessing::IDD )

BW_END_NAMESPACE

#endif // PAGE_POST_PROCESSING_HPP
