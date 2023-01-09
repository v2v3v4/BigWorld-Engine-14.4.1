#ifndef GENERAL_PROPERTIES_HPP
#define GENERAL_PROPERTIES_HPP

/**
 *	@file	This file contains implementations of real properties used
 *	by all sorts of objects, and supporting classes. The general editor
 *	file contains the abstract base classes for these.
 */

#include "general_editor.hpp"
#include "undoredo.hpp"
#include "resmgr/datasection.hpp"
#include "moo/moo_math.hpp" // For Moo::Colour

BW_BEGIN_NAMESPACE

class Matrix;

class ChunkItem;
typedef SmartPointer<ChunkItem> ChunkItemPtr;

class MatrixProxy;
typedef SmartPointer<MatrixProxy> MatrixProxyPtr;

/**
 *	Default implementation for MatrixProxy::getChunkItemDefault.
 */
class MatrixProxyCreator : public SafeReferenceCount
{
public:
	virtual MatrixProxyPtr operator()( ChunkItemPtr pItem ) = 0;
};

typedef SmartPointer<MatrixProxyCreator> MatrixProxyCreatorPtr;

/**
 *	This is an interface to get and set a matrix. It is intended to abstract
 *	the complexities often involved in modifying a matrix, such as that
 *	which controls the position of a chunk item.
 */
class MatrixProxy : public ReferenceCount
{
public:
	typedef Matrix Data;
	virtual ~MatrixProxy() { }
	virtual Matrix get( bool world = true )
	{
		Matrix m;
		getMatrix( m, world );
		return m;
	}
	virtual void getMatrix( Matrix & m, bool world = true ) = 0;
	virtual void getMatrixContext( Matrix & m ) = 0;
	virtual void getMatrixContextInverse( Matrix & m ) = 0;
	virtual bool setMatrix( const Matrix & m ) = 0;
	virtual void setMatrixAlone( const Matrix & m ) {}

	virtual void recordState() = 0;
	virtual bool commitState( bool revertToRecord = false,
		bool addUndoBarrier = true ) = 0;

	/** If the state has changed since the last call to recordState() */
	virtual bool hasChanged() = 0;

	static MatrixProxyPtr getChunkItemDefault( ChunkItemPtr pItem );
	static void setMatrixProxyCreator( MatrixProxyCreatorPtr creator );
private:
	static MatrixProxyCreatorPtr s_pMatrixProxyCreator_;
};


/**
 *	This is a Matrix property .
 */
class GenMatrixProperty : public GeneralProperty
{
public:
	GenMatrixProperty( const Name& name, MatrixProxyPtr pMatrix );

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( MATRIX ); }

	MatrixProxyPtr	pMatrix()	{ return pMatrix_; }

	virtual PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

private:
	MatrixProxyPtr	pMatrix_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( GenMatrixProperty )
};


/**
 *	This is a position property that is based off a MatrixProxyPtr
 */
class GenPositionProperty : public GeneralProperty
{
public:
	GenPositionProperty( const Name& name, MatrixProxyPtr pMatrix, float size = 1000000.f );

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( VECTOR ); }

	MatrixProxyPtr	pMatrix()	{ return pMatrix_; }
	float size()	{	return size_;	}

	virtual PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

private:
	MatrixProxyPtr	pMatrix_;
	float size_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( GenPositionProperty )
};


/**
 *	This is a rotation property that is based off a MatrixProxyPtr
 */
class GenRotationProperty : public GeneralProperty
{
public:
	GenRotationProperty
	( 
		const Name&				name, 
		MatrixProxyPtr			pMatrix,
		bool					allowGizmo			= true
	);

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( VECTOR ); }

	MatrixProxyPtr	pMatrix()	{ return pMatrix_; }
	bool allowGizmo() const { return allowGizmo_; }

	virtual PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

private:
	MatrixProxyPtr	pMatrix_;
	bool			allowGizmo_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( GenRotationProperty )
};


/**
 *	This is a scale property that is based off a MatrixProxyPtr
 */
class GenScaleProperty : public GeneralProperty
{
public:
	GenScaleProperty
	(
		const Name&				name, 
		MatrixProxyPtr			pMatrix,
		bool					allowNonUniformScale	= true,
		bool					allowUniformScale		= true
	);

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( VECTOR ); }

	MatrixProxyPtr	pMatrix()	{ return pMatrix_; }

	bool allowNonUniformScale() const { return allowNonUniformScale_; }
	bool allowUniformScale() const { return allowUniformScale_; }

	virtual PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

