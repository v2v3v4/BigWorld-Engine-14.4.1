#include "pch.hpp"

#include "model/super_model.hpp"

#include "resmgr/xml_section.hpp"

#include "undo_redo.hpp"

#include "moo/geometrics.hpp"

#include "tools/common/utilities.hpp"

#include "me_consts.hpp"

#include "mutant.hpp"

DECLARE_DEBUG_COMPONENT2( "Mutant_LOD", 0 )

BW_BEGIN_NAMESPACE

float Mutant::lodExtent( const BW::string& modelFile )
{
	BW_GUARD;

	//First make sure the model exists
	if (models_.find( modelFile ) == models_.end())
	{
		return Model::LOD_HIDDEN;
	}

	return models_[modelFile]->readFloat( "extent", Model::LOD_HIDDEN );
}

void Mutant::lodExtent( const BW::string& modelFile, float extent )
{
	BW_GUARD;

	//First make sure the model exists
	if (models_.find(modelFile) == models_.end())
		return;

	UndoRedo::instance().add( new UndoRedoOp( 0, models_[modelFile], models_[modelFile] ));

	if (!isEqual( extent, Model::LOD_HIDDEN ))
	{
		models_[modelFile]->writeFloat( "extent", extent );
	}
	else
	{
		models_[modelFile]->delChild( "extent" );
	}
}

void Mutant::lodParents( BW::string modelName, BW::vector< BW::string >& parents )
{
	BW_GUARD;

	DataSectionPtr model = BWResource::openSection( modelName, false );
	while ( model )
	{
		parents.push_back( modelName );
		modelName = model->readString( "parent", "" ) + ".model";
		model = BWResource::openSection( modelName, false );
	}
}

bool Mutant::hasParent( const BW::string& modelName )
{
	BW_GUARD;

	return (models_.find(modelName) != models_.end());
}

bool Mutant::isHidden( const BW::string& modelFile )
{
	BW_GUARD;

	bool hidden = false;
	float extent = 0.f;

	for (size_t i=0; i<=lodList_.size(); i++)
	{
		if ((!isEqual( lodList_[i].second, Model::LOD_HIDDEN ) &&
			(lodList_[i].second <= extent)) ||
			isEqual( extent, Model::LOD_HIDDEN ))
		{
			hidden = true;
		}
		else
		{
			hidden = false;
			extent = lodList_[i].second;
		}
		if (lodList_[i].first.second == modelFile )
			break;
	}

	return hidden;
}

BW::string Mutant::lodParent( const BW::string& modelFile )
{
	BW_GUARD;

	//First make sure the model exists
	if (models_.find(modelFile) == models_.end())
		return "";

	return models_[modelFile]->readString( "parent", "" );
}

void Mutant::lodParent( const BW::string& modelFile, const BW::string& parent )
{
	BW_GUARD;

	//First make sure the model exists
	if (models_.find(modelFile) == models_.end())
		return;

	UndoRedo::instance().add( new UndoRedoOp( 0, models_[modelFile], models_[modelFile] ));

	if (parent != "")
	{
		models_[modelFile]->writeString( "parent", parent );
	}
	else
	{
		models_[modelFile]->delChild( "parent" );
		models_[modelFile]->delChild( "extent" );
	}
}

/*
 * This method commits a LOD list that has been edited by using the lod bar
 */
void Mutant::lodList( LODList* newList )
{
	BW_GUARD;

	//Update all the extents
	for (unsigned i=0; i<newList->size(); i++)
	{
		lodExtent((*newList)[i].first.second, (*newList)[i].second );
	}

	reloadAllLists();

}

/*
 * This method sets the virtual LOD distance
 */
void Mutant::virtualDist( float dist )
{
	virtualDist_ = dist;
}
BW_END_NAMESPACE

