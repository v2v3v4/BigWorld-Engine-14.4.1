#ifndef EDITOR_ENTITY_PROXY_HPP
#define EDITOR_ENTITY_PROXY_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "common/array_properties_helper.hpp"
#include "gizmo/general_properties.hpp"
#include "cstdmf/smartpointer.hpp"
#include "worldeditor/editor/link_gizmo.hpp"

BW_BEGIN_NAMESPACE

/**
 *	A helper class to access entity INT* properties
 */
class EntityIntProxy : public UndoableDataProxy<IntProxy>
{
public:
	EntityIntProxy( BasePropertiesHelper* props, int index );
	EntityIntProxy( BasePropertiesHelper* props, int index,
					int32 min, int32 max );

	virtual bool getRange( int32& min, int32& max ) const;

	virtual int32 get() const;

	virtual void setTransient( int32 i );

	virtual bool setPermanent( int32 i );

	virtual BW::string opName();

private:
	int32							min_;
	int32							max_;
	bool							ranged_;
	BasePropertiesHelper*			props_;
	int								index_;
	int32							transientValue_;
	bool							useTransient_;
};


/**
 *	A helper class to access entity UINT* properties
 */
class EntityUIntProxy : public UndoableDataProxy<UIntProxy>
{
public:
	EntityUIntProxy( BasePropertiesHelper* props, int index );
	EntityUIntProxy( BasePropertiesHelper* props, int index,
					uint32 min, uint32 max );

	virtual bool getRange( uint32& min, uint32& max ) const;

	virtual uint32 get() const;

	virtual void setTransient( uint32 i );

	virtual bool setPermanent( uint32 i );

	virtual BW::string opName();

private:
	uint32							min_;
	uint32							max_;
	bool							ranged_;
	BasePropertiesHelper*			props_;
	int								index_;
	uint32							transientValue_;
	bool							useTransient_;
};


/**
 *	A helper class to access entity FLOAT properties
 */
class EntityFloatProxy : public UndoableDataProxy<FloatProxy>
{
public:
	EntityFloatProxy( BasePropertiesHelper* props, int index );

	virtual float get() const;

	virtual void setTransient( float f );

	virtual bool setPermanent( float f );

	virtual BW::string opName();

private:
	BasePropertiesHelper*			props_;
	int								index_;
	float							transientValue_;
	bool							useTransient_;
};


/**
 *	A helper class to access entity ENUM FLOAT properties
 */
class EntityFloatEnumProxy : public UndoableDataProxy<IntProxy>
{
public:
	EntityFloatEnumProxy( BasePropertiesHelper* props, int index,
		BW::map<float,int> enumMap );

	virtual int32 get() const;

	virtual void setTransient( int32 i );

	virtual bool setPermanent( int32 i );

	virtual BW::string opName();

private:
	BasePropertiesHelper*			props_;
	int								index_;
	int32							transientValue_;
	bool							useTransient_;
	BW::map<float,int>				enumMapString_;
	BW::map<int,float>				enumMapInt_;
};


/**
 *	A helper class to access entity VECTOR2 properties
 */
class EntityVector2Proxy : public UndoableDataProxy<Vector2Proxy>
{
public:
	EntityVector2Proxy( BasePropertiesHelper* props, int index );

	virtual Vector2 get() const;

	virtual void setTransient( Vector2 v );

	virtual bool setPermanent( Vector2 v );

	virtual BW::string opName();

private:
	BasePropertiesHelper*			props_;
	int								index_;
	Vector2							transientValue_;
	bool							useTransient_;
};


/**
 *	A helper class to access entity ENUM VECTOR2 properties
 */
class EntityVector2EnumProxy : public UndoableDataProxy<IntProxy>
{
public:
	EntityVector2EnumProxy( BasePropertiesHelper* props, int index,
		BW::map<Vector2,int> enumMap );

	virtual int32 get() const;

	virtual void setTransient( int32 i );

	virtual bool setPermanent( int32 i );

	virtual BW::string opName();

private:
	BasePropertiesHelper*			props_;
	int								index_;
	int32							transientValue_;
	bool							useTransient_;
	BW::map<Vector2,int>			enumMapString_;
	BW::map<int,Vector2>			enumMapInt_;
};


/**
 *	A helper class to access entity VECTOR4 properties
 */
class EntityVector4Proxy : public UndoableDataProxy<Vector4Proxy>
{
public:
	EntityVector4Proxy( BasePropertiesHelper* props, int index );

	virtual Vector4 get() const;

	virtual void setTransient( Vector4 v );

