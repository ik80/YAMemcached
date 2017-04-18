/*
 * trbtree.hh
 *
 *  Created on: Dec 13, 2015
 *      Author: kalujny
 */

// borrows heavily from libavl by Ben Pfaff
#ifndef TRBTREE_HH_
#define TRBTREE_HH_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>

#ifndef UINTPTR_MAX
#error "Unsupported platform"
#endif


// TODO: save / load
// TODO: implement IBM/Adriver kind of intrusive spinlocking. Element should be struct { THint hint; TKVPair kvPair; }
//       where hint = struct 
//       {
//           TIndex leftIdx; TIndex rightIdx; // indexes into preallocated lists
//           TColor leftColor; TColor rightColor; // children colors
//           TTrail leftTrail; TTrail rightTrail; // children trail types
//           TMark mark; // flags for locking
//           TTag tag; // anti ABA tags
//       }; // of size 8/16 for CASing.
//       
//       Threaded RB Tree algorithm should be linearizable with spinlocking elements in up to 3 recolors/rotations
//       incrementing tags for all elements changed in them, and validating tags up the stack trail to the root

template<typename Key, typename T, class Compare = std::less<Key>, class Allocator = std::allocator<std::pair<const Key, T> > >
class TRBTree
{
    friend int main(int, char*[]); // TODO: debug
public:
    class TRBTreeIterator;
    class TRBTreeConstIterator;
    class TRBTreeReverseIterator;
    class TRBTreeConstReverseIterator;

    typedef Key key_type;
    typedef T mapped_type;
    typedef std::pair<const Key, T> value_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef Compare key_compare;
    typedef Allocator allocator_type;
    typedef std::pair<const Key, T> & reference;
    typedef const std::pair<const Key, T> & const_reference;
    typedef std::pair<const Key, T> * pointer;
    typedef const std::pair<const Key, T> * const_pointer;
    typedef TRBTreeIterator iterator;
    typedef TRBTreeConstIterator const_iterator;
    typedef TRBTreeReverseIterator reverse_iterator;
    typedef TRBTreeConstReverseIterator const_reverse_iterator;

    enum TRBTreeNodeColor
    {
        TRB_BLACK, TRB_RED, TRB_COLOR_MAX
    };
    static inline TRBTreeNodeColor otherColor (TRBTreeNodeColor color)
    {
        if (color == TRB_BLACK)
            return TRB_RED;
        else if (color == TRB_RED)
            return TRB_BLACK;
        return color;
    }

    enum TRBTreeNodeTrailTag
    {
        TRB_CHILD, TRB_TRAIL, TRB_TAG_MAX

    };
    static inline TRBTreeNodeTrailTag otherTag (TRBTreeNodeTrailTag tag)
    {
        if (tag == TRB_CHILD)
            return TRB_TRAIL;
        else if (tag == TRB_TRAIL)
            return TRB_CHILD;
        return tag;
    }

    enum TRBTreeNodeChildSide
    {
        TRB_LEFT, TRB_RIGHT, TRB_SIDE_MAX

    };
    static inline TRBTreeNodeChildSide otherSide (TRBTreeNodeChildSide side)
    {
        if (side == TRB_LEFT)
            return TRB_RIGHT;
        else if (side == TRB_RIGHT)
            return TRB_LEFT;
        return side;
    }

    struct TRBTreeNode
    {
        TRBTreeNode * pLinks[2];
        value_type data;
//        ~TRBTreeNode() { delete pData; pData = 0; }
    }__attribute__((aligned(sizeof(pointer)), packed)); // Alignment to size of pointer allows us to shave at least 2 bits of every pointer for our own needs

    // Bidirectional Iterator
    class TRBTreeIterator
    {
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef std::pair<const Key, T> value_type;
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        typedef std::pair<const Key, T> * pointer;
        typedef std::pair<const Key, T> & reference;

        TRBTreeIterator (TRBTree * inTree = 0, TRBTreeNode * inNode = 0) :
                        pTree (inTree), pNode (inNode)
        {
        }
        reference operator* ()
        {
            return pNode->data;
        }
        const reference operator* () const
        {
            return pNode->data;
        }
        TRBTreeIterator& operator++ ()
        {
            // TODO: forward navigation
            return *this;
        }
        TRBTreeIterator operator++ (int)
        {
            TRBTreeIterator temp = *this;
            ++*this;
            return temp;
        }
        TRBTreeIterator& operator-- ()
        {
            // TODO: backward navigation
            return *this;
        }
        TRBTreeIterator operator-- (int)
        {
            TRBTreeIterator temp = *this;
            ++*this;
            return temp;
        }
        bool operator== (const TRBTreeIterator& x) const
        {
            return pNode == x.pNode;
        }
        bool operator!= (const TRBTreeIterator& x) const
        {
            return pNode != x.pNode;
        }
    private:
        TRBTree * pTree;
        TRBTreeNode * pNode;
    };

    class TRBTreeValueCompare
    {
    public:
        typedef bool result_type;
        typedef value_type first_argument_type;
        typedef value_type second_argument_type;

