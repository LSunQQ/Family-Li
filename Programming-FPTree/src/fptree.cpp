#include "fptree/fptree.h"
#include <algorithm>
using namespace std;

int KEY = 0;

// Initial the new InnerNode
InnerNode::InnerNode(const int& d, FPTree* const& t, bool _isRoot) {
    // TODO
    this->isRoot = _isRoot;
    this->degree = d;
    this->tree = t;
    this->isLeaf = false;
    this->nKeys = 0;
    this->nChild = 0;
    /** because we insert before judge whether the node is full or not
      * so we initial with one more space for inserting*/
    this->keys = new Key [2 * d + 1];
    this->childrens = new Node * [2 * d + 2];
}

// delete the InnerNode
InnerNode::~InnerNode() {
    // TODO
    delete keys;
    delete childrens;
}

// binary search the first key in the innernode larger than input key
int InnerNode::findIndex(const Key& k) {
    // TODO
    // if there is only one node
    if (this->nKeys == 0) return 0;
    int place;
    for(place = 0; place < this->nKeys; ++place) {
        if(this->keys[place] > k)
            return place;
    }
    return place;
}

// insert the node that is assumed not full
// insert format:
// ======================
// | key | node pointer |
// ======================
// WARNING: can not insert when it has no entry
void InnerNode::insertNonFull(const Key& k, Node* const& node) {
    // TODO
    int place = findIndex(k);
    for(int i = this->nKeys; i > place; --i)
        this->keys[i] = this->keys[i - 1];
    for(int i = this->nChild; i > place + 1; --i)
        this->childrens[i] = this->childrens[i - 1];
    this->keys[place] = k;
    this->childrens[place + 1] = node;
    ++this->nKeys;
    ++this->nChild;
}

// insert func
// return value is not NULL if split, returning the new child and a key to insert
KeyNode* InnerNode::insert(const Key& k, const Value& v) {
    KeyNode* newChild = NULL;

    // 1.insertion to the first leaf(only one leaf)
    if (this->isRoot && this->nKeys == 0 && this->nChild == 0) {
        // TODO
        KeyNode* newChildR = new KeyNode[1];
        newChildR->node = new LeafNode(tree);
        newChildR->node->insert(k,v);
        newChildR->key = k;
        // we do not set the nKeys to 1
        this->childrens[0] = newChildR->node;
        this->nChild = 1;
        return NULL;
    }
    // 2.recursive insertion
    // TODO
	int place = findIndex(k);
    newChild = childrens[place]->insert(k,v);
    if (newChild == NULL)
        return NULL;
    else {
        insertNonFull(newChild->key, newChild->node);
        if (this->nKeys <= 2 * degree)
            return NULL;
        else {
            // if the node if full, split it
            newChild = this->split();
            if (this->isRoot) {
                /** if the node that has splited is the root
                  * it means that the tree has grown up a level
                  * we should change the root*/
                this->isRoot = false;
                InnerNode * newRoot = new InnerNode(this->degree, tree, true);
                newRoot->nKeys = 1;
                newRoot->nChild = 2;
                newRoot->keys[0] = newChild->key;
                newRoot->childrens[0] = this;
                newRoot->childrens[1] = newChild->node;
                this->tree->changeRoot(newRoot);
                return NULL;
            }
            return newChild;
        }
    }
    return newChild;
}

