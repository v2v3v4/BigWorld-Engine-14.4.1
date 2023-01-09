#ifndef TERRAIN_TEXTURE_LOD_CONTROL_HPP
#define TERRAIN_TEXTURE_LOD_CONTROL_HPP

#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "controls/auto_tooltip.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/embeddable_cdialog.hpp"
#include "worldeditor/gui/controls/limit_slider.hpp"
#include "cstdmf/bw_set.hpp"
#include <afxwin.h>

BW_BEGIN_NAMESPACE

/**
 * 
 * Interface for receiving  validation changes to the expand space control.
 * 
 */
class ITerrainTextureLodValueChangedListener
{
public:
	virtual void valueChanged() = 0;
};

/**
 * 
 * Common control for editing the terrain texture lod settings
 */
class TerrainTextureLodControl : public controls::EmbeddableCDialog
{
	DECLARE_DYNAMIC(TerrainTextureLodControl)

	static bool wasSlider(CWnd const &testScrollBar, CWnd const &scrollBar);
public:
	enum { IDD = IDD_EDIT_TERRAIN_TEXTURE_LOD };
	
	TerrainTextureLodControl(
		size_t & filterChange,
		ITerrainTextureLodValueChangedListener * listener );
	virtual ~TerrainTextureLodControl();

	void initializeToolTips();

	void terrainSettings( Terrain::TerrainSettingsPtr terrainSettings );
	Terrain::TerrainSettingsPtr terrainSettings() const;

	void reinitialise();
	void restoreSettings( bool disableInitialize );

private:
	float currentChunkSize();
	void formatChunkToKms( CString& str, const CString& chunkSizeStr );
	void formatPerChunkToPerMetre( CString &str );
	void formatPerChunkToMetresPer( CString &str );
	void editSpace(
		const BW::wstring & spaceName,
		int width, int height, float chunkSize,
		uint32 heightMapSize, uint32 normalMapSize,
		uint32 shadowMapSize, uint32 holeMapSize,
		uint32 blendMapSize );

	void OnTexLodEdit();
	void syncAllDialogs();

	Terrain::TerrainSettingsPtr terrainSettings_;
	static BW::set< TerrainTextureLodControl * > s_dialogs_;

protected:
	enum SliderMovementState
	{
		SMS_STARTED,
		SMS_MIDDLE,
		SMS_DONE
	};

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	afx_msg void OnHScroll(UINT /*sbcode*/, UINT /*pos*/, CScrollBar *scrollBar);
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );

	afx_msg void OnTexLodStartEditKillFocus();
	afx_msg void OnTexLodBlendEditKillFocus();
	afx_msg void OnTexLodPreloadEditKillFocus();

	void OnTexLodSlider(SliderMovementState sms);

	DECLARE_MESSAGE_MAP()

	DECLARE_AUTO_TOOLTIP(TerrainTextureLodControl, controls::EmbeddableCDialog)
public:
	controls::EditNumeric						texLodStartEdit_;
	LimitSlider									texLodStartSlider_;
	controls::EditNumeric						texLodDistEdit_;
	LimitSlider									texLodDistSlider_;
	controls::EditNumeric						texLodPreloadEdit_;
	LimitSlider									texLodPreloadSlider_;
	size_t &									filterChange_;
	ITerrainTextureLodValueChangedListener *	valueChangedListener_;
	size_t										changesMade_;
	bool										sliding_;
	bool										disableInitialize_;
};

BW_END_NAMESPACE

#endif // TERRAIN_TEXTURE_LOD_CONTROL_HPP