        TRBTreeValueCompare (Compare inComparator) :
                        comparator (inComparator)
        {
        }
        ;
        bool operator() (const value_type& lhs, const value_type& rhs) const
        {
            return comparator (lhs, rhs);
        }
        ;
    private:
        Compare comparator;
    };

    typedef TRBTreeValueCompare value_compare;

    // Constructors & destructor
    explicit TRBTree (const Compare& comp = Compare (), const Allocator& alloc = Allocator ());
    template<class InputIterator>
    TRBTree (InputIterator first, InputIterator last, const Compare& comp = Compare (), const Allocator& alloc = Allocator ());
    TRBTree (const TRBTree& other, bool isCopy = true);
    TRBTree (TRBTree& other, bool isCopy = true);
    ~TRBTree ();

    // Element access
    iterator begin ();
    const_iterator cbegin () const;
    iterator end ();
    const_iterator cend () const;
    reverse_iterator rbegin ();
    const_reverse_iterator crbegin () const;
    reverse_iterator rend ();
    const_reverse_iterator crend () const;

    // Capacity
    bool empty ();
    size_type size ();
    size_type max_size ();

    // Modifiers
    void clear ();
    std::pair<iterator, bool> insert (const_reference value);
    std::pair<iterator, bool> insert (const_iterator hint, const_reference value);
    template<class InputIterator>
    void insert (InputIterator first, InputIterator last);
    iterator erase (iterator pos);
    iterator erase (const_iterator pos);
    void erase (iterator first, iterator last);
    iterator erase (const_iterator first, const_iterator last);
    size_type erase (const key_type& key);
    void swap (TRBTree & other);

    // Lookup
    size_type count (const Key& key) const;
    iterator find (const Key& key);
    const_iterator find (const Key& key) const;
    std::pair<iterator, iterator> equal_range (const Key& key);
    std::pair<const_iterator, const_iterator> equal_range (const Key& key) const;
    iterator lower_bound (const Key& key);
    const_iterator lower_bound (const Key& key) const;
    iterator upper_bound (const Key& key);
    const_iterator upper_bound (const Key& key) const;
    key_compare key_comp () const { return comparator; };
    value_compare value_comp () const;
private:

    // Node helper functions
    static inline void set_link (TRBTreeNode * pNode, TRBTreeNode * pLink, TRBTreeNodeChildSide childSide = TRB_SIDE_MAX, TRBTreeNodeTrailTag threadTag =
                                                 TRB_TAG_MAX);
    static inline TRBTreeNode * get_link (TRBTreeNode * pNode, TRBTreeNodeChildSide childSide = TRB_SIDE_MAX);
    static inline void set_trail_tag (TRBTreeNode * pNode, TRBTreeNodeChildSide childSide = TRB_SIDE_MAX, TRBTreeNodeTrailTag threadTag = TRB_TAG_MAX);
    static inline TRBTreeNodeTrailTag get_trail_tag (TRBTreeNode * pNode, TRBTreeNodeChildSide childSide = TRB_SIDE_MAX);
    static inline TRBTreeNodeColor get_color (TRBTreeNode * pNode);
    static inline void set_color (TRBTreeNode * pNode, TRBTreeNodeColor color);
    static inline bool copy_node (TRBTreeNode * pDst, TRBTreeNode * pSrc, TRBTreeNodeChildSide side, bool copyData = true);
    TRBTreeNode * try_insert (const_reference value, bool copyData = true);
    TRBTreeNode * try_remove (const key_type & key);
    TRBTreeNode * try_find (const key_type & key);

    // DEBUG!
    /* Prints the structure of |node|,
       which is |level| levels from the top of the tree. */
    void
    print_tree_structure (struct TRBTreeNode * node, int level)
    {
      int i;

      /* You can set the maximum level as high as you like.
         Most of the time, you'll want to debug code using small trees,
         so that a large |level| indicates a ``loop'', which is a bug. */
      if (level > 16)
        {
          printf ("[...]");
          return;
        }

      if (node == NULL)
        {
          printf ("<nil>");
          return;
        }

      printf ("%d(", node->data);

      for (i = 0; i <= 1; i++)
        {
          if (get_trail_tag(node, i ? TRB_RIGHT : TRB_LEFT) == TRB_CHILD)
            {
              if (get_link(node, i ? TRB_RIGHT : TRB_LEFT) == node)
                printf ("loop");
              else
                print_tree_structure (get_link(node, i ? TRB_RIGHT : TRB_LEFT), level + 1);
            }
          else if (get_link(node, i ? TRB_RIGHT : TRB_LEFT) != NULL)
            printf (">%d",
                    get_link(node, i ? TRB_RIGHT : TRB_LEFT)->data.first);
          else
            printf (">>");

          if (i == 0)
            fputs (", ", stdout);
        }

      putchar (')');
    }

    /* Prints the entire structure of |tree| with the given |title|. */
    void
    print_whole_tree (const char *title)
    {
      printf ("%s: ", title);
      print_tree_structure (pRoot, 0);
      putchar ('\n');
    }
    // END DEBUG!

