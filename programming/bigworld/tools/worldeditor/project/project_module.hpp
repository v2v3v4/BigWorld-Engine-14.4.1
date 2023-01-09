#ifndef PROJECT_MODULE_HPP
#define PROJECT_MODULE_HPP


#include "common/grid_coord.hpp"
#ifndef BIGWORLD_CLIENT_ONLY
#include "common/bwlockd_connection.hpp"
#endif // BIGWORLD_CLIENT_ONLY
#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/project/lock_map.hpp"
#include "worldeditor/project/space_map.hpp"
#include "appmgr/module.hpp"
#include "ashes/simple_gui.hpp"
#include "ashes/text_gui_component.hpp"
#include "romp/font.hpp"

BW_BEGIN_NAMESPACE

class ProjectModule : public FrameworkModule
{
public:
	ProjectModule();
	~ProjectModule();

	virtual bool init( DataSectionPtr pSection );

	virtual void onStart();
	virtual int  onStop();

	virtual void onPause();
	virtual void onResume( int exitCode );

	virtual bool updateState( float dTime );
	virtual void updateAnimations();
	virtual void render( float dTime );

	virtual bool handleKeyEvent( const KeyEvent & event );
	virtual bool handleMouseEvent( const MouseEvent & event );

	virtual void setApp( App * app ) {};
	virtual void setMainFrame( IMainFrame * mainFrame ) {};

	bool isReadyToLock() const;
	bool isReadyToCommitOrDiscard() const;

	// the locked chunks under selection
	BW::set<BW::string> lockedChunksFromSelection( bool insideOnly );

	// the graph file under selection
	BW::set<BW::string> graphFilesFromSelection();

	// the vlo file under selection
	BW::set<BW::string> vloFilesFromSelection();

	void getChunksFromRectRecursive( const BW::string& spaceDir, const BW::string& subDir,
							BW::set<BW::string>& chunks, BWLock::Rect rect, bool insideOnly );
							
	void getChunksFromRect( BW::set<BW::string>& chunks, BWLock::Rect rect, bool insideOnly );

	/** Lock the current selection, if any, returns if the lock was successful */
	bool lockSelection( const BW::string& description );

	/** Discard any locks we have, returning if we were able */
	bool discardLocks( const BW::string& description );

	/** Inform bigbangd we just commited the space */
	void commitDone();

	/** Get the updated lock data from bigbangd, and update the lockTexture */
	void updateLockData();

	/** Set blend value for space map **/
	void projectMapAlpha( float a );
	float projectMapAlpha();

    /** Draw all dirty chunks **/
    static void regenerateAllDirty();

	/** Get the current instance on the module stack, if any */
	static ProjectModule* currentInstance();
	static BW::string currentSpaceDir();
	static BW::string currentSpaceResDir();
	static BW::string currentSpaceAbsoluteDir();
private:
	ProjectModule( const ProjectModule& );
	ProjectModule& operator=( const ProjectModule& );

	void writeStatus();

	LockMap		lockMap_;
	float		spaceAlpha_;
	uint32		spaceColour_;
	Moo::BaseTexturePtr handDrawnMap_;

	/** Camera position, X & Y specify position, Z specifies zoom */
	Vector3 viewPosition_;
	float	minViewZ_;
	//we must allow at least a far plane of minViewZ when viewing the bitmap.
	//this allows us to return bigbang to its previous state.
	float	savedFarPlane_;	

	/** Metres per grid square */
	float gridSize_;
	/** Extent of the grid, in number of chunks. It starts at 0,0 */
	unsigned int gridWidth_;
	unsigned int gridHeight_;
	int minX_;
	int minY_;

	/**
	 * Add to a GridCoord to transform it from a local (origin at 0,0) to a
	 * world (origin in middle of map) grid coord
	 */
	GridCoord localToWorld_;

	/**
	 * Where the cursor was when we started looking around,
	 * so we can restore it to here when done
	 */
	POINT lastCursorPosition_;

	/** The start of the current selection */
	Vector2 selectionStart_;

	/** The currently selecting, with the mouse, rect */
	GridRect currentSelection_;
	GridCoord currentSelectedCoord_;

	SmartPointer<ClosedCaptions> cc_;

	/** Where in the grid the mouse is currently pointing */
	Vector2 currentGridPos();

	/** Where in the grid for a certain pixel position */
	Vector2 pixelToGridPos( int x, int y );

	/** Get a world position from a grid position */
	Vector3 gridPosToWorldPos( Vector2 gridPos );

    /** Get a 2d world position from a screen position. */
    Vector2 gridPos(POINT pt) const;

	/** The last created ProjectModule */
	static ProjectModule* currentInstance_;

	// true iff the lmb was depressed over the 3D window
	bool mouseDownOn3DWindow_;

	TextGUIComponent*	text_;
	TextGUIComponent*	shadow_;

	FontPtr				font_;
	FontPtr				boldFont_;
};

BW_END_NAMESPACE

#endif // PROJECT_MODULE_HPP
