#ifndef PAGE_CHUNK_TEXTURE_HPP
#define PAGE_CHUNK_TEXTURE_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "worldeditor/gui/controls/terrain_textures_control.hpp"
#include "guitabs/guitabs_content.hpp"
#include "controls/auto_tooltip.hpp"
#include "controls/separator.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/themed_image_button.hpp"
#include "resmgr/string_provider.hpp"
#include "ual/ual_drop_manager.hpp"

BW_BEGIN_NAMESPACE

class PageChunkTexture : public CFormView, public GUITABS::Content
{
	IMPLEMENT_BASIC_CONTENT( Localise(L"WORLDEDITOR/GUI/PAGE_CHUNK_TEXTURE/SHORT_NAME"),
		Localise(L"WORLDEDITOR/GUI/PAGE_CHUNK_TEXTURE/LONG_NAME"), 290, 460, NULL )

	DECLARE_AUTO_TOOLTIP_EX( PageChunkTexture )

public:
	PageChunkTexture();
	virtual ~PageChunkTexture();

	static const BW::string chunk();

	static void chunk( const BW::string& chunk, bool show = false );

    static void refresh( const BW::string& chunk );

	static bool trackCursor();

	static void trackCursor( bool track );

// Dialog Data
	enum { IDD = IDD_PAGE_CHUNK_TEXTURE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg LRESULT OnActivateTool(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNewSpace(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTerrainTextureSelected(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTerrainTextureRightClick(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnClosing(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg void OnBnClickedLatch();
	afx_msg void OnBnClickedTrack();
	afx_msg void OnBnClickedShow();
	afx_msg void OnEnChangeWarning();
	afx_msg void OnBnClickedWarning();
	afx_msg void OnKeyDown(UINT key, UINT repCnt, UINT flags);
	DECLARE_MESSAGE_MAP()

	virtual BOOL PreTranslateMessage(MSG *msg);

	bool onDropTexture(UalItemInfo *ii);

	RectInt onDropTextureTest( UalItemInfo *ii );

private:
	void InitPage();
	const BW::string chunkName() const;
	void setChunk( const BW::string& chunk );
	void setTextureControl( EditorChunkTerrain* terrain );
    CRect textureControlExtents(int cx = 0, int cy = 0) const;
	void editLayerInfo( int idx );
	void setTextureMask( int idx );
	void deleteLayers( BW::vector<size_t> const &selection );
	void mergeLayers( BW::vector<size_t> const &selection );
	void enableControls( bool enable );
	void addBumpMap(int idx);
	void deleteBumpMap(int idx);

	bool 							pageReady_;
	bool 							enabled_;
	bool 							resize_;

	CButton							latch_;
	CEdit							chunkID_;
	controls::ThemedImageButton		chunkTrack_;
	controls::ThemedImageButton		chunkShow_;
	controls::EditNumeric			warningNum_;
	controls::ThemedImageButton		warningShow_;
    controls::Separator             textureManagementSeparator_;
    controls::Separator             texturesChunkSeparator_;
    TerrainTexturesControl          texturesControl_;
	HICON							chunkTrackButtonImg_;
	HICON							chunkTrackChkButtonImg_;
	HICON							chunkShowButtonImg_;
	HICON							chunkShowChkButtonImg_;
};

IMPLEMENT_BASIC_CONTENT_FACTORY( PageChunkTexture )

BW_END_NAMESPACE

#endif // PAGE_CHUNK_TEXTURE_HPP
