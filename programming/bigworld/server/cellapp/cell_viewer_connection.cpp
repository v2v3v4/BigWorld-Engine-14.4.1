#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "cell_viewer_connection.hpp"
#include "cellapp.hpp"
#include "cell.hpp"
#include "space.hpp"
#include "entity.hpp"
#include "math/boundbox.hpp"
#include "network/event_dispatcher.hpp"
#include "chunk/chunk_space.hpp"

DECLARE_DEBUG_COMPONENT(0)


BW_BEGIN_NAMESPACE

/**
 *	This constructor initialises the CellViewerConnection given an existing
 *	socket.
 */
CellViewerConnection::CellViewerConnection(
	Mercury::EventDispatcher & dispatcher, int fd, const CellApp & cellApp ) :
	pDispatcher_( &dispatcher ), cellApp_( cellApp )
{
	socket_.setFileDescriptor( fd );
	pDispatcher_->registerFileDescriptor( socket_.fileno(), this,
		"CellViewerConnection" );
}


/**
 *	This is the destructor.
 *	It deregisters the socket with Mercury. Note that the Endpoint
 *	destructor will close the socket.
 */
CellViewerConnection::~CellViewerConnection()
{
	pDispatcher_->deregisterFileDescriptor( socket_.fileno() );
}

/**
 *	This method is called by Mercury when the socket is ready for reading.
 *	It processes user input from the socket, and sends it to Python.
 *
 *	@param fd	Socket that is ready for reading.
 *  something interesting on fd, read something off it.
 */