    static const size_t maxHeight = 48;
    TRBTreeNode * pRoot;
    const key_compare & comparator;
    const allocator_type & allocator;
    size_t numElements;
};

// Node helper functions
template<typename Key, typename T, class Compare, class Allocator>
inline void TRBTree<Key, T, Compare, Allocator>::set_link (TRBTree<Key, T, Compare, Allocator>::TRBTreeNode * pNode,
                                                                  TRBTree<Key, T, Compare, Allocator>::TRBTreeNode * pLink,
                                                                  TRBTree<Key, T, Compare, Allocator>::TRBTreeNodeChildSide childSide,
                                                                  TRBTree<Key, T, Compare, Allocator>::TRBTreeNodeTrailTag threadTag)
{
    pNode->pLinks[childSide] = (TRBTreeNode *) (((uintptr_t) pLink & ~3UL) | (threadTag == TRB_TAG_MAX ? (get_trail_tag(pNode, childSide)  == TRB_TRAIL ? 2UL : 0UL) : (threadTag == TRB_TRAIL ? 2UL : 0UL))
                    | (TRBTree<Key, T, Compare, Allocator>::get_color (pNode) == TRB_RED ? 1UL : 0UL));
}

template<typename Key, typename T, class Compare, class Allocator>
inline typename TRBTree<Key, T, Compare, Allocator>::TRBTreeNode * TRBTree<Key, T, Compare, Allocator>::get_link (
                TRBTree<Key, T, Compare, Allocator>::TRBTreeNode * pNode, TRBTree<Key, T, Compare, Allocator>::TRBTreeNodeChildSide childSide)
{
    return (TRBTreeNode *) (((uintptr_t) pNode->pLinks[childSide]) & ~3UL);
}

template<typename Key, typename T, class Compare, class Allocator>
inline void TRBTree<Key, T, Compare, Allocator>::set_trail_tag (TRBTree<Key, T, Compare, Allocator>::TRBTreeNode * pNode,
                                                                       TRBTree<Key, T, Compare, Allocator>::TRBTreeNodeChildSide childSide,
                                                                       TRBTree<Key, T, Compare, Allocator>::TRBTreeNodeTrailTag threadTag)
{
    pNode->pLinks[childSide] = (TRBTreeNode *) (((uintptr_t) pNode->pLinks[childSide] & ~2UL) | (threadTag == TRB_TRAIL ? 2UL : 0UL));
}

template<typename Key, typename T, class Compare, class Allocator>
inline typename TRBTree<Key, T, Compare, Allocator>::TRBTreeNodeTrailTag TRBTree<Key, T, Compare, Allocator>::get_trail_tag (
                TRBTree<Key, T, Compare, Allocator>::TRBTreeNode * pNode, TRBTree<Key, T, Compare, Allocator>::TRBTreeNodeChildSide childSide)
{
    if (((uintptr_t) pNode->pLinks[childSide] & 2UL))
        return TRB_TRAIL;
    else
        return TRB_CHILD;
}

template<typename Key, typename T, class Compare, class Allocator>
inline void TRBTree<Key, T, Compare, Allocator>::set_color (TRBTree<Key, T, Compare, Allocator>::TRBTreeNode * pNode,
                                                                   TRBTree<Key, T, Compare, Allocator>::TRBTreeNodeColor color)
{
    pNode->pLinks[TRB_LEFT] = (TRBTreeNode *) (((uintptr_t) pNode->pLinks[TRB_LEFT] & ~1UL) | (color == TRB_RED ? 1UL : 0UL));
}

template<typename Key, typename T, class Compare, class Allocator>
inline typename TRBTree<Key, T, Compare, Allocator>::TRBTreeNodeColor TRBTree<Key, T, Compare, Allocator>::get_color (
                TRBTree<Key, T, Compare, Allocator>::TRBTreeNode * pNode)
{
    if (((uintptr_t) pNode->pLinks[TRB_LEFT] & 1UL))
        return TRB_RED;
    else
        return TRB_BLACK;
}

template<typename Key, typename T, class Compare, class Allocator>
TRBTree<Key, T, Compare, Allocator>::TRBTree (const Compare& inComp, const Allocator& inAlloc) :
                pRoot (0), comparator (inComp), allocator (inAlloc), numElements (0)
{
}

template<typename Key, typename T, class Compare, class Allocator>
template<class InputIterator>
TRBTree<Key, T, Compare, Allocator>::TRBTree (InputIterator first, InputIterator last, const Compare& inComp, const Allocator& inAlloc) :
                pRoot (0), comparator (inComp), allocator (inAlloc), numElements (0)
{
    // TODO: insert elements from first to last
}