private:
	MatrixProxyPtr	pMatrix_;
	bool			allowNonUniformScale_;
	bool			allowUniformScale_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( GenScaleProperty )
};




/**
 *	This class wraps up a float with virtual functions to get and set it.
 *
 *	Note: When set permanently, it will first be set transiently back to
 *	its original value (if there were intermediate transient sets)
 */
class FloatProxy : public ReferenceCount
{
public:
	typedef float Data;

	virtual float get() const = 0;
	virtual void set( float f, bool transient,
							bool addBarrier = true ) = 0;
	virtual bool getRange( float& min, float& max, int& digits ) const
	{
		return false;
	}
	virtual bool getDefault( float& def ) const
	{
		return false;
	}
	virtual bool isDefault() const	{	return false;	}
	virtual void setToDefault(){}
};

typedef SmartPointer<FloatProxy> FloatProxyPtr;


/**
 *	This is a general float property that is based off a FloatProxyPtr
 */
class GenFloatProperty : public GeneralProperty
{
public:
	GenFloatProperty( const Name& name, FloatProxyPtr pFloat );

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( FLOAT ); }

	FloatProxyPtr	pFloat()	{ return pFloat_; }

	virtual PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

private:
	FloatProxyPtr	pFloat_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( GenFloatProperty )
};


/**
 *	This is a radius property that is based off a FloatProxyPtr
 */
class GenRadiusProperty : public GenFloatProperty
{
public:
	GenRadiusProperty( const Name& name,
		FloatProxyPtr pFloat,
		MatrixProxyPtr pCenter,
		uint32 widgetColour = 0xffff0000,
		float widgetRadius = 2.0f );

	MatrixProxyPtr	pCenter()		{ return pCenter_; }
	uint32			widgetColour()	{ return widgetColour_; }
	float			widgetRadius()	{ return widgetRadius_; }

private:
	MatrixProxyPtr	pCenter_;
	uint32			widgetColour_;
	float			widgetRadius_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( GenRadiusProperty )
};


/**
 *	This class wraps up a Vector4 with virtual functions to get and set it.
 *
 *	@see FloatProxy
 */
class Vector4Proxy : public ReferenceCount
{
public:
	typedef Vector4 Data;

	virtual Vector4 get() const = 0;
	virtual void set( Vector4 v, bool transient,
							bool addBarrier = true ) = 0;
};

typedef SmartPointer<Vector4Proxy> Vector4ProxyPtr;


/**
 *	This is a Vector4 property .
 */
class Vector4Property : public GeneralProperty
{
public:
	Vector4Property( const Name& name, Vector4ProxyPtr pColour );

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( VECTOR ); }

	Vector4ProxyPtr	pVector4()	{ return pVector4_; }

	virtual PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

private:
	Vector4ProxyPtr	pVector4_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( Vector4Property )
};


/**
 *	This class wraps up a Vector3 with virtual functions to get and set it.
 *
 *	@see FloatProxy
 */
class Vector3Proxy : public ReferenceCount
{
public:
	typedef Vector3 Data;

	virtual Vector3 get() const = 0;
	virtual void set( Vector3 v, bool transient,
							bool addBarrier = true ) = 0;
};

typedef SmartPointer<Vector3Proxy> Vector3ProxyPtr;


/**
 *	This is a Vector3 property .
 */
class Vector3Property : public GeneralProperty
{
public:
	Vector3Property( const Name& name, Vector3ProxyPtr pColour );

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( VECTOR ); }

	Vector3ProxyPtr	pVector3()	{ return pVector3_; }

	virtual PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

private:
	Vector3ProxyPtr	pVector3_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( Vector3Property )
};


/**
 *	This class wraps up a Vector2 with virtual functions to get and set it.
 *
 *	@see FloatProxy
 */
class Vector2Proxy : public ReferenceCount
{
public:
	typedef Vector2 Data;

	virtual Vector2 get() const = 0;
	virtual void set( Vector2 v, bool transient,
							bool addBarrier = true ) = 0;

	virtual bool getRange( Vector2& min, Vector2& max, int& digits ) const
	{
		return false;
	}
};


typedef SmartPointer<Vector2Proxy> Vector2ProxyPtr;


/**
 *	This is a Vector2 property .
 */
