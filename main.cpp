#include "job_concurrency.h"
#include <condition_variable>
#include <iostream>
#include <mutex>

//---------------------------------------------------------------------------------
// Configurations
//---------------------------------------------------------------------------------
const int STATISTICS_DISPLAY_INTERVAL_MS = 2000;


// Synchronization
std::mutex g_streamMutex;
std::mutex g_workQueueMutex;
std::mutex g_jobMutex;
std::condition_variable jobAvailableConditionVar;

// Job queues
std::list<Job_t*> g_workQueue;
std::list<Job_t*> g_allJobs;

std::list<ThreadInfo_t*> g_threadInfo;

const std::string JOB_STATUS[] = { "PENDING", "RUNNING", "COMPLETE" };

//---------------------------------------------------------------------------------
// @name                : StartJob
//
// @description         : Start execution of job using thread identified byworkerThreadId.
//---------------------------------------------------------------------------------
void StartJob(Job_t* job, std::thread::id workerThreadId)
{
    // Mark job as running
    g_jobMutex.lock();
    job->jobStatus = JOB_STATUS[RUNNING];
    job->startTime = time(nullptr);
    job->workerThreadId = workerThreadId;
    g_jobMutex.unlock();

    // Do some time consuming operation on this job.
    for (size_t i = 0; i < job->stringVal.size(); i++)
    {
        job->jobResult += static_cast<int>(job->stringVal[i]);
        Sleep(rand() % 1000);
    }

    // Mark job as complete
    g_jobMutex.lock();
    job->jobStatus = JOB_STATUS[COMPLETE];
    job->endTime = time(nullptr);
    g_jobMutex.unlock();
}

//---------------------------------------------------------------------------------
// @name                : WorkerThread
//
// @description         : Function for worker thread
//---------------------------------------------------------------------------------
void WorkerThread(ThreadInfo_t * threadInfo)
{
    std::thread::id threadId = std::this_thread::get_id();
    threadInfo->workerThreadId = threadId;

    g_streamMutex.lock();
    std::cout << "Worker Thread #" << threadId << " started" << std::endl;
    g_streamMutex.unlock();
    time_t idleEnd = time(nullptr);

    while (1 /* Runs forever */)
    {
        time_t idleStart = time(nullptr);
        Job_t* job = RetrieveJobFromWorkQueue();
        if (job)
        {
            g_streamMutex.lock();
            std::cout << "Processing [" << job->stringVal << "] using Worker thread: " << threadId << std::endl;
            g_streamMutex.unlock();

            // Do work
            time_t workStart = time(nullptr);
            StartJob(job, threadId);
            time_t workEnd = time(nullptr);

            // Update thread info
            threadInfo->jobsCompleted++;
            threadInfo->workingTime += workEnd - workStart;
        }
        else
        {
            idleEnd = time(nullptr);
            threadInfo->idleTime += idleEnd - idleStart;
        }
    }
}

//---------------------------------------------------------------------------------
// @name                : RetrieveJobFromWorkQueue
//
// @description         : Fetches and then removes job from work queue.
//---------------------------------------------------------------------------------
Job_t * RetrieveJobFromWorkQueue()
{
    Job_t* job = nullptr;
    g_workQueueMutex.lock();
    if (!g_workQueue.empty())
    {
        job = g_workQueue.front();
        g_workQueue.pop_front();
    }
    g_workQueueMutex.unlock();

    return job;
}

//---------------------------------------------------------------------------------
// @name                : AddToWorkQueue
//
// @description         : Adds a job to the work queue.
//---------------------------------------------------------------------------------
void AddToWorkQueue(Job_t* job)
{
    g_workQueueMutex.lock();
    g_workQueue.push_back(job);
    auto jobCount = g_workQueue.size();
    g_workQueueMutex.unlock();

    g_streamMutex.lock();
    std::cout << "[Job #" << job->jobId << "] added to work queue. Queued jobs: " << jobCount << std::endl;
    g_streamMutex.unlock();
}

