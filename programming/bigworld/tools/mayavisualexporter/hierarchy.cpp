#include "pch.hpp"

#include "hierarchy.hpp"

#include "skin.hpp"

#include "utility.hpp"

#include "cstdmf/bw_list.hpp"

BW_BEGIN_NAMESPACE

MDagPath findDagPath( BW::string path )
{
	MItDag it( MItDag::kDepthFirst, MFn::kDagNode );
	
	do
	{
		if( path == it.fullPathName().asChar() )
		{
			MDagPath result;
			it.getPath( result );
			return result;
		}
	}
	while( it.next() == MStatus::kSuccess );
		
	return MDagPath();
}


Hierarchy::Hierarchy( Hierarchy* parent ) : parent_( parent )
{
}

Hierarchy::~Hierarchy()
{
	clear();
}

void Hierarchy::clear()
{
	for( BW::map<BW::string, Hierarchy*>::iterator it = _children.begin(); it != _children.end(); ++it )
		delete it->second;
	
	_children.clear();

	_name = "";
}

void Hierarchy::getSkeleton( Skin& skin )
{
	for (uint32 i = 0; i < skin.meshes().length(); i++)
	{
		MStatus stat;
		MFnTransform trans(skin.meshes()[i].transform(), &stat );
		if (stat == MS::kSuccess)
		{
			BW::string nodeName = trans.fullPathName().asChar();
			Hierarchy* pNode = &find( nodeName );
			while (pNode && pNode != this)
			{
				if (!pNode->children().size())
				{
					Hierarchy* pParent = pNode->getParent();
					if (pParent)
					{
						pParent->removeNode( pNode );
					}
					pNode = pParent;
				}
				else
				{
					pNode = NULL;
				}
			}
		}
	}
	for( uint32 i = 0; i < skin.numberOfBones(); ++i )
		addNode( skin.boneNameFull( i ), skin.boneDAG( i ) );
}

void Hierarchy::getMeshes( Mesh& mesh )
{
	BlendShapes blendShapes;
	
	for( uint32 i = 0; i < (int)mesh.meshes().length(); ++i )
	{
		bool bspNode = (toLower(mesh.meshes()[i].fullPathName().asChar()).find("_bsp") != BW::string::npos);
		//dont add bsp nodes
		if( (bspNode==false) && (blendShapes.isBlendShapeTarget( mesh.meshes()[i].node() ) == false) )
		{
			MStatus stat;
			MFnTransform trans(mesh.meshes()[i].transform(), &stat );
			if (stat == MS::kSuccess)
			{
				addNode( trans.fullPathName().asChar(), findDagPath( trans.fullPathName().asChar() ) );
			}
		}
	}
	fflush( stdout );
}

void Hierarchy::removeNode( const BW::string& name )
{
	_children.erase( _children.find( name ) );
}

BW::string Hierarchy::removeNode( Hierarchy* node )
{
	BW::string result;

	BW::map<BW::string, Hierarchy*>::iterator it = _children.begin();

	while( it != _children.end() )
	{
		if( it->second == node )
		{
			result = it->first;
			_children.erase( it );
			break;
		}
		++it;
	}
	return result;
}

void Hierarchy::addNode( const BW::string& name, Hierarchy& h )
{
	_children[ name ] = &h;
	h.parent_ = this;
}

Hierarchy* Hierarchy::recursiveFind( const BW::string& name )
{
	if( parent_ == NULL && name == "Scene Root" )
		return this;
	BW::map<BW::string, Hierarchy*>::iterator it = _children.begin();

	while( it != _children.end() )
	{
		if( it->first == name )
			return it->second;
		++it;
	}

	it = _children.begin();
	while( it != _children.end() )
	{
		Hierarchy* h = it->second->recursiveFind( name );
		if( h )
			return h;
		++it;
	}
	return NULL;
}