class Vector2Property : public GeneralProperty
{
public:
	Vector2Property( const Name& name, Vector2ProxyPtr pColour );

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( VECTOR ); }

	Vector2ProxyPtr	pVector2()	{ return pVector2_; }

	virtual PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

private:
	Vector2ProxyPtr	pVector2_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( Vector2Property )
};


/**
 *	This class wraps up a colour with virtual functions to get and set it.
 *
 *	@see FloatProxy
 */
class ColourProxy : public ReferenceCount
{
public:
	typedef Moo::Colour Data;

	virtual Moo::Colour get() const = 0;
	virtual void set( Moo::Colour v, bool transient,
							bool addBarrier = true ) = 0;
};

typedef SmartPointer<ColourProxy> ColourProxyPtr;


/**
 *	This is a colour property that can come from either a ColourProxy or a Vector4Proxy
 */
class ColourProperty : public GeneralProperty
{
public:
	ColourProperty( const Name& name, ColourProxyPtr pColour );
	ColourProperty( const Name& name, Vector4ProxyPtr pVector4 );

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( COLOUR ); }

	ColourProxyPtr	pColour()	{ return pColour_; }
	Vector4ProxyPtr	pVector()	{ return pVector4_; }

	virtual PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

private:
	ColourProxyPtr	pColour_;
	Vector4ProxyPtr	pVector4_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( ColourProperty )
};


/**
 *	This class wraps up a colour and a scalar with virtual functions to get and set it.
 *	The scalar part can be clamped to whatever range you like.
 *
 *	@see Vector4Proxy
 */
class ColourScalarProxy : public Vector4Proxy
{
public:
	virtual bool getRange( float& min, float& max, int& digits ) const = 0;	
};
typedef SmartPointer<ColourScalarProxy> ColourScalarProxyPtr;


/**
 *	This is an RGB only colour property with an associated float scalar
 */
class ColourScalarProperty : public GeneralProperty
{
public:
	ColourScalarProperty( const Name& name, const Name& scalarName, ColourScalarProxyPtr pColour );	

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( COLOUR ); }

	ColourScalarProxyPtr	pColour()	{ return pColour_; }
	const Name& scalarName() const { return scalarName_; }

	virtual PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

private:
	ColourScalarProxyPtr	pColour_;
	Name					scalarName_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( ColourScalarProperty )
};




/**
 *	This is an angle property that is based off a FloatProxyPtr
 */
class AngleProperty : public GenFloatProperty
{
public:
	AngleProperty( const Name& name,
		FloatProxyPtr pFloat,
		MatrixProxyPtr pCenter );

	MatrixProxyPtr	pCenter()	{ return pCenter_; }

private:
	MatrixProxyPtr	pCenter_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( AngleProperty )
};


/**
 *	This class wraps up a string with virtual functions to get and set it.
 *
 *	@see FloatProxy
 */
class StringProxy : public ReferenceCount
{
public:
	typedef BW::string Data;

	virtual BW::string get() const = 0;
	virtual void set( BW::string v, bool transient,
							bool addBarrier = true ) = 0;
};

typedef SmartPointer<StringProxy> StringProxyPtr;



/**
 *	This is a simple read-only text property
 */
class StaticTextProperty : public GeneralROProperty
{
public:
	StaticTextProperty( const Name& name, StringProxyPtr text,
						bool isDate = false );

	virtual const ValueType & valueType() const;

	PyObject * pyGet();

	StringProxyPtr	pString()	{ return text_; }
	
private:
	StringProxyPtr		text_;
	bool				isDate_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( StaticTextProperty )
};


/**
 *	This is a simple read-only text property
 */
class TextLabelProperty : public GeneralROProperty
{
public:
	TextLabelProperty( const Name& name, void * userObject = NULL, bool highlight = false );

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( STRING ); }

	void * getUserObject() { return userObject_; }
	bool highlight() { return highlight_; }

private:
	void * userObject_;
	bool highlight_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( TextLabelProperty )
};


/**
 *	This is a simple text string property. Its use is discouraged
 *	for anything other than names or descriptions.
 */
