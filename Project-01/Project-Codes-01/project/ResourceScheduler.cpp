#define _CRT_SECURE_NO_WARNINGS

#include "ResourceScheduler.h"

ResourceScheduler::ResourceScheduler(int tasktype, int caseID) {
	taskType = tasktype;
	string filePath = "../project/input/task" + to_string(taskType) + "_case" + to_string(caseID) + ".txt";
	freopen(filePath.c_str(), "r", stdin);
	cin >> numJob >> numHost >> alpha;
	if (taskType == 2)
		cin >> St;
	
	hostCore.resize(numHost);
	coreSum = 0;
	for (int i = 0; i < numHost; i++) {
		cin >> hostCore[i];     //主机核数
		coreSum += hostCore[i];
	}

	jobBlock.resize(numJob);
	for (int i = 0; i < numJob; i++)
		cin >> jobBlock[i];     //任务块数

	Sc.resize(numJob);
	for (int i = 0; i < numJob; i++)
		cin >> Sc[i];           //传输速度

	dataSize.resize(numJob);
	for (int i = 0; i < numJob; i++) {
		dataSize[i].resize(jobBlock[i]);
		for (int j = 0; j < jobBlock[i]; j++)
			cin >> dataSize[i][j];      //任务i的第j块的大小
	}

	location.resize(numJob);
	blockLocation.resize(numJob);
	temp_blockLocation.resize(numJob);
	for (int i = 0; i < numJob; i++) {
		location[i].resize(jobBlock[i]);
		blockLocation[i].resize(jobBlock[i]);
		temp_blockLocation[i].resize(jobBlock[i]);
		for (int j = 0; j < jobBlock[i]; j++) {
			cin >> location[i][j];       //每个块的初始位置
			blockLocation[i][j] = location[i][j];
			temp_blockLocation[i][j] = location[i][j];
		}
	}


	jobFinishTime.resize(numJob, 0);

	runLoc.resize(numJob);
	for (int i = 0; i < numJob; i++)
		runLoc[i].resize(jobBlock[i]);

	hostCoreTask.resize(numHost);
	for (int i = 0; i < numHost; i++)
		hostCoreTask[i].resize(hostCore[i]);

	hostCoreFinishTime.resize(numHost);
	for (int i = 0; i < numHost; i++)
		hostCoreFinishTime[i].resize(hostCore[i], 0);
    
	jobCore.resize(numJob);
    jobOrder.resize(numJob+1);
	total_sol.resize(numJob);
	

	temp_jobCore.resize(numJob);
	temp_jobOrder.resize(numJob+1);
	temp_total_sol.resize(numJob);

}


void ResourceScheduler::schedule() {
	initialize_sol();
	double finish = 0;
	double temp_finish = 0;

	//模拟退火过程 
	double t = 1000;  //初始温度
	double stop = 0.0001;   //停火温度
	double alpha = 0.999;     //退火速率
	double p = 1;            //概率
	double dt = 0;
	int iter_max = 1000000;
	while (iter_max--) {
		move_temp();
		finish = finishTime();
		temp_finish = temp_finishTime();
		dt = temp_finish - finish;
		//cout << finish << " " << temp_finish << endl;
		p = rand() % 10000;
		p = p / 10000;
		if (dt<0) {
			accept_move();
		}
		else if(exp(-dt/t)>p) {
			accept_move();
		}
		t = t * alpha;
		if (t < stop)break;
	}

    write_back();

}



