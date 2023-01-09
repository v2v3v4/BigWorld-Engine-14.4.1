#ifndef ENTITY_PROPERTY_PARSER_HPP
#define ENTITY_PROPERTY_PARSER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This interface is used by EditorChunkEntity to parse entity properties
 *  and their widgets, such as ENUM, RADIUS, etc. Extend it in the cpp to
 *  add support for other widgets.
 */
class EntityPropertyParser : public ReferenceCount
{
public:
	// interface
	virtual GeneralProperty* createProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType,
		DataSectionPtr widget,
		MatrixProxy* pMP ) = 0;


	// statics
	class Factory: public ReferenceCount
	{
	public:
		virtual EntityPropertyParserPtr create( PyObject* pyClass,
			const Name& name, DataTypePtr dataType, DataSectionPtr widget ) = 0;
	};
	typedef SmartPointer<Factory> FactoryPtr;

	static void registerFactory( FactoryPtr factory );
	static EntityPropertyParserPtr create(
		PyObject* pyClass, const Name& name, DataTypePtr dataType, DataSectionPtr widget );

private:
	static BW::vector<FactoryPtr> s_factories_;
};


BW_END_NAMESPACE

#endif // ENTITY_PROPERTY_PARSER_HPP