template<typename Key, typename T, class Compare, class Allocator>
inline bool TRBTree<Key, T, Compare, Allocator>::copy_node (TRBTree<Key, T, Compare, Allocator>::TRBTreeNode * pDst,
                                                                   TRBTree<Key, T, Compare, Allocator>::TRBTreeNode * pSrc,
                                                                   TRBTree<Key, T, Compare, Allocator>::TRBTreeNodeChildSide side, bool copyData)
{
    /* Creates a new node as a child of |dst| on side |dir|.
     Copies data and |trb_color| from |src| into the new node,
     applying |copy()|, if non-null.
     Returns nonzero only if fully successful.
     Regardless of success, integrity of the tree structure is assured,
     though failure may leave a null pointer in a |trb_data| member. */

    TRBTreeNode * pNewNode = new TRBTreeNode ();
    if (!pNewNode)
        return false;

    set_link (pNewNode, get_link (pDst, side), side, TRB_TRAIL);
    set_link (pNewNode, pDst, otherSide (side), TRB_TRAIL);
    set_link (pDst, pNewNode, side, TRB_CHILD);

    set_color (pNewNode, get_color (pSrc));

/*    if (!copyData) // this is actually move
    {
        pNewNode->pData = pSrc->pData;
        pSrc->pData = 0;
    }
    else
    {
        pNewNode->pData = new pointer (*(pSrc->pData));

        if (!pNewNode->pData)
            return false;
    }*/
    (key_type&)pNewNode->data.first = pSrc->data.first;
    (mapped_type&)pNewNode->data.second = pSrc->data.second;


    return true;
}

template<typename Key, typename T, class Compare, class Allocator>
TRBTree<Key, T, Compare, Allocator>::TRBTree (const TRBTree<Key, T, Compare, Allocator> & other, bool isCopy) :
                pRoot (0), comparator (other.comparator ()), allocator (other.allocator ()), numElements (0)
{
    const TRBTreeNode *p;
    TRBTreeNode *q;
    TRBTreeNode rp, rq;

    numElements = other.numElements;
    if (!numElements)
        return;

    p = &rp;
    set_link (rp, other.pRoot, TRB_LEFT, TRB_CHILD);

    q = &rq;
    set_link (rq, 0, TRB_LEFT, TRB_TRAIL);

    while (true)
    {
        if (get_trail_tag (p, TRB_LEFT) == TRB_CHILD)
        {
            if (!copy_node (q, get_link (p, TRB_LEFT), TRB_LEFT, isCopy))
                abort ();

            p = get_link (p, TRB_LEFT);
            q = get_link (q, TRB_LEFT);
        }
        else
        {
            while (get_trail_tag (p, TRB_RIGHT) == TRB_TRAIL)
            {
                p = get_link (p, TRB_RIGHT);
                if (!p)
                {
                    set_link (q, 0, TRB_RIGHT, TRB_CHILD);
                    pRoot = get_link (rq, TRB_LEFT);
                    return;
                }
                q = get_link (q, TRB_RIGHT);
            }

            p = get_link (p, TRB_RIGHT);
            q = get_link (q, TRB_RIGHT);
        }

        if ((get_trail_tag (p, TRB_RIGHT) == TRB_CHILD) && !copy_node (q, get_link (p, TRB_RIGHT), TRB_RIGHT, isCopy))
            abort ();
    }
}

template<typename Key, typename T, class Compare, class Allocator>
TRBTree<Key, T, Compare, Allocator>::TRBTree (TRBTree<Key, T, Compare, Allocator> & other, bool isCopy)
{
    const TRBTreeNode *p;
    TRBTreeNode *q;
    TRBTreeNode rp, rq;

    numElements = other.numElements;
    if (!numElements)
        return;

    p = &rp;
    set_link (rp, other.pRoot, TRB_LEFT, TRB_CHILD);

    q = &rq;
    set_link (rq, 0, TRB_LEFT, TRB_TRAIL);

    while (true)
    {
        if (get_trail_tag (p, TRB_LEFT) == TRB_CHILD)
        {
            if (!copy_node (q, get_link (p, TRB_LEFT), TRB_LEFT, isCopy))
                abort ();

            p = get_link (p, TRB_LEFT);
            q = get_link (q, TRB_LEFT);
        }
        else
        {
            while (get_trail_tag (p, TRB_RIGHT) == TRB_TRAIL)
            {
                p = get_link (p, TRB_RIGHT);
                if (!p)
                {
                    set_link (q, 0, TRB_RIGHT, TRB_CHILD);
                    pRoot = get_link (rq, TRB_LEFT);
                    return;
                }
                q = get_link (q, TRB_RIGHT);
            }

            p = get_link (p, TRB_RIGHT);
            q = get_link (q, TRB_RIGHT);
        }

        if ((get_trail_tag (p, TRB_RIGHT) == TRB_CHILD) && !copy_node (q, get_link (p, TRB_RIGHT), TRB_RIGHT, isCopy))
            abort ();
    }
}

template<typename Key, typename T, class Compare, class Allocator>
TRBTree<Key, T, Compare, Allocator>::~TRBTree ()
{
    TRBTreeNode * p;
    TRBTreeNode * n;

    p = pRoot;
    while (p && get_trail_tag (p, TRB_LEFT) == TRB_CHILD)
        p = get_link (p, TRB_LEFT);

    while (p)
    {
        n = get_link (p, TRB_RIGHT);
        if (get_trail_tag (p, TRB_RIGHT) == TRB_CHILD)
            while (get_trail_tag (n, TRB_LEFT) == TRB_CHILD)
                n = get_link (n, TRB_LEFT);

        delete p;
        p = n;
    }
}