void ResourceScheduler::initialize_sol() { 

	for (int i = 0; i < numJob; i++) {
		jobCore[i] = rand() % min(min(10,jobBlock[i]),coreSum)+1;    //选核数量,不能为0
		temp_jobCore[i] = jobCore[i];
		vector<tuple<int, int, double>>coreLength(jobCore[i]);           // core->(host_id, core_id, length) 
		vector<int>coreSet(coreSum, 0);                         //记录已分配的核下标,1表示已分配
		int count = 0;
		total_sol[i].resize(jobCore[i]);     
		temp_total_sol[i].resize(temp_jobCore[i]);
		while(count<jobCore[i]) {         //选核下标
			int c = rand() % (coreSum);
			if (coreSet[c] == 0) {
				coreSet[c] = 1;
				int host_id = 0;
				c++;               //实际选了第几个核
				while (c > hostCore[host_id]) {
					c -= hostCore[host_id];
					host_id++;
				}				
				coreLength[count] = make_tuple(host_id, c-1, 0);
				count++;
			}
		}
        
		
		//按顺序分配
		vector<vector<int> >coreBlock(jobCore[i]);   //core->（block->block_id）
		int index=0;
		for (int j = 0; j <jobBlock[i]; j++) {
			coreBlock[index].push_back(j);
			blockLocation[i][j] = get<0>(coreLength[index]);
			temp_blockLocation[i][j]= get<0>(coreLength[index]);
			index = (index + 1) % jobCore[i];
		}
		
		for (int j = 0; j < jobCore[i]; j++) {
			total_sol[i][j] = make_tuple(get<0>(coreLength[j]), get<1>(coreLength[j]), coreBlock[j]);
			temp_total_sol[i][j] = make_tuple(get<0>(coreLength[j]), get<1>(coreLength[j]), coreBlock[j]);
		}

		jobOrder[i+1] = i ;
		temp_jobOrder[i+1] = i;
		
	 }

}


void ResourceScheduler::move_temp() {
	int i = rand() % 2;
	if (numJob>=2&&i == 0) {                   //换任务顺序
		int c = rand() % (numJob - 1)+1;
		swap(temp_jobOrder[c], temp_jobOrder[c + 1]); // 1<=c<=numJob-1
	}
	
	i = rand() % 2;
	if (i != 0) {                              //换核数量
		int job_id = rand() % numJob;          //随机挑选job
		temp_jobCore[job_id] = rand() % min(min(10, jobBlock[job_id]), coreSum) + 1;      //随机选核
		vector<tuple<int, int, double>>coreLength(temp_jobCore[job_id]);                      // core->(host_id, core_id, length) 
		vector<int>coreSet(coreSum, 0);
		int count = 0;
		temp_total_sol[job_id].resize(temp_jobCore[job_id]);
		while (count < temp_jobCore[job_id]) {         //选核下标
			int c = rand() % (coreSum);
			if (coreSet[c] == 0) {
				coreSet[c] = 1;
				int host_id = 0;
				c++;               //实际选了第几个核
				while (c > hostCore[host_id]) {
					c -= hostCore[host_id];
					host_id++;
				}
				coreLength[count] = make_tuple(host_id, c - 1, 0);
				count++;
			}
		}

		greedy_allocation(job_id, temp_jobCore[job_id], coreLength);
	}
	
}


//贪心过程 假设数据块大小都小于10000000
void ResourceScheduler::greedy_allocation(int i, int number, vector<tuple<int, int, double>>coreLength) {
	vector<tuple<int,int>>jobBlock_sorted(jobBlock[i]);      // block->(block_id , datasize)
	for (int j = 0; j < jobBlock[i]; j++)
		jobBlock_sorted[j] = make_tuple(j, dataSize[i][j]);
	
	int flag = 1;  //从大到小排列
	for (int j = 0; j < jobBlock[i] - 1 && flag==1; j++) {
		flag = 0;
		for (int k = 0; k < jobBlock[i] - 1 - j; k++) {
			if (get<1>(jobBlock_sorted[k]) < get<1>(jobBlock_sorted[k + 1])) {
				swap(jobBlock_sorted[k], jobBlock_sorted[k + 1]);
				flag = 1;
			}
		}
	}

	int min = 10000000;
	int index = 0;

	vector<vector<int> >coreBlock(number);   //core->block->block_id
	for (int j = 0; j < jobBlock[i]; j++) {
		int max_data_index = get<0>(jobBlock_sorted[j]);
		min = 10000000;
		index = 0;

		for (int k = 0; k < number; k++) {
			if (location[i][j] == get<0>(coreLength[k])) {
				if (get<2>(coreLength[k]) + dataSize[i][max_data_index] < min) {
					min = get<2>(coreLength[k]) + dataSize[i][max_data_index];
					index = k;
				}
			}
			else {
				if (get<2>(coreLength[k]) + dataSize[i][max_data_index] * (1 + Sc[i] * g(number) / St) < min) {
					min = get<2>(coreLength[k]) + dataSize[i][max_data_index] * (1 + Sc[i] * g(number) / St);
					index = k;
				}
			}
		}

		if (location[i][j] == get<0>(coreLength[index])) {
			get<2>(coreLength[index]) += dataSize[i][max_data_index];
		}
		else {
			get<2>(coreLength[index]) += dataSize[i][max_data_index] * (1 + Sc[i] * g(number) / St);
		}

		coreBlock[index].push_back(max_data_index);
		temp_blockLocation[i][j] = get<0>(coreLength[index]);         //block_id=j 从属的host
	}


	for (int j = 0; j < number; j++) {
		temp_total_sol[i][j] = make_tuple(get<0>(coreLength[j]), get<1>(coreLength[j]), coreBlock[j]);
	}

}



