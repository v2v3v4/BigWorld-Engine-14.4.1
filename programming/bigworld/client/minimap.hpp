#ifndef MINIMAP_HPP
#define MINIMAP_HPP

#include "ashes/simple_gui_component.hpp"
#include "math/colour.hpp"
#include "moo/custom_mesh.hpp"
#include "romp/py_texture_provider.hpp"
#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

typedef uint32	MinimapHandle;
#define INCLUDE_CELL_BOUNDARY_VIZ 1


/*~ class GUI.Minimap
 *	This class is a GUI component that can display a
 *	scrolling map, with entries displayed on the map.
 */
/**
 *	This class is a GUI component that can display a
 *	scrolling map, with entries displayed on the map.
 */
class Minimap : public SimpleGUIComponent
{
	Py_Header( Minimap, SimpleGUIComponent )

public:
	Minimap( PyTypeObject * pType = &s_type_ );
	~Minimap();	

	MinimapHandle		add( MatrixProviderPtr pMatrix, SimpleGUIComponentPtr pComponent );
	MinimapHandle		addSimple( MatrixProviderPtr pMatrix, const Vector4& colour );
	void				remove( MinimapHandle entry );

	const BW::string&	maskName() const;
	virtual void		maskName( const BW::string& name );

	const BW::string&	simpleEntryMap() const;
	virtual void		simpleEntryMap( const BW::string& name );

	void				addTextureLayer( uint32 layer, PyTextureProviderPtr pTexture, const Vector2& worldSize, const Vector2& worldAnchor );
	void				delTextureLayer( uint32 layer );
	void				delAllTextureLayers();

	PY_FACTORY_DECLARE()

	PY_RW_ATTRIBUTE_DECLARE( viewpoint_, viewpoint );
	PY_RW_ATTRIBUTE_DECLARE( range_, range );
	PY_RW_ATTRIBUTE_DECLARE( currentRange_, currentRange );
	PY_RW_ATTRIBUTE_DECLARE( zoomSpeed_, zoomSpeed );
	PY_RW_ATTRIBUTE_DECLARE( simpleEntrySize_, simpleEntrySize );
	PY_RW_ATTRIBUTE_DECLARE( mouseEntryPicking_, mouseEntryPicking );
	PY_RW_ATTRIBUTE_DECLARE( simpleEntriesVisible_, simpleEntriesVisible );
	PY_RW_ATTRIBUTE_DECLARE( pointSizeScaling_, pointSizeScaling );
#if INCLUDE_CELL_BOUNDARY_VIZ
	PY_RW_ATTRIBUTE_DECLARE( cellBoundsHolder_, cellBounds );
	PY_RW_ATTRIBUTE_DECLARE( cellBoundsVisible_, cellBoundsVisible );	
#endif
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, maskName, maskName )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, simpleEntryMap, simpleEntryMap )

	PY_RW_ATTRIBUTE_DECLARE( rotate_, rotate );

	PY_AUTO_METHOD_DECLARE( RETDATA, add, NZARG( MatrixProviderPtr, NZARG( SimpleGUIComponentPtr, END )))
	PY_AUTO_METHOD_DECLARE( RETDATA, addSimple, NZARG( MatrixProviderPtr, ARG( Vector4, END )))
	PY_AUTO_METHOD_DECLARE( RETVOID, remove, ARG( MinimapHandle, END ) )
	PY_AUTO_METHOD_DECLARE( RETVOID, addTextureLayer, ARG( uint32, ARG( PyTextureProviderPtr, ARG( Vector2, ARG( Vector2, END )))))
	PY_AUTO_METHOD_DECLARE( RETVOID, delTextureLayer, ARG( uint32, END ))
	PY_AUTO_METHOD_DECLARE( RETVOID, delAllTextureLayers, END )

	void				update( float dTime, float relParentWidth, float relParentHeight );
	void				drawSelf( bool reallyDraw, bool ovelay = true );

