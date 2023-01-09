#include "pch.hpp"
#include "worldeditor/gui/pages/page_chunk_watcher.hpp"
#include "worldeditor/gui/pages/chunk_watcher.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/geometry_mapping.hpp"
#include "controls/utils.hpp"
#include "common/format.hpp"
#include "common/user_messages.hpp"
#include "appmgr/options.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	const int GROUP_EDGE		= 16; // gap between group control and children
	const int GROUP_EDGE_BOTTOM =  4; // less room is needed on the bottom

	const char *CHUNKS_OPTIONS_STR			= "chunk_watcher/chunks"; 
	const char *FRUSTUM_OPTIONS_STR			= "chunk_watcher/frustum";
	const char *GRID_OPTIONS_STR			= "chunk_watcher/grid";
	const char *UNLOADABLE_OPTIONS_STR		= "chunk_watcher/unloadable"; 
	const char *PROJECTVIEW_OPTIONS_STR		= "chunk_watcher/projectview"; 
}


BEGIN_MESSAGE_MAP(PageChunkWatcher, CDialog)
	ON_MESSAGE(WM_NEW_SPACE          , OnNewSpace       )
	ON_MESSAGE(WM_CHANGED_CHUNK_STATE, OnChangedChunk   )
	ON_MESSAGE(WM_WORKING_CHUNK      , OnNewWorkingChunk)
	ON_MESSAGE(WM_UPDATE_CONTROLS    , OnUpdateControls )
	ON_COMMAND(IDC_CHUNKS_CB     , OnUpdateOptions)
	ON_COMMAND(IDC_FRUSTUM_CB    , OnUpdateOptions)
	ON_COMMAND(IDC_GRID_CB       , OnUpdateOptions)
	ON_COMMAND(IDC_UNLOADABLE_CB , OnUpdateOptions)
	ON_COMMAND(IDC_PROJECTVIEW_CB, OnUpdateOptions)
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_MESSAGE(WM_CHUNK_WATCH_MOUSE_MOVE, OnMouseMoveChunkWatchControl)
END_MESSAGE_MAP()


const BW::wstring PageChunkWatcher::contentID = L"PageChunkWatcher";


/**
 *	This is the PageChunkWatcher constructor.
 */
PageChunkWatcher::PageChunkWatcher():
	CDialog(PageChunkWatcher::IDD),
	numLoaded_(0),
	numUnloaded_(0),
	numDirty_(0),
	numCalced_(0)
{
}


/**
 *	This is the PageChunkWatcher destructor.
 */
PageChunkWatcher::~PageChunkWatcher()
{
}


/**
 *	This is called to initialise the dialog.
 *
 *  @return			TRUE if successfully initialised.
 */