void ResourceScheduler::move_temp2() {
	int i = rand() % 2;
	if (numJob >= 2 && i != 0) {                   //换任务顺序
		int c = rand() % (numJob - 1) + 1;
		swap(temp_jobOrder[c], temp_jobOrder[c + 1]); // 1<=c<=numJob-1
	}

	
	i = rand() % 4;
	if (i != 0) {                              //换某个核的下标
		int job_id = rand() % numJob;          //随机挑选job
		if (temp_jobCore[job_id] < coreSum) {       //如果没有用到全部的核
			vector<int>coreSet(coreSum, 0);
			int host_id, core_id, c;
			for (int k = 0; k < temp_jobCore[job_id]; k++) {         //换某个核下标
				host_id = get<0>(temp_total_sol[job_id][k]);
				core_id = get<1>(temp_total_sol[job_id][k]);
				c = core_id;
				for (int j = 0; j < host_id; j++)
					c += hostCore[j];
				coreSet[c] = 1;
			}
			c = rand() % coreSum;
			while (coreSet[c] == 1) {          //找没被选到的核
				c = rand() % coreSum;
			}
			c++;                        //实际选了第几个核
			host_id = 0;
			while (c > hostCore[host_id]) {
				c -= hostCore[host_id];
				host_id++;
			}

			int core = rand() % temp_jobCore[job_id];
			get<0>(temp_total_sol[job_id][core]) = host_id;
			get<1>(temp_total_sol[job_id][core]) = c - 1;

			vector<int>block_id = get<2>(temp_total_sol[job_id][core]);
			for (int j = 0; j < block_id.size(); j++) {
				temp_blockLocation[job_id][block_id[j]] = host_id;
			}
		}
	}
	

	i = rand() % 8;
	if (i != 0) {
		int job_id = rand() % numJob;          //随机挑选job
		if (jobBlock[job_id] > 1) {
			int old_core = rand() % temp_jobCore[job_id];
			vector<int>block_id = get<2>(temp_total_sol[job_id][old_core]);
			int block_index = rand() % block_id.size();
			int new_core = rand()%(temp_jobCore[job_id]+1);      //可能会多选一个
			while (new_core == old_core) {
				new_core = rand() % (temp_jobCore[job_id] + 1);
			}

			if (new_core == temp_jobCore[job_id] && temp_jobCore[job_id] <= min(9,coreSum-1)) {     //多选一个
				if (block_id.size() == 1) {         //相当于置换核
					vector<int>coreSet(coreSum, 0);
					int host_id, core_id, c;
					for (int k = 0; k < temp_jobCore[job_id]; k++) {       
						host_id = get<0>(temp_total_sol[job_id][k]);
						core_id = get<1>(temp_total_sol[job_id][k]);
						c = core_id;
						for (int j = 0; j < host_id; j++)
							c += hostCore[j];
						coreSet[c] = 1;
					}
					c = rand() % coreSum;
					while (coreSet[c] == 1) {          //找没被选到的核
						c = rand() % coreSum;
					}
					c++;                        //实际选了第几个核
					host_id = 0;
					while (c > hostCore[host_id]) {
						c -= hostCore[host_id];
						host_id++;
					}

					get<0>(temp_total_sol[job_id][old_core]) = host_id;
					get<1>(temp_total_sol[job_id][old_core]) = c - 1;
					temp_blockLocation[job_id][block_id[block_index]] = host_id;

				}
				else {                     //多选一个核，只放入id_block
					int id_block = block_id[block_index];
					get<2>(temp_total_sol[job_id][old_core]).erase(get<2>(temp_total_sol[job_id][old_core]).begin() + block_index);

					vector<int>coreSet(coreSum, 0);
					int host_id, core_id, c;
					for (int k = 0; k < temp_jobCore[job_id]; k++) {
						host_id = get<0>(temp_total_sol[job_id][k]);
						core_id = get<1>(temp_total_sol[job_id][k]);
						c = core_id;
						for (int j = 0; j < host_id; j++)
							c += hostCore[j];
						coreSet[c] = 1;
					}
					c = rand() % coreSum;
					while (coreSet[c] == 1) {          //找没被选到的核
						c = rand() % coreSum;
					}
					c++;                        //实际选了第几个核
					host_id = 0;
					while (c > hostCore[host_id]) {
						c -= hostCore[host_id];
						host_id++;
					}

					temp_jobCore[job_id]++;
					temp_blockLocation[job_id][id_block] = host_id;
					vector<int>new_block;
					new_block.push_back(id_block);
					temp_total_sol[job_id].push_back( make_tuple(host_id, c - 1,new_block) );
				}
			}
			else if (new_core< temp_jobCore[job_id]) {           //只用置换block
				if (block_id.size() == 1) {                     //要少一个核了
					temp_jobCore[job_id]--;
					int id_block = block_id[block_index];
					get<2>(temp_total_sol[job_id][new_core]).push_back(id_block);

					temp_blockLocation[job_id][id_block] = get<0>(temp_total_sol[job_id][new_core]);
				    temp_total_sol[job_id].erase(temp_total_sol[job_id].begin() + old_core);
				}
				else {
					int id_block = block_id[block_index];
					get<2>(temp_total_sol[job_id][new_core]).push_back(id_block);
					temp_blockLocation[job_id][id_block] = get<0>(temp_total_sol[job_id][new_core]);
					get<2>(temp_total_sol[job_id][old_core]).erase(get<2>(temp_total_sol[job_id][old_core]).begin() + block_index);
				}
			}
			
		}

	}

}



