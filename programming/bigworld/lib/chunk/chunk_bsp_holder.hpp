#ifndef CHUNK_BSP_HOLDER
#define CHUNK_BSP_HOLDER

// Only enable in non-consumer client
#define ENABLE_BSP_MODEL_RENDERING (!MF_SERVER && !CONSUMER_CLIENT_BUILD)

#if ENABLE_BSP_MODEL_RENDERING

#include "cstdmf/bw_map.hpp"
#include "cstdmf/concurrency.hpp"
#include "math/matrix.hpp"
#include "moo/moo_math.hpp"
#include "moo/render_context.hpp"
#include "moo/vertex_buffer.hpp"
#include "moo/vertex_formats.hpp"

#include <map>
#include <string>
#include <vector>

BW_BEGIN_NAMESPACE


/**
 *	This class is for visualising the BSP scene.
 */
class ChunkBspHolder
{
public:
	static const size_t NO_BSP_MODEL;

	ChunkBspHolder();
	~ChunkBspHolder();

	bool isBspCreated() const;
	void drawBsp( Moo::RenderContext& rc,
		const Matrix& world,
		bool renderSelection,
		bool setStates = true ) const;
	void batchBsp( const Matrix& world ) const;
	void addBsp( const BW::vector< Moo::VertexXYZL >& verts,
		const BW::string& name );
	void delBsp();
	static void drawBsps( Moo::RenderContext& rc );

#ifdef EDITOR_ENABLED
	void postClone();
#endif // EDITOR_ENABLED

	static const bool getDrawBsp();
	const int& getDrawBspAsInt() const;
	void setDrawBspAsInt( const int& value );

	static const bool getDrawBspSpeedTrees();
	const int& getDrawBspSpeedTreesAsInt() const;
	void setDrawBspSpeedTreesAsInt( const int& value );

	static const bool getDrawBspOtherModels();
	const int& getDrawBspOtherModelsAsInt() const;
	void setDrawBspOtherModelsAsInt( const int& value );

protected:
	size_t getBspIndex( const BW::string& name );
	void getRandomBspColour( Moo::Colour& colour ) const;

private:
	struct Info
	{
		UINT primitiveCount_;
		Moo::VertexBuffer vb_;
	};
	typedef BW::map<size_t, ChunkBspHolder::Info> Infos;

	struct BspPrimitive
	{
		size_t index_;
		Matrix world_;
		Moo::Colour colour_;
	};
	typedef BW::vector<BspPrimitive> BspPrimitives;

	struct BspStatics
	{
		BspStatics()
			: s_nextBspModelIndex_( 0 )
			, chunkBspHolderCount_( 0 )
			, drawBsp_( false )
			, drawBspInt_( 0 )
			, bspCount_( 0 )
			, drawBspSpeedTrees_( false )
			, drawBspSpeedTreesInt_( 0 )
			, drawBspOtherModels_( false )
			, drawBspOtherModelsInt_( 0 )
#ifdef EDITOR_ENABLED
			, settingsMark_( -16 )
#endif
		{
			REGISTER_SINGLETON( BspStatics )
		}

		volatile uint32 s_nextBspModelIndex_;

		BspPrimitives bspPrimitives_;

		SimpleMutex infoMutex_;
		Infos infos_;

		bw_atomic32_t chunkBspHolderCount_;

		bool drawBsp_;
		int drawBspInt_;
		size_t bspCount_;

		bool drawBspSpeedTrees_;
		bool drawBspOtherModels_;
		int drawBspSpeedTreesInt_;
		int drawBspOtherModelsInt_;
#ifdef EDITOR_ENABLED
		uint32 settingsMark_;
#endif

		static BspStatics & instance()
		{
			SINGLETON_MANAGER_WRAPPER( BspStatics )
			static BspStatics s_BspStatics;
			return s_BspStatics;
		}
	};

#ifdef EDITOR_ENABLED
	static void readSettings( BspStatics& statics );
#endif
	static void setRenderStates( Moo::RenderContext& rc, bool renderSelection );
	static void drawColourBsp( Moo::RenderContext& rc,
		const size_t& index,
		const Matrix& world,
		const Moo::Colour& colour,
		bool renderSelection,
		bool setStates );

	Moo::Colour colour_;
	size_t bspModelIndex_;
};

BW_END_NAMESPACE

#endif // ENABLE_BSP_MODEL_RENDERING

#endif // CHUNK_BSP_HOLDER