template<typename Key, typename T, class Compare, class Allocator>
typename TRBTree<Key, T, Compare, Allocator>::TRBTreeNode * TRBTree<Key, T, Compare, Allocator>::try_insert (TRBTree<Key, T, Compare, Allocator>::const_reference item,
                                                                                                    bool copyData)
{
    TRBTreeNode * pa[maxHeight]; /* pointers to ancestors: ancestor nodes stack. */
    TRBTreeNodeChildSide da[maxHeight]; /* directions from ancestors: directions moved from stack nodes. */
    int k; /* ancestor stack height. */

    TRBTreeNode * p; /* Traverses tree looking for insertion point. */
    TRBTreeNode * n; /* Newly inserted node. */
    TRBTreeNodeChildSide dir; /* Side of |p| on which |n| is inserted. */

    da[0] = TRB_LEFT;
    pa[0] = (TRBTreeNode *) &pRoot; // hoho
    k = 1;
    if (pRoot)
    {
        for (p = pRoot;; p = get_link (p, dir))
        {
            bool itemKeyLess = comparator(item.first, p->data.first);
            bool itemKeyMore = comparator(p->data.first, item.first);
            if (!itemKeyLess && !itemKeyMore)
                return p;

            pa[k] = p;
            da[k++] = dir = itemKeyMore ? TRB_RIGHT : TRB_LEFT;

            if (get_trail_tag (p, dir) == TRB_TRAIL)
                break;
        }
    }
    else
    {
        p = (TRBTreeNode *) &pRoot;
        dir = TRB_LEFT;
    }

    n = new TRBTreeNode ();
    if (!n)
        abort (); // std::bad_alloc will be called above

    ++numElements;
/*    if (copyData)
        n->pData = new value_type (item);
    else
        n->pData = (pointer) &item;*/
    (key_type&)n->data.first = item.first;
    (mapped_type&)n->data.second = item.second;

    set_trail_tag (n, TRB_LEFT, TRB_TRAIL);
    set_trail_tag (n, TRB_RIGHT, TRB_TRAIL);
    set_link (n, get_link (p, dir), dir);
    if (pRoot)
    {
        set_trail_tag (p, dir, TRB_CHILD);
        set_link (n, p, otherSide (dir));
    }
    else
        set_link (n, 0, TRB_RIGHT);

    set_link (p, n, dir);
    set_color (n, TRB_RED);

    while (k >= 3 && get_color (pa[k - 1]) == TRB_RED)
    {
        if (da[k - 2] == TRB_LEFT)
        {
            TRBTreeNode * y = get_link (pa[k - 2], TRB_RIGHT);
            if (get_trail_tag (pa[k - 2], TRB_RIGHT) == TRB_CHILD && get_color (y) == TRB_RED)
            {
                set_color (pa[k - 1], TRB_BLACK);
                set_color (y, TRB_BLACK);
                set_color (pa[k - 2], TRB_RED);
                k -= 2;
            }
            else
            {
                TRBTreeNode * x;

                if (da[k - 1] == TRB_LEFT)
                    y = pa[k - 1];
                else
                {
                    x = pa[k - 1];
                    y = get_link (x, TRB_RIGHT);
                    set_link (x, get_link (y, TRB_LEFT), TRB_RIGHT);
                    set_link (y, x, TRB_LEFT);
                    set_link (pa[k - 2], y, TRB_LEFT);

                    if (get_trail_tag (y, TRB_LEFT) == TRB_TRAIL)
                    {
                        set_trail_tag (y, TRB_LEFT, TRB_CHILD);
                        set_trail_tag (x, TRB_RIGHT, TRB_TRAIL);
                        set_link (x, y, TRB_RIGHT);
                    }
                }

                x = pa[k - 2];
                set_color (x, TRB_RED);
                set_color (y, TRB_BLACK);
                set_link (x, get_link (y, TRB_RIGHT), TRB_LEFT);
                set_link (y, x, TRB_RIGHT);
                set_link (pa[k - 3], y, da[k - 3]);

                if (get_trail_tag (y, TRB_RIGHT) == TRB_TRAIL)
                {
                    set_trail_tag (y, TRB_RIGHT, TRB_CHILD);
                    set_trail_tag (x, TRB_LEFT, TRB_TRAIL);
                    set_link (x, y, TRB_LEFT);
                }
                break;
            }
        }
        else
        {
            TRBTreeNode * y = get_link (pa[k - 2], TRB_LEFT);

            if (get_trail_tag (pa[k - 2], TRB_LEFT) == TRB_CHILD && get_color (y) == TRB_RED)
            {
                set_color (pa[k - 1], TRB_BLACK);
                set_color (y, TRB_BLACK);
                set_color (pa[k - 2], TRB_RED);
                k -= 2;
            }
            else
            {
                TRBTreeNode * x;

                if (da[k - 1] == TRB_RIGHT)
                    y = pa[k - 1];
                else
                {
                    x = pa[k - 1];
                    y = get_link (x, TRB_LEFT);
                    set_link (x, get_link (y, TRB_RIGHT), TRB_LEFT);
                    set_link (y, x, TRB_RIGHT);
                    set_link (pa[k - 2], y, TRB_RIGHT);

                    if (get_trail_tag (y, TRB_RIGHT) == TRB_TRAIL)
                    {
                        set_trail_tag (y, TRB_RIGHT, TRB_CHILD);
                        set_trail_tag (x, TRB_LEFT, TRB_TRAIL);
                        set_link (x, y, TRB_LEFT);
                    }
                }

                x = pa[k - 2];
                set_color (x, TRB_RED);
                set_color (y, TRB_BLACK);
                set_link (x, get_link (y, TRB_LEFT), TRB_RIGHT);
                set_link (y, x, TRB_LEFT);
                set_link (pa[k - 3], y, da[k - 3]);

                if (get_trail_tag (y, TRB_LEFT) == TRB_TRAIL)
                {
                    set_trail_tag (y, TRB_LEFT, TRB_CHILD);
                    set_trail_tag (x, TRB_RIGHT, TRB_TRAIL);
                    set_link (x, y, TRB_RIGHT);

                }
                break;
            }
        }
    }
    set_color (pRoot, TRB_BLACK);

    return n;
}

