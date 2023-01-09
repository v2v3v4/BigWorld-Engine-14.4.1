#include "pch.hpp"

// Module Interface
#include "speedtree_config.hpp"

// BW Tech Headers
#include "cstdmf/debug.hpp"

#if SPEEDTREE_SUPPORT

// BW Tech Headers
#include "cstdmf/dogwatch.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/smartpointer.hpp"
#include "resmgr/bwresource.hpp"
#include "moo/render_context.hpp"
#include "romp/enviro_minder.hpp"
#include "romp/weather.hpp"

#include "speedtree_tree_type.hpp"

// SpeedTree API
#include <SpeedTreeRT.h>
#include <SpeedTreeAllocator.h>

BW_BEGIN_NAMESPACE

//-- start unnamed namespace.
//--------------------------------------------------------------------------------------------------
namespace
{
	// Named constants.
	const float c_maxWindVelocity = 15.0f;
}
//--------------------------------------------------------------------------------------------------
//-- end unnamed namespace.


namespace speedtree
{

	float							TSpeedTreeType::s_windVelX_ = 0;
	float							TSpeedTreeType::s_windVelZ_ = 0;
	bool							TSpeedTreeType::s_useMapWind = true;
	TSpeedTreeType::WindAnimation	TSpeedTreeType::s_defaultWind_;
	TSpeedTreeType::Unique			TSpeedTreeType::s_materialHandles_;
	int								TSpeedTreeType::DrawData::s_count_ = 0;
	BW::vector< BW::string > 		TSpeedTreeType::vBushNames_;

	//-- Stores the current wind information. It will later be used to animate the trees to the wind.
	//----------------------------------------------------------------------------------------------
	void TSpeedTreeType::saveWindInformation( EnviroMinder * envMinder )
	{
		BW_GUARD;
		if ( envMinder != NULL && TSpeedTreeType::s_useMapWind )
		{
			TSpeedTreeType::s_windVelX_ = envMinder->weather()->wind().x;
			TSpeedTreeType::s_windVelZ_ = envMinder->weather()->wind().y;
		}
	}

	//----------------------------------------------------------------------------------------------
	TSpeedTreeType::WindAnimation::WindAnimation()
		:	m_lastTickTime(-1.0f), m_leafAnglesCount(0), m_hasLeaves(true)
	{
		m_speedWind.reset(new CSpeedWind());
	}

	//----------------------------------------------------------------------------------------------
	TSpeedTreeType::WindAnimation::~WindAnimation( )
	{

	}

	//-- Updates all wind related data.
	//----------------------------------------------------------------------------------------------
	void TSpeedTreeType::WindAnimation::update()
	{
		BW_GUARD;

		if (m_lastTickTime != TSpeedTreeType::s_time_)
		{
			const float windVelX = TSpeedTreeType::s_windVelX_;
			const float windVelZ = TSpeedTreeType::s_windVelZ_;
			
			//-- setup SpeedTree's wind handler with our current wind direction and strength.
			Vector3 wind( windVelX, 0, windVelZ );
			float strength = std::min( wind.length()/c_maxWindVelocity, 1.0f );
			wind.normalise();
			m_speedWind->SetWindStrengthAndDirection(strength, wind.x, wind.y, wind.z);

			//-- update SpeedTree's wind handler based on the current camera position.
			float camDir[3], camPos[3];
			CSpeedTreeRT::GetCamera( camPos, camDir );
			m_speedWind->Advance(TSpeedTreeType::s_time_, true, true, camDir[0], camDir[1], camDir[2]);

			//-- update wind matrices.
			//-- ToDo: reconsider. Very ugly.
			for (uint i = 0; i < m_speedWind->GetNumWindMatrices(); ++i)
			{
				Matrix&		 oData = m_windMatrices[i];
				const float* iData = m_speedWind->GetWindMatrix(i);

				for (uint j = 0; j < 4; ++j)
					for (uint k = 0; k < 4; ++k)
						oData[j][k] = iData[j * 4 + k];
			}

			//-- update leaves angles only if we really need them.
			if (m_hasLeaves)
			{
				//-- upload leaf angle table
				static const int numAngles = 128;
				m_leafAnglesCount = m_speedWind->GetNumLeafAngleMatrices();

				MF_ASSERT(m_leafAnglesCount <= numAngles);
				
				static float windRockAngles[numAngles];
				static float windRustleAngles[numAngles];

				bool success = (
					m_speedWind->GetRockAngles(windRockAngles) &&
					m_speedWind->GetRustleAngles(windRustleAngles)
					);

				if (success)
				{
					m_anglesTable.resize(m_leafAnglesCount * 4);
					for (uint i = 0; i < m_leafAnglesCount; ++i)
					{
						m_anglesTable[i * 4 + 0] = DEG_TO_RAD(windRockAngles[i]);
						m_anglesTable[i * 4 + 1] = DEG_TO_RAD(windRustleAngles[i]);
					}
				}
			}

			//-- mark last tick as happened.
			m_lastTickTime = TSpeedTreeType::s_time_;
		}
	}

