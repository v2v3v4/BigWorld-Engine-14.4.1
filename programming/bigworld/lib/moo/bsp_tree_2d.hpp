#pragma once

BW_BEGIN_NAMESPACE

//-- represents 2D rectangle region.
struct Rect2D
{
	Rect2D()
		: m_left(0), m_top(0), m_right(0), m_bottom(0) { }

	Rect2D(uint16 left, uint16 top, uint16 right, uint16 bottom)
		: m_left(left), m_top(top), m_right(right), m_bottom(bottom) { }

	uint16 m_left;
	uint16 m_top;
	uint16 m_right;
	uint16 m_bottom;
};

//-- 2D BSP tree
//--------------------------------------------------------------------------------------------------
template<typename T>
class BSPTree2D
{
public:

	//--
	struct Node
	{
		Node() : m_value(NULL)
		{
			m_childs[0] = m_childs[1] = NULL;
		}

		~Node()
		{
			delete m_childs[0];
			delete m_childs[1];
		}

		Node*	m_childs[2];
		Rect2D	m_rect;
		T*		m_value;

		Node* insert(uint width, uint height, T* value);
		Node* search(const T* value);
		void clear();
	private:
		// non-copyable
		Node( const Node & );
		Node & operator=( const Node & rhs );
	};

public:
	BSPTree2D(uint32 width, uint32 height);
	~BSPTree2D();

	bool insert(Rect2D& oRect, uint width, uint height, T* value);
	bool search(Rect2D& oRect, const T* value) const;
	bool remove(const T* value);
	void clear();

private:
	mutable Node m_root;
};

//--------------------------------------------------------------------------------------------------
template< typename T >
void BSPTree2D< T >::Node::clear()
{
	bw_safe_delete( m_childs[0] );
	bw_safe_delete( m_childs[1] );
	m_rect = Rect2D();
	m_value = NULL;
}

//--------------------------------------------------------------------------------------------------
template<typename T>
typename BSPTree2D<T>::Node* BSPTree2D<T>::Node::insert(uint width, uint height, T* value)
{
	//-- if we're not a leaf then.
	if (m_childs[0] && m_childs[1])
	{
		//-- try inserting into first child.
		Node* o = m_childs[0]->insert(width, height, value);
		if (o != NULL)
		{
			return o;
		}

		//-- no room, insert into second.
		return m_childs[1]->insert(width, height, value);
	}
	else
	{
		//-- if there's already a texture here, return.
		if (m_value)
		{
			return NULL;
		}

		//-- if we're too small, return.
		uint16 roomWidth  = m_rect.m_right - m_rect.m_left + 1;
		uint16 roomHeight = m_rect.m_bottom - m_rect.m_top + 1;

		//-- if doesn't fit in node's rect.
		if (roomWidth < width || roomHeight < height)
		{
			return NULL;
		}

		//-- if we're just right, accept because image fits perfectly in node's rect.
		if (roomWidth == width && roomHeight == height)
		{
			//-- now save value in this node.
			m_value = value;
			return this;
		}

		//-- otherwise, gotta split this node and create some kids.
		m_childs[0] = new Node();
		m_childs[1] = new Node();

		//-- decide which way to split.
		uint16 dw = roomWidth - width;
		uint16 dh = roomHeight - height;

		if (dw > dh)
		{
			m_childs[0]->m_rect = Rect2D(m_rect.m_left, m_rect.m_top, 
				m_rect.m_left + width - 1, m_rect.m_bottom
				);

			m_childs[1]->m_rect = Rect2D(m_rect.m_left + width, m_rect.m_top, 
				m_rect.m_right, m_rect.m_bottom
				);
		}
		else
		{
			m_childs[0]->m_rect = Rect2D(m_rect.m_left, m_rect.m_top, 
				m_rect.m_right, m_rect.m_top + height - 1
				);

			m_childs[1]->m_rect = Rect2D(m_rect.m_left, m_rect.m_top + height, 
				m_rect.m_right, m_rect.m_bottom
				);
		}

		//-- insert into first child we created.
		return m_childs[0]->insert(width, height, value);
	}
}

//--------------------------------------------------------------------------------------------------
template<typename T>
typename BSPTree2D<T>::Node* BSPTree2D<T>::Node::search(const T* value)
{
	//-- if we're not a leaf then.
	if (m_childs[0] && m_childs[1])
	{
		//-- try to find in first child.
		Node* o = m_childs[0]->search(value);
		if (o != NULL)
		{
			return o;
		}

		//-- then try to find in second.
		return m_childs[1]->search(value);
	}
	else
	{
		//-- compare desired value with stored in that node.
		if (m_value != value)
		{
			return NULL;
		}
		else
		{
			return this;
		}
	}
}

//--------------------------------------------------------------------------------------------------
template<typename T>
BSPTree2D<T>::BSPTree2D(uint32 width, uint32 height)
{
	m_root.m_rect = Rect2D(0, 0, width - 1, height - 1);
}

//--------------------------------------------------------------------------------------------------
template<typename T>
BSPTree2D<T>::~BSPTree2D()
{

}

//--------------------------------------------------------------------------------------------------
template<typename T>
bool BSPTree2D<T>::insert(Rect2D& oRect, uint width, uint height, T* value)
{
	Node* node = m_root.insert(width, height, value);
	if (node != NULL)
	{
		oRect = node->m_rect;
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------------------------------
template<typename T>
bool BSPTree2D<T>::search(Rect2D& oRect, const T* value) const
{
	Node* node = m_root.search(value);
	if (node != NULL)
	{
		oRect = node->m_rect;
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------------------------------
template<typename T>
bool BSPTree2D<T>::remove(const T* value)
{
	Node* node = m_root.search(value);
	if (node != NULL)
	{
		node->m_value = NULL;
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------------------------------
template<typename T>
void BSPTree2D<T>::clear()
{
	//-- save previous size of the tree.
	uint width  = m_root.m_rect.m_right;
	uint height = m_root.m_rect.m_bottom;

	//-- just recreate root node it recursively destroy all nodes of the tree.
	m_root.clear();
	m_root.m_rect = Rect2D(0, 0, width, height);
}

BW_END_NAMESPACE