template<typename Key, typename T, class Compare, class Allocator>
typename TRBTree<Key, T, Compare, Allocator>::TRBTreeNode * TRBTree<Key, T, Compare, Allocator>::try_remove (const TRBTree<Key, T, Compare, Allocator>::key_type & key)
{
    TRBTreeNode * pa[maxHeight]; /* Nodes on stack. */
    TRBTreeNodeChildSide da[maxHeight]; /* Directions moved from stack nodes. */
    int k = 0; /* Stack height. */

    TRBTreeNode * p;
    TRBTreeNode * res;
    TRBTreeNodeChildSide dir;

    if (pRoot == NULL)
        return NULL;

    p = (TRBTreeNode *)&pRoot;
    bool itemLessThan = key_comp()(key, p->data.first);
    bool itemGreaterThan = key_comp()(p->data.first, key);
    for (; itemLessThan || itemGreaterThan; itemLessThan = key_comp()(key, p->data.first), itemGreaterThan = key_comp()(p->data.first, key))
    {
        dir = itemLessThan ? TRB_LEFT : TRB_RIGHT;
        pa[k] = p;
        da[k++] = dir;

        if (get_trail_tag(p, dir) == TRB_TRAIL)
        {
            return NULL;
        }
        p = get_link(p, dir);
    }
    res = p;

    if (get_trail_tag(p, TRB_RIGHT) == TRB_TRAIL)
    {
        if (get_trail_tag(p, TRB_LEFT) == TRB_CHILD)
        {
            TRBTreeNode *t = get_link(p, TRB_LEFT);
            while (get_trail_tag(t, TRB_RIGHT) == TRB_CHILD)
                t = get_link(t, TRB_RIGHT);
            set_link(t, get_link(p, TRB_RIGHT), TRB_RIGHT);
            set_link(pa[k - 1], get_link(p, TRB_LEFT), da[k - 1]);
        }
        else
        {
            set_link(pa[k - 1], get_link(p, da[k - 1]), da[k - 1]);
            if (pa[k - 1] != (TRBTreeNode *) &pRoot)
                set_trail_tag(pa[k - 1], da[k - 1], TRB_TRAIL);
        }
    }
    else
    {
        TRBTreeNodeColor t;
        TRBTreeNode * r = get_link(p, TRB_RIGHT);

        if (get_trail_tag(r, TRB_LEFT) == TRB_TRAIL)
        {
            set_link(r, get_link(p, TRB_LEFT), TRB_LEFT);
            set_trail_tag(r, TRB_LEFT, get_trail_tag(p, TRB_LEFT));
            if (get_trail_tag(r, TRB_LEFT) == TRB_CHILD)
            {
                TRBTreeNode * t = get_link(r, TRB_LEFT);
                while (get_trail_tag(t, TRB_RIGHT) == TRB_CHILD)
                    t = get_link(t, TRB_RIGHT);
                set_link(t, r, TRB_RIGHT);
            }
            set_link(pa[k - 1], r, da[k - 1]);
            t = get_color(r);
            set_color(r, get_color(p));
            set_color(p, t);
            da[k] = TRB_RIGHT;
            pa[k++] = r;
        }
        else
        {
            TRBTreeNode * s;
            int j = k++;

            for (;;)
            {
                da[k] = TRB_LEFT;
                pa[k++] = r;
                s = get_link(r, TRB_LEFT);
                if (get_trail_tag(s, TRB_LEFT) == TRB_TRAIL)
                    break;
                r = s;
            }

            da[j] = TRB_RIGHT;
            pa[j] = s;
            if (get_trail_tag(s, TRB_RIGHT) == TRB_CHILD)
                set_link(r, get_link(s, TRB_RIGHT), TRB_LEFT);
            else
            {
                set_link(r, s, TRB_LEFT);
                set_trail_tag(r, TRB_LEFT, TRB_TRAIL);
            }

            set_link(s, get_link(p, TRB_LEFT), TRB_LEFT);
            if (get_trail_tag(p, TRB_LEFT) == TRB_CHILD)
            {
                TRBTreeNode * t = get_link(p, TRB_LEFT);
                while (get_trail_tag(t, TRB_RIGHT) == TRB_CHILD)
                    t = get_link(t, TRB_RIGHT);
                set_link(t, s, TRB_RIGHT);
                set_trail_tag(s, TRB_LEFT, TRB_CHILD);
            }

            set_link(s, get_link(p, TRB_RIGHT), TRB_RIGHT);
            set_trail_tag(s, TRB_RIGHT, TRB_CHILD);

            t = get_color(s);
            set_color(s, get_color(p));
            set_color(p, t);

            set_link(pa[j - 1], s, da[j - 1]);
        }
    }

    if (get_color(p) == TRB_BLACK)
    {
        for (; k > 1; k--)
        {
            if (get_trail_tag(pa[k - 1], da[k - 1]) == TRB_CHILD)
            {
                TRBTreeNode * x = get_link(pa[k - 1], da[k - 1]);
                if (get_color(x) == TRB_RED)
                {
                    set_color(x, TRB_BLACK);
                    break;
                }
            }

            if (da[k - 1] == TRB_LEFT)
            {
                TRBTreeNode * w = get_link(pa[k - 1], TRB_RIGHT);

                if (get_color(w) == TRB_RED)
                {
                    set_color(w, TRB_BLACK);
                    set_color(pa[k - 1], TRB_RED);

                    set_link(pa[k - 1], get_link(w, TRB_LEFT), TRB_RIGHT);
                    set_link(w, pa[k - 1], TRB_LEFT);
                    set_link(pa[k - 2], w, da[k - 2]);

                    pa[k] = pa[k - 1];
                    da[k] = TRB_LEFT;
                    pa[k - 1] = w;
                    k++;

                    w = get_link(pa[k - 1], TRB_RIGHT);
                }

                if ((get_trail_tag(w, TRB_LEFT) == TRB_TRAIL || get_color(get_link(w, TRB_LEFT)) == TRB_BLACK) && (get_trail_tag(w, TRB_RIGHT) == TRB_TRAIL || get_color(get_link(w, TRB_RIGHT)) == TRB_BLACK))
                {
                    set_color(w, TRB_RED);
                }
                else
                {
                    if (get_trail_tag(w, TRB_RIGHT) == TRB_TRAIL || get_color(get_link(w, TRB_RIGHT)) == TRB_BLACK)
                    {
                        TRBTreeNode * y = get_link(w, TRB_LEFT);
                        set_color(y, TRB_BLACK);
                        set_color(w, TRB_RED);
                        set_link(w, get_link(y, TRB_RIGHT), TRB_LEFT);
                        set_link(y, w, TRB_RIGHT);
                        set_link(pa[k - 1], y, TRB_RIGHT);
                        w = y;

                        if (get_trail_tag(w, TRB_RIGHT) == TRB_TRAIL)
                        {
                            set_trail_tag(w, TRB_RIGHT, TRB_CHILD);
                            set_trail_tag(get_link(w, TRB_RIGHT), TRB_LEFT, TRB_TRAIL);
                            set_link(get_link(w, TRB_RIGHT), w, TRB_LEFT);
                        }
                    }

                    set_color(w, get_color(pa[k - 1]));
                    set_color(pa[k - 1], TRB_BLACK);
                    set_color(get_link(w, TRB_RIGHT), TRB_BLACK);
                    set_link(pa[k - 1], get_link(w, TRB_LEFT), TRB_RIGHT);
                    set_link(w, pa[k - 1], TRB_LEFT);
                    set_link(pa[k - 2], w, da[k - 2]);

                    if (get_trail_tag(w, TRB_LEFT) == TRB_TRAIL)
                    {
                        set_trail_tag(w, TRB_LEFT, TRB_CHILD);
                        set_trail_tag(pa[k - 1], TRB_RIGHT, TRB_TRAIL);
                        set_link(pa[k - 1], w, TRB_RIGHT);
                    }
                    break;
                }
            }
            else
            {
                TRBTreeNode * w = get_link(pa[k - 1], TRB_LEFT);

                if (get_color(w) == TRB_RED)
                {
                    set_color(w, TRB_BLACK);
                    set_color(pa[k - 1], TRB_RED);

                    set_link(pa[k - 1], get_link(w, TRB_RIGHT), TRB_LEFT);
                    set_link(w, pa[k - 1], TRB_RIGHT);
                    set_link(pa[k - 2], w, da[k - 2]);

                    pa[k] = pa[k - 1];
                    da[k] = TRB_RIGHT;
                    pa[k - 1] = w;
                    k++;


                    w = get_link(pa[k - 1], TRB_LEFT);
                }

                if ((get_trail_tag(w, TRB_LEFT) == TRB_TRAIL || get_color(get_link(w, TRB_LEFT))== TRB_BLACK) && (get_trail_tag(w, TRB_RIGHT) == TRB_TRAIL || get_color(get_link(w, TRB_RIGHT)) == TRB_BLACK))
                {
                    set_color(w, TRB_RED);
                }
                else
                {
                    if (get_trail_tag(w, TRB_LEFT) == TRB_TRAIL || get_color(get_link(w, TRB_LEFT)) == TRB_BLACK)
                    {
                        TRBTreeNode * y = get_link(w, TRB_RIGHT);
                        set_color(y, TRB_BLACK);
                        set_color(w, TRB_RED);
                        set_link(w, get_link(y, TRB_LEFT), TRB_RIGHT);
                        set_link(y, w, TRB_LEFT);
                        set_link(pa[k - 1], y, TRB_LEFT);
                        w = y;

                        if (get_trail_tag(w, TRB_RIGHT) == TRB_TRAIL)
                        {
                            set_trail_tag(w, TRB_LEFT, TRB_CHILD);
                            set_trail_tag(get_link(w, TRB_LEFT), TRB_RIGHT, TRB_TRAIL);
                            set_link(get_link(w, TRB_LEFT), w, TRB_RIGHT);
                        }
                    }

                    set_color(w, get_color(pa[k - 1]));
                    set_color(pa[k - 1], TRB_BLACK);
                    set_color(get_link(w, TRB_LEFT), TRB_BLACK);

                    set_link(pa[k - 1], get_link(w, TRB_RIGHT), TRB_LEFT);
                    set_link(w, pa[k - 1], TRB_RIGHT);
                    set_link(pa[k - 2], w, da[k - 2]);

                    if (get_trail_tag(w, TRB_RIGHT) == TRB_TRAIL)
                    {
                        set_trail_tag(w, TRB_RIGHT, TRB_CHILD);
                        set_trail_tag(pa[k - 1], TRB_LEFT, TRB_TRAIL);
                        set_link(pa[k - 1], w, TRB_LEFT);
                    }
                    break;
                }
            }
        }

        if (pRoot != NULL)
            set_color(pRoot, TRB_BLACK);
    }

    delete res;
    --numElements;

    return 0; // TODO: return next(res)
}