	virtual bool setPermanent( Vector4 v );

	virtual BW::string opName();

private:
	BasePropertiesHelper*			props_;
	int								index_;
	Vector4							transientValue_;
	bool							useTransient_;
};


/**
 *	A helper class to access entity ENUM VECTOR4 properties
 */
class EntityVector4EnumProxy : public UndoableDataProxy<IntProxy>
{
public:
	EntityVector4EnumProxy( BasePropertiesHelper* props, int index,
		BW::map<Vector4,int> enumMap );

	virtual int32 get() const;

	virtual void setTransient( int32 i );

	virtual bool setPermanent( int32 i );

	virtual BW::string opName();

private:
	BasePropertiesHelper*			props_;
	int								index_;
	int32							transientValue_;
	bool							useTransient_;
	BW::map<Vector4,int>			enumMapString_;
	BW::map<int,Vector4>			enumMapInt_;
};


/**
 *	A helper class to access entity STRING properties
 */
class EntityStringProxy : public UndoableDataProxy<StringProxy>
{
public:
	EntityStringProxy( BasePropertiesHelper* props, int index );

	virtual BW::string get() const;

	virtual void setTransient( BW::string v );

	virtual bool setPermanent( BW::string v );

	virtual BW::string opName();

private:
	BasePropertiesHelper*			props_;
	int								index_;
};


/**
 *	A helper class to access entity ARRAY properties
 */
class EntityArrayProxy : public ArrayProxy
{
public:
	EntityArrayProxy( BasePropertiesHelper* props, DataTypePtr dataType,
		int index );
	virtual ~EntityArrayProxy();

	virtual void elect( GeneralProperty* parent );
	virtual void expel( GeneralProperty* parent );
	virtual void select( GeneralProperty* parent );

	virtual BasePropertiesHelper* propsHelper();
	virtual int index();
	virtual ArrayPropertiesHelper* arrayPropsHelper();

	virtual bool addItem();
	virtual void delItem( int index );
	virtual bool delItems();

private:
	BasePropertiesHelper*			props_;
	ArrayPropertiesHelper			array_;
	SequenceDataTypePtr				dataType_;
	int								index_;
	static LinkGizmoPtr				s_pGizmo_;
	static int						s_gizmoCount_;
	bool							alwaysShowGizmo_;
	bool							needsGizmo_;
	BW::vector<GeneralProperty*>	properties_;

	virtual bool deleteArrayItem( int index );
	virtual void createProperties( GeneralProperty* parent );
	virtual void clearProperties();
};
typedef SmartPointer<EntityArrayProxy> EntityArrayProxyPtr;


/**
 *	A helper class to manage linking through the link gizmo
 */
class EntityArrayLinkProxy : public LinkProxy
{
public:
	EntityArrayLinkProxy( EntityArrayProxy* arrayProxy, EditorChunkItem* item, const BW::string& propName );

	virtual LinkType linkType() const;

	virtual MatrixProxyPtr createCopyForLink();

    virtual TargetState canLinkAtPos(ToolLocatorPtr locator) const;

    virtual void createLinkAtPos(ToolLocatorPtr locator);

    virtual ToolLocatorPtr createLocator() const;

private:
	EntityArrayProxy* arrayProxy_;
	EditorChunkItem* item_;
	BW::string propName_;
};


/**
 *	A helper class to access entity ENUM STRING properties
 */
class EntityStringEnumProxy : public UndoableDataProxy<IntProxy>
{
public:
	EntityStringEnumProxy( BasePropertiesHelper* props, int index,
		BW::map<BW::string,int> enumMap );

	virtual int32 get() const;

	virtual void setTransient( int32 i );

	virtual bool setPermanent( int32 i );

	virtual BW::string opName();

private:
	BasePropertiesHelper*			props_;
	int								index_;
	int32							transientValue_;
	bool							useTransient_;
	BW::map<BW::string,int>		enumMapString_;
	BW::map<int,BW::string>		enumMapInt_;
};


/**
 *	A helper class to access the entity specific properties
 */
class EntityPythonProxy : public UndoableDataProxy<PythonProxy>
{
public:
	EntityPythonProxy( BasePropertiesHelper* props, int index );

	virtual PyObjectPtr get() const;

	virtual void setTransient( PyObjectPtr v );

	virtual bool setPermanent( PyObjectPtr v );

	virtual BW::string opName();

private:
	BasePropertiesHelper*			props_;
	int								index_;
};

BW_END_NAMESPACE

#endif // EDITOR_ENTITY_PROXY_HPP
