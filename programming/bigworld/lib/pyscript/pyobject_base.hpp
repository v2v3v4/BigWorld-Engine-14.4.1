#include "cstdmf/config.hpp"

#if ENABLE_DOC_STRINGS
#define PY_GET_DOC( DOC_STRING ) DOC_STRING
#else
#define PY_GET_DOC( DOC_STRING ) ((char *)NULL)
#endif

#define PY_BASETYPEOBJECT( THIS_CLASS )									\
	PY_BASETYPEOBJECT_WITH_DOC( THIS_CLASS, 0 )

#define PY_BASETYPEOBJECT_WITH_DOC( THIS_CLASS, DOC_STRING )			\
static void THIS_CLASS##_tp_dealloc( PyObject * pObj )					\
{																		\
	/* Don't call delete here because we were not allocated with new */ \
	static_cast< THIS_CLASS * >( pObj )->~THIS_CLASS();					\
	THIS_CLASS::s_type_.tp_free( pObj );								\
}																		\
																		\
/* TEMPORARY This should be replaced with a macro */					\
/* PY_TYPEOBJECT_SPECIALISE_GC */										\
static int THIS_CLASS##_tp_traverse(									\
	PyObject * pObj, visitproc visit, void * arg )						\
{																		\
	return 0;															\
}																		\
																		\
static int THIS_CLASS##_tp_clear( PyObject * pObj )						\
{																		\
	return 0;															\
}																		\
/* end of TEMPORARY */													\
																		\
PyTypeObject THIS_CLASS::s_type_ =										\
{																		\
	PyObject_HEAD_INIT(&PyType_Type)									\
	0,								/* ob_size */						\
	#THIS_CLASS,					/* tp_name */						\
	sizeof(THIS_CLASS),				/* tp_basicsize */					\
	0,								/* tp_itemsize */					\
																		\
	/* methods */														\
	THIS_CLASS##_tp_dealloc,		/* tp_dealloc */					\
	0,								/* tp_print */						\
	0,								/* tp_getattr */					\
	0,								/* tp_setattr */					\
	0,								/* tp_compare */					\
	_tp_repr,						/* tp_repr */						\
	0,								/* tp_as_number */					\
	0, /*SEQ,*/						/* tp_as_sequence */				\
	0, /* MAP, */					/* tp_as_mapping */					\
	0,								/* tp_hash */						\
	0, /* CALL, */					/* tp_call */						\
	0,								/* tp_str */						\
	_tp_getattro,					/* tp_getattro */					\
	_tp_setattro,					/* tp_setattro */					\
	0,								/* tp_as_buffer */					\
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |							\
		Py_TPFLAGS_HAVE_GC,			/* tp_flags */						\
	PY_GET_DOC( DOC_STRING ),		/* tp_doc */						\
	THIS_CLASS##_tp_traverse,		/* tp_traverse */					\
	THIS_CLASS##_tp_clear,			/* tp_clear */						\
	0,								/* tp_richcompare */				\
	0,								/* tp_weaklistoffset */				\
	0,								/* tp_iter */						\
	0,								/* tp_iternext */					\
	THIS_CLASS::s_getMethodDefs(),	/* tp_methods */					\
	0,								/* tp_members */					\
	THIS_CLASS::s_getAttributeDefs(),/* tp_getset */					\
	&Super::s_type_,				/* tp_base */						\
	0,								/* tp_dict */						\
	0,								/* tp_descr_get */					\
	0,								/* tp_descr_set */					\
	0,								/* tp_dictoffset */					\
	0,								/* tp_init */						\
	0,								/* tp_alloc */						\
	0,								/* tp_new */						\
	PyObject_GC_Del,				/* tp_free */						\
	0,								/* tp_is_gc */						\
	0,								/* tp_bases */						\
	0,								/* tp_mro */						\
	0,								/* tp_cache */						\
	0,								/* tp_subclasses */					\
	0,								/* tp_weaklist */					\
	0,								/* tp_del */						\
	0,								/* tp_version_tag */				\
};																		\

// pyobject_base.hpp
