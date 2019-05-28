#include "fptree/fptree.h"
#include <leveldb/db.h>
#include <string>
#include <fstream>
#include <assert.h>
#define KEY_LEN 8
#define VALUE_LEN 8
using namespace std;

const string workload = "../../workloads/"; // TODO: the workload folder filepath

const string load = workload + "1w-rw-50-50-load.txt"; // TODO: the workload_load filename
const string run  = workload + "1w-rw-50-50-run.txt"; // TODO: the workload_run filename
const string filePath = "";
const int READ_WRITE_NUM = 10000; // TODO: amount of operations

int main()
{        
    FPTree fptree(1028);
    uint64_t inserted = 0, queried = 0, t = 0;
    uint64_t* key = new uint64_t[2200000];
    bool* ifInsert = new bool[2200000];
	ifstream ycsb_load, ycsb_run;
	char buf[6];
    struct timespec start, finish;
    double single_time;

    printf("===================FPtreeDB===================\n");
    printf("Load phase begins \n");

    // TODO: read the ycsb_load
    ycsb_load.open(load);
    
    for(int i = 0; i < READ_WRITE_NUM; i++){
	ycsb_load >> buf;
	if(buf[0] == 'I' && buf[1] == 'N')
	{
	    ycsb_load >> key[i];
	    ifInsert[i] = true;
	    inserted++;
	}
    }
    clock_gettime(CLOCK_MONOTONIC, &start);
    // TODO: load the workload in the fptree
    for(int i = 0; i < READ_WRITE_NUM; i++){
	if(ifInsert[i])
	{
	    fptree.insert(key[i],key[i]);
	}
    }

    clock_gettime(CLOCK_MONOTONIC, &finish);
    ycsb_load.close();
	single_time = (finish.tv_sec - start.tv_sec) * 1000000000.0 + (finish.tv_nsec - start.tv_nsec);
    printf("Load phase finishes: %d items are inserted \n", inserted);
    printf("Load phase used time: %fs\n", single_time / 1000000000.0);
    printf("Load phase single insert time: %fns\n", single_time / inserted);

	printf("Run phase begins\n");

    int operation_num = 1000;
    inserted = 0;
    bool* ifRead = new bool[2200000];		
    // TODO: read the ycsb_run
    ycsb_run.open(run);
    
    for(int i = 0; i < READ_WRITE_NUM; i++){
	ycsb_run >> buf;
	if(buf[0] == 'R' && buf[1] == 'E')
	{
	    ycsb_run >> key[i];
	    ifRead[i] = true;
	    queried++;
	}
	else
	{
	    ycsb_run >> key[i];
	    ifRead[i] = false;
	    inserted++;
	}
    }
    clock_gettime(CLOCK_MONOTONIC, &start);

    // TODO: operate the fptree
    for(int i = 0; i < operation_num; i++){
	string s = to_string(key[i]);
	if(ifRead[i]){
	    fptree.find(key[i]);
	}
	else
	{
	    fptree.insert(key[i],key[i]);
	}
    }
	clock_gettime(CLOCK_MONOTONIC, &finish);
    ycsb_run.close();
	single_time = (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Run phase finishes: %d/%d items are inserted/searched\n", inserted, operation_num - inserted);
    printf("Run phase throughput: %f operations per second \n", READ_WRITE_NUM/single_time);	
    
    // LevelDB
    printf("===================LevelDB====================\n");
    const string filePath = ""; // data storing folder(NVM)

    memset(key, 0, 2200000);
    memset(ifInsert, 0, 2200000);
    memset(ifRead, 0, 2200000);

    leveldb::DB* db;
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, "/tmp/testdb", &db);
    assert(status.ok());
    leveldb::WriteOptions write_options;
    // TODO: open and initial the levelDB
    inserted = 0;
    printf("Load phase begins \n");
    // TODO: read the ycsb_load and store
    ycsb_load.open(load);
    
    for(int i = 0; i < READ_WRITE_NUM; i++){
	ycsb_load >> buf;
	if(buf[0] == 'I' && buf[1] == 'N')
	{
	    ycsb_load >> key[i];
	    ifInsert[i] = true;
	    inserted++;
	}
    }
    clock_gettime(CLOCK_MONOTONIC, &start);

    // TODO: load the workload in LevelDB
    for(int i = 0; i < READ_WRITE_NUM; i++){
	string s;
	s = to_string(key[i]);
	status = db->Put(leveldb::WriteOptions(), s, s);
	assert(status.ok());
    }

    clock_gettime(CLOCK_MONOTONIC, &finish);
    ycsb_load.close();
	single_time = (finish.tv_sec - start.tv_sec) * 1000000000.0 + (finish.tv_nsec - start.tv_nsec);

    printf("Load phase finishes: %d items are inserted \n", inserted);
    printf("Load phase used time: %fs\n", single_time / 1000000000.0);
    printf("Load phase single insert time: %fns\n", single_time / inserted);

    inserted = 0;		
    // TODO:read the ycsb_run and store
    ycsb_run.open(run);
    
    for(int i = 0; i < operation_num; i++){
	ycsb_run >> buf;
	if(buf[0] == 'R' && buf[1] == 'E')
	{
	    ycsb_run >> key[i];
	    ifRead[i] = true;
	    queried++;
	}
	else
	{
	    ycsb_run >> key[i];
	    ifRead[i] = false;
	    inserted++;
	}
    }
    clock_gettime(CLOCK_MONOTONIC, &start);

    // TODO: operate the levelDB	
    for(int i = 0; i < operation_num; i++){
	string s = to_string(key[i]);
	if(ifRead[i]){
	    status = db->Get(leveldb::ReadOptions(), s, &s);
	    assert(status.ok());
	}
	else
	{
	    status = db->Put(leveldb::WriteOptions(), s, s);
	    assert(status.ok());
	}
    }
    clock_gettime(CLOCK_MONOTONIC, &finish);
    ycsb_run.close();
	single_time = (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Run phase finishes: %d/%d items are inserted/searched\n",  inserted,operation_num - inserted);
    printf("Run phase throughput: %f operations per second \n", READ_WRITE_NUM/single_time);	
    return 0;
}
