#ifndef GENERAL_EDITOR_HPP
#define GENERAL_EDITOR_HPP

#include "cstdmf/bw_list.hpp"
#include "cstdmf/smartpointer.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include "value_type.hpp"

BW_BEGIN_NAMESPACE

class GeneralEditor;
typedef SmartPointer<GeneralEditor> GeneralEditorPtr;

class GeneralProperty;


class Name
{
	class NameHolder
	{
		BW::list<BW::string> names_;

	public:
		const char* add( const char* str )
		{
			names_.push_back( str );

			return names_.back().c_str();
		}
	};

	const char* str_;
	size_t size_;
	Name* next_;

	static NameHolder* holder_;
	static const int NAME_BUCKET = 4096;
	static int hashIndex_[ NAME_BUCKET ];
	static Name* names_[ NAME_BUCKET ];

	static int hash( const char* str )
	{
		int hash = ( strlen( str ) + 8731 ) % NAME_BUCKET;

		while (*str)
		{
			hash = ( hash ^ ( 255 & *str ) ) % NAME_BUCKET;
	        hash = hashIndex_[ hash ];
			++str;
		}

		return hash;
	}

	void init( const char* str )
	{
		Name*& head = names_[ hash( str ) ];

		while (head)
		{
			if (strcmp( str, head->str_ ) == 0)
			{
				*this = *head;

				return;
			}

			head = head->next_;
		}

		str_ = holder_->add( str );
		size_ = strlen( str_ );

		head = new Name( *this );
	}

public:
	Name()
		: str_( "" ), size_( 0 ), next_( 0 )
	{}

	Name( const BW::string& str )
		: next_( 0 )
	{
		init( str.c_str() );
	}

	Name( const char* str )
		: next_( 0 )
	{
		init( str );
	}

	void clear()
	{
		*this = Name();
	}

	char operator[]( int index ) const
	{
		return str_[ index ];
	}

	const char* c_str() const
	{
		return str_;
	}

	BW::string str() const
	{
		return str_;
	}

	bool empty() const
	{
		return str_[ 0 ] == 0;
	}

	size_t size() const
	{
		return size_;
	}

	const char* begin() const
	{
		return str_;
	}

	const char* end() const
	{
		return str_ + size_;
	}

	char back() const
	{
		return str_[ size_ - 1 ];
	}

	static void init()
	{
		holder_ = new NameHolder;

		for (int i = 0; i < NAME_BUCKET; ++i)
		{
			hashIndex_[ i ] = i;
		}

		std::random_shuffle( hashIndex_, hashIndex_ + NAME_BUCKET );
	}

	static void fini()
	{
		bw_safe_delete(holder_);
	}
};


inline bool operator== ( const Name& n1, const Name& n2 )
{
	return strcmp( n1.c_str(), n2.c_str() ) == 0;
}

inline bool operator== ( const char* n1, const Name& n2 )
{
	return strcmp( n1, n2.c_str() ) == 0;
}

inline bool operator== ( const Name& n1, const char* n2 )
{
	return strcmp( n1.c_str(), n2 ) == 0;
}

inline bool operator== ( const BW::string& n1, const Name& n2 )
{
	return strcmp( n1.c_str(), n2.c_str() ) == 0;
}

inline bool operator== ( const Name& n1, const BW::string& n2 )
{
	return strcmp( n1.c_str(), n2.c_str() ) == 0;
}

inline bool operator!= ( const Name& n1, const Name& n2 )
{
	return strcmp( n1.c_str(), n2.c_str() ) != 0;
}

inline bool operator!= ( const char* n1, const Name& n2 )
{
	return strcmp( n1, n2.c_str() ) != 0;
}

inline bool operator!= ( const Name& n1, const char* n2 )
{
	return strcmp( n1.c_str(), n2 ) != 0;
}

inline bool operator!= ( const BW::string& n1, const Name& n2 )
{
	return strcmp( n1.c_str(), n2.c_str() ) != 0;
}

inline bool operator!= ( const Name& n1, const BW::string& n2 )
{
	return strcmp( n1.c_str(), n2.c_str() ) != 0;
}


/**
 *	This class controls and defines the editing operations which
 *	can be performed on a general object.
 */
class GeneralEditor : public PyObjectPlus
{
	Py_Header( GeneralEditor, PyObjectPlus )
public:
	GeneralEditor( PyTypeObject * pType = &s_type_ );
	virtual ~GeneralEditor();

