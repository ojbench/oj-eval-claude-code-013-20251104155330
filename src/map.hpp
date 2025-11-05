/**
* implement a container like std::map
*/
#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

// only for std::less<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {

template<
   class Key,
   class T,
   class Compare = std::less <Key>
   > class map {
  public:
   /**
  * the internal type of data.
  * it should have a default constructor, a copy constructor.
  * You can use sjtu::map as value_type by typedef.
    */
   typedef pair<const Key, T> value_type;

  private:
   struct Node {
     value_type value;
     Node *left;
     Node *right;
     Node *parent;
     int height;
     Node(const value_type &v, Node *p = nullptr)
       : value(v), left(nullptr), right(nullptr), parent(p), height(1) {}
   };

   Node *root = nullptr;
   size_t n = 0;
   Compare comp = Compare();

   static int h(Node *x) { return x ? x->height : 0; }
   static int max2(int a, int b) { return a > b ? a : b; }
   static void upd(Node *x) { if (x) x->height = 1 + max2(h(x->left), h(x->right)); }
   static int bf(Node *x) { return x ? h(x->left) - h(x->right) : 0; }

   Node *rotateRight(Node *y) {
     Node *x = y->left;
     Node *T2 = x->right;
     x->right = y; y->parent = x;
     y->left = T2; if (T2) T2->parent = y;
     upd(y); upd(x);
     x->parent = y->parent; // will be corrected by caller
     return x;
   }
   Node *rotateLeft(Node *x) {
     Node *y = x->right;
     Node *T2 = y->left;
     y->left = x; x->parent = y;
     x->right = T2; if (T2) T2->parent = x;
     upd(x); upd(y);
     y->parent = x->parent; // will be corrected by caller
     return y;
   }

   Node *rebalance(Node *node) {
     upd(node);
     int b = bf(node);
     if (b > 1) { // left heavy
       if (bf(node->left) < 0) {
         // LR
         node->left = rotateLeft(node->left);
         if (node->left) node->left->parent = node;
       }
       Node *newRoot = rotateRight(node);
       if (newRoot->right) newRoot->right->parent = newRoot;
       return newRoot;
     } else if (b < -1) { // right heavy
       if (bf(node->right) > 0) {
         // RL
         node->right = rotateRight(node->right);
         if (node->right) node->right->parent = node;
       }
       Node *newRoot = rotateLeft(node);
       if (newRoot->left) newRoot->left->parent = newRoot;
       return newRoot;
     }
     return node;
   }

   Node *minNode(Node *x) const {
     if (!x) return nullptr;
     while (x->left) x = x->left;
     return x;
   }
   Node *maxNode(Node *x) const {
     if (!x) return nullptr;
     while (x->right) x = x->right;
     return x;
   }

   Node *successor(Node *x) const {
     if (!x) return nullptr;
     if (x->right) {
       Node *t = x->right;
       while (t->left) t = t->left;
       return t;
     }
     Node *p = x->parent;
     while (p && x == p->right) { x = p; p = p->parent; }
     return p;
   }
   Node *predecessor(Node *x) const {
     if (!x) return nullptr;
     if (x->left) {
       Node *t = x->left;
       while (t->right) t = t->right;
       return t;
     }
     Node *p = x->parent;
     while (p && x == p->left) { x = p; p = p->parent; }
     return p;
   }

   Node *findNode(const Key &key) const {
     Node *cur = root;
     while (cur) {
       if (comp(key, cur->value.first)) cur = cur->left;
       else if (comp(cur->value.first, key)) cur = cur->right;
       else return cur;
     }
     return nullptr;
   }

   Node *insertNode(Node *node, Node *parent, const value_type &val, bool &inserted, Node **insertedPtr) {
     if (!node) {
       Node *nn = new Node(val, parent);
       inserted = true; *insertedPtr = nn; ++n;
       return nn;
     }
     if (comp(val.first, node->value.first)) {
       node->left = insertNode(node->left, node, val, inserted, insertedPtr);
     } else if (comp(node->value.first, val.first)) {
       node->right = insertNode(node->right, node, val, inserted, insertedPtr);
     } else {
       inserted = false; *insertedPtr = node;
       return node;
     }
     Node *res = rebalance(node);
     // fix parent after rotations and set subtree root parent
     if (res->left) res->left->parent = res;
     if (res->right) res->right->parent = res;
     res->parent = parent;
     return res;
   }

   // Erase by node pointer (handles const Key in value_type)
   Node *eraseByNode(Node *node, Node *target, bool &erased) {
     if (!node) return nullptr;
     if (node == target) {
       // do NOT set erased/n here for two-children case
       if (!node->left || !node->right) {
         erased = true; --n;
         Node *child = node->left ? node->left : node->right;
         if (child) child->parent = node->parent;
         Node *ret = child;
         delete node;
         return ret;
       } else {
         // two children: copy successor's value into this node via placement-new, then delete successor
         Node *s = minNode(node->right);
         value_type tmp = s->value;
         node->value.~value_type();
         new (&node->value) value_type(tmp);
         bool erased2 = false;
         node->right = eraseByNode(node->right, s, erased2);
         if (node->right) node->right->parent = node;
         erased = true; // logical erase done
       }
     } else if (comp(target->value.first, node->value.first)) {
       node->left = eraseByNode(node->left, target, erased);
       if (node->left) node->left->parent = node;
     } else {
       node->right = eraseByNode(node->right, target, erased);
       if (node->right) node->right->parent = node;
     }
     Node *res = rebalance(node);
     if (res->left) res->left->parent = res;
     if (res->right) res->right->parent = res;
     // preserve original parent for subtree root
     // NOTE: 'node' might have changed to 'res' after rotations
     // The caller will hook res to its parent; here keep current parent
     // (res->parent already points to old node->parent through linkage)
     return res;
   }

   void destroy(Node *x) {
     if (!x) return;
     destroy(x->left);
     destroy(x->right);
     delete x;
   }

   Node *clone(Node *x, Node *parent) {
     if (!x) return nullptr;
     Node *y = new Node(x->value, parent);
     y->height = x->height;
     y->left = clone(x->left, y);
     y->right = clone(x->right, y);
     return y;
   }

  public:
   class const_iterator;
   class iterator {
      private:
       Node *cur = nullptr;
       const map *owner = nullptr;
      public:
       iterator() {}

       iterator(Node *c, const map *o) : cur(c), owner(o) {}

       iterator(const iterator &other) : cur(other.cur), owner(other.owner) {}

       /**
    * TODO iter++
        */
       iterator operator++(int) {
         if (!owner) throw invalid_iterator();
         if (cur == nullptr) throw invalid_iterator();
         iterator tmp = *this;
         cur = owner->successor(cur);
         return tmp;
       }

       /**
    * TODO ++iter
        */
       iterator &operator++() {
         if (!owner) throw invalid_iterator();
         if (cur == nullptr) throw invalid_iterator();
         cur = owner->successor(cur);
         return *this;
       }

       /**
    * TODO iter--
        */
       iterator operator--(int) {
         if (!owner) throw invalid_iterator();
         iterator tmp = *this;
         if (cur == nullptr) {
           if (!owner->root) throw invalid_iterator();
           // end()-- -> max on non-empty
           cur = owner->maxNode(owner->root);
           return tmp;
         }
         // check begin()
         if (cur == owner->minNode(owner->root)) throw invalid_iterator();
         cur = owner->predecessor(cur);
         return tmp;
       }

       /**
    * TODO --iter
        */
       iterator &operator--() {
         if (!owner) throw invalid_iterator();
         if (cur == nullptr) {
           if (!owner->root) throw invalid_iterator();
           cur = owner->maxNode(owner->root);
           return *this;
         }
         if (cur == owner->minNode(owner->root)) throw invalid_iterator();
         cur = owner->predecessor(cur);
         return *this;
       }

       /**
    * a operator to check whether two iterators are same (pointing to the same memory).
        */
       value_type &operator*() const {
         if (!owner || cur == nullptr) throw invalid_iterator();
         return cur->value;
       }

       bool operator==(const iterator &rhs) const { return owner == rhs.owner && cur == rhs.cur; }

       bool operator==(const const_iterator &rhs) const { return owner == rhs.owner && cur == rhs.cur; }

       /**
    * some other operator for iterator.
        */
       bool operator!=(const iterator &rhs) const { return !(*this == rhs); }

       bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }

       /**
    * for the support of it->first.
    * See <http://kelvinh.github.io/blog/2013/11/20/overloading-of-member-access-operator-dash-greater-than-symbol-in-cpp/> for help.
        */
       value_type *operator->() const
           noexcept {
         if (!owner || cur == nullptr) return nullptr;
         return &cur->value;
       }

       friend class map;
   };
   class const_iterator {
       // it should has similar member method as iterator.
       //  and it should be able to construct from an iterator.
      private:
       Node *cur = nullptr;
       const map *owner = nullptr;
      public:
       const_iterator() {}

       const_iterator(Node *c, const map *o) : cur(c), owner(o) {}

       const_iterator(const const_iterator &other) : cur(other.cur), owner(other.owner) {}

       const_iterator(const iterator &other) : cur(other.cur), owner(other.owner) {}
       // And other methods in iterator.
       // And other methods in iterator.
       // And other methods in iterator.

       const value_type &operator*() const {
         if (!owner || cur == nullptr) throw invalid_iterator();
         return cur->value;
       }
       const value_type *operator->() const noexcept {
         if (!owner || cur == nullptr) return nullptr;
         return &cur->value;
       }
       const_iterator operator++(int) {
         if (!owner) throw invalid_iterator();
         if (cur == nullptr) throw invalid_iterator();
         const_iterator tmp = *this;
         cur = owner->successor(cur);
         return tmp;
       }
       const_iterator &operator++() {
         if (!owner) throw invalid_iterator();
         if (cur == nullptr) throw invalid_iterator();
         cur = owner->successor(cur);
         return *this;
       }
       const_iterator operator--(int) {
         if (!owner) throw invalid_iterator();
         const_iterator tmp = *this;
         if (cur == nullptr) {
           if (!owner->root) throw invalid_iterator();
           cur = owner->maxNode(owner->root); return tmp; }
         if (cur == owner->minNode(owner->root)) throw invalid_iterator();
         cur = owner->predecessor(cur);
         return tmp;
       }
       const_iterator &operator--() {
         if (!owner) throw invalid_iterator();
         if (cur == nullptr) {
           if (!owner->root) throw invalid_iterator();
           cur = owner->maxNode(owner->root); return *this; }
         if (cur == owner->minNode(owner->root)) throw invalid_iterator();
         cur = owner->predecessor(cur);
         return *this;
       }

       bool operator==(const const_iterator &rhs) const { return owner == rhs.owner && cur == rhs.cur; }
       bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
       bool operator==(const iterator &rhs) const { return owner == rhs.owner && cur == rhs.cur; }
       bool operator!=(const iterator &rhs) const { return !(*this == rhs); }

       friend class map;
   };

   /**
  * TODO two constructors
    */
   map() {}

   map(const map &other) {
     root = clone(other.root, nullptr);
     n = other.n;
     if (root) {
       if (root->left) root->left->parent = root;
       if (root->right) root->right->parent = root;
     }
   }

   /**
  * TODO assignment operator
    */
   map &operator=(const map &other) {
     if (this == &other) return *this;
     clear();
     root = clone(other.root, nullptr);
     n = other.n;
     if (root) {
       if (root->left) root->left->parent = root;
       if (root->right) root->right->parent = root;
     }
     return *this;
   }

   /**
  * TODO Destructors
    */
   ~map() { clear(); }

   /**
  * TODO
  * access specified element with bounds checking
  * Returns a reference to the mapped value of the element with key equivalent to key.
  * If no such element exists, an exception of type `index_out_of_bound'
    */
   T &at(const Key &key) {
     Node *x = findNode(key);
     if (!x) throw index_out_of_bound();
     return x->value.second;
   }

   const T &at(const Key &key) const {
     Node *x = findNode(key);
     if (!x) throw index_out_of_bound();
     return x->value.second;
   }

   /**
  * TODO
  * access specified element
  * Returns a reference to the value that is mapped to a key equivalent to key,
  *   performing an insertion if such key does not already exist.
    */
   T &operator[](const Key &key) {
     Node *x = findNode(key);
     if (x) return x->value.second;
     value_type v(key, T());
     bool inserted = false; Node *ins = nullptr;
     root = insertNode(root, nullptr, v, inserted, &ins);
     if (root) root->parent = nullptr;
     return ins->value.second;
   }

   /**
  * behave like at() throw index_out_of_bound if such key does not exist.
    */
   const T &operator[](const Key &key) const {
     Node *x = findNode(key);
     if (!x) throw index_out_of_bound();
     return x->value.second;
   }

   /**
  * return a iterator to the beginning
    */
   iterator begin() { return iterator(minNode(root), this); }

   const_iterator cbegin() const { return const_iterator(minNode(root), this); }

   /**
  * return a iterator to the end
  * in fact, it returns past-the-end.
    */
   iterator end() { return iterator(nullptr, this); }

   const_iterator cend() const { return const_iterator(nullptr, this); }

   /**
  * checks whether the container is empty
  * return true if empty, otherwise false.
    */
   bool empty() const { return n == 0; }

   /**
  * returns the number of elements.
    */
   size_t size() const { return n; }

   /**
  * clears the contents
    */
   void clear() {
     destroy(root);
     root = nullptr;
     n = 0;
   }

   /**
  * insert an element.
  * return a pair, the first of the pair is
  *   the iterator to the new element (or the element that prevented the insertion),
  *   the second one is true if insert successfully, or false.
    */
   pair<iterator, bool> insert(const value_type &value) {
     bool inserted = false; Node *ins = nullptr;
     root = insertNode(root, nullptr, value, inserted, &ins);
     if (root) root->parent = nullptr;
     return pair<iterator, bool>(iterator(ins, this), inserted);
   }

   /**
  * erase the element at pos.
  *
  * throw if pos pointed to a bad element (pos == this->end() || pos points an element out of this)
    */
   void erase(iterator pos) {
     if (pos.owner != this || pos.cur == nullptr) throw invalid_iterator();
     bool erased = false;
     root = eraseByNode(root, pos.cur, erased);
     if (root) root->parent = nullptr;
     if (!erased) throw invalid_iterator();
   }

   /**
  * Returns the number of elements with key
  *   that compares equivalent to the specified argument,
  *   which is either 1 or 0
  *     since this container does not allow duplicates.
  * The default method of check the equivalence is !(a < b || b > a)
    */
   size_t count(const Key &key) const { return findNode(key) ? 1 : 0; }

   /**
  * Finds an element with key equivalent to key.
  * key value of the element to search for.
  * Iterator to an element with key equivalent to key.
  *   If no such element is found, past-the-end (see end()) iterator is returned.
    */
   iterator find(const Key &key) {
     return iterator(findNode(key), this);
   }

   const_iterator find(const Key &key) const { return const_iterator(findNode(key), this); }
};

}

#endif
