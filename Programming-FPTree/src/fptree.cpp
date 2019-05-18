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
        // this->keys[0] = k;
        this->childrens[0] = newChildR->node;

        // this->nKeys = 1;
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
            newChild = this->split();
            if (this->isRoot) {
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
