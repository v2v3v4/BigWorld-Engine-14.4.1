#ifndef BACKUP_HASH_CHAIN_HPP
#define BACKUP_HASH_CHAIN_HPP

#include "network/basictypes.hpp"
#include "backup_hash.hpp"
#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;
class BinaryOStream;

class BackupHashChain
{
public:
	BackupHashChain();
	~BackupHashChain();

	void adjustForDeadBaseApp( const Mercury::Address & deadApp, 
							   const BackupHash & hash );
	Mercury::Address addressFor( const Mercury::Address & address,
								 EntityID entityID ) const;

private:
	typedef BW::map< Mercury::Address, BackupHash > History;
	History history_;

	friend BinaryOStream & operator<<( BinaryOStream & os, 
		const BackupHashChain & hashChain );
	friend BinaryIStream & operator>>( BinaryIStream & is, 
		BackupHashChain & hashChain );
};

BinaryOStream & operator<<( BinaryOStream & os, 
	const BackupHashChain & hashChain );


BinaryIStream & operator>>( BinaryIStream & is, 
		BackupHashChain & hashChain );

BW_END_NAMESPACE

#endif // BACKUP_HASH_CHAIN_HPP
