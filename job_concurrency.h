#pragma once
#include <string>
#include <thread>
#include <list>
#include <Windows.h>

//---------------------------------------------------------------------------------
// Job Status
//---------------------------------------------------------------------------------
enum
{
    PENDING,
    RUNNING,
    COMPLETE
};

//---------------------------------------------------------------------------------
// Job structure
//---------------------------------------------------------------------------------
typedef struct Job_tag
{
    size_t jobId;

    // Job assigned to which worker thread
    std::thread::id workerThreadId;

    // Job status
    std::string jobStatus;

    // Job created, start and completion time
    time_t createTime;
    time_t startTime;
    time_t endTime;

    // Parameter to process for this job. This is just
    // for simulation. Actual params could be anything
    std::string stringVal;

    // Job result stored here. This is just for simulation,
    // Actual result could be anything- string, int, custom object etc.
    unsigned int jobResult;
}Job_t;


//---------------------------------------------------------------------------------
// Structure storing information of its job execution details
//---------------------------------------------------------------------------------
typedef struct ThreadInfo_tag
{
    std::thread::id workerThreadId;
    size_t jobsCompleted;
    time_t workingTime;
    time_t idleTime;

}ThreadInfo_t;

Job_t* RetrieveJobFromWorkQueue();
void AddToWorkQueue(Job_t* job);
void ShowStatisticsThread();
