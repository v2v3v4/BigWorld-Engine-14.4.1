import sys, encodings
from sqlalchemy import create_engine, MetaData
from sqlalchemy.orm import sessionmaker
from sqlalchemy.util import warn

def warn_exception( func, *args, **kwargs ):
	"""
	This method executes the given function, catches all exceptions and
	converts it to a warning. The functions return value is returned
	in a tuple with a success flag.
	"""
	try:
		return ( True, func( *args, **kwargs ) )
	except:
		warn( "%s('%s') ignored" % sys.exc_info()[0:2] )
		return ( False, None  )


class SQLAlchemyConnInfo( object ):
	"""
    This class encapsulates persistent SQLAlchemy information that is used
    when creating separate connections to a database.
    """

	def __init__( self, db_uri ):
		self.db_engine = create_engine( db_uri )
		self._Session = sessionmaker( bind=self.db_engine )

	def createSession( self ):
		return self._Session()