/*virtual*/ BOOL PageChunkWatcher::OnInitDialog()
{
	BW_GUARD;

	BOOL ok = CDialog::OnInitDialog();

	if (ok == FALSE)
		return ok;

	CRect extents;
	GetClientRect(extents);
	chunkWatchCtrl_.Create(extents, this);

	// Setup layout:
	controls::RowSizer *sizer = 
		new controls::RowSizer(controls::RowSizer::VERTICAL);
	// The chunk watch control:
	sizer->addChild(new controls::WndSizer(&chunkWatchCtrl_), 1, controls::RowSizer::WEIGHT);
	// Options group:
	controls::GridSizer *groupSizer1 =
		new controls::GridSizer(2, 3, this, IDC_OPTIONS_GROUP);
	groupSizer1->leftEdgeGap  (GROUP_EDGE       );
	groupSizer1->topEdgeGap   (GROUP_EDGE       );
	groupSizer1->rightEdgeGap (GROUP_EDGE       );
	groupSizer1->bottomEdgeGap(GROUP_EDGE_BOTTOM);
	groupSizer1->setChild(0, 0, new controls::WndSizer(this, IDC_CHUNKS_CB));
	groupSizer1->setChild(1, 0, new controls::WndSizer(this, IDC_FRUSTUM_CB));
	groupSizer1->setChild(0, 1, new controls::WndSizer(this, IDC_GRID_CB));
	groupSizer1->setChild(1, 1, new controls::WndSizer(this, IDC_UNLOADABLE_CB));
	groupSizer1->setChild(0, 2, new controls::WndSizer(this, IDC_PROJECTVIEW_CB));
	sizer->addChild(groupSizer1);
	// Statistics group:
	controls::GridSizer *groupSizer2 = 
		new controls::GridSizer(2, 3, this, IDC_STATISTICS_GRP);
	groupSizer2->leftEdgeGap  (GROUP_EDGE       );
	groupSizer2->topEdgeGap   (GROUP_EDGE       );
	groupSizer2->rightEdgeGap (GROUP_EDGE       );
	groupSizer2->bottomEdgeGap(GROUP_EDGE_BOTTOM);
	controls::RowSizer *row1 = 
		new controls::RowSizer(controls::RowSizer::HORIZONTAL);
	row1->addChild(new controls::WndSizer(&unloadClrStatic_));
	row1->addChild(new controls::WndSizer(this, IDC_UNLOADED_TEXT));
	row1->addChild(new controls::WndSizer(this, IDC_UNLOADED_NUM));
	groupSizer2->setChild(0, 0, row1);
	controls::RowSizer *row2 = 
		new controls::RowSizer(controls::RowSizer::HORIZONTAL);
	row2->addChild(new controls::WndSizer(&loadClrStatic_));
	row2->addChild(new controls::WndSizer(this, IDC_LOADED_TEXT));
	row2->addChild(new controls::WndSizer(this, IDC_LOADED_NUM));
	groupSizer2->setChild(0, 1, row2);
	controls::RowSizer *row3 = 
		new controls::RowSizer(controls::RowSizer::HORIZONTAL);
	row3->addChild(new controls::WndSizer(&dirtyClrStatic_));
	row3->addChild(new controls::WndSizer(this, IDC_NEEDSSHADOW_TEXT));
	row3->addChild(new controls::WndSizer(this, IDC_NEEDSSHADOW_NUM));
	groupSizer2->setChild(1, 0, row3);
	controls::RowSizer *row4 = 
		new controls::RowSizer(controls::RowSizer::HORIZONTAL);
	row4->addChild(new controls::WndSizer(&shadowedClrStatic_));
	row4->addChild(new controls::WndSizer(this, IDC_DONESHADOW_TEXT));
	row4->addChild(new controls::WndSizer(this, IDC_DONESHADOW_NUM));
	groupSizer2->setChild(1, 1, row4);
	groupSizer2->setChild(0, 2, new controls::WndSizer(this, IDC_MOUSEPOS));
	groupSizer2->setChild(1, 2, new controls::WndSizer(this, IDC_CHUNKID));
	sizer->addChild(groupSizer2);
	rootSizer_ = sizer;

	// Set the colours:
	unloadClrStatic_  .SetBkColour(ChunkWatchControl::loadColour  ());
	loadClrStatic_    .SetBkColour(ChunkWatchControl::unloadColour());
	dirtyClrStatic_   .SetBkColour(ChunkWatchControl::dirtyColour ());
	shadowedClrStatic_.SetBkColour(ChunkWatchControl::calcedColour());

	// Set the state of the check boxes:
	setDisplayOption
	(
		IDC_CHUNKS_CB, 
		CHUNKS_OPTIONS_STR, 
		ChunkWatchControl::DRAW_CHUNKS
	);
	setDisplayOption
	(
		IDC_FRUSTUM_CB, 
		FRUSTUM_OPTIONS_STR, 
		ChunkWatchControl::DRAW_FRUSTUM
	);
	setDisplayOption
	(
		IDC_GRID_CB, 
		GRID_OPTIONS_STR, 
		ChunkWatchControl::DRAW_GRID
	);
	setDisplayOption
	(
		IDC_UNLOADABLE_CB, 
		UNLOADABLE_OPTIONS_STR, 
		ChunkWatchControl::DRAW_UNLOADABLE
	);
	setDisplayOption
	(
		IDC_PROJECTVIEW_CB, 
		PROJECTVIEW_OPTIONS_STR, 
		ChunkWatchControl::DRAW_PROJECTVIEW
	);

	OnNewSpace(0, 0);

	INIT_AUTO_TOOLTIP();

	return ok;
}



/**
 *	This is used for data validation.  We use this functionality to subclass
 *	some controls.
 */
/*virtual*/ void PageChunkWatcher::DoDataExchange(CDataExchange* pDX)
{
	BW_GUARD;

	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_UNLOADED_CLR   , unloadClrStatic_  );
	DDX_Control(pDX, IDC_LOADED_CLR     , loadClrStatic_    );
	DDX_Control(pDX, IDC_NEEDSSHADOW_CLR, dirtyClrStatic_   );
	DDX_Control(pDX, IDC_DONESHADOW_CLR , shadowedClrStatic_);
}


