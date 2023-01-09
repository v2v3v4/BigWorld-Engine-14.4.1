#ifndef CHUNK_CONVERTER_HPP
#define CHUNK_CONVERTER_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/stringmap.hpp"
#include "math/matrix.hpp"

namespace BW {

class CommandLine;
class DataSection;
typedef SmartPointer<DataSection> DataSectionPtr;

namespace CompiledSpace {

class ChunkConversionContext
{
public:
	bool includeInsideChunks;
	int gridX; 
	int gridZ;
	float gridSize;
	BW::string spaceDir;
	BW::string chunkID;
	Matrix chunkTransform;
	DataSectionPtr pSpaceSettings;
	DataSectionPtr pChunkDS;
	DataSectionPtr pCDataDS;
	const ChunkConversionContext* pOverlaps;
};

class ChunkConverter
{
public:
	ChunkConverter();
	typedef ChunkConversionContext ConversionContext;

private:

	class ItemHandler
	{
	public:
		virtual void handleItem( const ConversionContext& ctx, 
			const DataSectionPtr& pItemDS, const BW::string& vloUid ) = 0;
	};

	template <typename TargetType>
	class ItemHandlerDelegate : 
		public ItemHandler
	{
	public:
		typedef void (TargetType::*HandlerFunc)(
			const ConversionContext&, 
			const DataSectionPtr&, 
			const BW::string& );

		ItemHandlerDelegate( TargetType* pTarget, 
			HandlerFunc func )
			: pTarget_( pTarget )
			, pFunc_( func )
		{
		}

		virtual void handleItem( const ConversionContext& ctx,
			const DataSectionPtr& pItemDS, const BW::string& vloUid )
		{
			return (pTarget_->*pFunc_)( ctx, pItemDS, vloUid );
		}

	private:
		HandlerFunc pFunc_;
		TargetType* pTarget_;
	};

	template <typename TargetType>
	ItemHandler* createHandler( TargetType* pTarget, 
		typename ItemHandlerDelegate<TargetType>::HandlerFunc func )
	{
		ItemHandlerDelegate<TargetType>* pHandler = 
			new ItemHandlerDelegate<TargetType>( pTarget, func );
		return pHandler;
	}

	void processItem( const BW::string& itemTypeName, 
		const ConversionContext& ctx, 
		const DataSectionPtr& pItemDS, const BW::string& uid );
	void ignoreHandler( const ConversionContext& ctx, 
		const DataSectionPtr& pItemDS, const BW::string& uid );
	void convertChunk( const ConversionContext& ctx );
	void convertOverlapper( const ConversionContext& ctx, 
		const DataSectionPtr& pItemDS, const BW::string& uid );
	void convertVLO( const ConversionContext& ctx, 
		const DataSectionPtr& pItemDS, const BW::string& uid );

public:

	bool initialize( const BW::string& spaceDir,
		const DataSectionPtr& pSpaceSettings,
		CommandLine& commandLine );
	void process();

	template <typename TargetType>
	void addItemHandler( const char* itemType, TargetType* pTarget, 
		typename ItemHandlerDelegate<TargetType>::HandlerFunc func )
	{
		typeHandlers_[ itemType ].push_back( createHandler( pTarget, func ) );
	}

	void addIgnoreHandler( const char* itemType );

private:
	typedef BW::vector< ItemHandler* > HandlerList;
	typedef BW::StringHashMap< HandlerList > HandlerMap;
	typedef BW::vector< BW::string > VLOTypeList;
	typedef StringHashMap< VLOTypeList > VLOTypeMap;
	HandlerMap typeHandlers_;
	VLOTypeMap vlos_;

	float gridSize_;
	int32 boundsMinX_;		
	int32 boundsMaxX_;			
	int32 boundsMinZ_;
	int32 boundsMaxZ_;
	BW::string spaceDir_;
	bool convertInsideChunks_;
	DataSectionPtr pSpaceSettings_;
};

} // namespace CompiledSpace
} // namespace BW

#endif // CHUNK_CONVERTER_HPP