// ensure that the leaves inserted are ordered
// used by the bulkLoading func
// inserted data: | minKey of leaf | LeafNode* |
KeyNode* InnerNode::insertLeaf(const KeyNode& leaf) {
    KeyNode* newChild = NULL;
    // first and second leaf insertion into the tree
    if (this->isRoot && this->nKeys == 0 && this->nChild==0) {
        // TODO
        this->keys[0] = leaf.key;
        this->childrens[1] = leaf.node;
        LeafNode * newnode = new LeafNode(tree);
        this->childrens[0] = newnode;
        this->nChild = 2;
        this->nKeys = 1;
        return NULL;
    }
    
    // recursive insert
    // Tip: please judge whether this InnerNode is full
    // next level is not leaf, just insertLeaf
    // TODO
    int place = findIndex(leaf.key);
    if (this->childrens[0]->ifLeaf()) {
        this->insertNonFull(leaf.key, leaf.node);
        if (this->nKeys == 2 * this->degree + 1) {
            // if the node if full, split it
            KeyNode * newInnernode = this->split();
            if(this->isRoot) {
                /** if the node that has splited is the root
                  * it means that the tree has grown up a level
                  * we should change the root*/
                InnerNode *newroot = new InnerNode(this->degree, this->tree, true);
                this->isRoot = 0;
                newroot->childrens[0] = this;
                newroot->nChild = 2;
                newroot->insertNonFull(newInnernode->key,newInnernode->node);
                newroot->nKeys = 1;
                this->tree->changeRoot(newroot);
                return NULL;
            }
            else return newInnernode;
        }
        else return NULL;
    }
    else{
        newChild = ((InnerNode*)this->childrens[place])->insertLeaf(leaf);
        if(newChild == NULL) return newChild;
        else {
            insertNonFull(newChild->key,newChild->node);
            if (this->nKeys == 2 * this->degree + 1) {
                KeyNode * newInnernode = this->split();
                if(this->isRoot) {
                    /** if the node that has splited is the root
                      * it means that the tree has grown up a level
                      * we should change the root*/
                    InnerNode *newroot = new InnerNode(this->degree, this->tree, true);
                    this->isRoot = 0;
                    newroot->childrens[0] = this;
                    newroot->nChild = 2;
                    newroot->insertNonFull(newInnernode->key,newInnernode->node);
                    newroot->nKeys = 1;
                    this->tree->changeRoot(newroot);
                    return NULL;
                }
                else return newInnernode;
            }
            else return NULL;
        }
    }
    return newChild;
}

KeyNode* InnerNode::split() {
    KeyNode* newChild = new KeyNode();
    // right half entries of old node to the new node, others to the old node. 
    // TODO
    newChild->key = this->keys[this->degree];
    InnerNode *temp = new InnerNode(this->degree, this->tree, false);
    // move the nodes to the new node
    temp->nKeys = this->degree;
    temp->nChild = this->degree + 1;
    for (int i = 0; i <= this->degree; ++i) {
        if (i < this->degree)
            temp->keys[i] = this->keys[this->degree + i + 1];
        temp->childrens[i] = this->childrens[this->degree + i + 1];
    }
    this->nChild /= 2;
    this->nKeys /= 2;
    newChild->node = temp;
    return newChild;
}

// remove  the target entry
// return TRUE if the children node is deleted after removement.
// the InnerNode need to be redistributed or merged after deleting one of its children node.
bool InnerNode::remove(const Key& k, const int& index, InnerNode* const& parent, bool &ifDelete) {
    bool ifRemove = false;
    // only have one leaf
    // TODO
    if (this->isRoot && this->nKeys == 0 && this->nChild == 1) {
        ifRemove = ((LeafNode *)this->childrens[0])->remove(k, index, parent, ifDelete);
        if(ifDelete && ifRemove) {
            ifDelete = false;
            this->nChild--;
        }
        return ifRemove;
    }
    // recursive remove
    // TODO
    int place = this->findIndex(k);
    ifRemove = this->childrens[place]->remove(k, place, this, ifDelete);
    if (ifDelete) {
        this->removeChild(place, place + 1);
        if (this->nKeys < this->degree && !this->isRoot) {
            // if the keys in the nodes are not enough, it should reditribute or merge

            // get the node's brothers first
            InnerNode *left_bro, *right_bro;
            this->getBrother(index, parent, left_bro, right_bro);

            // The following situations are classified in order
            // redistribute first
            if (right_bro != NULL && right_bro->nKeys > this->degree) {
                this->redistributeRight(index, right_bro, parent);
                ifDelete = false;
            }
            else if (left_bro != NULL && left_bro->nKeys > this->degree) {
                this->redistributeLeft(index, left_bro, parent);
                ifDelete = false;
            }
            // and then merge
            else if (right_bro != NULL) {
                // if it's parent is the root, there will create a new root
                if (parent->isRoot && parent->nKeys == 1) {
                    this->mergeParentRight(parent, right_bro);
                    ifDelete = false;
                }
                else {
                    this->mergeRight(right_bro, parent->keys[index]);
                    ifDelete = true;
                }
            }
            else if (left_bro != NULL) {
                // if it's parent is the root, there will create a new root
                if (parent->isRoot && parent->nKeys == 1) {
                    this->mergeParentLeft(parent, left_bro);
                    ifDelete = false;
                }
                else {
                    this->mergeLeft(left_bro, parent->keys[index - 1]);
                    ifDelete = true;
                }
            }
        }
        else ifDelete = false;
    }
    return ifRemove;
}