/**
 *	This is called when a new space is loaded.
 *
 *  @param wparam	Not used.  Here to match a function definition.
 *  @param lparam	Not used.  Here to match a function definition.
 *	@return			0.  Not used.  Here to match a function definition.
 */
/*afx_msg*/ LRESULT 
PageChunkWatcher::OnNewSpace
(
	WPARAM		/*wparam*/, 
	LPARAM		/*lparam*/
)
{
	BW_GUARD;

	GeometryMapping *mapping = WorldManager::instance().geometryMapping();
	if (mapping != NULL)
	{
		chunkWatchCtrl_.onNewSpace
		(
			mapping->pSpace()->gridSize(),
			mapping->minLGridX(), 
			mapping->minLGridY(), 
			mapping->maxLGridX() + 1, 
			mapping->maxLGridY() + 1
		);
	}
	return 0;
}


/**
 *	This is called when a chunk's state is changed.
 *
 *  @param wparam	The x coordinate of the changed chunk.
 *  @param lparam	The z coordinate of the changed chunk.
 *	@return			0.  Not used.  Here to match a function definition.
 */
/*afx_msg*/ LRESULT
PageChunkWatcher::OnChangedChunk
(
	WPARAM		wparam, 
	LPARAM		lparam
)
{
	BW_GUARD;

	int16 x = (int16)wparam;
	int16 z = (int16)lparam;
	chunkWatchCtrl_.onChangedChunk(x, z);
	return 0;
}


/**
 *	This is called when a chunk is being worked on by WorldEditor for saving or some
 *	calculations etc.
 *
 *  @param wparam	Not used.  Here to match a function definition.
 *  @param lparam	Not used.  Here to match a function definition.
 *	@return			0.  Not used.  Here to match a function definition.
 */
/*afx_msg*/ LRESULT PageChunkWatcher::OnNewWorkingChunk(WPARAM /*wparam*/, LPARAM /*lparam*/)
{
	BW_GUARD;

	chunkWatchCtrl_.RedrawWindow();
	return 0;
}


/**
 *	This is called on every frame.
 *
 *  @param wparam	Not used.  Here to match a function definition.
 *  @param lparam	Not used.  Here to match a function definition.
 *	@return			0.  Not used.  Here to match a function definition.
 */
/*afx_msg*/ LRESULT 
PageChunkWatcher::OnUpdateControls
(
	WPARAM		/*wparam*/, 
	LPARAM		/*lparam*/
)
{
	BW_GUARD;

	if (chunkWatchCtrl_.GetSafeHwnd() != NULL)
		chunkWatchCtrl_.OnUpdateControl();
	ChunkWatcher const &cw = WorldManager::instance().chunkWatcher();
	if (numUnloaded_ != cw.numberUnloadedChunks())
	{
		numUnloaded_ = cw.numberUnloadedChunks();
		controls::setWindowText(*this, IDC_UNLOADED_NUM, sformat("{0}", numUnloaded_));
	}
	if (numLoaded_ != cw.numberLoadedChunks())
	{
		numLoaded_ = cw.numberLoadedChunks();
		controls::setWindowText(*this, IDC_LOADED_NUM, sformat("{0}", numLoaded_));
	}
	if (numDirty_ != cw.numberDirtyChunks())
	{
		numDirty_ = cw.numberDirtyChunks();
		controls::setWindowText(*this, IDC_NEEDSSHADOW_NUM, sformat("{0}", numDirty_));
	}
	if (numCalced_ != cw.numberCalcedChunks())
	{
		numCalced_ = cw.numberCalcedChunks();
		controls::setWindowText(*this, IDC_DONESHADOW_NUM, sformat("{0}", numCalced_));
	}

	return 0;
}


/**
 *	This is called when the user selects an options check box.
 */
