#include "cellapp_comparer.hpp"

#include "cellapp.hpp"
#include "cell_app_group.hpp"
#include "space.hpp"

#include "cellappmgr_config.hpp"

BW_BEGIN_NAMESPACE


#define DEFINE_SCORER_START( NAME )											\
class NAME : public CellAppScorer											\
{																			\
protected:																	\
	virtual float getScore( const CellApp * pApp,							\
			const Space * pSpace ) const;									\
	virtual const char * name() const { return #NAME; }

#define DEFINE_SCORER_FINISH												\
};

#define DEFINE_SCORER( NAME )												\
	DEFINE_SCORER_START ( NAME )											\
	DEFINE_SCORER_FINISH

DEFINE_SCORER( CellAppLoadScorer )
DEFINE_SCORER( CellAppGroupLoadScorer )
DEFINE_SCORER( CellCellTrafficScorer )
DEFINE_SCORER( BaseCellTrafficScorer )

DEFINE_SCORER_START( LimitedSpacesScorer )
public:
	LimitedSpacesScorer() : maxSpacesPerCellApp_( 0 ) {}
	virtual bool hasValidScore( const CellApp * pApp,
		const Space * pSpace ) const;
	virtual bool init( const DataSectionPtr & ptr );
private:
	int maxSpacesPerCellApp_;
DEFINE_SCORER_FINISH



/**
 *	Constructor
 */
CellAppComparer::CellAppComparer()
{
	this->init();
}

/**
 *	Destructor
 */
CellAppComparer::~CellAppComparer()
{
	Scorers::iterator iter = attributes_.begin();

	while (iter != attributes_.end())
	{
		bw_safe_delete( *iter );
		++iter;
	}
}


void CellAppComparer::init()
{
	const char * sectionName = "cellAppMgr/metaLoadBalancePriority";
	DataSectionPtr pSection = BWConfig::getSection( sectionName );

	if (!pSection)
	{
		ERROR_MSG( "CellAppComparer::init: Failed to open %s\n",
				sectionName );

		return;
	}

	DataSectionIterator iter;
	iter = pSection->begin();

	int numScorers = 0;

	CONFIG_INFO_MSG( "Meta load-balance prioritisation:\n" );

	while (iter != pSection->end())
	{
		if (this->addScorer( iter.tag(), *iter ))
		{
			CONFIG_INFO_MSG( "  %d: %s",
					++numScorers, iter.tag().c_str() );
		}

		++iter;
	}
}


/**
 *	This method generates the criteria on which CellApps are scored.
 */
bool CellAppComparer::addScorer( const BW::string & name,
	const DataSectionPtr & pSection )
{
	CellAppScorer * pScorer;


	if (name == "limitedSpaces")
	{
		pScorer = new LimitedSpacesScorer;
	}
	else if (name == "cellAppLoad")
	{
		pScorer = new CellAppLoadScorer;
	}
	else if (name == "groupLoad")
	{
		pScorer = new CellAppGroupLoadScorer;
	}
	else if (name == "cellCellTraffic")
	{
		pScorer = new CellCellTrafficScorer;
	}
	else if (name == "baseCellTraffic")
	{
		pScorer = new BaseCellTrafficScorer;
	}
	else
	{
		ERROR_MSG( "CellAppComparer::addScorer: "
				"Unknown meta load-balancing priority option '%s'\n",
				name.c_str() );
		return false;
	}

	if (!pScorer->init( pSection ))
	{
		bw_safe_delete( pScorer );
		return false;
	}
	else
	{
		this->addScorer( pScorer );
		return true;
	}
}


/**
 *	This method adds a single scoring criterion to the vector. The order the
 *	attributes are added will determine their priority, as they will be
 *	processed via iteration through them.
 */
void CellAppComparer::addScorer( const CellAppScorer * pScorer )
{
	attributes_.push_back( pScorer );
}


/**
 * This method checks if any of the scoreres rejects a CellApp
 */
bool CellAppComparer::isValidApp( const CellApp * pApp,
		const Space * pSpace ) const
{
	Scorers::const_iterator iter = attributes_.begin();

	while (iter != attributes_.end())
	{
		const CellAppScorer & attribute = **iter;
		if (!attribute.hasValidScore( pApp, pSpace ))
		{
			return false;
		}
		++iter;
	}

	return true;
}


/**
 *	This method compares two CellApps, returning true if the second is better
 *	than the first, and false otherwise.
 *	The result is calculated by comparing the CellApps on each criteria, in
 *	order of their priority, until they are found to differ on a criterion.
 */
bool CellAppComparer::isABetterCellApp(
		const CellApp * pOld, const CellApp * pNew, const Space * pSpace ) const
{
	if (pOld == NULL)
	{
		return true;
	}

	Scorers::const_iterator iter = attributes_.begin();

	while (iter != attributes_.end())
	{
		const CellAppScorer & attribute = **iter;

		float result = attribute.compareCellApps( pOld, pNew, pSpace );

		if (result < 0.f)
		{
			return false;
		}
		else if (result > 0.f)
		{
			return true;
		}

		++iter;
	}

	return false;
}


/**
 *	This method compares two CellApps using this criterion.
 *
 *	@return Float less than, equal to or greater than zero if pNew is found,
 *		respectively, to be worse, the same or better than pOld.
 */
float CellAppScorer::compareCellApps(
		const CellApp * pOld, const CellApp * pNew, const Space * pSpace ) const
{
	float oldScore = this->getScore( pOld, pSpace );
	float newScore = this->getScore( pNew, pSpace );

	// This debug information can used by QA to verify this behaviour.
	if (CellAppMgrConfig::shouldShowMetaLoadBalanceDebug())
	{
		DEBUG_MSG( "CellAppScorer::compareCellApps( %s ): "
						"Old (App %d. Score %f). New (App %d. Score %f)\n",
					this->name(),
					pOld->id(), oldScore,
					pNew->id(), newScore );
	}

	return newScore - oldScore;
}


/**
 *	This method scores a CellApp based on its load. In order to score a low
 *	load more highly than a high load, it uses the negative of the load.
 */
float CellAppLoadScorer::getScore( const CellApp * pApp,
		const Space * /*pSpace*/ ) const
{
	return -pApp->estimatedLoad();
}


/**
 *	This method scores a CellApp based on the average load of its CellApp group.
 *	In order to score a low load more highly than a high load, it uses the
 *	negative of the load.
 */
float CellAppGroupLoadScorer::getScore( const CellApp * pApp,
		const Space * /*pSpace*/ ) const
{
	CellAppGroup * pGroup = pApp->pGroup();

	if (pGroup == NULL)
	{
		return -pApp->estimatedLoad();
	}

	return -pGroup->avgLoad();
}


/**
 *	This method calculates the score for a CellApp's cell-to-cell traffic.
 *	This is determined by inspecting the other CellApps on this CellApp's group.
 *	The score is equal to the number of those CellApps that share this CellApp's
 *	physical machine. A higher value will mean a larger amount of cell-to-cell
 *	traffic will be on the same machine, reducing network load. This method
 *	returns the number of CellApps in this CellApp's group that share this
 *	CellApp's IP address.
 */
float CellCellTrafficScorer::getScore( const CellApp * pApp,
		const Space * pSpace ) const
{
	return static_cast< float >( pSpace->numCellAppsOnIP( pApp->addr().ip ) );
}


/**
 *	This method calculates the score for a CellApp's base-to-cell traffic.
 *	This is determined by comparing the IP address of the CellApp with the
 *	preferred IP of the space on which a new cell is being added. If this
 *	CellApp is running on the preferred machine, then it is likely that many
 *	of the space's Base entities will exist on that machine. This means that
 *	much of the base-to-cell traffic will occur between processes on the same
 *	machine, reducing network load.
 *	This method returns 1 if the CellApp is on the preferred IP, and 0 if not.
 */
float BaseCellTrafficScorer::getScore( const CellApp * pApp,
		const Space * pSpace ) const
{
	MF_ASSERT( pSpace );

	return (pApp->addr().ip == pSpace->preferredIP()) ? 1.f : 0.f;
}


/**
 *	This method calculates the score for a CellApp's spaces count.
 *	It always returns 0 so the CellApps are equal as we need only validity
 *	check which is performed in hasValidScore()
 */
float LimitedSpacesScorer::getScore( const CellApp * pApp,
		const Space * pSpace ) const
{
	return 0.f;
}


/**
 *  This method checks if a CellApp provided is allowed to be used
 *  @param pApp CellApp to be checked
 *  @param pSpace space to be checked
 */
bool LimitedSpacesScorer::hasValidScore( const CellApp * pApp,
		const Space * pSpace ) const
{
	int max = maxSpacesPerCellApp_;
	int cur = pApp->numCells(); // effectively equals to the number of spaces

	return cur < max;
}


/**
 * This method initializes LimitedSpacesScorer instance
 */
bool LimitedSpacesScorer::init( const DataSectionPtr & pSection )
{
	maxSpacesPerCellApp_ = pSection->asInt();
	INFO_MSG( "LimitedSpacesScorer::init: "
		"maxSpacesPerCellApp = %u.\n", maxSpacesPerCellApp_ );
	if (CellAppMgrConfig::shouldMetaLoadBalance())
	{
		WARNING_MSG( "LimitedSpacesScorer::init: "
				"unusual configuration: "
				"cellAppMgr/shouldMetaLoadBalance is true while "
				"cellAppMgr/metaLoadBalancePriority/limitedSpaces is used.\n" );
	}

	return maxSpacesPerCellApp_ > 0;
}

BW_END_NAMESPACE

// cellapp_comparer.cpp
