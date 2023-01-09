#ifndef CELLAPP_COMPARER_HPP
#define CELLAPP_COMPARER_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"

#include "cellappmgr_config.hpp"

BW_BEGIN_NAMESPACE

class CellApp;
class CellAppScorer;
class Space;


/**
 *	This class handles the set of criteria on which CellApps can be compared for
 *	applicability for use for a new cell.
 *	It is used to compare two CellApps based on these criteria.
 */
class CellAppComparer
{
public:
	CellAppComparer();

	~CellAppComparer();

	void init();

	bool isABetterCellApp( const CellApp * pOld, const CellApp * pNew,
			const Space * pSpace ) const;
	bool isValidApp( const CellApp * pApp, const Space * pSpace ) const;

	bool addScorer( const BW::string & name, const DataSectionPtr & ptr );

private:
	void addScorer( const CellAppScorer * pScorer );

	typedef BW::vector< const CellAppScorer * > Scorers;
	Scorers attributes_;
};


/**
 *	This class represents a single criterion on which CellApps can be compared.
 */
class CellAppScorer
{
public:
	virtual ~CellAppScorer() {}

	float compareCellApps( const CellApp * pOld,
			const CellApp * pNew, const Space * pSpace ) const;

	virtual bool hasValidScore( const CellApp * pApp,
		const Space * pSpace ) const
	{
		return true;
	}

	virtual bool init( const DataSectionPtr & ptr ) { return true; }

protected:
	virtual float getScore( const CellApp * pApp,
			const Space * pSpace ) const = 0;

	virtual const char * name() const = 0;
};

BW_END_NAMESPACE

#endif // CELLAPP_COMPARER_HPP

// cellapp_comparer.hpp