class TextProperty : public GeneralProperty
{
public:
	TextProperty( const Name& name, StringProxyPtr text );

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( STRING ); }

	PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

	StringProxyPtr	pString()	{ return text_; }
	
	void fileFilter( const Name& fileFilter )	{ fileFilter_ = fileFilter; }
	const Name& fileFilter() const { return fileFilter_; }

	void defaultDir( const BW::wstring& defaultDir )	{ defaultDir_ = defaultDir; }
	const BW::wstring& defaultDir()	{ return defaultDir_; }

	void canTextureFeed( bool val ) { canTextureFeed_ = val; }
	bool canTextureFeed() { return canTextureFeed_; }
	
	void textureFeed( const BW::wstring& textureFeed )	{ textureFeed_ = textureFeed; }
	const BW::wstring& textureFeed()	{ return textureFeed_; }

private:
	StringProxyPtr		text_;
	Name				fileFilter_;
	BW::wstring		defaultDir_;
	bool				canTextureFeed_;
	BW::wstring		textureFeed_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( TextProperty )
};


/**
 *	This is a read-only text property that represents an ID string
 */
class IDProperty : public GeneralProperty
{
public:
	IDProperty( const Name& name, StringProxyPtr text );

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( STRING ); }

	PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

	StringProxyPtr	pString()	{ return text_; }

private:
	StringProxyPtr		text_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( IDProperty )
};


/**
 *	This is a fake property that groups things to help the user interface
 */
class GroupProperty : public GeneralProperty
{
public:
	GroupProperty( const Name& name );

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( STRING ); }

#if 0
	PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );
#endif // 0

private:
	GENPROPERTY_VIEW_FACTORY_DECLARE( GroupProperty )
};


/**
 *	This is a string list property.
 */
class ListTextProperty : public GeneralProperty
{
public:
	ListTextProperty( const Name& name, StringProxyPtr text, 
		const BW::vector<BW::wstring> & possibleValues );

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( STRING ); }

	PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

	StringProxyPtr	pString()	{ return text_; }

	BW::vector<BW::wstring>& possibleValues() { return possibleValues_; }

private:
	StringProxyPtr		text_;
	BW::vector<BW::wstring> possibleValues_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( ListTextProperty )
}


/**
 *	This is a general resource property based of a StringProxyPtr being
 *	the id of the resource. It can take an optional extension for the
 *	resource id's extension (which should include the period) and an
 *	optional checker for its contents.
 */
class ResourceProperty : public GeneralProperty
{
public:
	/**
	 *	This class checks whether or not the given resource is suitable
	 *	for setting into the property. The check should be fast rather
	 *	than 100% accurate - it's ok for a property set to fail (esp. if
	 *	using UndoableDataProxy) even if the checker says it's fine.
	 */
	class Checker
	{
	public:
		virtual bool check( DataSectionPtr pRoot ) const { return true; }

		static Checker instance;
	};
	
	ResourceProperty( const Name& name, StringProxyPtr pString,
		const BW::string & extension = "",
		const Checker & checker = Checker::instance );

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( STRING ); }

	StringProxyPtr		pString()		{ return pString_; }
	const BW::string &	extension()		{ return extension_; }
	const Checker &		checker()		{ return checker_; }

	virtual PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

private:
	StringProxyPtr	pString_;
	BW::string		extension_;
	const Checker &	checker_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( ResourceProperty )
};




/**
 *	This class wraps up a bool with virtual functions to get and set it.
 *
 *	@see FloatProxy
 */
class BoolProxy : public ReferenceCount
{
public:
	typedef bool Data;

	virtual bool get() const = 0;
	virtual void set( bool v, bool transient,
							bool addBarrier = true ) = 0;
};

typedef SmartPointer<BoolProxy> BoolProxyPtr;


/**
 *	This is a general bool property that is based off a BoolProxyPtr
 *
 *	I wonder now whether the whole data proxy path should have been taken
 *	at all, rather than just deriving from the general properties and
 *	implementing more stuff on top of them. Well, at least tool functors
 *	can take a proxy as input instead of the whole property... hmmmm.
 *	Not very convincing.
 */
class GenBoolProperty : public GeneralProperty
{
public:
	GenBoolProperty( const Name& name, BoolProxyPtr pBool );

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( BOOL ); }

	BoolProxyPtr	pBool()	{ return pBool_; }

	virtual PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

private:
	BoolProxyPtr	pBool_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( GenBoolProperty )
};



/**
 *	This class wraps up an int with virtual getter/setter functions.
 *
 *	@see FloatProxy
 */
class IntProxy : public ReferenceCount
{
public:
	typedef int32 Data;

	virtual Data get() const = 0;
	virtual void set( Data v, bool transient,
							bool addBarrier = true ) = 0;
	virtual bool getRange( Data & min, Data & max ) const
	{
		return false;
	}
};

