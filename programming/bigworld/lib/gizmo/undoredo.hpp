#ifndef UNDOREDO_HPP
#define UNDOREDO_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class Chunk;

/**
 *	This class keeps track of undo and redo stuff
 */
class UndoRedo
{
public:
	UndoRedo();
	~UndoRedo();

	bool canUndo() const;
	bool canRedo() const;

	void setSavePoint() { savePoint_ = undoList_.size(); }
	bool needsSave() const { return (savePoint_ != undoList_.size());  }
	void forceSave() { savePoint_ = undoList_.size() - 1; }

	class Environment
	{
	public:
		class Operation : public ReferenceCount
		{
		public:
			virtual ~Operation(){}
			virtual void exec() = 0;
		};
		Environment();
		virtual ~Environment();

		typedef SmartPointer<Operation> OperationPtr;
		typedef BW::set<OperationPtr> Operations;

		virtual OperationPtr internalRecord() = 0;

		static Operations record();
		static void replay( const Operations& ops );

		static BW::set<Environment*>& env();
	};
	/**
	 *	This class is one undo operation between undo/redo barriers.
	 *	Two operations are considered equal if they refer to the
	 *	same data on the same object, and are replacing not compounding
	 *
	 *	Use size_t(typeid(*this).name()) for the 'kind' of such ops.
	 */
	class Operation
	{
	public:
		Operation( size_t kind ) : kind_( kind )
		{
			env_ = Environment::record();
		}
		virtual ~Operation() { }

		virtual void undo() = 0;

		bool operator==( const Operation & oth ) const
		{
			if (kind_ != oth.kind_ || !kind_) return false;
			return this->iseq( oth );
		}

		bool operator!=( const Operation & oth ) const
		{
			return !( *this == oth );
		}

		virtual bool iseq( const Operation & oth ) const = 0;
		void record()
		{
			env_ = Environment::record();
		}
		void replay()
		{
			Environment::replay( env_ );
		}
		void addChunk( Chunk* chunk )
		{
			affectedChunks_.insert( chunk );
		}
		void markChunks();
	protected:
		size_t kind_;
		Environment::Operations env_;
		BW::set<Chunk*> affectedChunks_;
	};

	typedef BW::vector<Operation*>	Operations;


	/**
	 *	This method adds an operation to the current undo or redo list,
	 *	depending on what's happening at the time. The UndoRedo object
	 *	takes ownership of the object passed in (i.e. deletes it when done)
	 */
	void add( Operation * op )
	{
		if (!undoing_)
			this->addUndo( op );
		else
			this->addRedo( op );
	}

	void undo();
	void redo();

	bool barrierNeeded();
	void barrier( const BW::string & what, bool skipIfNoChange );

	const BW::string & undoInfo( uint32 level ) const;
	const BW::string & redoInfo( uint32 level ) const;

	/** Clears both the undo and redo lists */
	void clear();
	void markChunk();

	static UndoRedo & instance();

	bool isUndoing() const { return undoing_ || redoing_; }

	// flag which forces add() to do nothing (required for group
	// operations which use atomic sub-operations with individual undo/redo's 
	// but require group undo support)
	bool disableAdd() const { return disableAdd_; }
	void disableAdd(bool disable) { disableAdd_ = disable; }

private:
	UndoRedo( const UndoRedo& );
	UndoRedo& operator=( const UndoRedo& );

	void addUndo( Operation * op );
	void addRedo( Operation * op );

	// when changes are made, 'addUndo' is called
	// when changes are undone by 'undo', 'addRedo' is called
	// when changes are redone by 'redo', 'addUndo' is called again

	void barrierInternal( const BW::string & what );
	void clearRedos();

	class Barrier : public ReferenceCount
	{
	public:
		~Barrier();

		BW::string	what_;
		Operations	ops_;
	};

	typedef SmartPointer<Barrier> BarrierPtr;
	typedef BW::vector<BarrierPtr> Barriers;

	Barriers	undoList_;	// back is additions since latest barrier
	Barriers	redoList_;	// back is next Operations to redo
							//  (except during undo when it's additions)
	bool		undoing_;
	bool		redoing_;

	bool		disableAdd_;

	size_t		savePoint_;
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "undoredo.ipp"
#endif

#endif // UNDOREDO_HPP
