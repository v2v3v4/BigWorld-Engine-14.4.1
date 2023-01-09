#ifndef POST_PROC_PROPERTY_EDITOR_HPP
#define POST_PROC_PROPERTY_EDITOR_HPP


#include "gizmo/general_editor.hpp"

BW_BEGIN_NAMESPACE

// Forward declarations.
class BasePostProcessingNode;
typedef SmartPointer< BasePostProcessingNode > BasePostProcessingNodePtr;


/**
 *	This class allows editing a node's properties on a property list, 
 *	query a node's properties, or set a node property's value.
 */
class PostProcPropertyEditor : public GeneralEditor
{
public:
	static const int TEXTURES = 1;
	static const int RENDER_TARGETS = 2;
	static const int SHADERS = 4;

	PostProcPropertyEditor( BasePostProcessingNodePtr node );

	void getProperties( int types, BW::vector< BW::string > & retProps ) const;

	void setProperty( const BW::string & name, const BW::string & value );

private:
	BasePostProcessingNodePtr node_;
};

typedef SmartPointer< PostProcPropertyEditor > PostProcPropertyEditorPtr;

BW_END_NAMESPACE

#endif // POST_PROC_PROPERTY_EDITOR_HPP
