#ifndef SCENE_OBJECT_FLAGS_HPP
#define SCENE_OBJECT_FLAGS_HPP

#include "cstdmf/bw_hash.hpp"
#include "cstdmf/stdmf.hpp"
#include "forward_declarations.hpp"
#include "scene_dll.hpp"

namespace BW
{

class SCENE_API SceneObjectFlags
{
public:
	
	typedef uint8 StorageType;

	enum Flags
	{
		IS_DYNAMIC_OBJECT = 1 << 0,
		IS_REFLECTION_VISIBLE = 1 << 1,
		IS_CASTING_STATIC_SHADOW = 1 << 2,
		IS_CASTING_DYNAMIC_SHADOW = 1 << 3,
		IS_TERRAIN_OBJECT = 1 << 4
	};

	SceneObjectFlags();
	SceneObjectFlags( const SceneObjectFlags& other );
	explicit SceneObjectFlags( StorageType flags );

	bool isDynamic() const;
	void isDynamic( bool value );

	bool isReflectionVisible() const;
	void isReflectionVisible( bool value );

	bool isCastingStaticShadow() const;
	void isCastingStaticShadow( bool value );

	bool isCastingDynamicShadow() const;
	void isCastingDynamicShadow( bool value );

	bool isTerrain() const;
	void isTerrain( bool value );

private:
	friend struct BW_HASH_NAMESPACE hash<SceneObjectFlags>;

	StorageType flags_;
};

} // namespace BW

BW_HASH_NAMESPACE_BEGIN

template<>
struct hash<BW::SceneObjectFlags> : 
	public std::unary_function<BW::SceneObject, std::size_t>
{
	std::size_t operator()(const BW::SceneObjectFlags& s) const
	{
		hash<BW::SceneObjectFlags::StorageType> hasher;
		return hasher( s.flags_ );
	}
};

BW_HASH_NAMESPACE_END

#endif // SCENE_OBJECT_FLAGS_HPP

