#ifndef PROTO_MODULE_HPP
#define PROTO_MODULE_HPP

#include <object.h>
#include "pyscript/script.hpp"
#include "moo/base_texture.hpp"
#include "appmgr/module.hpp"
#include "grid_coord.hpp"

#include "tools/common/utilities.hpp"
#include "tools/modeleditor_core/i_model_editor_app.hpp"

#include "tools/modeleditor_core/Models/mutant.hpp"

BW_BEGIN_NAMESPACE

class IMainFrame;
class ClosedCaptions;
class ParticleSystem;

class MeModule : public FrameworkModule
{
public:
	MeModule();
	~MeModule();

	virtual bool init( DataSectionPtr pSection );

	virtual void onStart();
	virtual int  onStop();

	virtual bool updateState( float dTime );
	bool renderThumbnail( const BW::string& fileName );

	virtual void updateAnimations();
	virtual void render( float dTime );

	virtual bool handleKeyEvent( const KeyEvent & event );
	virtual bool handleMouseEvent( const MouseEvent & event );

	static MeModule& instance() { SINGLETON_MANAGER_WRAPPER( MeModule ) MF_ASSERT(s_instance_); return *s_instance_; }

	float averageFPS() const { return averageFps_; }

	bool materialPreviewMode() const { return materialPreviewMode_; }
	void materialPreviewMode( bool on ) { materialPreviewMode_ = on; }

	typedef MeModule This;

	PY_MODULE_STATIC_METHOD_DECLARE( py_render )
	PY_MODULE_STATIC_METHOD_DECLARE( py_onEditorReady )

	void setApp( App * app ) { app_ = app; }
	void setMainFrame( IMainFrame * mainFrame ) { mainFrame_ = mainFrame; }

private:
	MeModule( const MeModule& );
	MeModule& operator=( const MeModule& );
	
	void beginRender();
	void endRender();

	void renderChunks( Moo::DrawContext& drawContext );
	void renderTerrain( float dTime = 0.f, bool shadowing = false );
	void renderOpaque( Moo::DrawContext& drawContext, float dTime);
	void renderFixedFunction( Moo::DrawContext& drawContext, float dTime );
    void updateModel( Moo::DrawContext& drawContext, float dTime );

	void setLights( bool checkForSparkles, bool useCustomLighting );

	bool initPyScript();
	bool finiPyScript();

	static void chunksLoadedCallback( const char * );

	Moo::BaseTexturePtr mapTexture_;

	/** The last created MeModule */
	static MeModule* s_instance_;

	/** Camera position, X & Y specify position, Z specifies zoom */
	Vector3 viewPosition_;

	/**
	 * Where the cursor was when we started looking around,
	 * so we can restore it to here when done
	 */
	POINT lastCursorPosition_;

	/**
	 * Add to a GridCoord to transform it from a local (origin at 0,0) to a
	 * world (origin in middle of map) grid coord
	 */
	GridCoord localToWorld_;

	/** The start of the current selection */
	Vector2 selectionStart_;

	/** The currently selecting, with the mouse, rect */
	GridRect currentSelection_;

	/** Extent of the grid, in number of chunks. It starts at 0,0 */
	unsigned int gridWidth_;
	unsigned int gridHeight_;

	Vector2 currentGridPos();
	Vector3 gridPosToWorldPos( Vector2 gridPos );

	float averageFps_;

	PyObject * scriptDict_;

	ScriptObject editorReadyCallback_;

	SmartPointer<ClosedCaptions> cc_;

	class WireframeMaterialOverride* wireframeRenderer_;

	bool materialPreviewMode_;

	bool renderingThumbnail_;
	SmartPointer< Moo::RenderTarget > rt_;

	int		lastHDREnabled_;
	int		lastSSAOEnabled_;

	bool lastShadowsOn_;
	uint32 lastShadowsQuality_;
	MutantShadowRender shadowRenderItem_;
	float lastDTime_;
	App *			app_;
	IMainFrame *	mainFrame_;
};

BW_END_NAMESPACE

#endif // PROTO_MODULE_HPP