	virtual void addProperty( GeneralProperty * pProp );
	// desired views:
	//  gizmos (can be left to item itself 'tho... maybe all should be?)
	//  text (I guess)
	//  Borland controls
	//  python (combine with text...?)

	virtual void addingAssetMetadata( bool assetMetadata ) {}
	virtual bool useFullDateFormat() { return false; }

	void elect();
	void expel();

	typedef BW::vector< GeneralEditorPtr > Editors;

	static const Editors & currentEditors( void );
	static bool settingMultipleEditors() { return s_settingMultipleEditors_; }
	static void currentEditors( const Editors & editors );

	static void createViews( bool doCreate ) { s_createViews_ = doCreate; }
	static bool createViews() { return s_createViews_; }

	ScriptObject	pyGetAttribute( const ScriptString & attrObj );
	bool			pySetAttribute( const ScriptString & attrObj,
		const ScriptObject & value );

	void		pyAdditionalMembers( const ScriptList & pList ) const;

	PY_MODULE_STATIC_METHOD_DECLARE( py_getCurrentEditors )
	PY_MODULE_STATIC_METHOD_DECLARE( py_setCurrentEditors )

protected:
	typedef BW::vector< GeneralProperty * > PropList;
	PropList		properties_;

	bool			constructorOver_;

	// this keeps track of the last item edited
	Name			lastItemName_;

private:
	GeneralEditor( const GeneralEditor& );
	GeneralEditor& operator=( const GeneralEditor& );

	static Editors	s_currentEditors_;
	static int s_currentEditorIndex_;
	static int s_lastEditorIndex_;
	static bool s_settingMultipleEditors_;
	static bool s_createViews_;
};

// move this down.... theres already a propertymanager...
namespace PropManager
{
	typedef void (*PropFini)( void );

	void registerFini(PropFini fn);
	void fini();
};

/**
 *	This macro declares the view factory stuff for a property class
 */
#define GENPROPERTY_VIEW_FACTORY_DECLARE( PROPCLASS )						\
	public:																	\
		typedef View * (*ViewFactory)( PROPCLASS & prop );					\
		static void registerViewFactory( int vkid, ViewFactory fn );		\
		static void fini();													\
																			\
	private:																\
		static BW::vector<ViewFactory>	* viewFactories_;					\
};																			\
extern "C" void PROPCLASS##_registerViewFactory(							\
				int vkid, PROPCLASS::ViewFactory fn );						\
namespace DodgyNamespace {													\


/**
 *	This macro implements the view factory stuff for a property class
 */
#define GENPROPERTY_VIEW_FACTORY( PROPCLASS )								\
	BW::vector<PROPCLASS::ViewFactory> * PROPCLASS::viewFactories_ = NULL;	\
																			\
	void PROPCLASS::registerViewFactory( int vkid, ViewFactory fn )			\
	{																		\
		if (viewFactories_ == NULL)											\
			viewFactories_ = new BW::vector<ViewFactory>;					\
		while (int(viewFactories_->size()) <= vkid)							\
			viewFactories_->push_back( NULL );								\
		(*viewFactories_)[ vkid ] = fn;										\
		PropManager::registerFini( &fini );									\
	}																		\
	void PROPCLASS::fini( )													\
	{																		\
		bw_safe_delete(viewFactories_);										\
	}																		\
																			\
	void PROPCLASS##_registerViewFactory(									\
							int vkid, PROPCLASS::ViewFactory fn )			\
	{																		\
		PROPCLASS::registerViewFactory( vkid, fn );							\
	}																		\


class PropertyManager : public ReferenceCount
{
public:
	virtual bool canRemoveItem() { return false; }
	virtual void removeItem() = 0;

	virtual bool canAddItem() { return false; }
	virtual void addItem() = 0;
};
typedef SmartPointer<PropertyManager> PropertyManagerPtr;


#define RETURN_VALUETYPE( TYPE )						\
	static ValueType s_##TYPE( ValueTypeDesc::TYPE );	\
	return s_##TYPE;


//TODO: just revert back and add the vectors to a global list to be destructed at some stage....

class GeneralProperty
{
public:

	GeneralProperty( const Name& name );

    void WBEditable( bool editable );
    bool WBEditable() const;

    void descName( const Name& descName );
	const Name& descName();
	
	void UIName( const Name& name );
    const Name& UIName();