int CellViewerConnection::handleInputNotification( int fd )
{
	MF_ASSERT(fd == socket_.fileno());

	char buf;

	int bytesRead = socket_.recv( &buf, sizeof(char) );

	if (bytesRead == -1)
	{
		INFO_MSG("CellViewerConnection %d: Read error\n", fd);
		delete this;
		return 1;
	}

	if (bytesRead == 0)
	{
		delete this;
		return 1;
	}

	switch (buf)
	{
		case CellViewerExport::GET_ENTITY_TYPE_NAMES:
		{
			//Get the entity types vector
			EntityTypes& entityTypes = EntityType::getTypes();

			uint32 numTypes = entityTypes.size();

			//Send the number of types first
			//use 4 bytes so that it will not break space viewer with old cellapps
			this->sendReply_brief( &numTypes, sizeof(uint32) );

			for (unsigned i=0; i<numTypes; i++)
			{
				const char* name = entityTypes[i]->name();

				//Send the string length (Pascal style)...
				uint16 nameSize = strlen(name);
				this->sendReply_brief( &nameSize, sizeof(uint16) );

				//Finally send back the type name string
				this->sendReply_brief( name, strlen(name)*sizeof(char) );
			}

			break; // All done for entity type name
		}

		case CellViewerExport::GET_ENTITIES:
		{
			// socket_ is non-blocking. This will be here, since this is sent with
			// with command (upon which this handleInputNotification executed).

			bool isOkay = false;
			CellViewerExport::EntitySelect which;
			SpaceID spaceID;
			char whichChar;
			this->receiveData(whichChar);
			this->receiveData(spaceID);
			which = (CellViewerExport::EntitySelect)whichChar;

			Space const * space = cellApp_.findSpace( spaceID );

			if( space == NULL )
			{
				WARNING_MSG( "CellViewerConnection::handleInputNotification:"
					" space not found.\n" );
			}
			else
			{
				Cell * pCell = space->pCell();
				if (pCell == NULL)
				{
					WARNING_MSG( "CellViewerConnection::handleInputNotification:"
						" cell not found.\n" );
				}
				else
				{
					isOkay = true;

					if (which == CellViewerExport::REAL)
					{
						// At this point, have reference to cell that want to
						// get entities for.
						uint32 nrec = pCell->realEntities().size();
						this->sendReply_brief( &nrec, sizeof(uint32) );

						Cell::Entities & entities = pCell->realEntities();
						for (Cell::Entities::iterator iter = entities.begin();
							   iter != entities.end(); iter++ )
						{
							Entity & entity = **iter;
							// TODO: make this more efficient.
							// construct buf on heap, add all entities to it
							// then send in one go.
							float x = entity.position()[0];
							float z = entity.position()[2];
							uint16 etype = entity.entityTypeID();
							EntityID id = entity.id();
							this->sendReply_brief( &x, sizeof(x) );
							this->sendReply_brief( &z, sizeof(z) );
							this->sendReply_brief( &etype, sizeof(etype) );
							this->sendReply_brief( &id, sizeof(id) );
						}
					}
					else
					{
						// At this point, have reference to cell that want to
						// get entities for.
						uint32 nrec = 0;
						const SpaceEntities & entities = space->spaceEntities();
						for ( SpaceEntities::const_iterator i = entities.begin();
							    i != entities.end(); ++i )
						{
							if ( !(*i)->isReal() )
							{
								++nrec;
							}
						}

						this->sendReply_brief( &nrec, sizeof(uint32) );

						for (Cell::Entities::const_iterator iter = entities.begin();
							  iter != entities.end(); iter++ )
						{
							Entity & entity = **iter;
							// TODO: make this more efficient.
							// construct buf on heap, add all entities to it
							// then send in one go.
							if (!entity.isReal()) {
								float x = entity.position()[0];
								float z = entity.position()[2];
								uint16 etype = entity.entityTypeID();
								EntityID id = entity.id();
								this->sendReply_brief( &x, sizeof(x) );
								this->sendReply_brief( &z, sizeof(z) );
								this->sendReply_brief( &etype, sizeof(etype) );
								this->sendReply_brief( &id, sizeof(id) );
							}
						}
					}
				}
			}

			if (!isOkay)
			{
				int errorCode = -1;
				this->sendReply_brief( &errorCode, sizeof( errorCode ) );
			}
			break;
		}

		case CellViewerExport::GET_GRID:
		{
			//DEBUG_MSG("GET_CHUNKS\n");
			uint32 spaceID;
			this->receiveData(spaceID);
			//DEBUG_MSG( "space = %d\n", spaceID );

			Space * space = cellApp_.findSpace( spaceID );
			if ( space == NULL )
			{
				ERROR_MSG( "CellViewerConnection::handleInputNotification: "
					"space == NULL\n" );
				uint ret = 0;
				this->sendReply_brief( &ret, sizeof(uint) );
			}
			else
			{
				Cell * pCell = space->pCell();
				if ( pCell == NULL )
				{
					ERROR_MSG( "CellViewerConnection::handleInputNotification: "
						"pCell == NULL\n" );
					uint ret = 0;
					this->sendReply_brief( &ret, sizeof(uint) );
				}
				else
				{
					// first send back bounding box.
					uint ret = 1;
					this->sendReply_brief( &ret, sizeof(ret) );

					BoundingBox bbox = space->pPhysicalSpace()->subBounds();
					Vector3 minBounds = bbox.minBounds();
					Vector3 maxBounds = bbox.maxBounds();
					this->sendReply_brief( &minBounds[0], sizeof(float) );
					this->sendReply_brief( &minBounds[1], sizeof(float) );
					this->sendReply_brief( &minBounds[2], sizeof(float) );
					this->sendReply_brief( &maxBounds[0], sizeof(float) );
					this->sendReply_brief( &maxBounds[1], sizeof(float) );
					this->sendReply_brief( &maxBounds[2], sizeof(float) );

					// if Physical Space has a grid (of square chunks),
					// get the chunk size; otherwise, take a minimum
					// of space depth and width (x and z)
					// ToDo: maybe should replace by maximum?
					const float gridSize = space->pChunkSpace() ?
							space->pChunkSpace()->gridSize() :
							std::min(bbox.width(), bbox.depth());
					// Warn if the dummy singleton "grid cell" is not a square:
					if (!space->pChunkSpace() &&
							(bbox.width() > bbox.depth() ||
									bbox.depth() > bbox.width())) {
						WARNING_MSG(
								"CellViewerConnection::handleInputNotification: "
								"physical space is neither a grid nor a square.\n");
					}

					this->sendReply_brief(
							&gridSize, sizeof(float) );

					// then grid positions.
					// TODO: This is no longer used. The CellAppMgr sends the
					// information about what chunks have been loaded. Remove
					// this code.
#if 0
					Cell::GridPositions gps = pCell->getLoadedGridPositions();
					uint numToSend = gps.size();
					this->sendReply_brief( &numToSend, sizeof(uint) );

					uint cnt = 0;
					Cell::GridPositions::iterator iter = gps.begin();
					for ( ; iter != gps.end(); ++iter )
					{
						++cnt;
						int gridy = iter->first;
						int gridx1 = iter->second.first;
						int gridx2 = iter->second.second;
						this->sendReply_brief( &gridy, sizeof(int) );
						this->sendReply_brief( &gridx1, sizeof(int) );
						this->sendReply_brief( &gridx2, sizeof(int) );
					}
					if ( cnt != numToSend )
					{
						ERROR_MSG( "CellViewerConnection::handleInputNotification: "
							"sanity check failed\n" );
					}
#else
					uint numToSend = 0;
					this->sendReply_brief( &numToSend, sizeof(uint) );
#endif
				}
			}

			break; // can't forget the break!
		}

		default:
		{
			//Empty the command stream if we get an unknown command
			char ignore;
			do {
				bytesRead = socket_.recv( &ignore, sizeof(char) );
			} while (bytesRead > 0);

			//Send a zero as a reply
			this->sendReply( NULL, 0 );
			break;
		}

	}

	return 1;
}

template <typename A>
void CellViewerConnection::receiveData( A & buf )
{
	// TODO: What happens if we get a partial receive?
	int32 bytesRead = socket_.recv( &buf, sizeof(A) );
	if( bytesRead < 0 )
	{
		WARNING_MSG( "CellViewerConnection::receiveData:"
			" Error reading socket.\n" );
	}
	if( bytesRead != sizeof(A) )
	{
		WARNING_MSG( "CellViewerConnection::receiveData:"
			" Received wrong number of bytes.\n" );
	}
}


/**
 *	This method sends data over the connection. The size of the data is sent
 *	before the actual data.
 */
void CellViewerConnection::sendReply( const void * buf, uint32 len )
{
	if (socket_.send(&len,sizeof(len)) != 4)
	{
		WARNING_MSG( "CellViewerConnection::sendReply: Error on write (0)\n" );
		return;
	}

	if (len > 0)
	{
		if (socket_.send(buf,len) != int(len))
		{
			WARNING_MSG( "CellViewerConnection::sendReply: Error on write.\n" );
		}
	}
}


/**
 *	This method sends data over the connection. The size of the data is not sent
 *	before the actual data.
 */
void CellViewerConnection::sendReply_brief( const void * buf, uint32 len )
{
	if ( socket_.send(buf,len) != int(len) )
	{
		WARNING_MSG( "CellViewerConnection::sendReply_brief: Error on write.\n" );
	}
}

BW_END_NAMESPACE

// cell_viewer_connection.cpp

