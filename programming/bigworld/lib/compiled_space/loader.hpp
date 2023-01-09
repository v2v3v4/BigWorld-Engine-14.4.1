#ifndef COMPILED_SPACE_LOADER_INTERFACE_HPP
#define COMPILED_SPACE_LOADER_INTERFACE_HPP

#include "compiled_space/forward_declarations.hpp"
#include "cstdmf/smartpointer.hpp"

#include "space/forward_declarations.hpp"

BW_BEGIN_NAMESPACE

	class DataSection;
	typedef SmartPointer<DataSection> DataSectionPtr;

	class Scene;

BW_END_NAMESPACE

namespace BW {

class Matrix;

namespace CompiledSpace {

class BinaryFormat;
class StringTable;

class ILoader
{
public:
	ILoader();
	virtual ~ILoader() {}

	virtual bool loadMustSucceed() const { return false; }

	bool loadFromSpace( ClientSpace * pSpace,
		BinaryFormat& reader,
		const DataSectionPtr& pSpaceSettings,
		const Matrix& mappingTransform,
		const StringTable& stringTable );
	bool bind();
	void unload();
	float loadStatus() const;

	ClientSpace& space() const;

protected:
	
	virtual float percentLoaded() const = 0;

	virtual bool doLoadFromSpace( ClientSpace * pSpace,
		BinaryFormat& reader,
		const DataSectionPtr& pSpaceSettings,
		const Matrix& mappingTransform,
		const StringTable& stringTable ) = 0;
	
	virtual bool doBind();
	virtual void doUnload() = 0;

private:
	enum LoadStatus
	{
		UNLOADED = 0,
		LOADING,
		LOADED,
		FAILED
	};

	mutable LoadStatus loadStatus_;
	ClientSpace* pSpace_;
};

} // namespace CompiledSpace
} // namespace BW

#endif // COMPILED_SPACE_LOADER_INTERFACE_HPP
