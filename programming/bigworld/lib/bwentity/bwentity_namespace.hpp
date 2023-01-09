#if !defined(BWENTITY_BW_NAMESPACE_HPP)
#define BWENTITY_BW_NAMESPACE_HPP

#define BWENTITY_BW_BEGIN_NAMESPACE		namespace BW{

#define BWENTITY_BW_END_NAMESPACE			}

#define BWENTITY_BEGIN_NAMESPACE	BWENTITY_BW_BEGIN_NAMESPACE 	namespace EntityDef{
#define BWENTITY_END_NAMESPACE		} BWENTITY_BW_END_NAMESPACE

/**
 * BW classes forward declarations
 */
BWENTITY_BW_BEGIN_NAMESPACE

class BaseUserDataObjectDescription;
class BinaryIStream;
class BinaryOStream;
class DataDescription;
class DataSink;
class DataSource;
class EntityDefConstants;
class EntityDelegate;
class IEntityDelegateFactory;
class EntityDescription;
class EntityDescriptionMap;
class EntityMethodDescriptions;
class MemberDescription;
class MethodDescription;
class ScriptDataSink;
class ScriptDataSource;
class ScriptDict;
class ScriptObject;

BWENTITY_BW_END_NAMESPACE

/**
 * BW::EntityDef classes forward and type declarations
 */
BWENTITY_BEGIN_NAMESPACE

class EntityDescriptionMap;
class EntityDescriptionMapFactory;

BWENTITY_END_NAMESPACE

#endif /* BWENTITY_BW_NAMESPACE_HPP */