//---------------------------------------------------------------------------------
// @name                : ShowStatisticsThread
//
// @description         : Displays statistics of all jobs
//---------------------------------------------------------------------------------
void ShowStatisticsThread()
{
    while (1 /* This runs forever */)
    {
        size_t totalJobs = g_allJobs.size();
        size_t pendingJobs = 0;
        size_t runningJobs = 0;
        size_t completedJobs = 0;
        time_t totalWaitingTime = 0;
        time_t totalRunningTime = 0;

        for (auto it = g_allJobs.begin(); it != g_allJobs.end(); it++)
        {
            Job_t* job = *it;

            if (job->jobStatus == JOB_STATUS[PENDING])
            {
                pendingJobs++;
            }
            else if (job->jobStatus == JOB_STATUS[RUNNING])
            {
                runningJobs++;
            }
            else if (job->jobStatus == JOB_STATUS[COMPLETE])
            {
                completedJobs++;

                totalWaitingTime += job->startTime - job->createTime;
                totalRunningTime += job->endTime - job->startTime;
            }
            else
            {
                // NOTHING
            }

        }// End of all jobs

        // Avg time stats for completed jobs
        float avgWaitingTime = (completedJobs > 0) ? static_cast<float>(totalWaitingTime) / completedJobs : 0;
        float avgRunningTime = (completedJobs > 0) ? static_cast<float>(totalRunningTime) / completedJobs : 0;

        g_streamMutex.lock();
        std::cout << "\n+-------------------------------------------------------------------------------------+\n";
        std::cout <<   "|                         J O B    S U M M A R Y                                      |\n";
        std::cout <<   "+-------------------------------------------------------------------------------------+\n";
        std::cout <<   "| Total jobs                  : " << totalJobs << std::endl;
        std::cout <<   "| Pending jobs                : " << pendingJobs << std::endl;
        std::cout <<   "| Running jobs                : " << runningJobs << std::endl;
        std::cout <<   "| Completed jobs              : " << completedJobs << std::endl;
        std::cout <<   "| Avg waiting time            : " << avgWaitingTime << " s. " << std::endl;
        std::cout <<   "| Avg running time            : " << avgRunningTime << " s. " << std::endl;
        std::cout <<   "+-------------------------------------------------------------------------------------+\n\n";
        g_streamMutex.unlock();
        Sleep(STATISTICS_DISPLAY_INTERVAL_MS);
    }
}

//---------------------------------------------------------------------------------
// @name                : ShowThreadWorkInfoThread
//
// @description         : Displays statistics of running threads
//---------------------------------------------------------------------------------
void ShowThreadWorkInfoThread()
{
    while (1 /* This runs forever */)
    {
        int index = 1;
        g_streamMutex.lock();
        std::cout << "\n+-------------------------------------------------------------------------------------+\n";
        std::cout <<   "|                 W O R K E R    T H R E A D    S U M M A R Y                         |\n";
        std::cout <<   "+-------------------------------------------------------------------------------------+\n";
        for (auto it = g_threadInfo.begin(); it != g_threadInfo.end(); it++)
        {
            ThreadInfo_t* threadInfo = *it;
            std::cout << "|" << std::endl;
            std::cout << "| " << index << ") Thread ID                      : " << threadInfo->workerThreadId << std::endl;
            std::cout <<            "|     Jobs Completed                : " << threadInfo->jobsCompleted << std::endl;
            std::cout <<            "|     Working time                  : " << threadInfo->workingTime << " s." << std::endl;
            std::cout <<            "|     Idle time                     : " << threadInfo->idleTime << " s." << std::endl;
            index++;
        }

        std::cout << "+-------------------------------------------------------------------------------------+\n\n";
        g_streamMutex.unlock();
        Sleep(STATISTICS_DISPLAY_INTERVAL_MS + 500);
    }
}

//---------------------------------------------------------------------------------
// @name                : CreateJob
//
// @description         : Creates a new job with the specified user parameter. 
//                        Specifies 
//---------------------------------------------------------------------------------
Job_t* CreateJob(const std::string& param)
{
    Job_t* job = new Job_t{};
    job->jobId = g_allJobs.size() + 1;
    job->createTime = time(nullptr);
    job->jobStatus = JOB_STATUS[PENDING];
    job->stringVal = param;

    // Add this to the list of all jobs
    g_allJobs.push_back(job);

    return job;
}

//---------------------------------------------------------------------------------
// M A I N 
//---------------------------------------------------------------------------------
int main()
{
    srand(time(nullptr));

    ThreadInfo_t* thread1_info = new ThreadInfo_t{};
    std::thread workerThread1(WorkerThread, thread1_info);
    g_threadInfo.push_back(thread1_info);

    ThreadInfo_t* thread2_info = new ThreadInfo_t{};
    std::thread workerThread2(WorkerThread, thread2_info);
    g_threadInfo.push_back(thread2_info);

    ThreadInfo_t* thread3_info = new ThreadInfo_t{};
    std::thread workerThread3(WorkerThread, thread3_info);
    g_threadInfo.push_back(thread3_info);

    ThreadInfo_t* thread4_info = new ThreadInfo_t{};
    std::thread workerThread4(WorkerThread, thread4_info);
    g_threadInfo.push_back(thread4_info);

    // No need to wait for worker threads to complete.
    workerThread1.detach();
    workerThread2.detach();
    workerThread3.detach();
    workerThread4.detach();

    // Spawn a thread that displays statistics continuously
    std::thread statisticsThread(ShowStatisticsThread);
    statisticsThread.detach();

    // Displays work info by each running thread
    std::thread workInfoThread(ShowThreadWorkInfoThread);
    workInfoThread.detach();

    Sleep(100);
    g_streamMutex.lock();
    std::cout << "\nWaiting for user input..." << std::endl;
    g_streamMutex.unlock();

    while (1 /* Process user input */)
    {
        std::string userInput;
        std::cin >> userInput;

        Job_t *job = CreateJob(userInput);
        AddToWorkQueue(job);
    }// End of while loop


    return 0;
}