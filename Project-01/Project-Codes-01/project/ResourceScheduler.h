#pragma once
#include<vector>
#include<algorithm>
#include<iostream>
#include<iomanip>
#include<numeric>
#include<ctime>
#include<set>
#include<unordered_set>
#include<unordered_map>
#include <sstream>
#include <assert.h>
#include<cmath>

#define _for(i,a,b) for( int i=(a); i<(b); ++i)
#define __for(i,a,b) for( int i=(a); i<=(b); ++i)
#define mfor(i,a,b) for(int i=(a);i>(b);--i)
#define mmfor(i,a,b) for(int i=(a);i>=(b);--i)
#define _CRT_SECURE_NO_WARNINGS

using namespace std;

class ResourceScheduler {
public:
	int taskType; // 1 or 2
	int caseID;   // 1
	int numJob; // No. 0 ~ numJob-1
	int numHost;// No. 0 ~ numHost-1
	double St;  // Speed of Transimision 
	double alpha;   // g(e)=1-alpha(e-1) alpha>0, e is the number of cores allocated to a single job
	int coreSum; //the number of all cores

	vector<int> hostCore;              // The number of cores for each host
	vector<int> jobBlock;              // The number of blocks for each job
	vector<double> Sc;                    // Speed of calculation for each job
	vector<vector<double>> dataSize;      // Job-> block number-> block size
	vector<vector<int>> location;         // Job-> block number-> block location (host number)


	vector<double> jobFinishTime;                 // The finish time of each job 
	vector<vector<tuple<int, int, int>>> runLoc; // Block perspective: job number->block number->(hostID, coreID,rank), rank=1 means that block is the first task running on that core of that host

	vector<vector<vector<tuple<int, int, double, double>>>> hostCoreTask; // Core perspective: host->core->task-> <job,block,startRunningTime,endRunningTime>
	vector<vector<double>> hostCoreFinishTime; // host->core->finishTime
	//unordered_map<int, unordered_map<int, tuple<int, int, double>>> transferMap; // Transmission situation: (job, block) --> (initHost, destinationHost, propagation time)

	vector<int>jobCore;                       // The number of cores allocated to each job.
	vector<int>jobOrder;                              //jobOrder[1] stores the first job
	vector<vector<tuple<int, int, vector<int> > > > total_sol;//the information of the number of cores are stored in jobCore
													  //job-> core ->(host_id, core_id, vector of block_id)
	vector<vector<int> >blockLocation;        //Job-> block number-> block location (host number)

	//modify the solution temporarily
	vector<int>temp_jobCore;
	vector<int>temp_jobOrder;
	vector<vector<tuple<int, int, vector<int> > > >  temp_total_sol;
	vector<vector<int> >temp_blockLocation;

	ResourceScheduler(int, int);
	void schedule();
	void initialize_sol();
	void greedy_allocation(int ,int, vector<tuple<int, int, double>>);    // (job_id,  num of cores allocated to the job, coreLength)
	void move_temp();   //change temp_total_sol
	void move_temp2();   //consider randomly changing the block location
	void accept_move();  //total_sol=temp_total_sol
	void write_back();    //write the solution back to variables that are necessary in output 
	void outputSolutionFromBlock();
	void outputSolutionFromCore();
	void outputRatio();
	void validFromBlock();
	void validFromCore();
	void visualization(); // An optional function.
	double finishTime();    //computing the finish time using total_sol , jobCore, jobOrder 
	double temp_finishTime(); //computing the finish time using temp_total_sol , temp_jobCore, temp_jobOrder 
	double g(int);
};
