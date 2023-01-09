#ifndef TERRAIN_TEXTURES_CONTROL_HPP
#define TERRAIN_TEXTURES_CONTROL_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "controls/dib_section32.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This class is used to show textures within a chunk in list form.
 */
class TerrainTexturesControl : public CWnd
{
public:
    TerrainTexturesControl();
    ~TerrainTexturesControl();

    BOOL Create(DWORD style, CRect const &extents, CWnd *parent, UINT id = 0);

    void terrain(EditorChunkTerrainPtr terrain);
	EditorChunkTerrainPtr terrain() const;
    
    Terrain::EditorBaseTerrainBlock* terrainBlock() const;

	void refresh();

    int height() const;

	int layerAtPoint( const CPoint& point ) const;
	bool layerRectAtPoint( const CPoint& point, CRect &rect ) const; 

	void resizeFullHeight( int width );

	BW::vector<size_t> const &selection() const;
	void selection(BW::vector<size_t> const &sel);

    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC *dc);

protected:
    typedef BW::vector<controls::DibSection32Ptr>  DibSecVec;

    void rebuildTextureLayerEntries();

    void addLayer
    (
        Terrain::TerrainTextureLayer    const &layer,  
        BW::vector<BW::string>		const &oldTextureNames,
        DibSecVec						const &oldTextures
    );

    CRect textureExtent(size_t idx, CRect const *client = NULL) const;

    CRect blendExtent(size_t idx, CRect const *client = NULL) const;

	int selectionIdx(size_t idx) const;

	afx_msg void OnLButtonDown( UINT nFlags, CPoint point );
	afx_msg void OnRButtonDown( UINT nFlags, CPoint point );
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point );
    DECLARE_MESSAGE_MAP()

private:
    EditorChunkTerrainPtr			terrain_;
    DibSecVec                       textures_;
    DibSecVec                       blends_;
    BW::vector<BW::string>        textureNames_;
	BW::vector<size_t>				selection_;
};

BW_END_NAMESPACE

#endif // TERRAIN_TEXTURES_CONTROL_HPP