	//-- Initialise the SpeedWind object.
	//----------------------------------------------------------------------------------------------
	bool TSpeedTreeType::WindAnimation::init( const BW::string& iniFile )
	{
		BW_GUARD;

		m_speedWind->Reset();
		DataSectionPtr rootSection = BWResource::instance().rootSection();
		BinaryPtr windIniData = rootSection->readBinary( iniFile );
		if ( windIniData.exists() )
		{
			if (m_speedWind->Load(windIniData->cdata(), windIniData->len()))
			{
				//-- The number of matrices is tightly linked to the shaders...
				m_windMatrices.resize(m_speedWind->GetNumWindMatrices());
				MF_ASSERT(m_speedWind->GetNumWindMatrices( ) == 6)
				return true;
			}
		}
		return false;
	}

	//----------------------------------------------------------------------------------------------
	void TSpeedTreeType::updateWind()
	{
		BW_GUARD;

		if (TSpeedTreeType::s_playAnimation_ && pWind_)
			pWind_->update();
	}

	/**
	 *	Sets up the SpeedWind object for this tree type. First, 
	 *	try to load a tree specific setup file. If that can't be 
	 *	found, the default wind is used.
	 *
	  *	@param	speedTree		the speedtree object that will use this speedwind.
	 *	@param	datapath		data path where to look for the tree specific setup.
	 *	@param	defaultWindIni	default ini file to use if tree specific not found.
	 */
	//----------------------------------------------------------------------------------------------
	TSpeedTreeType::WindAnimation* TSpeedTreeType::setupSpeedWind(
		CSpeedTreeRT& speedTree, const BW::string& datapath, const BW::string& defaultWindIni)
	{
		BW_GUARD;

		TSpeedTreeType::WindAnimation* pWind = NULL;
		static bool first = true;
		if ( first )
		{
			first = false;
			if ( !TSpeedTreeType::s_defaultWind_.init( defaultWindIni ) )
			{
				CRITICAL_MSG( "SpeedTree default wind .ini file not found\n" );
			}
		}

		BW::string resName  = datapath + "speedwind.ini";
		// Check if there's an overriding ini
		if ( BWResource::fileExists( resName ) )
		{
			pWind = new TSpeedTreeType::WindAnimation();
			if ( !pWind->init( resName ) )
			{
				//Failed, fallback to the default wind.
				delete pWind;
				pWind = &TSpeedTreeType::s_defaultWind_;
			}
		}
		else
		{
			pWind = &TSpeedTreeType::s_defaultWind_;
		}
		speedTree.SetLeafRockingState(true);
		return pWind;
	}

