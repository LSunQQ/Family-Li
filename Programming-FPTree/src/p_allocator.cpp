#include "utility/p_allocator.h"
#include <iostream>
#include <cstring>
using namespace std;

// the file that store the information of allocator
const string P_ALLOCATOR_CATALOG_NAME = "p_allocator_catalog";
// a list storing the free leaves
const string P_ALLOCATOR_FREE_LIST    = "free_list";
size_t mapped_len;
PAllocator* PAllocator::pAllocator = new PAllocator();

PAllocator* PAllocator::getAllocator() {
    if (PAllocator::pAllocator == NULL) {
        PAllocator::pAllocator = new PAllocator();
    }
    return PAllocator::pAllocator;
}

/* data storing structure of allocator
   In the catalog file, the data structure is listed below
   | maxFileId(8 bytes) | freeNum = m | treebeginLeaf(the PPointer) |
   In freeList file:
   | freeList{(fId, offset)1,...(fId, offset)m} |
*/
PAllocator::PAllocator() {
    string allocatorCatalogPath = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
    string freeListPath         = DATA_DIR + P_ALLOCATOR_FREE_LIST;
    ifstream allocatorCatalog(allocatorCatalogPath, ios::in|ios::binary);
    ifstream freeListFile(freeListPath, ios::in|ios::binary);
    // judge if the catalog exists
    if (allocatorCatalog.is_open() && freeListFile.is_open()) {
        // exist
        // TODO
        allocatorCatalog.read((char*)&(this->maxFileId), sizeof(this->maxFileId));
        allocatorCatalog.read((char*)&(this->freeNum), sizeof(this->freeNum));
        allocatorCatalog.read((char*)&(this->startLeaf.fileId), sizeof(this->startLeaf.fileId));
        allocatorCatalog.read((char*)&(this->startLeaf.offset), sizeof(this->startLeaf.offset));
        this->freeList.resize(this->freeNum);
        for(uint64_t i = 0; i < this->freeNum; i++) {
            freeListFile.read((char*)&(this->freeList[i].fileId), sizeof(this->freeList[i].fileId));
            freeListFile.read((char*)&(this->freeList[i].offset), sizeof(this->freeList[i].offset));
        }
        allocatorCatalog.close();
        freeListFile.close();
    } else {
        // not exist, create catalog and free_list file, then open.
        // TODO
        ofstream allocatorCatalogOut(allocatorCatalogPath, ios::out|ios::binary);
        ofstream freeListFileOut(freeListPath, ios::out|ios::binary);
        this->maxFileId = 1;
        this->freeNum = 0;
        this->startLeaf.fileId = 0;
        this->startLeaf.offset = LEAF_GROUP_HEAD;
        allocatorCatalogOut.write((char*)&this->maxFileId, sizeof(this->maxFileId));
        allocatorCatalogOut.write((char*)&this->freeNum, sizeof(this->freeNum));
        allocatorCatalogOut.write((char*)&(startLeaf), sizeof(PPointer));
        allocatorCatalogOut.close();
        freeListFileOut.close();
    }
    this->initFilePmemAddr();
}

PAllocator::~PAllocator() {
     // TODO
    persistCatalog();
    this->pAllocator = NULL;
}

// memory map all leaves to pmem address, storing them in the fId2PmAddr
void PAllocator::initFilePmemAddr() { //ok
    // TODO
    char *pmemaddr;
    size_t mapped_len;
    int is_pmem;
    for (uint64_t i = 1; i < this->maxFileId; ++i) {
        // initial the pmemaddr
        pmemaddr = (char*)pmem_map_file((DATA_DIR + to_string(i)).c_str(), LEAF_GROUP_HEAD + LEAF_GROUP_AMOUNT * calLeafSize(),
                    PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem);
        // put the pmemaddr into the map
        this->fId2PmAddr.insert(pair<uint64_t, char*>(i, pmemaddr));
        // peresist the pmemaddr
        pmem_persist(pmemaddr, mapped_len);
    }
}

// get the pmem address of the target PPointer from the map fId2PmAddr
char* PAllocator::getLeafPmemAddr(PPointer p) {
    // TODO
    // find the fileId in the map and return the pmemaddr
    map<uint64_t, char*>::iterator it = this->fId2PmAddr.find(p.fileId);
    if (it != this->fId2PmAddr.end()) {
        return it->second + p.offset;
    }
    return NULL;
}

// get and use a leaf for the fptree leaf allocation
// return 
bool PAllocator::getLeaf(PPointer &p, char* &pmem_addr) {
    // TODO
    // if there is no any free leaf, it should create a new leafgroup first
    if (this->freeNum == 0) {
        if (!this->newLeafGroup()) return false;
    }

    // get and use a leaf from the freelist
    p.offset = this->freeList[this->freeNum - 1].offset;
    p.fileId = this->freeList[this->freeNum - 1].fileId;
    pmem_addr = getLeafPmemAddr(p);
    if (pmem_addr == NULL) return false;
    this->freeList.pop_back();

    // and then change the information in the map
    uint64_t *usedNum = (uint64_t*)this->fId2PmAddr[p.fileId];
    *usedNum = *usedNum + 1;
    this->fId2PmAddr[p.fileId] = (char*)usedNum;
    this->fId2PmAddr[p.fileId] += ((p.offset - LEAF_GROUP_HEAD) / calLeafSize() + sizeof(uint64_t));
    *this->fId2PmAddr[p.fileId] = 1;
    this->fId2PmAddr[p.fileId] -= ((p.offset - LEAF_GROUP_HEAD) / calLeafSize() + sizeof(uint64_t));
    pmem_persist(this->fId2PmAddr[p.fileId], LEAF_GROUP_HEAD + LEAF_GROUP_AMOUNT * calLeafSize());
    --this->freeNum;
    return true;
}