typedef SmartPointer<IntProxy> IntProxyPtr;



/**
 *	This class wraps up an unsigned int with virtual getter/setter functions.
 *
 *	@see FloatProxy
 */
class UIntProxy : public ReferenceCount
{
public:
	typedef uint32 Data;

	virtual Data get() const = 0;
	virtual void set( Data v, bool transient,
							bool addBarrier = true ) = 0;
	virtual bool getRange( Data & min, Data & max ) const
	{
		return false;
	}
};

typedef SmartPointer<UIntProxy> UIntProxyPtr;



/**
 *	This is a general int property that is based off an IntProxyPtr
 */
class GenIntProperty : public GeneralProperty
{
public:
	GenIntProperty( const Name& name, IntProxyPtr pInt );

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( INT ); }

	IntProxyPtr	pInt()	{ return pInt_; }

	virtual PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

private:
	IntProxyPtr		pInt_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( GenIntProperty )
};


/**
 *	This is a general unsigned int property that is based off an UIntProxyPtr
 */
class GenUIntProperty : public GeneralProperty
{
public:
	GenUIntProperty( const Name& name, UIntProxyPtr pUInt );

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( INT ); }

	UIntProxyPtr pUInt()	{ return pUInt_; }

	virtual PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

private:
	UIntProxyPtr	pUInt_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( GenUIntProperty )
};


/**
 *	This is a choice property. Like an int property, just better
 *	as the data is represented to the user as strings
 */
class ChoiceProperty : public GeneralProperty
{
public:
	ChoiceProperty( const Name& name, IntProxyPtr pInt,
		DataSectionPtr pChoices, bool sanitiseNames_ = false );

	ChoiceProperty( const Name& name, UIntProxyPtr pUInt,
		DataSectionPtr pChoices, bool sanitiseNames_ = false );

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( STRING ); }

	DataSectionPtr pChoices();

	virtual PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

	BW::string getName(const BW::string& name, DataSectionPtr section) const;

	uint32 proxyGet() const
	{
		return pInt_ ? uint32( pInt_->get() ) : pUInt_->get();
	}

	void proxySet( uint32 v, bool transient, bool addBarrier)
	{
		return pInt_ ? pInt_->set( int( v ), transient, addBarrier ) : pUInt_->set( v, transient, addBarrier );
	}

private:
	IntProxyPtr		pInt_;
	UIntProxyPtr	pUInt_;
	DataSectionPtr	pChoices_;
	bool			sanitise_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( ChoiceProperty )
}

class PythonProxy : public ReferenceCount
{
public:
	typedef PyObjectPtr Data;

	virtual PythonProxy::Data get() const = 0;
	virtual void set( PythonProxy::Data value, bool transient,
							bool addBarrier = true ) = 0;
};

typedef SmartPointer<PythonProxy> PythonProxyPtr;


/**
 *	This is a Python property.
 */
class PythonProperty : public GeneralProperty
{
public:
	PythonProperty( const Name& name, PythonProxyPtr pProxy );

	virtual PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

private:
	PythonProxyPtr	pProxy_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( PythonProperty )
}


/**
 *	This class wraps up an array with virtual functions to get and set it.
 */
class ArrayProxy : public ReferenceCount
{
public:
	virtual bool readOnly()	{	return false;	}
	virtual BW::string asString() const { return ""; }

	virtual void elect( GeneralProperty* parent ) = 0;
	virtual void expel( GeneralProperty* parent ) = 0;
	virtual void select( GeneralProperty* parent ) = 0;

	virtual bool addItem() = 0;
	virtual void delItem( int index ) = 0;
	virtual bool delItems() = 0;
};
typedef SmartPointer<ArrayProxy> ArrayProxyPtr;


/**
 *	This is an array property.
 */
class ArrayProperty : public GeneralProperty
{
public:
	ArrayProperty( const Name& name, ArrayProxyPtr proxy );

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( STRING ); }

	virtual void elect();
	virtual void expel();
	virtual void select();
	void delItem( int index )	{	proxy_->delItem( index );	}

	virtual PyObject * pyGet();

	virtual ArrayProxyPtr proxy() const;

private:
	ArrayProxyPtr proxy_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( ArrayProperty )
};


/**
 *	This is an operation on a piece of data through an undoable proxy
 */
