#ifndef WE_PYTHON_ADAPTER_HPP
#define WE_PYTHON_ADAPTER_HPP

#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

#include "common/python_adapter.hpp"

BW_BEGIN_NAMESPACE

class ChunkItemGroup;
typedef SmartPointer<ChunkItemGroup> ChunkItemGroupPtr;


class WEPythonAdapter : public PythonAdapter
{
public:
	WEPythonAdapter();
	~WEPythonAdapter();

	void onPageControlTabSelect( const BW::string& fnPrefix, const BW::string& theTabName );

	void onBrowserObjectItemSelect( const BW::string& tabName, 
							const BW::string& itemName, bool dblClick );
	void onBrowserObjectItemAdd();

	void onBrowserTextureItemSelect( const BW::string& itemName );
	
	void setTerrainPaintMode(int mode);

    void setTerrainPaintBrush( TerrainPaintBrushPtr paintBrush );

	void onLimitSliderAdjust( const BW::string& name, float pos, float min, float max );
	void limitSliderUpdate( LimitSlider* control, const BW::string& controlName );

	void selectFilterChange( const BW::string& value );
	void selectFilterUpdate( CComboBox* comboList );

	void coordFilterChange( const BW::string& value );
	void coordFilterUpdate( CComboBox* comboList );

	void projectLock(const BW::string& commitMessage);
	void commitChanges(const BW::string& commitMessage, bool keepLocks);
	void discardChanges(const BW::string& commitMessage, bool keepLocks);
	void updateSpace();
	void calculateMap();
	void exportMap();
	bool createSpaceLevelImage( const BW::string& path, int imgSize, int ChunkSizeX, int ChunkSizeY, float pixelsPerMetre, int topLeftGribPosX,  int topLeftGribPosY, int blockSize );
	bool createSpaceSingleMap( const BW::string& path, int imgSize, int ChunkSizeX, int ChunkSizeY, int outputWidth, int outputHeight );

	bool canSavePrefab();
	void saveSelectionPrefab( BW::string fileName );

	void expandSpace( const BW::string& spacePath, const BW::string& blankCDataFilePath, int westCnt, int eastCnt, int northCnt, int southCnt, bool chunkWithTerrain );

	void deleteChunkItems( ChunkItemGroupPtr group );
	PyObjectPtr addChunkItem( const BW::string &resourceID );
	void setSelection( ChunkItemGroupPtr group );

	void selectAll();

private:
	void fillFilterKeys( CComboBox* comboList );
	void fillCoordFilterKeys( CComboBox* comboList );
};

BW_END_NAMESPACE

#endif // WE_PYTHON_ADAPTER_HPP