template<typename Key, typename T, class Compare, class Allocator>
typename TRBTree<Key, T, Compare, Allocator>::TRBTreeNode * TRBTree<Key, T, Compare, Allocator>::try_find (const TRBTree<Key, T, Compare, Allocator>::key_type & key)
{
    TRBTreeNode * p;
    TRBTreeNodeChildSide dir;

    if (pRoot == NULL)
        return NULL;

    p = pRoot;
    bool itemLessThan = key_comp()(key, p->data.first);
    bool itemGreaterThan = key_comp()(p->data.first, key);
    for (; itemLessThan || itemGreaterThan; itemLessThan = key_comp()(key, p->data.first), itemGreaterThan = key_comp()(p->data.first, key))
    {
        dir = itemLessThan ? TRB_LEFT : TRB_RIGHT;

        if (get_trail_tag(p, dir) == TRB_TRAIL)
        {
            return NULL;
        }
        p = get_link(p, dir);
    }
    return p;
}


template<typename Key, typename T, class Compare, class Allocator>
bool operator== (const TRBTree<Key, T, Compare, Allocator>& lhs, const TRBTree<Key, T, Compare, Allocator>& rhs);

template<typename Key, typename T, class Compare, class Allocator>
bool operator!= (const TRBTree<Key, T, Compare, Allocator>& lhs, const TRBTree<Key, T, Compare, Allocator>& rhs);