	//----------------------------------------------------------------------------------------------
	void TSpeedTreeType::collectTreeDrawData(
		DrawData& drawData, bool collectBillboards, bool semiTransparent)
	{
		BW_GUARD;	

		++TSpeedTreeType::s_deferredCount_;

		//-- collect semitransparent trees only for main color pass.
		if (semiTransparent)
		{
			drawDataCollector_.m_semitransparentTrees.push_back(&drawData);
			return;
		}

		//-- collect all needed data for the later efficient rendering.
		{
			const LodData&		lod  = drawData.lod_;
			DrawData::Instance& inst = drawData.m_instData;

			//-- process branches.
			{
				//-- check all needed states to be sure we really ready to draw.
				bool readyToDraw = 
					TSpeedTreeType::s_drawBranches_ &&
					lod.branchDraw_ &&
					lod.branchLod_ < G_MAX_LOD_COUNT &&
					!treeData_.branches_.lod_.empty() &&
					treeData_.branches_.lod_[lod.branchLod_].index_
					;

				if (readyToDraw)
				{
					inst.m_materialID = static_cast<float>(materialHandle_ * 4 + 0);
					inst.m_alphaRef   = lod.branchAlpha_ / 255.0f;
					drawDataCollector_.m_branches[lod.branchLod_].push_back(inst);
				}
			}

			//-- process fronds.
			{
				//-- check all needed states to be sure we really ready to draw.
				bool readyToDraw = 
					TSpeedTreeType::s_drawFronds_ &&
					lod.frondDraw_ &&
					lod.frondLod_ < G_MAX_LOD_COUNT &&
					!treeData_.fronds_.lod_.empty() &&
					treeData_.fronds_.lod_[lod.frondLod_].index_
					;

				if (readyToDraw)
				{
					inst.m_materialID = static_cast<float>(materialHandle_ * 4 + 1);
					inst.m_alphaRef   = lod.frondAlpha_ / 255.0f;
					drawDataCollector_.m_fronds[lod.frondLod_].push_back(inst);
				}
			}

			//-- process leaves.
			{
				//-- check all needed states to be sure we really ready to draw.
				bool readyToDraw =
					TSpeedTreeType::s_drawLeaves_ &&
					lod.leafDraw_ &&
					lod.leafLodCount_ > 0 &&
					!treeData_.leaves_.lod_.empty()
					;

				if (readyToDraw)
				{
					for (int i = 0; i < lod.leafLodCount_; ++i)
					{
						int lodID = lod.leafLods_[i];
						if (lodID < G_MAX_LOD_COUNT && treeData_.leaves_.lod_[lodID].index_)
						{
							inst.m_materialID = static_cast<float>(materialHandle_ * 4 + 2);
							inst.m_alphaRef   = lod.leafAlphaValues_[i] / 255.0f;
							drawDataCollector_.m_leaves[lodID].push_back(inst);
						}
					}
				}
			}

			//-- process billboards.
			{
				//-- check all needed states to be sure we really ready to draw.
				bool readyToDraw =
					TSpeedTreeType::s_drawBillboards_ &&
					collectBillboards &&
					lod.billboardDraw_ &&
					!treeData_.billboards_.lod_.empty() &&
					treeData_.billboards_.lod_[0].index_;

				if (readyToDraw)
				{
					const float& minAlpha  = BillboardVertex::s_minAlpha_;
					const float& maxAlpha  = BillboardVertex::s_maxAlpha_;
					const float& fadeValue = lod.billboardFadeValue_;

					inst.m_materialID = static_cast<float>(materialHandle_ * 4 + 3);
					inst.m_alphaRef   = 1.0f + fadeValue * (minAlpha / maxAlpha - 1.0f);
					drawDataCollector_.m_billboards.push_back(inst);
				}
			}
		}
	}

	//----------------------------------------------------------------------------------------------
	void TSpeedTreeType::DrawDataCollector::sort()
	{
		BW_GUARD;

		//-- sorting functor.
		struct Sorter
		{
			Sorter(const Instances& instances, const Vector3& camPos) : m_instances(instances), m_camPos(camPos) { }

			bool operator() (const DrawData::Instance& lft, const DrawData::Instance& rht) const 
			{
				float sqrDistLft = (lft.m_position - m_camPos).lengthSquared();
				float sqrDistRht = (rht.m_position - m_camPos).lengthSquared();

				return sqrDistLft < sqrDistRht;
			}

			const Instances& m_instances;
			const Vector3&	 m_camPos;
		};

		//--
		const Vector3& camPos = TSpeedTreeType::s_lodCamera_.applyToOrigin();

		//--
		for (uint i = 0; i < G_MAX_LOD_COUNT; ++i)
		{
			std::sort(m_branches[i].begin(), m_branches[i].end(), Sorter(m_branches[i], camPos));
			std::sort(m_fronds[i].begin(),	 m_fronds[i].end(),	  Sorter(m_fronds[i],   camPos));
			std::sort(m_leaves[i].begin(),	 m_leaves[i].end(),	  Sorter(m_leaves[i],   camPos));
		}
		std::sort(m_billboards.begin(), m_billboards.end(), Sorter(m_billboards, camPos));
	}

	//----------------------------------------------------------------------------------------------
	void TSpeedTreeType::loadBushNames()
	{
		DataSectionPtr pDS = BWResource::instance().openSection( "speedtree/bushes.xml" );
		if ( !pDS ) return;
		for( int i = 0; i < pDS->countChildren(); i++ )
		{
			DataSectionPtr pDSName = pDS->openChild( i );
			BW::string strName = pDSName->sectionName();
			strlwr( &strName[0] );
			vBushNames_.push_back( strName );
		}
	}

	//----------------------------------------------------------------------------------------------
	bool TSpeedTreeType::isBushName( const BW::string &strName )
	{
		BW::vector< BW::string >::const_iterator it;
		for( it = vBushNames_.begin(); it != vBushNames_.end(); it++ )
			if ( strstr( strName.c_str(), it->c_str() ) != 0 )
				return true;
		return false;
	}

}  // namespace speedtree

BW_END_NAMESPACE

#endif // SPEEDTREE_SUPPORT

// speedtree_tree_type.cpp