void ResourceScheduler::accept_move() {
	for (int i = 0; i < numJob; i++) {
		jobCore[i] = temp_jobCore[i];
		jobOrder[i] = temp_jobOrder[i];
		total_sol[i].resize(temp_jobCore[i]);
		for (int j = 0; j < temp_jobCore[i]; j++) 
			total_sol[i][j] = make_tuple(get<0>(temp_total_sol[i][j]), get<1>(temp_total_sol[i][j]), get<2>(temp_total_sol[i][j]));
		for (int j = 0; j < jobBlock[i]; j++)
			blockLocation[i][j] = temp_blockLocation[i][j];
	}
	jobOrder[numJob] = temp_jobOrder[numJob];
}



double ResourceScheduler::finishTime() {
	double finish_time = 0;
	vector<vector<double>> tf(numHost);       //host_id->core_id finish time
	for (int i = 0; i < numHost; ++i) {
		tf[i].resize(hostCore[i]);
		for (int j = 0; j < hostCore[i]; ++j)
			tf[i][j] = 0; //init tf
	}

	for (int i = 1; i <= numJob; ++i) // traverse every job following jobOrder[]
	{
		int jobi = jobOrder[i], host_id, core_id;
		double t_i = 0, tpi = 0, tf_jobi;
		vector<double> tp_i(jobCore[jobi]);

		for (int j = 0; j < jobCore[jobi]; ++j)// traverse all jobi-th job's core and calculate the running time on each core
		{
			host_id = get<0>(total_sol[jobi][j]);
			core_id = get<1>(total_sol[jobi][j]);
			vector<int> block_id = get<2>(total_sol[jobi][j]);

			double tp_ji = 0;
			for (int k = 0; k < block_id.size(); ++k)
			{
				if (location[jobi][block_id[k]] == blockLocation[jobi][block_id[k]]) tp_ji += dataSize[jobi][block_id[k]] / (Sc[jobi] * g(jobCore[jobi]));
				else tp_ji += dataSize[jobi][block_id[k]] * ((1 / St) + (1 / (Sc[jobi] * g(jobCore[jobi]))));
			}			
			tp_i[j] = tp_ji;      //calculate tp_j^i and store it for finding maximum then

			if (t_i < tf[host_id][core_id]) t_i = tf[host_id][core_id];      //find the starting time (the maximum one)
		}
		for (int k = 0; k < jobCore[jobi]; ++k)
			if (tpi < tp_i[k]) tpi = tp_i[k];      //find maximum tp_j^i
		tf_jobi = t_i + tpi;

		for (int j = 0; j < jobCore[jobi]; ++j) //refresh every item in tf that relates to jobi-th job, tf = tf_jobi
		{
			host_id = get<0>(total_sol[jobi][j]);
			core_id = get<1>(total_sol[jobi][j]);
			tf[host_id][core_id] = tf_jobi;
		}

		if (finish_time < tf_jobi) finish_time = tf_jobi; //find maximum tf_jobi and refresh finish_time = max(tf_jobi)
	}


	return finish_time;
}



