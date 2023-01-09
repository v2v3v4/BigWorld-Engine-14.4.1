#ifndef ECOTYPE_GENERATORS_HPP
#define ECOTYPE_GENERATORS_HPP

#include "ecotype.hpp"
#include "flora_vertex_type.hpp"

#include "moo/reload.hpp"


BW_BEGIN_NAMESPACE

/** 
 *	This interface describes a class that generates the
 *	vertex and texture data for an ecotype.
 */
class EcotypeGenerator
{
public:
	static EcotypeGenerator* create(DataSectionPtr pSection,
			Ecotype& target );
	virtual ~EcotypeGenerator() {};
	virtual bool load( DataSectionPtr pSection,
			Ecotype& target ) = 0;

	virtual uint32 generate(			
			const Vector2& uvOffset,
			class FloraVertexContainer* start,
			uint32 id,
			uint32 maxVerts,
			const Matrix& objectToWorld,
			const Matrix& objectToChunk,
			BoundingBox& bb ) = 0;	

	virtual bool isEmpty() const = 0;	

#ifdef EDITOR_ENABLED
	virtual bool highLight( FloraVertexContainer* pVerts, BW::list<int>& indexPath, bool highLight  ){ return false; }
#endif
};


/** 
 *	This class implements the ecoytpe generator interface,
 *	and simply creates degenerate triangles so no flora is
 *	drawn.
 */
class EmptyEcotype : public EcotypeGenerator
{
	uint32 generate(		
		const Vector2& uvOffset,
		class FloraVertexContainer* pVerts,
		uint32 id,
		uint32 maxVerts,
		const Matrix& objectToWorld,
		const Matrix& objectToChunk,
		BoundingBox& bb );

	bool load( DataSectionPtr pSection, Ecotype& target )
	{
		target.textureResource("");
		target.pTexture(NULL);
		return true;
	}

	bool isEmpty() const
	{
		return true;
	}
};

/**
 *	This class implements the ecotype interface, using
 *	an array of visuals as source material.  The visuals
 *	are simple placed down on the terrain and oriented.
 */
class VisualsEcotype : public EcotypeGenerator
{
public:
	VisualsEcotype();
	~VisualsEcotype();
	bool load( DataSectionPtr pSection, Ecotype& target );
	uint32 generate(		
		const Vector2& uvOffset,
		class FloraVertexContainer* pVerts,
		uint32 id,
		uint32 maxVerts,
		const Matrix& objectToWorld,
		const Matrix& objectToChunk,
		BoundingBox& bb );
	bool isEmpty() const;	

	static bool findTextureName( const BW::StringRef& resourceID, BW::string& texName );

#ifdef EDITOR_ENABLED
	virtual bool highLight( FloraVertexContainer* pVerts, BW::list<int>& indexPath, bool highLight  );
#endif// EDITOR_ENABLED



#if ENABLE_RELOAD_MODEL
	bool updateVisual( const BW::StringRef& visualResourceID, Moo::Visual* pVisual );
	bool hasVisual( const BW::StringRef& visualResourceID );
	class FloraTexture* floraTexture() const;
#endif//RELOAD_MODEL_SUPPORT

protected:	
	static bool findTextureResource(const BW::StringRef& resourceID, Ecotype& target);

	/**
	 *	This structure copies a visual into a triangle list of FloraVertices
	 */
	struct VisualCopy
	{
		bool set( Moo::VisualPtr, 
			float flex, float scaleVariation, 
			float tintFactor, class Flora* flora, Ecotype* pTarget );
		bool pullInfoFromVisual( Moo::Visual* pVisual );
#if ENABLE_RELOAD_MODEL
		void refreshTexture();
#endif//RELOAD_MODEL_SUPPORT

		BW::vector< FloraVertex > vertices_;
		float scaleVariation_;

		float flex_;
		Flora* flora_;
		BW::string visual_;
		Ecotype* pTarget_;

#ifdef EDITOR_ENABLED
		bool highLight_;
#endif// EDITOR_ENABLED

	};

	typedef BW::vector<VisualCopy>	VisualCopies;
	VisualCopies	visuals_;
	float			density_;
	class Flora*	flora_;
};



#if ENABLE_RELOAD_MODEL

class FloraVisualManager : 
	public ResourceModificationListener
{
public:
	void init();
	void fini();

	bool hasVisual( const BW::StringRef& visualResourceID );
	static bool updateVisual( const BW::StringRef& visualResourceID );

	static void addVisualsEcotype( VisualsEcotype* pVisualEcotype );
	static void delVisualsEcotype( VisualsEcotype* pVisualEcotype );

	static void addFloraTextureDependency( const BW::string& textureName,
										VisualsEcotype* visualEcotype );
protected:
	void onResourceModified(
		const BW::StringRef& basePath,
		const BW::StringRef& resourceID,
		ResourceModificationListener::Action action);
private:
	class DependencyReloadContext;
	class VisualReloadContext;

	static BW::list<VisualsEcotype*> visualEcotypes_;
	typedef BW::vector<VisualsEcotype*> VisualEcotypes;
	typedef StringRefMap< VisualEcotypes > StringRefVisualEcotypesMap;
	static StringRefVisualEcotypesMap floraTextureDependencies_;
	static SimpleMutex mutex_;

};


#endif//RELOAD_MODEL_SUPPORT


/**
 *	This class implements the ecotype interface, using
 *	a vector of Ecotypes (sub-types).  Rules are applied at
 *	any geographical location to determine which sub-type
 *	is chosen.
 */
class ChooseMaxEcotype : public EcotypeGenerator
{
public:	
	~ChooseMaxEcotype();
	bool load( DataSectionPtr pSection, Ecotype& target );
	uint32 generate(		
		const Vector2& uvOffset,
		class FloraVertexContainer* pVerts,
		uint32 id,
		uint32 maxVerts,
		const Matrix& objectToWorld,
		const Matrix& objectToChunk,
		BoundingBox& bb );	

	/**
	 * TODO: to be documented.
	 */
	class Function
	{
	public:
		virtual ~Function() {}
		virtual void load(DataSectionPtr pSection) = 0;
		virtual float operator() (const Vector2& input) = 0;
	};

	bool isEmpty() const;	

#ifdef EDITOR_ENABLED
	virtual bool highLight( FloraVertexContainer* pVerts, BW::list<int>& indexPath, bool highLight );
#endif

private:
	static Function* createFunction( DataSectionPtr pSection );
	EcotypeGenerator* chooseGenerator(const Vector2& position);
	struct SubType
	{
		Function* first;
		EcotypeGenerator* second;
	};
	typedef BW::vector< SubType >	SubTypes;
	SubTypes	subTypes_;
};

BW_END_NAMESPACE

#endif // ECOTYPE_GENERATORS_HPP

