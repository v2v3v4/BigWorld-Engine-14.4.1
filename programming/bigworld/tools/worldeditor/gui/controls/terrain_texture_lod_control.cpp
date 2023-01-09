#include "pch.hpp"
#include "worldeditor/gui/controls/terrain_texture_lod_control.hpp"
#include "worldeditor/terrain/space_recreator.hpp"
#include "worldeditor/misc/progress_bar_helper.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "appmgr/options.hpp"
#include "resmgr/datasection.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/auto_config.hpp"
#include "chunk/chunk_format.hpp"
#include "chunk/chunk_clean_flags.hpp"
#include "controls/dir_dialog.hpp"
#include "controls/utils.hpp"
#include "chunk/chunk_grid_size.hpp"
#include "terrain/terrain_settings.hpp"
#include "terrain/terrain_texture_layer.hpp"
#include "terrain/terrain2/editor_terrain_block2.hpp"
#include "moo/effect_visual_context.hpp"
#include "cstdmf/debug.hpp"
#include "editor_shared/dialogs/file_dialog.hpp"
#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	const float LOD_EPSILON	=   0.1f;	// minimum precision required for a texture lod change

	// This class is for terrain lod undo/redo operations.
	class TerrainLodUndo : public UndoRedo::Operation
	{
	public:
		TerrainLodUndo( TerrainTextureLodControl * terrainTextureLodDlg );

		/*virtual*/ void undo();

		/*virtual*/ bool iseq(UndoRedo::Operation const &other) const;

	private:
		float			lodTextureStart_;
		float			lodTextureDistance_;
		float			blendPreloadDistance_;
		bool			environmentChanged_;
		TerrainTextureLodControl * terrainTextureLodDlg_;
	};

	TerrainLodUndo::TerrainLodUndo( TerrainTextureLodControl * terrainTextureLodDlg ):
		UndoRedo::Operation((size_t)(typeid(TerrainLodUndo).name())),
		lodTextureStart_     ( terrainTextureLodDlg->terrainSettings()->lodTextureStart     ()),
		lodTextureDistance_  ( terrainTextureLodDlg->terrainSettings()->lodTextureDistance  ()),
		blendPreloadDistance_( terrainTextureLodDlg->terrainSettings()->blendPreloadDistance()),
		terrainTextureLodDlg_( terrainTextureLodDlg ),
		environmentChanged_( WorldManager::instance().hasEnvironmentChanged() )
	{
	}

	void TerrainLodUndo::undo()
	{
		BW_GUARD;

		// Save the current state to the undo/redo stack:
		UndoRedo::instance().add(new TerrainLodUndo( terrainTextureLodDlg_ ) );

		// Restore the saved texture lod values:
		Terrain::TerrainSettingsPtr pTerrainSettings
			= terrainTextureLodDlg_->terrainSettings();

		pTerrainSettings->lodTextureStart     (lodTextureStart_     );
		pTerrainSettings->lodTextureDistance  (lodTextureDistance_  );
		pTerrainSettings->blendPreloadDistance(blendPreloadDistance_);

		// Force an update of the page if it exists:
		if ( terrainTextureLodDlg_ != NULL )
		{
			terrainTextureLodDlg_->reinitialise();
		}
		WorldManager::instance().environmentChanged( environmentChanged_ );
	}

	/*virtual*/ bool 
		TerrainLodUndo::iseq
		(
		UndoRedo::Operation         const &other
		) const
	{
		return false;
	}
}


bool TerrainTextureLodControl::wasSlider( CWnd const & testScrollBar,
	CWnd const & scrollBar )
{
	BW_GUARD;

	return 
		testScrollBar.GetSafeHwnd() == scrollBar.GetSafeHwnd()
		&&
		::IsWindow(scrollBar.GetSafeHwnd()) != FALSE;
}


BW::set< TerrainTextureLodControl * > TerrainTextureLodControl::s_dialogs_;

// TerrainTextureLod dialog

IMPLEMENT_DYNAMIC(TerrainTextureLodControl, controls::EmbeddableCDialog)
TerrainTextureLodControl::TerrainTextureLodControl(
	size_t & filterChange,
	ITerrainTextureLodValueChangedListener * listener )
	: EmbeddableCDialog( IDD )
	, filterChange_( filterChange )
	, valueChangedListener_( listener )
	, sliding_( false )
	, changesMade_( 0 )
	, disableInitialize_( false )
{
	s_dialogs_.insert( this );
}