double ResourceScheduler::temp_finishTime() {
	double finish_time = 0;
	vector<vector<double>> tf(numHost);       //host_id->core_id finish time
	for (int i = 0; i < numHost; ++i) {
		tf[i].resize(hostCore[i]);
		for (int j = 0; j < hostCore[i]; ++j)
			tf[i][j] = 0; //init tf
	}

	for (int i = 1; i <= numJob; ++i) // traverse every job following jobOrder[]
	{
		int jobi = temp_jobOrder[i], host_id, core_id;
		double t_i = 0, tpi = 0, tf_jobi;
		vector<double> tp_i(temp_jobCore[jobi]);

		for (int j = 0; j < temp_jobCore[jobi]; ++j)// traverse all jobi-th job's core and calculate the running time on each core
		{
			host_id = get<0>(temp_total_sol[jobi][j]);
			core_id = get<1>(temp_total_sol[jobi][j]);
			vector<int> block_id = get<2>(temp_total_sol[jobi][j]);

			double tp_ji = 0;
			for (int k = 0; k < block_id.size(); ++k)
			{
				if (location[jobi][block_id[k]] == temp_blockLocation[jobi][block_id[k]]) tp_ji += dataSize[jobi][block_id[k]] / (Sc[jobi] * g(temp_jobCore[jobi]));
				else tp_ji += dataSize[jobi][block_id[k]] * ((1 / St) + (1 / (Sc[jobi] * g(temp_jobCore[jobi]))));
			}
			tp_i[j] = tp_ji;      //calculate tp_j^i and store it for finding maximum then

			if (t_i < tf[host_id][core_id]) t_i = tf[host_id][core_id];      //find the starting time (the maximum one)
		}
		for (int k = 0; k < temp_jobCore[jobi]; ++k)
			if (tpi < tp_i[k]) tpi = tp_i[k];      //find maximum tp_j^i
		tf_jobi = t_i + tpi;

		for (int j = 0; j < temp_jobCore[jobi]; ++j) //refresh every item in tf that relates to jobi-th job, tf = tf_jobi
		{
			host_id = get<0>(temp_total_sol[jobi][j]);
			core_id = get<1>(temp_total_sol[jobi][j]);
			tf[host_id][core_id] = tf_jobi;
		}

		if (finish_time < tf_jobi) finish_time = tf_jobi; //find maximum tf_jobi and refresh finish_time = max(tf_jobi)
	}


	return finish_time;
}



