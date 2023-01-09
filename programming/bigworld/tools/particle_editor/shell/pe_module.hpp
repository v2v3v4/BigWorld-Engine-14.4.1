#ifndef PROTO_MODULE_HPP
#define PROTO_MODULE_HPP

#include "model/forward_declarations.hpp"
#include "moo/base_texture.hpp"
#include "appmgr/module.hpp"
#include "bigbang/grid_coord.hpp"

BW_BEGIN_NAMESPACE

class ClosedCaptions;
class ParticleSystem;
class SuperModel;

class PeModule : public FrameworkModule
{
public:
	PeModule();
	~PeModule();

	virtual bool init(DataSectionPtr pSection);

	virtual void onStart();
	virtual int  onStop();

	virtual bool updateState(float dTime);
	virtual void updateAnimations();
	virtual void render( float dTime );

	virtual bool handleKeyEvent(const KeyEvent &event);
	virtual bool handleMouseEvent(const MouseEvent &event);

	virtual void setApp( App * app ) {};
	virtual void setMainFrame( IMainFrame * mainFrame ) {};

	static PeModule& instance() { ASSERT(s_instance_); return *s_instance_; }

	ParticleSystem * activeParticleSystem() const { return particleSystem_; }

	float lastTimeStep() const { return lastTimeStep_; }
	float averageFPS() const { return averageFps_; }
	 
	typedef PeModule This;
	PY_MODULE_STATIC_METHOD_DECLARE( py_setParticleSystem )

	const BW::string &helperModelName() const;
	bool helperModelName( const BW::string &name );
	BW::vector<BW::string> &getHelperModelHardPointNames() { return helperModelHardPointNames_; }
	void helperModelCenterOnHardPoint(uint idx);
	uint helperModelCenterOnHardPoint() const;
	void drawHelperModel(bool draw);
	bool drawHelperModel() const { return drawHelperModel_; }

protected:
	void beginRender();
	void endRender();

	void renderChunks( Moo::DrawContext& drawContext );
	void renderTerrain(float dTime);
    void renderScale();
	void renderParticles( Moo::DrawContext& drawContext );
	void renderGizmo( Moo::DrawContext& drawContext );
	void renderAndUpdateBound();

	bool initPyScript();
	bool finiPyScript();

	Vector2 currentGridPos();
	Vector3 gridPosToWorldPos(Vector2 gridPos);

private:
	PeModule(PeModule const&);              // not permitted
	PeModule& operator=(PeModule const&);   // not permitted

	bool loadHelperModel( const BW::string &name );

private:
	Moo::BaseTexturePtr         mapTexture_;
    static PeModule             *s_instance_;           // The last created PeModule 
	Vector3                     viewPosition_;          // x, y camera position, z is zoom
	POINT                       lastCursorPosition_;    // Pos. of camera when started to look about
	GridCoord                   localToWorld_;          
	Vector2                     selectionStart_;        // Start of current selection
    GridRect                    currentSelection_;      // Current selection with mouse
	unsigned int                gridWidth_;             // Width of grid in chunks
	unsigned int                gridHeight_;            // Height of grid in chunks
	float                       lastTimeStep_;
	float                       averageFps_;
	PyObject                    *scriptDict_;
	ParticleSystem              *particleSystem_;
	SmartPointer<ClosedCaptions> cc_;

	bool						drawHelperModel_;
	BW::string					helperModelName_;
	ModelPtr					helperModel_;
	BW::vector<BW::string>		helperModelHardPointNames_;
	BW::vector<Matrix>			helperModelHardPointTransforms_;
};

BW_END_NAMESPACE

#endif // PROTO_MODULE_HPP