TerrainTextureLodControl::~TerrainTextureLodControl()
{
	s_dialogs_.erase( this );
}


void TerrainTextureLodControl::initializeToolTips()
{
	INIT_AUTO_TOOLTIP();
}

void TerrainTextureLodControl::terrainSettings(
	Terrain::TerrainSettingsPtr terrainSettings )
{
	terrainSettings_ = terrainSettings;
}


Terrain::TerrainSettingsPtr TerrainTextureLodControl::terrainSettings() const
{
	if( terrainSettings_ == NULL )
	{
		return WorldManager::instance().pTerrainSettings();
	}
	return terrainSettings_;
}


/*virtual */void TerrainTextureLodControl::DoDataExchange(CDataExchange* pDX)
{
	controls::EmbeddableCDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TEXLOD_START_EDIT      , texLodStartEdit_         );
	DDX_Control(pDX, IDC_TEXLOD_START_SLIDER    , texLodStartSlider_       );
	DDX_Control(pDX, IDC_TEXLOD_DIST_EDIT       , texLodDistEdit_          );
	DDX_Control(pDX, IDC_TEXLOD_DIST_SLIDER     , texLodDistSlider_        );
	DDX_Control(pDX, IDC_TEXLOD_PRELOAD_EDIT    , texLodPreloadEdit_       );
	DDX_Control(pDX, IDC_TEXLOD_PRELOAD_SLIDER  , texLodPreloadSlider_     );
}

/*afx_msg */void TerrainTextureLodControl::OnHScroll(
	UINT sbcode, UINT pos, CScrollBar *scrollBar)
{
	BW_GUARD;

	controls::EmbeddableCDialog::OnHScroll(sbcode, pos, scrollBar);

	CWnd *wnd = (CWnd *)scrollBar;

	if( wasSlider(*wnd, texLodStartSlider_) ||
		wasSlider(*wnd, texLodDistSlider_) ||
		wasSlider(*wnd, texLodPreloadSlider_) )
	{
		// Work out whether this is the first part of a slider movement,
		// in the middle of a slider movement or the end of a slider movement:
		SliderMovementState sms = SMS_MIDDLE;
		if (!sliding_)
		{
			sms = SMS_STARTED;
			sliding_ = true;
		}
		if (sbcode == TB_ENDTRACK)
		{
			sms = SMS_DONE;
			sliding_ = false;
		}

		OnTexLodSlider( sms );
	}
}

/**
 *  This is called when each item is about to be drawn.  We want limit slider edit
 *	to be highlighted is they are out of bounds.
 *
 *	@param pDC	Contains a pointer to the display context for the child window.
 *	@param pWnd	Contains a pointer to the control asking for the color.
 *	@param nCtlColor	Specifies the type of control.
 *	@return	A handle to the brush that is to be used for painting the control
 *			background.
 */
/*afx_msg*/ HBRUSH TerrainTextureLodControl::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	BW_GUARD;

	HBRUSH brush = controls::EmbeddableCDialog::OnCtlColor( pDC, pWnd, nCtlColor );

	texLodStartEdit_.SetBoundsColour( pDC, pWnd,
		texLodStartSlider_.getMinRangeLimit(), texLodStartSlider_.getMaxRangeLimit() );
	texLodDistEdit_.SetBoundsColour( pDC, pWnd,
		texLodDistSlider_.getMinRangeLimit(), texLodDistSlider_.getMaxRangeLimit() );
	texLodPreloadEdit_.SetBoundsColour( pDC, pWnd,
		texLodPreloadSlider_.getMinRangeLimit(), texLodPreloadSlider_.getMaxRangeLimit() );

	return brush;
}