void ResourceScheduler::write_back() {
	vector<vector<double>> tf(numHost); //release time
	vector<vector<int>> rank(numHost);
	for (int i = 0; i < numHost; ++i)
	{
		tf[i].resize(hostCore[i]);
		rank[i].resize(hostCore[i]);
		for (int j = 0; j < hostCore[i]; ++j)
		{
			tf[i][j] = 0;
			rank[i][j] = 0;
		}
	} //init

	for (int i = 1; i <= numJob; ++i) // traverse every job following jobOrder[]
	{
		int jobi = jobOrder[i], host_id, core_id;
		double t_i = 0, tpi = 0;
		vector<double> tp_i(jobCore[jobi]);

		for (int j = 0; j < jobCore[jobi]; ++j)// traverse all jobi-th job's core and find the starting time
		{
			host_id = get<0>(total_sol[jobi][j]);
			core_id = get<1>(total_sol[jobi][j]);
			if (t_i < tf[host_id][core_id]) t_i = tf[host_id][core_id];      // the maximum one
		}

		for (int j = 0; j < jobCore[jobi]; ++j)// traverse all jobi-th job's core and calculate the running time on each core
		{
			host_id = get<0>(total_sol[jobi][j]);
			core_id = get<1>(total_sol[jobi][j]);
			vector<int> block_id = get<2>(total_sol[jobi][j]);

			double tp_ji = 0;
			for (int k = 0; k < block_id.size(); ++k)
			{
				if (location[jobi][block_id[k]] == blockLocation[jobi][block_id[k]]) tp_ji += dataSize[jobi][block_id[k]] / (Sc[jobi] * g(jobCore[jobi]));
				else tp_ji += dataSize[jobi][block_id[k]] * ((1 / St) + (1 / (Sc[jobi] * g(jobCore[jobi]))));
				runLoc[jobi][block_id[k]] = make_tuple(host_id, core_id, ++rank[host_id][core_id]);      //write runLoc
			}
			tp_i[j] = tp_ji;      //calculate tp_j^i and store it for finding maximum then
			hostCoreFinishTime[host_id][core_id] = t_i + tp_ji;      //write hostCoreFinishTime (smaller or equal than jobFinishTime)

			double block_begintime = t_i;
			for (int k = 0; k < block_id.size(); ++k)
			{
				double block_runtime;
				if (location[jobi][block_id[k]] == blockLocation[jobi][block_id[k]]) block_runtime = dataSize[jobi][block_id[k]] / (Sc[jobi] * g(jobCore[jobi]));
				else block_runtime = dataSize[jobi][block_id[k]] * ((1 / St) + (1 / (Sc[jobi] * g(jobCore[jobi]))));
				hostCoreTask[host_id][core_id].push_back(make_tuple(jobi, block_id[k], block_begintime, block_begintime + block_runtime));
				block_begintime += block_runtime;
			}
		}
		for (int k = 0; k < jobCore[jobi]; ++k)
			if (tpi < tp_i[k]) tpi = tp_i[k];      //find maximum tp_j^i
		jobFinishTime[jobi] = t_i + tpi;      //write jobFinishTime that all blocks are finished

		for (int j = 0; j < jobCore[jobi]; ++j) //refresh every item in tf that relates to jobi-th job, tf = tf_jobi
		{
			host_id = get<0>(total_sol[jobi][j]);
			core_id = get<1>(total_sol[jobi][j]);
			tf[host_id][core_id] = jobFinishTime[jobi];      //we still need tf to store every small finishtime by host_id and core_id
		}
	}

}


// 以数据块的视角打印解决方案
void ResourceScheduler::outputSolutionFromBlock() {
	cout << "\nTask" << taskType << " Solution (Block Perspective) of Teaching Assistant:\n\n";
	for (int i = 0; i < numJob; i++) {
		double speed = g(jobCore[i]) * Sc[i];
		cout << "Job" << i << " obtains " << jobCore[i] << " cores (speed=" << speed << ") and finishes at time " << jobFinishTime[i] << ": \n";
		for (int j = 0; j < jobBlock[i]; j++) {
			cout << "\tBlock" << j << ": H" << get<0>(runLoc[i][j]) << ", C" << get<1>(runLoc[i][j]) << ", R" << get<2>(runLoc[i][j]) << " (time=" << fixed << setprecision(2) << dataSize[i][j] / speed << ")" << " \n";
		}
		cout << "\n";
	}
}