void PageChunkWatcher::OnUpdateOptions()
{
	BW_GUARD;

	getDisplayOption
	(
		IDC_CHUNKS_CB, 
		CHUNKS_OPTIONS_STR, 
		ChunkWatchControl::DRAW_CHUNKS
	);
	getDisplayOption
	(
		IDC_FRUSTUM_CB, 
		FRUSTUM_OPTIONS_STR, 
		ChunkWatchControl::DRAW_FRUSTUM
	);
	getDisplayOption
	(
		IDC_GRID_CB, 
		GRID_OPTIONS_STR, 
		ChunkWatchControl::DRAW_GRID
	);
	getDisplayOption
	(
		IDC_UNLOADABLE_CB, 
		UNLOADABLE_OPTIONS_STR, 
		ChunkWatchControl::DRAW_UNLOADABLE
	);
	getDisplayOption
	(
		IDC_PROJECTVIEW_CB, 
		PROJECTVIEW_OPTIONS_STR, 
		ChunkWatchControl::DRAW_PROJECTVIEW
	);
	chunkWatchCtrl_.RedrawWindow();
}


/**
 *	This is called when the dialog is resized.
 *
 *  @param type		The type of resizing operation.
 *  @param cx		The new width.
 *  @param cy		The new height.
 */
/*afx_msg*/ void PageChunkWatcher::OnSize(UINT type, int cx, int cy)
{
	BW_GUARD;

	if (rootSizer_ != NULL)
		rootSizer_->onSize(cx, cy);
	CDialog::OnSize(type, cx, cy);
	Invalidate();
}


/**
 *	This is called to get the min/max information.
 *
 *  @param mmi		The min/max information.
 */
/*afx_msg*/ void PageChunkWatcher::OnGetMinMaxInfo(MINMAXINFO *mmi)
{
	BW_GUARD;

	CDialog::OnGetMinMaxInfo(mmi);
	if (rootSizer_ != NULL)
		rootSizer_->onGetMinMaxInfo(mmi);
}


/**
 *	This is called the mouse is moved over the ChunkWatchControl.
 *
 *  @param wparam	Not used.  Here to match a function definition.
 *  @param lparam	Not used.  Here to match a function definition.
 *	@return			0.  Not used.  Here to match a function definition.
 */
/*afx_msg*/ LRESULT 
PageChunkWatcher::OnMouseMoveChunkWatchControl
(
	WPARAM		/*wparam*/, 
	LPARAM		/*lparam*/
)
{
	BW_GUARD;

	BW::string mouseText = LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_CHUNK_WATCHER/POSITION");
	BW::string chunkText = LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_CHUNK_WATCHER/CHUNK");
	BW::string mouseInfo;
	BW::string chunkInfo;		
	CPoint point;
	::GetCursorPos(&point);
	chunkWatchCtrl_.ScreenToClient(&point);
	if (chunkWatchCtrl_.getInfoText(point, chunkInfo, mouseInfo))
	{
		mouseText += mouseInfo;
		chunkText += chunkInfo;		
	}
	controls::setWindowText(*this, IDC_MOUSEPOS, mouseText.c_str() );
	controls::setWindowText(*this, IDC_CHUNKID , chunkText.c_str() );
	return 0;
}


/** 
 *	This function sets the options file to match whether a given option is 
 *	turned on or off.
 *
 *  @param id			Id the of control displaying the option.
 *  @param optionText	The text of the option in the options file.
 *  @param value		The value of the option as a ChunkWatchControl display
 *						option.
 */
void PageChunkWatcher::setDisplayOption
(
	UINT		id, 
	BW::string const &optionText,
	uint32		value
)
{
	BW_GUARD;

	bool enabled = Options::getOptionBool(optionText, true);
	controls::checkButton(*this, id, enabled);
	if (enabled)
		chunkWatchCtrl_.enableDrawOptions(value, false); // don't force redraw
	else
		chunkWatchCtrl_.disableDrawOptions(value, false); // don't force redraw
}


/** 
 *	This function sets controls to match the given option in the options file.
 *
 *  @param id			Id the of control displaying the option.
 *  @param optionText	The text of the option in the options file.
 *  @param value		The value of the option as a ChunkWatchControl display
 *						option.
 */
void PageChunkWatcher::getDisplayOption
(
	UINT		id, 
	BW::string const &optionText,
	uint32		value
)
{
	BW_GUARD;

	bool enabled = controls::isButtonChecked(*this, id);
	Options::setOptionBool(optionText, enabled);
	if (enabled)
		chunkWatchCtrl_.enableDrawOptions(value, false); // don't force redraw
	else
		chunkWatchCtrl_.disableDrawOptions(value, false); // don't force redraw
}
BW_END_NAMESPACE