BEGIN_MESSAGE_MAP(TerrainTextureLodControl, controls::EmbeddableCDialog)
	ON_WM_SETCURSOR()
	ON_WM_HSCROLL()
	ON_WM_CTLCOLOR()
	ON_EN_CHANGE(IDC_TEXLOD_START_EDIT  , OnTexLodEdit      )
	ON_EN_CHANGE(IDC_TEXLOD_DIST_EDIT   , OnTexLodEdit      )
	ON_EN_CHANGE(IDC_TEXLOD_PRELOAD_EDIT, OnTexLodEdit      )
	ON_EN_KILLFOCUS(IDC_TEXLOD_START_EDIT  , OnTexLodStartEditKillFocus		)
	ON_EN_KILLFOCUS(IDC_TEXLOD_DIST_EDIT   , OnTexLodBlendEditKillFocus		)
	ON_EN_KILLFOCUS(IDC_TEXLOD_PRELOAD_EDIT, OnTexLodPreloadEditKillFocus	)
END_MESSAGE_MAP()


void TerrainTextureLodControl::reinitialise()
{
	if(IsWindow( *this ) == false||
		disableInitialize_)
	{
		return;
	}
	texLodStartEdit_  .SetAllowNegative(false);
	texLodDistEdit_   .SetAllowNegative(false);
	texLodPreloadEdit_.SetAllowNegative(false);

	texLodStartEdit_  .SetNumDecimals(1);
	texLodDistEdit_   .SetNumDecimals(1);
	texLodPreloadEdit_.SetNumDecimals(1);

	texLodStartSlider_.setDigits( 1 );
	texLodStartSlider_.setRangeLimit( 0.0f, 5000.0f );
	texLodStartSlider_.setRange( 0.0f, 5000.0f );

	texLodDistSlider_.setDigits( 1 );
	texLodDistSlider_.setRangeLimit( 0.0f, 5000.0f );
	texLodDistSlider_.setRange( 0.0f, 5000.0f );

	texLodPreloadSlider_.setDigits( 1 );
	texLodPreloadSlider_.setRangeLimit( 0.0f, 5000.0f );
	texLodPreloadSlider_.setRange( 0.0f, 5000.0f );

	if (terrainSettings()->version() >=
		Terrain::TerrainSettings::TERRAIN2_VERSION)
	{
		texLodStartEdit_    .EnableWindow(TRUE);
		texLodStartSlider_  .EnableWindow(TRUE);
		texLodDistEdit_     .EnableWindow(TRUE);
		texLodDistSlider_   .EnableWindow(TRUE);
		texLodPreloadEdit_  .EnableWindow(TRUE);
		texLodPreloadSlider_.EnableWindow(TRUE);

		float texLodStart	= terrainSettings()->lodTextureStart();
		float texLodDist	= terrainSettings()->lodTextureDistance();
		float texLodPreload	= terrainSettings()->blendPreloadDistance();

		texLodStartEdit_    .SetValue(texLodStart);
		texLodStartSlider_  .setValue(texLodStart);
		texLodDistEdit_     .SetValue(texLodDist);
		texLodDistSlider_   .setValue(texLodDist);
		texLodPreloadEdit_  .SetValue(texLodPreload);
		texLodPreloadSlider_.setValue(texLodPreload);
	}
	else
	{
		texLodStartEdit_    .EnableWindow(FALSE);
		texLodStartSlider_  .EnableWindow(FALSE);
		texLodDistEdit_     .EnableWindow(FALSE);
		texLodDistSlider_   .EnableWindow(FALSE);
		texLodPreloadEdit_  .EnableWindow(FALSE);
		texLodPreloadSlider_.EnableWindow(FALSE);
	}
}


void TerrainTextureLodControl::restoreSettings( bool disableInitialize )
{
	BW_GUARD;
	filterChange_++;
	disableInitialize_ = disableInitialize;
	for( size_t i = 0; i < changesMade_; ++i )
	{
		UndoRedo::instance().undo();
	}
	syncAllDialogs();
	changesMade_ = 0;
	disableInitialize_ = false;
	filterChange_--;
}