// 以核的视角打印解决方案
void ResourceScheduler::outputSolutionFromCore() {
	
	cout << "\nTask" << taskType << " Solution (Core Perspective) of Teaching Assistant:\n\n";
	double maxHostTime = 0, totalRunningTime = 0.0;
	for (int i = 0; i < numHost; i++) {
		double hostTime = *max_element(hostCoreFinishTime[i].begin(), hostCoreFinishTime[i].end());       //选最长的核运行时间
		maxHostTime = max(hostTime, maxHostTime);                                                         //最长的主机运行时间
		cout << "Host" << i << " finishes at time " << hostTime << ":\n\n";
		for (int j = 0; j < hostCore[i]; j++) {                                   
			cout << "\tCore" << j << " has " << hostCoreTask[i][j].size() << " tasks and finishes at time " << hostCoreFinishTime[i][j] << ":\n";
			for (int k = 0; k < hostCoreTask[i][j].size(); k++) {
				cout << "\t\tJ" << setw(2) << setfill('0') << get<0>(hostCoreTask[i][j][k]) << ", B" << setw(2) << setfill('0') << get<1>(hostCoreTask[i][j][k]) << ", runTime " << fixed << setprecision(1) << setw(5) << setfill('0') << get<2>(hostCoreTask[i][j][k]) << " to " << fixed << setprecision(1) << setw(5) << setfill('0') << get<3>(hostCoreTask[i][j][k]) << "\n";
				totalRunningTime += get<3>(hostCoreTask[i][j][k]) - get<2>(hostCoreTask[i][j][k]);
			}
			cout << "\n";
		}
		cout << "\n\n";
	}

	cout << "The maximum finish time of hosts: " << maxHostTime << "\n";
	cout << "The total efficacious running time: " << totalRunningTime << "\n";
	cout << "Utilization rate: " << totalRunningTime / accumulate(hostCore.begin(), hostCore.end(), 0.0) / maxHostTime << "\n\n";
}                                              


void ResourceScheduler::outputRatio() {
	for (int i = 0; i < numJob; i++) {
		double max = 0;
		double length = 0;
		double total_length = 0;
		for (int j = 0; j < jobCore[i]; j++) {
			for (int k = 0; k < get<2>(total_sol[i][j]).size(); k++) {
				length += dataSize[i][get<2>(total_sol[i][j])[k]];
			}
			total_length += length;
			if (length > max)max = length;
		}
		cout << "For job" << i << ", the cube length greedy algorithm gives is " << max << ". " << "The theoretical shortest length is " << total_length / jobCore[i] <<". " << endl;
		cout << "The ratio is " << jobCore[i] * max/total_length << ". " << endl;
	}
}




