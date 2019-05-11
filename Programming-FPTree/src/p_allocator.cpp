#include"utility/p_allocator.h"
#include<iostream>
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
PAllocator::PAllocator() { //ok
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
        this->freeList.resize(freeNum);
        for(int i = 0; i < this->freeNum; i++) {
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
        allocatorCatalogOut.write((char*)&this->maxFileId, sizeof(this->maxFileId));
        allocatorCatalogOut.write((char*)&this->freeNum, sizeof(this->freeNum));
        allocatorCatalogOut.close();
        freeListFileOut.close();
    }
    this->initFilePmemAddr();
}

PAllocator::~PAllocator() {
     // TODO
    string allocatorCatalogPath = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
    string freeListPath         = DATA_DIR + P_ALLOCATOR_FREE_LIST;
    ofstream allocatorCatalog(allocatorCatalogPath, ios::in|ios::binary);
    ofstream freeListFile(freeListPath, ios::in|ios::binary);
    allocatorCatalog.write((char*)&this->maxFileId, sizeof(this->maxFileId));
    allocatorCatalog.write((char*)&this->freeNum, sizeof(this->freeNum));
    allocatorCatalog.write((char*)&(this->startLeaf.fileId), sizeof(this->startLeaf.fileId));
    allocatorCatalog.write((char*)&(this->startLeaf.offset), sizeof(this->startLeaf.offset));
    for (int i = 0; i < this->freeNum; ++i) {
        freeListFile.write((char*)&(this->freeList[i].fileId), sizeof(this->freeList[i].fileId));
        freeListFile.write((char*)&(this->freeList[i].offset), sizeof(this->freeList[i].offset));
    }
    allocatorCatalog.close();
    freeListFile.close();
    this->pAllocator = NULL;
}

// memory map all leaves to pmem address, storing them in the fId2PmAddr
void PAllocator::initFilePmemAddr() { //ok
    // TODO
    char *pmemaddr;
    size_t mapped_len;
    int is_pmem;
    for (int i = 1; i < this->maxFileId; ++i) {
        pmemaddr = (char*)pmem_map_file((DATA_DIR + to_string(i)).c_str(), LEAF_GROUP_HEAD + LEAF_GROUP_AMOUNT * calLeafSize(),
        			PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem);
        this->fId2PmAddr.insert(pair<uint64_t, char*>(i, pmemaddr));
        pmem_persist(pmemaddr, mapped_len);
    }
}

// get the pmem address of the target PPointer from the map fId2PmAddr
char* PAllocator::getLeafPmemAddr(PPointer p) { //ok
    // TODO
    map<uint64_t, char*>::iterator it = this->fId2PmAddr.find(p.fileId);
    if (it != this->fId2PmAddr.end()) {
    	return it->second + p.offset;
    }
    return NULL;
}

// get and use a leaf for the fptree leaf allocation
// return 
bool PAllocator::getLeaf(PPointer &p, char* &pmem_addr) { //ok
    // TODO
    if (freeNum == 0) newLeafGroup();
    if (freeNum > 0) {
        p.offset = this->freeList[freeNum - 1].offset;
        p.fileId = this->freeList[freeNum - 1].fileId;
        pmem_addr = this->fId2PmAddr[p.fileId];
        pmem_addr[(p.offset - LEAF_GROUP_HEAD) / calLeafSize() + sizeof(uint64_t)] = 1;
        ((uint64_t*)pmem_addr)[0] += 1;
        this->freeNum--;
        this->freeList.pop_back();
        return true;
    }
    return false;
}

bool PAllocator::ifLeafUsed(PPointer p) { //ok
    // TODO
    if (!ifLeafExist(p)) return false;
    if (!ifLeafFree(p)) return true;
    return false;
}

bool PAllocator::ifLeafFree(PPointer p) { //ok
    // TODO
    for (int i = 0; i < this->freeList.size(); i++)
        if (p == this->freeList[i])
            return true;
    return false;
}

// judge whether the leaf with specific PPointer exists. 
bool PAllocator::ifLeafExist(PPointer p) { //ok
    // TODO
    if ((p.fileId < 0) || (p.fileId >= this->maxFileId)) return false;
    if ((p.offset < 0) || (p.offset < LEAF_GROUP_HEAD) || ((p.offset - LEAF_GROUP_HEAD) % calLeafSize() != 0)) return false;
    return true;
}

// free and reuse a leaf
bool PAllocator::freeLeaf(PPointer p) { //ok
    // TODO
    if (ifLeafUsed(p)) {
        this->fId2PmAddr[p.fileId][(p.offset - LEAF_GROUP_HEAD) / calLeafSize() + sizeof(uint64_t)] = 0;
        ((uint64_t*)this->fId2PmAddr[p.fileId])[0] -= 1;
        this->freeNum++;
        this->freeList.push_back(p);
        return true;
    }
    return false;
}

bool PAllocator::persistCatalog() {
    // TODO
    map<uint64_t, char*>::iterator it = this->fId2PmAddr.begin();
    while (it != this->fId2PmAddr.end()) {
        for (int i = 0; i < this->freeList.size(); ++i) {
            if (this->freeList[i].fileId == it->first)
                break;
        }
        pmem_persist(it->second, LEAF_GROUP_HEAD + LEAF_GROUP_AMOUNT * calLeafSize());
        ++it;
    }
    return false;
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
        newLeafGroupFile.close();
        char *pmemaddr;
        int is_pmem;
        size_t mapped_len;
        pmemaddr = (char *)pmem_map_file((DATA_DIR + to_string(this->maxFileId)).c_str(), LEAF_GROUP_HEAD + LEAF_GROUP_AMOUNT * calLeafSize(), PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem);
        this->fId2PmAddr[maxFileId] = pmemaddr;
        this->freeNum += LEAF_GROUP_AMOUNT;
        PPointer temp;
        temp.fileId = this->maxFileId;
        for (int i = 0; i < LEAF_GROUP_AMOUNT; ++i) {
            temp.offset = LEAF_GROUP_HEAD + i * calLeafSize();
            freeList.push_back(temp);
        }
        this->maxFileId++;
        return true;
    }
    return false;
}
