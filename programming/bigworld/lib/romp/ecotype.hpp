#ifndef ECOTYPE_HPP
#define ECOTYPE_HPP


BW_BEGIN_NAMESPACE

namespace Moo
{
	class BaseTerrainBlock;
	typedef SmartPointer<BaseTerrainBlock> BaseTerrainBlockPtr;
	class Visual;
	typedef SmartPointer<Visual> VisualPtr;
	class BaseTexture;
	typedef SmartPointer<BaseTexture> BaseTexturePtr;
}


/**
 *	This class represents a single ecotype in the flora system.
 *	Each ecotype has a unique ID that is baked into the terrain
 *	by the world editor.
 *	Each ecotype may have a single entry in the collated flora
 *	texture map, and is allocated a fixed number of vertices.
 *	Each ectoype has a generator class that provides geometry
 *	at any given location.
 */
class Ecotype
{
public:	
	typedef uint8 ID;
	static const uint32 ID_AUTO = 255;

	Ecotype( class Flora* flora );
	~Ecotype();

	void init( DataSectionPtr allEcotypesSection, uint8 id );
	void finz();

	//public member that will be set to false once the resources
	//have been loaded in the backgroudn thread.
	bool isLoading_;
	//public member that will be set to false when the ecotype
	//deallocates itself
	bool isInited_;
	void offsetUV( Vector2& offset );
	const Vector2& uvOffset() { return uvOffset_; }
	uint32 generate(		
		class FloraVertexContainer* pVerts,
		uint32 id,
		uint32 maxVerts,
		const Matrix& objectToWorld,
		const Matrix& objectToChunk,
		BoundingBox& bb );
	bool isEmpty() const;
	uint8 id() const								{ return id_; }
	BW::string& textureResource()					{ return textureResource_; }
	void textureResource( const BW::string& t )	{ textureResource_ = t; }
	Moo::BaseTexturePtr pTexture()					{ return pTexture_; }
	void pTexture( Moo::BaseTexturePtr p )			{ pTexture_ = p; }
	void generator( class EcotypeGenerator* g )		{ generator_ = g; }

	class EcotypeGenerator* generator() const	{ return generator_; }

	class Flora* flora() const { return flora_; }

	//reference counting
	void incRef();
	void decRef();

#ifdef EDITOR_ENABLED
	bool highLight( FloraVertexContainer* pVerts, BW::list<int>& indexPath, bool highLight );
#endif// EDITOR_ENABLED
	static SimpleMutex s_deleteMutex_;

#if ENABLE_RELOAD_MODEL
	void refreshTexture();
#endif //ENABLE_RELOAD_MODEL

private:	
	void grabTexture();
	void freeTexture();
	struct BkLoadInfo
	{					
		class BackgroundTask * loadingTask_;
		Ecotype* ecotype_;
		DataSectionPtr pSection_;
	};
	static void backgroundInit( void * );
	static void onBackgroundInitComplete( void * );
	BW::string textureResource_;
	Moo::BaseTexturePtr pTexture_;
	Vector2 uvOffset_;	
	int		refCount_;
	uint8	id_;
	class EcotypeGenerator* generator_;
	class Flora* flora_;
	BkLoadInfo* loadInfo_;
};

BW_END_NAMESPACE

#endif //#ifndef ECOTYPE_HPP