template <class DT> class DataProxyOperation : public UndoRedo::Operation
{
public:
	typedef DataProxyOperation<DT> This;
    typedef typename DT::Data DT_Data;

	DataProxyOperation( SmartPointer<DT> pProxy, const DT_Data oVal ) :
		UndoRedo::Operation( size_t(typeid(This).name()) ),
		pProxy_( pProxy ),
		oVal_( oVal )
	{ }

private:

	virtual void undo()
	{
		// first add the current state of this proxy to the undo/redo list
		UndoRedo::instance().add( new This( pProxy_, pProxy_->get() ) );

		// now change the value back
		pProxy_->setPermanent( oVal_ );
	}

	virtual bool iseq( const UndoRedo::Operation & oth ) const
	{		
		return pProxy_.getObject() ==
			static_cast<const DataProxyOperation<DT>&>( oth ).pProxy_.getObject();
	}

	SmartPointer<DT>	pProxy_;
	DT_Data				oVal_;
};


/**
 *	This helper class is an undoable proxy for all kinds of simple data.
 *
 *	Its template argument is some data proxy, such as FloatProxy.
 */
template <class DT> class UndoableDataProxy : public DT
{
public:
	virtual void setTransient( typename DT::Data f ) = 0;
	virtual bool setPermanent( typename DT::Data f ) = 0;
	virtual BW::string opName() = 0;

private:
	virtual void set( typename DT::Data f, bool transient,
							bool addBarrier = true )
	{
		DT::Data oVal = this->get();
		if (transient)
		{
			UndoRedo::instance().add(
			new DataProxyOperation< UndoableDataProxy<DT> >(
				this, oVal ) );
			this->setTransient( f );
			return;
		}

		// see if it likes that value
		if (!this->setPermanent( f ))
		{
			// nope, set it back to what it was
			// (if transient sets are used, transient value must be
			//  reset to initial before setting permanently)
			this->setPermanent( oVal );
			return;
		}

		// make an undo operation for it then
		UndoRedo::instance().add(
			new DataProxyOperation< UndoableDataProxy<DT> >(
				this, oVal ) );

		if (addBarrier)
		{
			// set the barrier with a meaningful name
			UndoRedo::instance().barrier( this->opName(), false );
		}
	}
};


/**
 *	This helper class is a proxy that uses simple a getter and a setter to
 *	edit a property.
 */
template <class CL, class DT> class GetterSetterProxy :
	public UndoableDataProxy<DT>
{
public:
	typedef typename DT::Data (CL::*GetFn)() const;
	typedef bool (CL::*SetFn)( const typename DT::Data & v );

	GetterSetterProxy( SmartPointer<CL> pItem,
			const BW::string & propDesc, GetFn getFn, SetFn setFn ) :
		pItem_( pItem ),
		propDesc_( propDesc ),
		getFn_( getFn ),
		setFn_( setFn )
	{
	}

	virtual typename DT::Data get() const
	{
		return ((*pItem_).*getFn_)();
	}

	virtual void setTransient( typename DT::Data v )
	{
		((*pItem_).*setFn_)( v );
	}

	virtual bool setPermanent( typename DT::Data v )
	{
		return ((*pItem_).*setFn_)( v );
	}

	virtual BW::string opName()
	{
		return "Set " + propDesc_;
	}

private:
	SmartPointer<CL>	pItem_;
	BW::string			propDesc_;
	GetFn				getFn_;
	SetFn				setFn_;
};


/**
 *	This helper class is a proxy for data that remains constant.
 */
template <class DT> class ConstantDataProxy : public DT
{
public:
	typedef typename DT::Data DT_Data;
	ConstantDataProxy( const DT_Data & val ) : val_( val ) { }

	virtual DT_Data get() const { return val_; }
	virtual void set( DT_Data v, bool transient,
							bool addBarrier = true ) { }

private:
	DT_Data	val_;
};


/**
 *	This helper class refresh the editors after modifying properties
 */
class PropertyModifyGuard
{
	GeneralEditor::Editors editors;
	static int s_stackDepth_;
public:
	PropertyModifyGuard()
		: editors( GeneralEditor::currentEditors() )
	{
		++s_stackDepth_;
	}
	~PropertyModifyGuard()
	{
		--s_stackDepth_;

		if (s_stackDepth_ == 0)
		{
			GeneralEditor::currentEditors( GeneralEditor::Editors() );
			GeneralEditor::currentEditors( editors );
		}
	}
};


BW_END_NAMESPACE
#endif // GENERAL_PROPERTIES_HPP