bool PAllocator::ifLeafUsed(PPointer p) {
    // TODO
    if (!this->ifLeafExist(p)) return false;
    if (!this->ifLeafFree(p)) return true;
    return false;
}

bool PAllocator::ifLeafFree(PPointer p) {
    // TODO
    for (uint64_t i = 0; i < this->freeList.size(); i++)
        if (p == this->freeList[i])
            return true;
    return false;
}

// judge whether the leaf with specific PPointer exists. 
bool PAllocator::ifLeafExist(PPointer p) {
    // TODO
    if (p.fileId >= this->maxFileId) return false;
    if ((p.offset < LEAF_GROUP_HEAD) || ((p.offset - LEAF_GROUP_HEAD) % calLeafSize() != 0)) return false;
    return true;
}

// free and reuse a leaf
bool PAllocator::freeLeaf(PPointer p) {
    // TODO
    if (this->ifLeafUsed(p)) {
        // set the bitmap to zero
        this->fId2PmAddr[p.fileId][(p.offset - LEAF_GROUP_HEAD) / calLeafSize() + sizeof(uint64_t)] = 0;
        // usedNum -= 1
        ((uint64_t*)this->fId2PmAddr[p.fileId])[0] -= 1;
        // update the freeList
        this->freeNum++;
        this->freeList.push_back(p);
        return true;
    }
    return false;
}

bool PAllocator::persistCatalog() {
    // TODO
    // write back the information to the files
    string allocatorCatalogPath = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
    string freeListPath         = DATA_DIR + P_ALLOCATOR_FREE_LIST;
    ofstream allocatorCatalog(allocatorCatalogPath, ios::in|ios::binary);
    ofstream freeListFile(freeListPath, ios::in|ios::binary);
    if (!(allocatorCatalog.is_open() && freeListFile.is_open())) return false;
    allocatorCatalog.write((char*)&this->maxFileId, sizeof(this->maxFileId));
    allocatorCatalog.write((char*)&this->freeNum, sizeof(this->freeNum));
    allocatorCatalog.write((char*)&(this->startLeaf.fileId), sizeof(this->startLeaf.fileId));
    allocatorCatalog.write((char*)&(this->startLeaf.offset), sizeof(this->startLeaf.offset));
    for (uint64_t i = 0; i < this->freeNum; ++i) {
        freeListFile.write((char*)&(this->freeList[i].fileId), sizeof(this->freeList[i].fileId));
        freeListFile.write((char*)&(this->freeList[i].offset), sizeof(this->freeList[i].offset));
    }
    allocatorCatalog.close();
    freeListFile.close();
    return true;
}

/*
  Leaf group structure: (uncompressed)
  | usedNum(8b) | bitmap(n * byte) | leaf1 |...| leafn |
*/
// create a new leafgroup, one file per leafgroup
bool PAllocator::newLeafGroup() {
    // TODO
    ofstream newLeafGroupFile(DATA_DIR + to_string(this->maxFileId), ios::out|ios::binary);
    if (newLeafGroupFile.is_open()) {
        // initial the usedNum
        uint64_t usedNum = 0;
        
        // initial the bitmap
        char bitmap[LEAF_GROUP_AMOUNT];
        for (int i = 0; i < LEAF_GROUP_AMOUNT; ++i) {
            bitmap[i] = '0';
        }

        // initial the leaves
        char leaf[LEAF_GROUP_AMOUNT * calLeafSize()];
        memset(leaf, 0, sizeof(leaf));

        // write the new leafgroup to the file
        newLeafGroupFile.write((char*)&(usedNum), sizeof(usedNum));
        newLeafGroupFile.write((char*)bitmap, sizeof(bitmap));
        newLeafGroupFile.write((char*)&leaf, sizeof(leaf));

        // map the pmemaddr with the fileId
        char *pmemaddr;
        int is_pmem;
        size_t mapped_len;
        pmemaddr = (char *)pmem_map_file((DATA_DIR + to_string(this->maxFileId)).c_str(), LEAF_GROUP_HEAD + LEAF_GROUP_AMOUNT * calLeafSize(), PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem);
        
        this->fId2PmAddr[this->maxFileId] = pmemaddr;
        this->freeNum += LEAF_GROUP_AMOUNT;
        PPointer temp;
        temp.fileId = this->maxFileId;

        // initial the freeList of the new leafgroup
        for (int i = 0; i < LEAF_GROUP_AMOUNT; ++i) {
            temp.offset = LEAF_GROUP_HEAD + i * calLeafSize();
            freeList.push_back(temp);
        }
        if(this->maxFileId == 1) {
            this->startLeaf.fileId = 1;
            this->startLeaf.offset = LEAF_GROUP_HEAD + (LEAF_GROUP_AMOUNT - 1) * calLeafSize();
        }
        this->maxFileId++;
        newLeafGroupFile.close();
        return persistCatalog();
    }
    else return persistCatalog();
}
