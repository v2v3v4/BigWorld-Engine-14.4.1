#ifndef PAGE_TERRAIN_TEXTURE_HPP
#define PAGE_TERRAIN_TEXTURE_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/gui/controls/limit_slider.hpp"
#include "worldeditor/gui/controls/terrain_textures_control.hpp"
#include "worldeditor/terrain/terrain_texture_utils.hpp"
#include "guitabs/guitabs_content.hpp"
#include "ual/ual_drop_manager.hpp"
#include "controls/dialog_toolbar.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/separator.hpp"
#include "controls/image_control.hpp"
#include "controls/image_button.hpp"
#include "controls/themed_image_button.hpp"
#include "controls/auto_tooltip.hpp"
#include "gizmo/tile_gizmo.hpp"
#include "math/simplex_noise.hpp"

BW_BEGIN_NAMESPACE

class PageTerrainTexture : public CFormView, public GUITABS::Content
{
	IMPLEMENT_BASIC_CONTENT( 
		Localise(L"WORLDEDITOR/GUI/PAGE_TERRAIN_TEXTURE/SHORT_NAME"),		// short name
        Localise(L"WORLDEDITOR/GUI/PAGE_TERRAIN_TEXTURE/LONG_NAME"),		// long name
        290, 380,													// width, height
        NULL )														// icon

public:
    enum { IDD = IDD_PAGE_TERRAIN_TEXTURE };

	enum Mode
	{
		PAINTING,
		REPLACING,
		EDITING
	};

	static PageTerrainTexture* instance();

	PageTerrainTexture();
	/*virtual*/ ~PageTerrainTexture();	

	void initPage();

    /*virtual*/ void DoDataExchange( CDataExchange* pDX ); 

