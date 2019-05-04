//#include "utility.h"
/*#include "p_allocator.h"
#include <iostream>
#include <utility>
#include <string>
using namespace std;*/

#include"utility/p_allocator.h"
#include<iostream>
using namespace std;

// the file that store the information of allocator
const string P_ALLOCATOR_CATALOG_NAME = "p_allocator_catalog";
// a list storing the free leaves
const string P_ALLOCATOR_FREE_LIST    = "free_list";

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
        string buf_catalog;
        getline(allocatorCatalog, buf_catalog);
        // allocatorCatalog.read(buf_catalog, sizeof(char) * 256);
        int i;
        this->maxFileId = 0;
        for (i = 0; i < 8; ++i) {
            this->maxFileId = this->maxFileId * 10 + buf_catalog[i] - '0';
        }
        this->freeNum = 0;
        for (; i < 16; ++i) {
            this->freeNum = this->freeNum * 10 + buf_catalog[i] - '0';
        }
        this->startLeaf.fileId = 0;
        for (; i < 24; ++i) {
            this->startLeaf.fileId = this->startLeaf.fileId * 10 + buf_catalog[i] - '0';
        }
        this->startLeaf.offset = 0;
        for (; i < 32; ++i) {
            this->startLeaf.offset = this->startLeaf.offset * 10 + buf_catalog[i] - '0';
        }
        // char buf_freelist[freeNum * 128];
        string buf_freelist;
        getline(freeListFile, buf_freelist);
        // freeListFile.read(buf_freelist, sizeof(char) * 256);
        int j;
        for (int i = 0; i < freeNum; ++i) {
            int temp_fid = 0;
            for (j = i * 2; j < i * 2 + 1; ++j) {
                temp_fid = temp_fid * 10 + buf_freelist[j] - '0';
            }
            int temp_offset = 0;
            for (; j < (i + 1) * 2; ++j) {
                temp_offset = temp_offset * 10 + buf_freelist[j] - '0';
            }
            PPointer temp;
            temp.fileId = temp_fid;
            temp.offset = temp_offset;
            this->freeList.push_back(temp);
        }
    } else {
        // not exist, create catalog and free_list file, then open.
        // TODO
        ofstream allocatorCatalog(allocatorCatalogPath, ios::out);
        ofstream freeListFile(freeListPath, ios::out);
        this->maxFileId = 1;
        this->freeNum =0;
        this->startLeaf.fileId = 0;
        this->startLeaf.offset = 0;
    }
    this->initFilePmemAddr();

}

PAllocator::~PAllocator() {
    // TODO
    string allocatorCatalogPath = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
	string freeListPath         = DATA_DIR + P_ALLOCATOR_FREE_LIST;
	ofstream allocatorCatalog(allocatorCatalogPath, ios::out|ios::binary);
	ofstream freeListFile(freeListPath, ios::out|ios::binary);
	uint64_t num1 = this->maxFileId;
	uint64_t num2 = this->freeNum;
	uint64_t num3 = this->startLeaf.fileId;
	uint64_t num4 = this->startLeaf.offset;
	char ch[256];
	for(int i = 63; i >= 0;) {
	    ch[i --] = num1 % 10 + '0';
	    num1 /= 10;
	}
	for(int i = 127; i >= 64;) {
	    ch[i --] = num2 % 10 + '0';
	    num2 /= 10;
	}
	for(int i = 191; i >= 128;) {
	    ch[i --] = num3 % 10+'0';
	    num3 /= 10;
	}
	for(int i = 255; i >= 192;) {
	    ch[i --] = num4 % 10 + '0';
	    num4 /= 10;
	}
	allocatorCatalog.write(ch, sizeof(ch));
	char s[freeNum * 128];
	uint64_t fileId;
	uint64_t offset;
	this->startLeaf.fileId = 0;
	this->startLeaf.offset = 0;
	for(int i = 0; i < freeList.size(); i ++) {
	    for(int j = 63; j >= 0; ) {
	        s[j --] = freeList[i].fileId % 10 + '0';
	        freeList[i].fileId /= 10;
	    }
	    for(int j = 127; j >= 64; ) {
	        s[j --]=freeList[i].offset % 10+'0';
	        freeList[i].offset /= 10;
	    }
	    
	}
	freeListFile.write(s, sizeof(s));
	allocatorCatalog.close();
	freeListFile.close();
}

// memory map all leaves to pmem address, storing them in the fId2PmAddr
void PAllocator::initFilePmemAddr() { //ok
    // TODO
    char *pmemaddr;
    size_t mapped_len;
    int is_pmem;
    for (int i = 1; i < this->maxFileId; ++i) {
        pmemaddr = (char*)pmem_map_file((DATA_DIR + '/' + to_string(i)).c_str(), LEAF_GROUP_HEAD + LEAF_GROUP_AMOUNT * calLeafSize(),
        			PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem);
        this->fId2PmAddr.insert(pair<uint64_t, char*>(freeList[i].fileId, pmemaddr));
    }
}

