#ifndef LIST_NODE_HPP
#define LIST_NODE_HPP

BW_BEGIN_NAMESPACE

/**
 *
 */
class ListNode
{
private:
	ListNode*		next_;
	ListNode*		prev_;

public:
	void			setAsRoot();

	void			addThisAfter( ListNode* prev );
	void			addThisBefore( ListNode* next );
	void			remove();

	ListNode*		getNext() const;
	ListNode*		getPrev() const;
};

/**
 *
 */
inline void ListNode::setAsRoot()
{
	next_ = this;
	prev_ = this;
}

/**
 *
 */
inline void ListNode::addThisAfter( ListNode* prev )
{
	next_ = prev->next_;
	next_->prev_ = this;
	prev_ = prev;
	prev->next_ = this;
}

/**
 *
 */
inline void ListNode::addThisBefore( ListNode* next )
{
	prev_ = next->prev_;
	prev_->next_ = this;
	next_ = next;
	next->prev_ = this;
}

/**
 *
 */
inline void ListNode::remove()
{
	prev_->next_ = next_;
	next_->prev_ = prev_;
}

/**
 *
 */
inline ListNode* ListNode::getNext() const
{
	return next_;
}

/**
 *
 */
inline ListNode* ListNode::getPrev() const
{
	return prev_;
}

/**
 *
 */
#define CAST_NODE( NODE, CLASS, FIELD ) bw_container_of( NODE, CLASS, FIELD )

BW_END_NAMESPACE

#endif // LIST_NODE_HPP