	afx_msg LRESULT OnActivateTool  ( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnDefaultPanels ( WPARAM wParam, LPARAM lParam );
    afx_msg LRESULT OnUpdateControls( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnClosing       ( WPARAM wParam, LPARAM lParam );

    afx_msg void OnHScroll( UINT sBCode, UINT pos, CScrollBar * pScrollBar );
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );

	void currentTexture( const BW::string & filename, const BW::string & bumpFileName, bool resetProjectionParams = true );
	void currentTexture( const BW::string & filename, const BW::string & bumpFileName, const Vector4 & u, const Vector4 & v, bool resetProjectionParams = true );

    Vector4 uProjection() const;
    Vector4 vProjection() const;

	void setTerrainBrush( TerrainPaintBrushPtr pBrush );
	const TerrainPaintBrushPtr getTerrainBrush() const;

	void lockProjectionScale( Matrix & m, const Matrix & initialM );

	Matrix textureProjection() const;
	void textureProjection( const Matrix & m, bool temporary = false );

	Matrix textureProjectionInverse() const;
	void textureProjectionInverse( const Matrix & m, bool temporary = false );

	Moo::BaseTexturePtr texture() const;
	bool hasTexture() const;

	Moo::BaseTexturePtr bumpTexture() const;
	bool hasBumpTexture() const;

	Mode mode() const;

	void beginEditLayers();
	void endEditLayers();

	Vector3 toolPos() const;

	void layersAtPoint( const Vector3 & point, TerrainTextureUtils::LayerVec & layers );
	bool canEditProjections() const;
	bool editTextureLayer( const TerrainTextureUtils::LayerInfo & info, const Vector3 * pPos = NULL );
	void editProjectionAtPoint( const Vector3 & point, size_t idx );
	void selectTextureAtPoint( const Vector3 & point, size_t idx );
	void setOpacity( float opacity );

	void selectTextureMaskAtPoint(Vector3 const &point, size_t idx, bool includeProj, bool includeBump);
	void selectTextureMask(
		const BW::string &		texture, 
		const BW::string &		bumpTexture,
		bool					includeProj,
		bool					includeBump,
		const Vector4 &			uproj, 
		const Vector4 &			vproj,
		bool					invert		= false	);

	void paintLayer( int idx );
	void paintPos( const Vector3 & pos );

	void onEscapeKey( int hitEscKey );

    void updatePythonMask();

	bool isActive() const;

protected:
	void checkTerrainVersion();

	void checkEditedLayer();

	void updateGizmo( bool forceReposition, const Vector3 * pPos = NULL );
	void translateGizmoToMiddle();

    void updateSliderEdits();

	void updateProjectionControls();

	afx_msg void OnEditingButton();

	afx_msg void OnSaveBrushButton();

    afx_msg void OnEnChangeSizeEdit();
    afx_msg void OnEnChangeStrengthEdit();
	afx_msg void OnEnChangeOpacityEdit();
    afx_msg void OnBnClickedSizeButton();

	afx_msg void OnMaxLayersCB();
	afx_msg void OnMaxLayersEdit();

    afx_msg void OnHeightButton();   
    afx_msg void OnEnChangeMinHeightEdit();
    afx_msg void OnEnChangeMaxHeightEdit();
    afx_msg void OnEnChangeFuzHeightEdit();

    afx_msg void OnSlopeButton();
    afx_msg void OnEnChangeMinSlopeEdit();
    afx_msg void OnEnChangeMaxSlopeEdit();
    afx_msg void OnEnChangeFuzSlopeEdit();

	afx_msg void OnTexMaskButton();
	afx_msg void OnTexMaskIncludeProjButton();
	afx_msg void OnTexMaskIncludeBumpButton();
	afx_msg void OnTexMaskInvButton();

    afx_msg void OnNoiseMaskButton();
    afx_msg void OnNoiseSetupButton();

	afx_msg void OnMaskImportButton();
	afx_msg void OnMaskBrowseButton();
	afx_msg void OnMaskAdjustButton();
	afx_msg void OnMaskStrengthEdit();
    afx_msg void OnAnticlockwiseBtn();
    afx_msg void OnClockwiseBtn    ();
    afx_msg void OnFlipXBtn        ();
    afx_msg void OnFlipYBtn        ();
    afx_msg void OnFlipHeightBtn   ();
    afx_msg void OnAnticlockwiseBtnEnable( CCmdUI *cmdui );
    afx_msg void OnClockwiseBtnEnable    ( CCmdUI *cmdui );
    afx_msg void OnFlipXBtnEnable        ( CCmdUI *cmdui );
    afx_msg void OnFlipYBtnEnable        ( CCmdUI *cmdui );
    afx_msg void OnFlipHeightBtnEnable   ( CCmdUI *cmdui );
    afx_msg void OnRecalcClrBtnEnable    ( CCmdUI *cmdui );

	afx_msg void OnMaskOverlayButton();

    afx_msg void OnEnChangeYawAngleEdit();
    afx_msg void OnEnChangePitchAngleEdit();
    afx_msg void OnEnChangeRollAngleEdit();
    afx_msg void OnEnChangeUSizeEdit();
    afx_msg void OnEnChangeVSizeEdit();
	
	afx_msg void OnBnClickedUVScaleLink();

	afx_msg void OnResetButton();
	afx_msg void OnAutofindBumpTextureButton();

	afx_msg void OnPlaceMask();

	afx_msg LRESULT OnNewSpace( WPARAM wParam, LPARAM lParam );

	afx_msg BOOL OnToolTipText( UINT, NMHDR * pNMHDR, LRESULT * pResult );

	DECLARE_MESSAGE_MAP()
	DECLARE_AUTO_TOOLTIP( PageTerrainTexture, CFormView )

    typedef PageTerrainTexture This;
    PY_MODULE_STATIC_METHOD_DECLARE( py_setCurrentTexture )	
    PY_MODULE_STATIC_METHOD_DECLARE( py_setCurrentTextureFull )
	PY_MODULE_STATIC_METHOD_DECLARE( py_setCurrentBrush )
	PY_MODULE_STATIC_METHOD_DECLARE( py_setPaintLayer )
	PY_MODULE_STATIC_METHOD_DECLARE( py_setPaintPos )
	PY_MODULE_STATIC_METHOD_DECLARE( py_setTerrainPaintEscKey )
	PY_MODULE_STATIC_METHOD_DECLARE( py_isTerrainTexture )

	bool onDropTexture( UalItemInfo * pItemInfo );
	bool onDropBumpTexture( UalItemInfo * pItemInfo );
	bool onDropTextureMask( UalItemInfo * pItemInfo );
	bool onDropBumpTextureMask( UalItemInfo * pItemInfo );
	bool onDropBrush( UalItemInfo * pItemInfo );

    void updateProjection( bool temporary = false );
    void updateEditsFromProjection( bool forceUpdateEdits = false );

    void updatePython();

	void buildEditors();
	void destroyEditors();

	bool alignTextureToNormal( const Vector3 & pos, bool temporary = false );

	void buildMaskImage();

	void onPainting( bool mouseUp );
	void onReplacing( bool mouseUp );
	void onSampleTexture(
		bool					mouseUp, 
		size_t					idx					= EditorChunkTerrain::NO_DOMINANT_BLEND,
		const Vector3 *			pPt			= NULL,
		bool					canSampleOpacity	= false	);
	void onSampleNormal( bool mouseUp );
	void onEditingGizmo( bool mouseUp );
	void onEditing();

	Vector3 middleOfScreen() const;

	bool dominantTexture(
		size_t *				pIdx,
		EditorChunkTerrain **	pChunkTerrain,
		Chunk **				pChunk ) const;

	size_t replaceDominantTexture();

	void setTextureMaskTexture(
		const BW::string &		textureFile,
		const BW::string &		bumpTextureFile,
		const Vector4 &			uProj,
		const Vector4 &			vProj,
		bool					enableTex,
		bool					includeProj,
		bool					includeBump,
		bool					invert,
		bool					updateProj );

	void updateEditedLayers( bool temporary );

	void onEnableMaskControls( bool forceUpdate = false );

	void mode( Mode m );

	void beginAdjustMask();

	void endAdjustMask();

	void paintBrushFromControls( TerrainPaintBrush & brush ) const;

	static Matrix 
	buildMatrix(
		const Vector4 &				u, 
		const Vector4 &				v );

private:
	void checkTextureAlpha( const BW::string fileName );
	void updateMaskStrength( float value );

    static PageTerrainTexture       *s_instance_;

    controls::EditNumeric           sizeEdit_;
    LimitSlider                     sizeSlider_;
    controls::ImageButton           sizeButton_;
    controls::EditNumeric           strengthEdit_;
    LimitSlider                     strengthSlider_;
	controls::EditNumeric           opacityEdit_;
	LimitSlider						opacitySlider_;
	controls::EditNumeric           maxLayersEdit_;
	controls::ImageButton			saveBrushButton_;

    controls::ImageControl32		currentTextureImage_;
	controls::ImageControl32		currentBumpTextureImage_;
    controls::EditNumeric           yawAngleEdit_;
	LimitSlider						yawAngleSlider_;
    controls::EditNumeric           pitchAngleEdit_;
	LimitSlider						pitchAngleSlider_;
    controls::EditNumeric           rollAngleEdit_;
	LimitSlider						rollAngleSlider_;
    controls::EditNumeric           uSizeEdit_;
	LimitSlider						uSizeSlider_;
    controls::EditNumeric           vSizeEdit_;
	LimitSlider						vSizeSlider_;
    BW::string                      currentTextureFilename_; 
	BW::string						currentBumpTextureFilename_;

	controls::ThemedImageButton		linkUVScale_;
	CStatic							linkUIcon_;
	CStatic							linkVIcon_;
	HICON							linkIcon_;

	controls::ImageButton			editProjButton_;

    controls::EditNumeric           minHeightEdit_;
    controls::EditNumeric           maxHeightEdit_;
    controls::EditNumeric           fuzHeightEdit_;

    controls::EditNumeric           minSlopeEdit_;
    controls::EditNumeric           maxSlopeEdit_;
    controls::EditNumeric           fuzSlopeEdit_;

	controls::ImageControl32		maskTexture_;
	controls::ImageControl32		maskBumpTexture_;
	controls::EditNumeric			maskTextureYawEdit_;
	controls::EditNumeric			maskTexturePitchEdit_;
	controls::EditNumeric			maskTextureRollEdit_;
	controls::EditNumeric			maskTextureUProjEdit_;
	controls::EditNumeric			maskTextureVProjEdit_;

	controls::ImageControl32		maskImage_;
	controls::EditNumeric			maskStrengthEdit_;
	LimitSlider						maskStrengthSlider_;
	controls::Separator				separator_;
	controls::DialogToolbar			actionTB_;

	bool							inited_;
	Matrix							invTexProjection_; // inverse texture projection, translation is gizmo pos
	Vector3							invTexProjTrans_;  // keep real translation seperately
	TexProjMatrixProxyPtr			pMatrixProxy_;
	size_t							filter_;
	bool							isActive_;
	TileGizmoPtr					pTileGizmo_;
	BW::string						oldCoordMode_;
	bool                            hasUVProjections_;
	TerrainTextureUtils::LayerSet	editedLayer_;
	TerrainTextureUtils::LayerInfo	seedTerrain_;
	bool							inHScroll_;
	bool							samplingNormal_;
	uint32							gizmoDrawState_;
	Mode							mode_;
	int								paintLayer_;
	Vector3							paintPos_;
	bool							isAdjustingMask_;
	TerrainPaintBrushPtr			pBrush_;

	float							yaw_;
	float							pitch_;
	float							roll_;
	float							uScale_;
	float							vScale_;
	int								layerUndoAdded_;

	struct Impl*					pImpl;
	friend Impl;
};


IMPLEMENT_BASIC_CONTENT_FACTORY(PageTerrainTexture)

BW_END_NAMESPACE

#endif // PAGE_TERRAIN_TEXTURE_HPP