template<typename Key, typename T, class Compare, class Allocator>
bool operator< (const TRBTree<Key, T, Compare, Allocator>& lhs, const TRBTree<Key, T, Compare, Allocator>& rhs);

template<typename Key, typename T, class Compare, class Allocator>
bool operator<= (const TRBTree<Key, T, Compare, Allocator>& lhs, const TRBTree<Key, T, Compare, Allocator>& rhs);

template<typename Key, typename T, class Compare, class Allocator>
bool operator> (const TRBTree<Key, T, Compare, Allocator>& lhs, const TRBTree<Key, T, Compare, Allocator>& rhs);

template<typename Key, typename T, class Compare, class Allocator>
bool operator>= (const TRBTree<Key, T, Compare, Allocator>& lhs, const TRBTree<Key, T, Compare, Allocator>& rhs);

/* Table traverser functions.
void trb_t_init (struct trb_traverser *, struct trb_table *);
void *trb_t_first (struct trb_traverser *, struct trb_table *);
void *trb_t_last (struct trb_traverser *, struct trb_table *);
void *trb_t_find (struct trb_traverser *, struct trb_table *, void *);
void *trb_t_insert (struct trb_traverser *, struct trb_table *, void *);
void *trb_t_copy (struct trb_traverser *, const struct trb_traverser *);
void *trb_t_next (struct trb_traverser *);
void *trb_t_prev (struct trb_traverser *);
void *trb_t_cur (struct trb_traverser *);
void *trb_t_replace (struct trb_traverser *, void *);
*/


#endif /* TRBTREE_HH_ */
