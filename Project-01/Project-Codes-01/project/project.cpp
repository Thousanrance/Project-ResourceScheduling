#include<iostream>
#include "ResourceScheduler.h"
#include "Util.h"
#include "time.h"

int main() {
	// Needs to ensure there is a file named "task_$taskType_case_$caseID.txt" in 'input' filefolder.
	// You can test more cases if you want.
	int taskType = 1, caseID = 1;
	srand(time(NULL));
	// Load data from file "task_$taskType_case_$caseID.txt" to class ResourceScheduler instance object.
	ResourceScheduler rs(taskType, caseID);
	// Carefully! It will write the randomly generated data into file "task_$taskType_case_$caseID.txt".
	//generator(rs, taskType, caseID); 

	// The main task you need to do is to implement this function.
	rs.schedule();
	
	rs.outputSolutionFromBlock();
	rs.outputSolutionFromCore();
	rs.outputRatio(); //For task1

	// rs.validFromBlock(); // Not necessary && Not Sufficient.

	return 0;
}