	void UIDesc( const Name& name );
    const Name& UIDesc();

	void exposedToScriptName( const Name& exposedToScriptName );
	const Name& exposedToScriptName();

	void canExposeToScript( bool canExposeToScript );
	bool canExposeToScript() const;

	virtual void fileFilter( const Name& fileFilter ) {}
	virtual const Name& fileFilter() const
	{
		static Name s_emptyFilter( "" );
		return s_emptyFilter;
	}

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( UNKNOWN ); }

	virtual void deleteSelf() { delete this; }

	virtual void elect();
	virtual void elected();
	virtual void expel();
	virtual void select();

	virtual PyObject * pyGet();
	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

	const Name& name() const		{ return name_; }

	void setGroup( const Name& groupName ) { group_ = groupName; }
	const Name& getGroup() { return group_; }

	class View
	{
	public:
		// Make the object responsible for deleting itself so that it is deleted
		// from the correct heap.
		virtual void deleteSelf()	{ delete this; }

		virtual void elect() = 0;
		virtual void elected() {};
		virtual void lastElected() { ERROR_MSG( "This view does not support 'lastElected'\n" ); }
		virtual bool isEditorView() const { return false; }
		virtual void expel() = 0;

		virtual void select() = 0;

		static void pLastElected( View* v ) { pLastElected_ = v; }
		static View* pLastElected() { return pLastElected_; }
	protected:
		// Always use deleteSelf. This is because the object may not have been
		// created in the DLL.
		virtual ~View() {}
		static View* pLastElected_;
	};

	static int nextViewKindID();

	void setPropertyManager( PropertyManagerPtr management ) { propManager_ = management; }
	PropertyManagerPtr getPropertyManager() const { return propManager_; }

	bool hasViews() const;

protected:
	virtual ~GeneralProperty();
	static int nextViewKindID_;

	class Views
	{
	public:
		Views();
		~Views();

		void set( int i, View * v );
		View * operator[]( int i );
		const View * operator[]( int i ) const;

	private:
		View **		e_;

		Views( const Views & oth );
		Views & operator=( const Views & oth );
	};

	Views				views_;
	Name				name_;
	Name				group_;

private:
	GeneralProperty( const GeneralProperty & oth );
	GeneralProperty & operator=( const GeneralProperty & oth );

	friend class Views;

	PropertyManagerPtr propManager_;
	uint32 flags_;

    bool            WBEditable_;
    Name		    descName_;
	Name		    UIName_;
    Name		    UIDesc_;
	Name		    exposedToScriptName_;
	bool			canExposeToScript_;

	GENPROPERTY_VIEW_FACTORY_DECLARE( GeneralProperty )
};


// Exported C style function for Borland to call.
extern "C" int GeneralProperty_nextViewKindID();


/**
 *	This macro sets up the views for a general property type.
 *
 *	It should be used in the constructor of all classes that derive from
 *	GeneralProperty.
 *
 *	Note that if a view kind has implementations for both derived and base
 *	classes of a property (which would not be not unusual), then the base
 *	class view will get created for a short time before it is deleted and
 *	replaced by the derived class view. If this turns out to be a problem
 *	it could be avoided, but I will leave it for now.
 */
#define GENPROPERTY_MAKE_VIEWS()											\
	if (GeneralEditor::createViews())										\
	{																		\
		if (viewFactories_ != NULL)											\
		{																	\
			MF_ASSERT( int(viewFactories_->size()) <= nextViewKindID_ );	\
																			\
			for (uint i = 0; i < viewFactories_->size(); i++)				\
			{																\
				ViewFactory vf = (*viewFactories_)[i];						\
				if (vf != NULL)												\
				{															\
					View * v = (*vf)( *this );								\
					if (v != NULL)											\
					{														\
						views_.set( i, v );									\
					}														\
				}															\
			}																\
		}																	\
		else																\
		{																	\
			MF_ASSERT( "View Factory NULL.  Try linking in the Enroller" )	\
		}																	\
	}



/**
 *	This is the base class for all read only properties
 */
class GeneralROProperty : public GeneralProperty
{
public:
	GeneralROProperty( const Name& name );

	virtual int pySet( PyObject * value, bool transient = false,
													bool addBarrier = true );

private:

	GENPROPERTY_VIEW_FACTORY_DECLARE( GeneralROProperty )
};



BW_END_NAMESPACE
#endif // GENERAL_EDITOR_HPP
