#ifndef SPEEDTREE_RENDERER_HPP
#define SPEEDTREE_RENDERER_HPP

// BW Tech Headers
#include "math/vector3.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/smartpointer.hpp"
#include "moo/draw_context.hpp"


BW_BEGIN_NAMESPACE

// Forward declarations
class Matrix;
class BSPTree;
class BoundingBox;
class EnviroMinder;
class ShadowCaster;
typedef SmartPointer<class DataSection> DataSectionPtr;

BW_END_NAMESPACE

#include "speedtree_config_lite.hpp"
#if SPEEDTREE_SUPPORT
#include "speedtree_tree_type.hpp"
#include "speedtree_renderer_common.hpp"
#include "pyscript/script_math.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
class ModelTextureUsageGroup;
}

namespace speedtree {

//-- Single instance of a speedtree in the world. Each tree chunk item in the space will own an
//-- instance of SpeedTreeRenderer. SpeedTreeRenderer on its turn, will point to a shared TSpeedTreeType
//-- that holds the actual data needed to render the tree model.
//--------------------------------------------------------------------------------------------------
class SpeedTreeRenderer
{

public:
	SpeedTreeRenderer();
	~SpeedTreeRenderer();

	//--
	static void			init();
	static void			fini();
	static void			loadSpaceCfg(const DataSectionPtr& cfg);

	void				load(const char* filename, uint seed, const Matrix& world);

	static void			tick(float dTime);
	static void			setCameraTarget(MatrixProvider* pTarget);

	//-- These three method is all what you need to draw speedtree models. Before scene traversing
	//-- we set once beginFrame and at the end of traversing we call endFrame to inform speedtree
	//-- sub-system that it can perform drawing.
	//-- Notice: We can also invoke flush method to make speedtree perform drawing immediately.
	//--		 But be careful with this method because it breaks common speedtree's rendering
	//--	     optimization like material sorting and instancing.
	//-- lodCamera here means camera based on which speedtree calculates tree's lods.
	static void			beginFrame(EnviroMinder* envMinder, Moo::ERenderingPassType pass, const Matrix& lodCamera);
	static void			flush();
	static void			endFrame();
	static void			drawSemitransparent(bool skipDrawing = false);

	void				draw(const Matrix& worldTransform);
	void				resetTransform(const Matrix& world, bool updateBB = true);

	const BoundingBox&	boundingBox() const;
	const BSPTree* bsp() const;

	const char * 		filename() const;
	uint				seed() const;

	static float		lodMode( float newValue );
	static float		maxLod( float newValue );
	static bool			enviroMinderLighting( bool newValue );
	static bool			drawTrees( bool newValue );
	static bool			useWind(bool useWind_);

	int					numTris() const;
	int					numPrims() const;
	int					getTextureMemory() const;
	int					getVertexBufferMemory() const;
	void				getUID( char * dst, size_t dstSize ) const;
	void				getTexturesAndVertices(BW::map<void*, int>& textures, BW::map<void*, int>& vertices);
	
#if ENABLE_WATCHERS
	static bool		isVisible;
#endif

	//-- should we at all draw trees transparent
	static bool	s_enableTransparency;

	void generateTextureUsage( Moo::ModelTextureUsageGroup & usageGroup );
	//-- trees hiding parameters.
	static bool		s_hideClosest;
	static float	s_sqrHideRadius;
	static float	s_hideRadius;
	static float	s_hideRadiusAlpha;
	static float	s_hiddenLeavesAlpha;
	static float	s_hiddenBranchAlpha;

private:
	bool				tryToUpdateBBOptimizer(const Matrix& worldTransform);
	static void			update();
	
	SmartPointer<class TSpeedTreeType>		treeType_;
	SmartPointer<class BillboardOptimiser>	bbOptimiser_;

	TSpeedTreeType::DrawData drawData_;
	int						 treeID_;
	float					 windOffset_;


	static Moo::ERenderingPassType	s_curRenderingPass_;
	static bool						s_bAlphaBlend;
	static MatrixProviderPtr		s_pTarget;
	static Vector2					s_vTargetScreenPos;

	static Vector3					s_vCamPos;
	static Vector3					s_vTargetPos;
	static float					s_fToTargetDistance;
	static Vector2					s_vCamFwd2;
#ifdef EDITOR_ENABLED
	static bool						s_paused;
#endif

	friend class TSpeedTreeType;
};

} // namespace speedtree

BW_END_NAMESPACE

#endif
#endif // SPEEDTREE_RENDERER_HPP