void Hierarchy::addNode( const BW::string& path, MDagPath dag, BW::string accumulatedPath )
{
	if( !parent_ )
	{
		// if there is more to the path
		BW::string child = path.substr( 1, path.length() );
		if( child.find( "|" ) != BW::string::npos )
		{
			child = child.substr( 0, child.find( "|" ) );
	
			if( _children.find( child ) == _children.end() )
			{
				_children[child] = new Hierarchy( this );
				_children[child]->_path = findDagPath( accumulatedPath + "|" + child );
			}
			_children[child]->addNode( path, dag, accumulatedPath + "|" + child );
		}
		else
		{
			if( _children.find( child ) == _children.end() )
			{
				_children[child] = new Hierarchy( this );
				_children[child]->_path = findDagPath( accumulatedPath + "|" + child );
			}
			_children[child]->addNode( path, dag, accumulatedPath + "|" + child );
		}

		return;
	}
	
	// if name is empty, set it
	if( _name == "" )
	{
		_name = path.substr( 1, path.length() );

		if( _name.find( "|" ) != BW::string::npos )
		{
			_name = _name.substr( 0, _name.find( "|" ) );
		}
		else
		{
			// root node so set dag
			_path = dag;
			return;
		}
	}
	
	// if path is not part of this node hierarchy, don't add it
	if( path.find( _name ) != 1 )
		return;
	
	BW::string subpath = path.substr( 1 + _name.length(), path.length() );
	
	// invalid path?
	if( subpath[0] != '|' )
	{
		if( BW::string( _path.fullPathName().asChar() ).length() == 0 )
			_path = dag;
		return;
	}
	
	// if there is more to the path
	BW::string child = subpath.substr( 1, subpath.length() );
	if( child.find( "|" ) != BW::string::npos )
	{
		child = child.substr( 0, child.find( "|" ) );

		if( _children.find( child ) == _children.end() )
		{
			_children[child] = new Hierarchy( this );
			_children[child]->_path = findDagPath( accumulatedPath + "|" + child );
		}
		_children[child]->addNode( subpath, dag, accumulatedPath + "|" + child );
	}
	else
	{
		if( _children.find( child ) == _children.end() )
		{
			_children[child] = new Hierarchy( this );
			_children[child]->_path = findDagPath( accumulatedPath + "|" + child );
		}
		_children[child]->addNode( subpath, dag, accumulatedPath + "|" + child );
	}
}

void Hierarchy::addNode( const BW::string& path, matrix4<float> worldTransform )
{
	BW::string child = path;//path.substr( 1 + _name.length() );

	_children[child] = new Hierarchy( this );

	Hierarchy& node = *_children[child];

	node._name = child;
	node._customPath = path;
	node._relativeTransform = transform( 0 ).inverse() * worldTransform;
	node._worldTransform = worldTransform;
}

void Hierarchy::dump( const BW::string& parent )
{
	if (BW::string( "" ) == _path.fullPathName().asChar())
		printf( "%s[%s]\n", parent.c_str(), _name.c_str() );
	else
		printf( "%s%s\n", parent.c_str(), _path.fullPathName().asChar() );
	
	for( BW::map<BW::string, Hierarchy*>::iterator it = _children.begin(); it != _children.end(); ++it )
		it->second->dump( parent + "    " );
}

/**
 *	Used to set the custom name of the root node.
 */
void Hierarchy::customName(BW::string customName)
{
	_customPath = customName;
}

/**
 *	Used to set the name of the root node.
 */
void Hierarchy::name(BW::string name)
{
	_name = name;
}

BW::string Hierarchy::name()
{
	return _name;
}

BW::string Hierarchy::path()
{
	if( _customPath != "" )
		return _customPath;
	else
		return _path.fullPathName().asChar();
}

BW::string Hierarchy::dag()
{
	return _path.fullPathName().asChar();
}

BW::vector<BW::string> Hierarchy::children( bool orderChildren /*= false*/ )
{
	BW::vector< BW::string > objectStrings;
	
	if (orderChildren)
	{
		BW::list< std::pair< BW::string, Hierarchy* > > objectPairs;
		
		// Order the immediate children by their number of immediate decendences in
		// decending order.  This is required to make sure that the root bone is
		// selected as the itinerant bone inside model editor.
		// 
		// 1) Populate a list 
		for (
			BW::map<BW::string, Hierarchy*>::iterator it = _children.begin();
			it != _children.end();
			++it )
		{ 
			objectPairs.push_back(
				std::pair< BW::string, Hierarchy* >( it->first, it->second ) );
		} 

		// 2) Sort list in descending order 
		objectPairs.sort( numChildDescending ); 

		// 3) Populate objectStrings 
		for (
			BW::list< std::pair< BW::string, Hierarchy* > >::iterator it = objectPairs.begin();
			it != objectPairs.end();
			++it )
		{
			objectStrings.push_back( it->first );
		}
	}
	else
	{
		// Populate objectStrings
		for (
			BW::map<BW::string, Hierarchy*>::iterator it = _children.begin();
			it != _children.end();
			++it )
		{
			objectStrings.push_back( it->first );
		}
	}
	
	return objectStrings;
}

