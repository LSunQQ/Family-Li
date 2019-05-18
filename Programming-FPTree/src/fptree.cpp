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

    // next level is leaf, insert to childrens array
    // TODO
    int place = findIndex(leaf.key);
    if (this->childrens[0]->ifLeaf()) {
        this->insertNonFull(leaf.key, leaf.node);
        if (this->nKeys == 2 * this->degree + 1) {
            KeyNode * newInnernode = this->split();
            if(this->isRoot) {
                InnerNode *newroot = new InnerNode(this->degree,this->tree,true);
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
                    InnerNode *newroot = new InnerNode(this->degree,this->tree,true);
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
    
    // recursive remove
    // TODO
    return ifRemove;
}

// If the leftBro and rightBro exist, the rightBro is prior to be used
void InnerNode::getBrother(const int& index, InnerNode* const& parent, InnerNode* &leftBro, InnerNode* &rightBro) {
    // TODO
}

// merge this node, its parent and left brother(parent is root)
void InnerNode::mergeParentLeft(InnerNode* const& parent, InnerNode* const& leftBro) {
    // TODO
}

// merge this node, its parent and right brother(parent is root)
void InnerNode::mergeParentRight(InnerNode* const& parent, InnerNode* const& rightBro) {
    // TODO
}

// this node and its left brother redistribute
// the left has more entries
void InnerNode::redistributeLeft(const int& index, InnerNode* const& leftBro, InnerNode* const& parent) {
    // TODO
}

// this node and its right brother redistribute
// the right has more entries
void InnerNode::redistributeRight(const int& index, InnerNode* const& rightBro, InnerNode* const& parent) {
    // TODO
}

// merge all entries to its left bro, delete this node after merging.
void InnerNode::mergeLeft(InnerNode* const& leftBro, const Key& k) {
    // TODO
}

// merge all entries to its right bro, delete this node after merging.
void InnerNode::mergeRight(InnerNode* const& rightBro, const Key& k) {
    // TODO
}

// remove a children from the current node, used by remove func
void InnerNode::removeChild(const int& keyIdx, const int& childIdx) {
    // TODO
}

// update the target entry, return true if the update succeed.
bool InnerNode::update(const Key& k, const Value& v) {
    // TODO
    if(nKeys == 0 && nChild == 0) return false;
    int idx = find(k);
    return childrens[idx]->update(k,v);
}

// find the target value with the search key, return MAX_VALUE if it fails.
Value InnerNode::find(const Key& k) {
    // TODO
    int place = findIndex(k);
    return childrens[place]->find(k);
}

// get the children node of this InnerNode
Node* InnerNode::getChild(const int& idx) {
    // TODO
    if (idx < this->nChild)
        return this->childrens[idx];
    return NULL;
}

// get the key of this InnerNode
Key InnerNode::getKey(const int& idx) {
    if (idx < this->nKeys) {
        return this->keys[idx];
    } else {
        return MAX_KEY;
    }
}

// print the InnerNode
void InnerNode::printNode() {
    cout << "||#|";
    for (int i = 0; i < this->nKeys; i++) {
        cout << " " << this->keys[i] << " |#|";
    }
    cout << "|" << "    ";
}

// print the LeafNode
void LeafNode::printNode() {
    cout << "||";
    for (int i = 0; i < 2 * this->degree; i++) {
        if (this->getBit(i)) {
            cout << " " << this->kv[i].k << " : " << this->kv[i].v << " |";
        }
    }
    cout << "|" << " ====>> ";
}

// new a empty leaf and set the valuable of the LeafNode
LeafNode::LeafNode(FPTree* t) {
    // TODO
    this->tree = t;
    PAllocator::getAllocator()->getLeaf(this->pPointer, this->pmem_addr);
    this->degree = LEAF_DEGREE;
    this->isLeaf = true;
    this->bitmapSize = (2 * this->degree + 7) / 8;
    this->bitmap = (Byte *)this->pmem_addr;
    this->pNext = (PPointer *)(this->pmem_addr + this->bitmapSize);
    this->fingerprints = (Byte *)(this->pmem_addr + this->bitmapSize + sizeof(this->pPointer));
    this->kv = (KeyValue *)(this->fingerprints + 2 * this->degree * sizeof(Byte));
    this->n = 0;
    this->prev = nullptr;
    this->next = nullptr;
    filePath=DATA_DIR+ to_string(this->pPointer.fileId);
}

// reload the leaf with the specific Persistent Pointer
// need to call the PAllocator
LeafNode::LeafNode(PPointer p, FPTree* t) {
    // TODO
    this->tree = t;
    this->pPointer = p;
    this->pmem_addr = PAllocator::getAllocator()->getLeafPmemAddr(this->pPointer);
    this->degree = LEAF_DEGREE;
    this->isLeaf = true;
    this->bitmapSize = (2 * this->degree + 7) / 8;
    this->bitmap = (Byte *)this->pmem_addr;
    this->pNext = (PPointer *)(this->pmem_addr + this->bitmapSize);
    this->fingerprints = (Byte *)(this->pmem_addr + this->bitmapSize + sizeof(this->pPointer));
    this->kv = (KeyValue *)(this->fingerprints + 2 * this->degree * sizeof(Byte));
    this->n = 0;
    this->prev = nullptr;
    Byte* tmp = this->bitmap;
    for(int i = 0; i < bitmapSize;i++) {
        n += countOneBits(*tmp);
        tmp++;
    }
    this->filePath = DATA_DIR + to_string(this->pPointer.fileId);
    if(pNext->fileId != 0) {
        this->next = new LeafNode(*pNext, t);
        this->next->prev = this;
    }
    else
        this->next = nullptr;
}

LeafNode::~LeafNode() {
    // TODO
    PAllocator::getAllocator()->freeLeaf(this->pPointer);
}

// insert an entry into the leaf, need to split it if it is full
KeyNode* LeafNode::insert(const Key& k, const Value& v) {
    KeyNode* newChild = NULL;
    // TODO
    if (n >= 2 * degree - 1) {
        insertNonFull(k, v);
        newChild = split();
        next = (LeafNode*)(newChild -> node);
        ((LeafNode*)(newChild -> node)) -> prev = this;
    }    
    else
        insertNonFull(k,v);
    return newChild;
}

// insert into the leaf node that is assumed not full
void LeafNode::insertNonFull(const Key& k, const Value& v) {
    // TODO
    int i = findFirstZero(); 
    this->fingerprints[i] = keyHash(k);
    this->bitmap[i / 8] |= 1 << (i % 8);
    this->kv[i].k = k;
    this->kv[i].v = v;
    n++;
    persist();
}

// split the leaf node
KeyNode* LeafNode::split() {
    KeyNode* newChild = new KeyNode();
    // TODO
    Key splitkey = findSplitKey();
    LeafNode *newNode = new LeafNode(this->tree);
    memset(bitmap, 0, bitmapSize);

    for(int i = 0; i < n; i ++) {
        if(i < n / 2) {
            this->fingerprints[i] = keyHash(kv[i].k);
            bitmap[i / 8] |= (1 << (7 - (i % 8)));
        }
        else {
            newNode->insertNonFull(kv[i].k, kv[i].v);
        }
    }

    n /= 2;
    *pNext = newNode->pPointer;
    newChild->key = splitkey;
    newChild->node = newNode;
    newNode->persist();
    this->persist();
    return newChild;
}

// use to find a mediant key and delete entries less then middle
// called by the split func to generate new leaf-node
// qsort first then find

int cmp(const void* kv1,const void* kv2) { return ((KeyValue* )kv1)->k > ((KeyValue* )kv2)->k;}

Key LeafNode::findSplitKey() {
    Key midKey = 0;
    // TODO
    qsort(kv, n, sizeof(KeyValue), cmp);
    midKey = kv[n / 2].k;
    return midKey;
}

// get the targte bit in bitmap
// TIPS: bit operation
int LeafNode::getBit(const int& idx) {
    // TODO
    Byte flag = (bitmap[idx / 8] >> (idx % 8)) & 1;
    return flag;
}

Key LeafNode::getKey(const int& idx) {
    return this->kv[idx].k;
}

Value LeafNode::getValue(const int& idx) {
    return this->kv[idx].v;
}

PPointer LeafNode::getPPointer() {
    return this->pPointer;
}

// remove an entry from the leaf
// if it has no entry after removement return TRUE to indicate outer func to delete this leaf.
// need to call PAllocator to set this leaf free and reuse it
bool LeafNode::remove(const Key& k, const int& index, InnerNode* const& parent, bool &ifDelete) {
    bool ifRemove = false;
    // TODO
    return ifRemove;
}

// update the target entry
// return TRUE if the update succeed
bool LeafNode::update(const Key& k, const Value& v) {
    bool ifUpdate = false;
    // TODO
    int hash = keyHash(k);
    for(int i = 0;i < 2 * degree; ++i){
        if(getBit(i) == 1 && (fingerprints[i] == hash)){
            if(getKey(i) == k){
                kv[i].v = v;
                ifUpdate = true;
                break;
            }
        }
    }
    persist();
    return ifUpdate;
}

// if the entry can not be found, return the max Value
Value LeafNode::find(const Key& k) {
    // TODO
    Byte* pointer = fingerprints;
    Byte ha = keyHash(k);
    for (int i = 0; i < degree * 2; ++i) {
        if (getBit(i) && (kv[i].k == k)&&(fingerprints[i] == ha)) {
            return kv[i].v;
        }
        pointer++;
    }
    return MAX_VALUE;
}

// find the first empty slot
int LeafNode::findFirstZero() {
    // TODO
    int i;
    for (i = 0; i < this->degree * 2; i++)
        if (this->getBit(i)==0)
            return i;
    return -1;
}

// persist the entire leaf
// use PMDK
void LeafNode::persist() {
    // TODO
    // pmem_msync(pmem_addr,calLeafSize());
    pmem_persist(pmem_addr,calLeafSize());
}

// call by the ~FPTree(), delete the whole tree
void FPTree::recursiveDelete(Node* n) {
    if (n->isLeaf) {
        delete n;
    } else {
        for (int i = 0; i < ((InnerNode*)n)->nChild; i++) {
            recursiveDelete(((InnerNode*)n)->childrens[i]);
        }
        delete n;
    }
}

FPTree::FPTree(uint64_t t_degree) {
    FPTree* temp = this;
    this->root = new InnerNode(t_degree, temp, true);
    this->degree = t_degree;
    bulkLoading();
}

FPTree::~FPTree() {
    recursiveDelete(this->root);
}

// get the root node of the tree
InnerNode* FPTree::getRoot() {
    return this->root;
}

// change the root of the tree
void FPTree::changeRoot(InnerNode* newRoot) {
    this->root = newRoot;
}

void FPTree::insert(Key k, Value v) {
    if (root != NULL) {
        root->insert(k, v);
    }
}

bool FPTree::remove(Key k) {
    if (root != NULL) {
        bool ifDelete = false;
        InnerNode* temp = NULL;
        return root->remove(k, -1, temp, ifDelete);
    }
    return false;
}

bool FPTree::update(Key k, Value v) {
    if (root != NULL) {
        return root->update(k, v);
    }
    return false;
}

Value FPTree::find(Key k) {
    if (root != NULL) {
        return root->find(k);
    }
}

// call the InnerNode and LeafNode print func to print the whole tree
// TIPS: use Queue
void FPTree::printTree() {
    // TODO
}

// bulkLoading the leaf files and reload the tree
// need to traverse leaves chain
// if no tree is reloaded, return FALSE
// need to call the PALlocator
bool FPTree::bulkLoading() {
    // TODO
    PPointer startpointer = PAllocator::getAllocator()->getStartPointer();
    if(startpointer.fileId != 0) {
        LeafNode* leaf = new LeafNode(startpointer, this);
        KeyNode releaf;
        while(leaf != nullptr){
            releaf.node = leaf;
            Key result = leaf->getKey(0);
            for(int i = 1; i < leaf->n; i++) {
                if(leaf->getKey(i) < result && leaf->getBit(i))
                    result = leaf->getKey(i);
            }
            releaf.key = result;
            this->root->insertLeaf(releaf);
            leaf = leaf->next;
        }
        return true;
    }
    return false;
}