// get the pmem address of the target PPointer from the map fId2PmAddr
char* PAllocator::getLeafPmemAddr(PPointer p) { //ok
    // TODO
    map<uint64_t, char*>::iterator it = this->fId2PmAddr.begin();
    while(it != this->fId2PmAddr.end()) {
    	if (it->first == p.fileId)
    		break;
    	++it;
    }
    if (it != this->fId2PmAddr.end()) {
    	return it->second + p.offset;
    }
    return NULL;
}

// get and use a leaf for the fptree leaf allocation
// return 
bool PAllocator::getLeaf(PPointer &p, char* &pmem_addr) {
    // TODO
    vector<PPointer>::iterator it = this->freeList.begin();
    bool find = false;
    while (it != this->freeList.end()) {
        if (it->fileId == p.fileId && it->offset == p.offset) {
            find = true;
            break;
        }
        ++it;
    }
    return find;
    /*map<uint64_t, char*>::iterator it = fId2PmAddr.begin();
    while (it != fId2PmAddr.end()) {
        if (it->first == p.fileId) {
            &pmem_addr = it->second + p.offset;
            return true;
        }
        ++it;
    }
    return true;*/
    /*if (this->freeNum == 0) newLeafGroup();
    vector<PPointer>::iterator it = this->freeList.begin();
    bool find = false;
    while (it != this->freeList.end()) {
    	if (it->fileId == p.fileId && it->offset == p.offset) {
    		find = true;
    		break;
    	}
    	++it;
    }
    if (find) {
    	this->freeList.erase(it);
        --freeNum;
        map<uint64_t, char*>::iterator ite = fId2PmAddr.begin();
        while (ite != fId2PmAddr.end()) {
            if (ite->first == p.fileId) {
                pmem_addr = ite->second + p.offset;
            }
            *(ite->second + 8 + (p.offset - LEAF_GROUP_HEAD) / calLeafSize()) = 1;
            ++ite;
            return true;
        }
    }
    return false;*/
}

bool PAllocator::ifLeafUsed(PPointer p) { //ok
    // TODO
    map<uint64_t, char*>::iterator it = fId2PmAddr.begin();
    while (it != fId2PmAddr.end()) {
    	if (it->first == p.fileId) break;
    	++it;
    }
    char c = *(it->second + 8 + (p.offset - LEAF_GROUP_HEAD) / calLeafSize());
    return c == '1';
}

bool PAllocator::ifLeafFree(PPointer p) { //ok
    // TODO
    bool find = false;
    for (int i = 0; i < freeList.size(); ++i) {
    	if (freeList[i] == p) {
    		find = true;
    		break;
    	}
    }
    if (find) return true;
    return false;
}

// judge whether the leaf with specific PPointer exists. 
bool PAllocator::ifLeafExist(PPointer p) {
    // TODO
    if (p.fileId >= this->maxFileId) return false;
    if (p.offset < 0) return false;
    return true;
}

// free and reuse a leaf
bool PAllocator::freeLeaf(PPointer p) {
    // TODO
    map<uint64_t, char*>::iterator it = this->fId2PmAddr.begin();
    while (it != this->fId2PmAddr.end()) {
    	if (it->first == p.fileId)
    		break;
    	++it;
    }
    *(it->second + 8 + (p.offset - LEAF_GROUP_HEAD) / calLeafSize()) = '0';
    int temp = 0;
    for (int i = 0; i < 8; ++i) {
    	temp = temp * 10 + it->second[i] - '0';
    }
    --temp;
    int i = 7;
    for (i = 7; i >= 0; --i) {
    	if (temp == 0) break;
    	it->second[i] = temp % 10 + '0';
    	temp /= 10;
    }
    for (; i >= 0; --i) {
    	it->second[i] = '0';
    }
    this->freeList.push_back(p);
    return true;
}

bool PAllocator::persistCatalog() { //ok
    // TODO
    map<uint64_t, char*>::iterator it = this->fId2PmAddr.begin();
    while (it != this->fId2PmAddr.end()) {
        for (int i = 0; i < this->freeList.size(); ++i) {
            if (this->freeList[i].fileId == it->first)
                break;
        }
        pmem_persist(it->second, LEAF_GROUP_HEAD + LEAF_GROUP_AMOUNT * calLeafSize());
    }
    return true;
}

/*
  Leaf group structure: (uncompressed)
  | usedNum(8b) | bitmap(n * byte) | leaf1 |...| leafn |
*/
// create a new leafgroup, one file per leafgroup
bool PAllocator::newLeafGroup() { //ok
    // TODO
    map<uint64_t, char*>::iterator it = fId2PmAddr.begin();
    while (it != fId2PmAddr.end()) {
    	for (int i = 0; i < LEAF_GROUP_HEAD; ++i) {
    		it->second[i] = '0';
    	}
    	++it;
    }
    return true;
}