void TerrainTextureLodControl::OnTexLodSlider(SliderMovementState sms)
{
	BW_GUARD;

	if (filterChange_ == 0)
	{
		++filterChange_;   

		// If started sliding then create and undo/redo block and tell WE
		// that the enviroment has changed.
		if (sms == SMS_STARTED)
		{
			changesMade_++;
			// Add a new undo/redo state:
			UndoRedo::instance().add(new TerrainLodUndo( this ));
			UndoRedo::instance().barrier
				(
				LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/SET_TERRAINLOD"), 
				false
				);

			// Tell WorldEditor that the environment has changed, and so saving 
			// should be done.
			WorldManager::instance().environmentChanged();
		}

		float texLodStart	= texLodStartSlider_  .getValue();
		float texLodDist	= texLodDistSlider_   .getValue();
		float texLodPreload	= texLodPreloadSlider_.getValue();

		float oldTexLodStart   = terrainSettings()->lodTextureStart     ();
		float oldTexLodDist    = terrainSettings()->lodTextureDistance  ();
		float oldTexLodPreload = terrainSettings()->blendPreloadDistance();

		// Update the edits:
		texLodStartEdit_  .SetValue(texLodStart	 );
		texLodDistEdit_   .SetValue(texLodDist	 );
		texLodPreloadEdit_.SetValue(texLodPreload);

		// Let the settings know about the changes:
		terrainSettings()->lodTextureStart     (texLodStart  );
		terrainSettings()->lodTextureDistance  (texLodDist   );
		terrainSettings()->blendPreloadDistance(texLodPreload);

		syncAllDialogs();

		--filterChange_;
	}
}


void TerrainTextureLodControl::OnTexLodEdit()
{
	BW_GUARD;

	if (filterChange_ == 0)
	{
		++filterChange_;

		float texLodStart	= texLodStartEdit_  .GetValue();
		float texLodDist	= texLodDistEdit_   .GetValue();
		float texLodPreload	= texLodPreloadEdit_.GetValue();

		float oldTexLodStart   = terrainSettings()->lodTextureStart     ();
		float oldTexLodDist    = terrainSettings()->lodTextureDistance  ();
		float oldTexLodPreload = terrainSettings()->blendPreloadDistance();

		// Filter out non-changes:
		if 
			(
			!almostEqual(texLodStart   ,oldTexLodStart  , LOD_EPSILON)
			|| 
			!almostEqual(texLodDist    ,oldTexLodDist   , LOD_EPSILON)
			||
			!almostEqual(texLodPreload ,oldTexLodPreload, LOD_EPSILON)
			)
		{
			changesMade_++;
			// Add a new undo/redo state:
			UndoRedo::instance().add(new TerrainLodUndo( this ));
			UndoRedo::instance().barrier
				(
				LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/SET_TERRAINLOD"), 
				false
				);

			// Update the sliders:
			texLodStartSlider_  .setValue(texLodStart);
			texLodDistSlider_   .setValue(texLodDist);
			texLodPreloadSlider_.setValue(texLodPreload);

			// Let the settings know about the changes:
			terrainSettings()->lodTextureStart     (texLodStartSlider_  .getValue());
			terrainSettings()->lodTextureDistance  (texLodDistSlider_   .getValue());
			terrainSettings()->blendPreloadDistance(texLodPreloadSlider_.getValue());

			// Tell WorldEditor that the environment has changed, and so saving 
			// should be done.
			WorldManager::instance().environmentChanged();
		}

		syncAllDialogs();

		--filterChange_;
	}
}

/**
 *	Called when the texture LOD start edit field loses focus.
 */
/*afx_msg*/ void TerrainTextureLodControl::OnTexLodStartEditKillFocus()
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;

        texLodStartEdit_.SetValue( texLodStartSlider_.getValue() );

        --filterChange_;        
    }
}


/**
 *	Called when the texture LOD blend edit field loses focus.
 */
/*afx_msg*/ void TerrainTextureLodControl::OnTexLodBlendEditKillFocus()
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;

        texLodDistEdit_.SetValue( texLodDistSlider_.getValue() );

        --filterChange_;        
    }
}


/**
 *	Called when the texture LOD preload edit field loses focus.
 */
/*afx_msg*/ void TerrainTextureLodControl::OnTexLodPreloadEditKillFocus()
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;

        texLodPreloadEdit_.SetValue( texLodPreloadSlider_.getValue() );

        --filterChange_;        
    }
}


void TerrainTextureLodControl::syncAllDialogs()
{
	BW::set< TerrainTextureLodControl * >::iterator it = s_dialogs_.begin();
	for( ; it != s_dialogs_.end(); ++it )
	{
		if( *it == this )
		{
			continue;
		}
		(*it)->filterChange_++;
		(*it)->reinitialise();
		(*it)->filterChange_--;
	}
	if( valueChangedListener_ != NULL )
	{
		valueChangedListener_->valueChanged();
	}
}
BW_END_NAMESPACE