Hierarchy& Hierarchy::child( const BW::string& name )
{
	BW::map<BW::string, Hierarchy*>::iterator it = _children.find( name );

	if( it == _children.end() )
		return *this; // TODO : error?
	else
		return *it->second;
}

Hierarchy& Hierarchy::find( const BW::string& path )
{
	if( path == _path.fullPathName().asChar() )
		return *this;

	BW::map<BW::string, Hierarchy*>::iterator it = _children.begin();

	while( it != _children.end() )
	{
		//~ if( path == it->second->_path.fullPathName().asChar() )
			//~ return *it->second;
		if( BW::string( it->second->_path.fullPathName().asChar() ) == path.substr( 0, BW::string( it->second->_path.fullPathName().asChar() ).length() ) &&
			((BW::string( it->second->_path.fullPathName().asChar() ).length() < path.length() && path[ BW::string( it->second->_path.fullPathName().asChar() ).length() ] == '|') ||
			 (BW::string( it->second->_path.fullPathName().asChar() ).length() == path.length()) ) )
			return it->second->find( path );
		
		++it;
	}
	
	return *this; // TODO : error?
}

uint32 Hierarchy::numberOfFrames()
{
	MAnimControl control;
	
	// note the values in MTime correspond to frames if start and end are in MTime::uiUnit() times
	MTime start = control.minTime();
	MTime end = control.maxTime();
	
	return (uint32)(end.as( MTime::uiUnit() ) - start.as( MTime::uiUnit() ) + 1);
}

uint32 Hierarchy::count()
{
	uint32 total = 0;
	
	if( parent_ )
		++total;

	for( uint32 i = 0; i < children().size(); ++i )
		total += _children[children()[i]]->count();
	
	return total;
}

matrix4<float> Hierarchy::transform( double frame, bool relative )
{
	if( _customPath != "" )
	{
		if( relative )
			return _relativeTransform;
		else
			return _worldTransform;
	}
	
	MAnimControl control;
	
	// note the values in MTime correspond to frames if start and end are in MTime::uiUnit() times
	MTime start = control.minTime();
	MTime end = control.maxTime();
	
	double numberOfFrames = end.as( MTime::uiUnit() ) - start.as( MTime::uiUnit() ) + 1;
	
	if( frame < 0 || frame >= numberOfFrames )
		return matrix4<float>(); // identity - TODO : error?

	// store current time
	MTime time = control.currentTime();
	
	// set the requested frame
	MTime frameTime = start + MTime( frame, MTime::uiUnit() );
	control.setCurrentTime( frameTime );
	
	matrix4<float> result;
	
	// get the transform
	MFloatMatrix matrix( _path.inclusiveMatrix().matrix );

	if (ExportSettings::instance().useLegacyOrientation())
	{
		// This legacy code came from the 1.7.2 release
		matrix[0][0] *= -1;
		matrix[0][1] *= -1;
		matrix[0][2] *= -1;
	}
	else
	{
		matrix[2][0] *= -1;
		matrix[2][1] *= -1;
		matrix[2][2] *= -1;
	}

	if( relative == false )
		memcpy( &result, matrix.matrix, sizeof(float) * 4 * 4 );
	else
	{
		MFloatMatrix parent( !parent_ ? _path.exclusiveMatrix().matrix : parent_->_path.inclusiveMatrix().matrix );

		if (ExportSettings::instance().useLegacyOrientation())
		{
			// This legacy code came from the 1.7.2 release
			parent[0][0] *= -1;
			parent[0][1] *= -1;
			parent[0][2] *= -1;
		}
		else
		{
			parent[2][0] *= -1;
			parent[2][1] *= -1;
			parent[2][2] *= -1;
		}

		MFloatMatrix relative = matrix * parent.inverse();
		memcpy( &result, relative.matrix, sizeof(float) * 4 * 4 );	
	}	

	// restore old time
	control.setCurrentTime( time );
	
	return result;
}

/** 
 *  Used to order a sequence of nodes based on the number of children each has. 
 * 
 *  @param  lhs The pair to the left hand side of the operation. 
 *  @param  rhs The pair to the right hand side of the operation. 
 *  @return True is lhs has more children than rhs, false otherwise. 
 */ 
bool numChildDescending(
	std::pair< BW::string, Hierarchy* > lhs,
	std::pair< BW::string, Hierarchy* > rhs )
{
	if (lhs.second->count() > rhs.second->count())
	{
        return true;
    }
    else
    {
        return false;
    }
} 

BW_END_NAMESPACE