private:
	Minimap( const Minimap& );
	Minimap& operator=( const Minimap& );

	MinimapHandle		nextHandle();
	bool buildMaterial( void );
	bool load( DataSectionPtr pSect, const BW::string& ownerName, LoadBindings & bindings );
	void save( DataSectionPtr pSect, SaveBindings & bindings );
	void updateEntries( const Vector3& camPos, float relParentWidth, float relParentHeight );
	void updateSimpleEntries( const Vector3& camPos, float relParentWidth, float relParentHeight );
	void doMousePick(const Vector3& camPos, const Vector3& center, const Vector2& size);
	float distanceToEntry( const Vector2& mpos, const Vector3& cpos, MatrixProvider* pMatrix );	
	Vector3 viewpointPosition() const;
	void onEntryBlur();
	void onEntryFocus();

	class Transform : public MatrixProvider
	{
	public:
		virtual void matrix( Matrix & m ) const
		{
			m = m_;
		}

		Matrix				m_;
	};

	///This is a simple class that stores a Matrix Provider, a gui component ptr,
	///and a simple matrix provider
	class EntryInfo
	{
	public:
		EntryInfo( MatrixProviderPtr pMatrix, SimpleGUIComponentPtr c ):
			pMatrix_( pMatrix ),
			c_( c )			
		{
			m_.setIdentity();
		};

		MatrixProviderPtr pMatrix_;
		SimpleGUIComponentPtr c_;
		Matrix m_;
	};

	class SimpleEntry
	{
	public:
		SimpleEntry( MatrixProviderPtr pMatrix, const Vector4& colour ):
			pMatrix_( pMatrix ),
			colour_(Colour::getUint32(colour))
		{
		};

		MatrixProviderPtr pMatrix_;
		uint32	 colour_;
	};

	class TextureLayer
	{
	public:
		PyTextureProviderPtr pTexture_;
		Vector2			worldSize_;
		Vector2			worldAnchor_;
		Vector4			clipRegion_;
	};	
	void calculateClipRegions( float relParentWidth, float relParentHeight );

	MatrixProviderPtr				viewpoint_;
	float							range_;	
	float							currentRange_;
	float							zoomSpeed_;
	bool							rotate_;
	MinimapHandle					currHandle_;
	Moo::BaseTexturePtr				mask_;
	typedef BW::map<MinimapHandle, EntryInfo> EntryMap;
	EntryMap						entries_;
	bool							mouseEntryPicking_;
	MinimapHandle					pickedEntry_;
	typedef BW::multimap<uint32, TextureLayer> TextureLayers;
	TextureLayers					textureLayers_;

	//This minimap has batching support for many entries,
	//these are called 'simple entries' because they cannot
	//have their own gui component, just a colour.
	typedef BW::map<MinimapHandle, SimpleEntry> SimpleEntryMap;
	SimpleEntryMap					simpleEntries_;
	Moo::BaseTexturePtr				simpleEntryMap_;
	float							simpleEntrySize_;
	Vector2							pointSizeScaling_;
	bool							simpleEntriesVisible_;
	typedef Moo::VertexXYZDUV		MinimapVertex;
	CustomMesh<MinimapVertex>		simpleEntriesVB_;

	float							margin_;
	Vector4							mapCoords_;

	//This minimap also has batching support for drawing
	//cell boundaries.  This is for tech demos and can
	//be removed easily
#if INCLUDE_CELL_BOUNDARY_VIZ
	CustomMesh<MinimapVertex>		cellBoundariesVB_;
	void updateCellBoundaries(
		const Vector3& camPos,
		float relParentWidth,
		float relParentHeight );
	BW::vector<float>				cellBounds_;
	PySTLSequenceHolder< BW::vector<float> >	cellBoundsHolder_;
	bool							cellBoundsVisible_;
#endif	

	COMPONENT_FACTORY_DECLARE( Minimap() )
};


#ifdef CODE_INLINE
#include "minimap.ipp"
#endif

BW_END_NAMESPACE

#endif // MINIMAP_HPP
