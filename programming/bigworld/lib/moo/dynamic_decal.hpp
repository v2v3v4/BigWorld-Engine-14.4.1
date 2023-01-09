#pragma once

#include "cstdmf/singleton.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

class Vector3;
class Vector2;

class DynamicDecalManager: public Singleton<DynamicDecalManager>
{
	struct DecalGroup
	{
		float m_lifeTime;
		BW::string m_name;
		DecalGroup(const BW::string& name, float lifeTime): m_name(name), m_lifeTime(lifeTime) {}
	};
	typedef BW::vector<DecalGroup> DecalGroups;
	DecalGroups m_groups;

	typedef BW::vector<BW::string> TextureMap;
	TextureMap m_textures;

	int32 cur_idx;
	float decalObjectImpactSizeCoefficient; // size coefficient of decal on impact into complex geometry objects to reduce artifacts
public:
	DynamicDecalManager();
	int addDecalGroup(const BW::string& groupName,	float lifeTime);
	float getLifeTime(const BW::string& groupName);
	float getLifeTime(int32 idx);
	int32 getGroupIndex(const BW::string& groupName);
	static void addDecal(	float lifeTime, const Vector3& start,	const Vector3& end,
							const Vector2& size, float yaw,	const BW::string& difTexName, const BW::string& bumpTexName, 
							const BW::string blenddifTexName = "", float textureBlendFactor = 0, bool useSimpleCollision = false );

	int textureIndex(const BW::string& texName);
	BW::string textureName(int idx);
};

typedef SmartPointer<DynamicDecalManager> DynamicDecalManagerPtr;

BW_END_NAMESPACE