// 从数据块的视角进行验证
void ResourceScheduler::validFromBlock() {

	// 1. 验证jobFinishTime: "jobFinishTime的和" 应该小于 "依次执行每个job, 所有数据块在各自初始所在主机的同一个核上直接运行的时间的和"
	double actualTime = accumulate(jobFinishTime.begin(), jobFinishTime.end(), 0.0);
	double maxRunningTime = 0.0;
	vector<unordered_set<int>> jobInitLocSet(numJob); // 每个job初始分散在哪几个主机上
	vector<int> jobTotalSize(numJob, 0); // 每个job所有数据块大小之和
	_for(i, 0, numJob) {
		_for(j, 0, location[i].size()) {
			jobInitLocSet[i].insert(location[i][j]);
			jobTotalSize[i] += dataSize[i][j];
		}
		maxRunningTime += (double)jobTotalSize[i] / (Sc[i]) / g(jobInitLocSet[i].size());
	}
	assert(maxRunningTime >= actualTime);

	// 2. 验证jobCore: 分配给每个Job的核数应该在 [1,总核数] 内
	int numCore = accumulate(hostCore.begin(), hostCore.end(), 0);
	_for(i, 0, numJob)
		assert(0 < jobCore[i] && jobCore[i] <= numCore);

	// 3. 验证runLoc: 每个核内的数据块运行次序不能相同, 必须是[1,...,n]的一个全排列, n是这个核上被调度的数据块个数
	//    顺便把从块视角的答案转化为从核视角的答案

	// 计算每个核被分配了多少数据块
	vector<vector<int>> hostCoreBlock(numHost);
	_for(i, 0, numHost)
		hostCoreBlock[i].resize(hostCore[i], 0);

	_for(i, 0, numJob) {
		_for(j, 0, jobBlock[i]) {
			int h = get<0>(runLoc[i][j]);
			int c = get<1>(runLoc[i][j]);
			if (h < 0 || h >= numHost || c < 0 || c >= hostCore[h])
				cerr << "Error: Job" << i << " block " << j << " should not be allocated in host " << h << " core " << c << "\n";
			hostCoreBlock[h][c]++;
		}
	}

	// 初始化主机-核的任务列表长度
	_for(i, 0, numHost)
		_for(j, 0, hostCore[i])
		hostCoreTask[i][j].resize(hostCoreBlock[i][j], make_tuple(-1, -1, -1, -1));

	// 尝试将每个数据块分配到主机-核的任务列表中
	_for(i, 0, numJob) {
		_for(j, 0, jobBlock[i]) {
			int h = get<0>(runLoc[i][j]);
			int c = get<1>(runLoc[i][j]);
			int r = get<2>(runLoc[i][j]);

			if (h < 0 || h >= numHost || c<0 || c >= hostCore[h] || r <= 0 || r>hostCoreBlock[h][c])
				cerr << "Error: Host " << h << " core " << c << " rank " << r << " should not be allocated by job " << i << " block " << j << "\n";
			else if (get<0>(hostCoreTask[h][c][r - 1]) != -1) // 注意 r 需要减一
				cerr << "Error: Host " << h << " core " << c << " rank " << r << " is already allocated by job " << get<0>(hostCoreTask[h][c][r - 1])
				<< " block " << get<1>(hostCoreTask[h][c][r - 1]) << ": " << get<2>(hostCoreTask[h][c][r - 1]) << " ~ " << get<3>(hostCoreTask[h][c][r - 1])
				<< " when allocate job " << i << " block " << j << "\n";

			hostCoreTask[h][c][r - 1] = make_tuple(i, j, -1.0, -1.0);
		}
	}

	// 计算主机-核上的运行情况, 可以把传输列表transferList也算出来

	// 初始化hostCoreFinishTime, 逐步模拟核当前运行到了哪个时间
	hostCoreFinishTime.resize(numHost);
	for (int i = 0; i < numHost; i++)
		hostCoreFinishTime[i].resize(hostCore[i], 0.0);

	int blockFinished = 0;
	int numTotalBlock = accumulate(jobBlock.begin(), jobBlock.end(), 0);
	vector<double> jobStartTime(numJob, 0.0); // 记录每个Job的开始时

	while (blockFinished < numTotalBlock) {
		// 1. 找最短运行时间的核, 尝试调度这个核上的下一个块对应的Job的所有数据块
		//    判断并记录需要传输的块到transferMap中

		// 2. 找最大的 这个job的 每个数据块所调度的核上完成上一个别的job数据块的结束时间, 
		//    将这个时间作为当前Job的起始时间jobStartTime

		// 3. 模拟完成当前Job的所有数据块
		//    更新hostCoreTask、hostCoreFinishTime、blockFinished

	}

}

void ResourceScheduler::validFromCore() {
	// 1. hostCoreTask上是否包含所有Job的所有数据块而且无重复, 得到runLoc


	// 2. hostCoreTask上所有数据块的运行时间不能有重叠, 得到jobStartTime和Job的运行顺序


	// 3. 根据每个Job的核数, hostCoreTask上每个数据块的运行时间应该在合理长度


}

void ResourceScheduler::visualization() {

}

double ResourceScheduler::g(int e) {
	return 1 - alpha * (e - 1);
}
