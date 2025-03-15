
/*-------------------------------------------------------------+
|                                                              |
|                   _________   ______ _    _____              |
|                  / / ____/ | / / __ \ |  / /   |             |
|             __  / / __/ /  |/ / / / / | / / /| |             |
|            / /_/ / /___/ /|  / /_/ /| |/ / ___ |             |
|            \____/_____/_/ |_/\____/ |___/_/  |_|             |
|                                                              |
|                        Jenova Runtime                        |
|                   Developed by Hamid.Memar                   |
|                                                              |
+-------------------------------------------------------------*/

// Jenova SDK
#include "Jenova.hpp"

// Threading SDK
#ifdef TARGET_PLATFORM_WINDOWS
    #define PTW32_STATIC_LIB
    #define _TIMESPEC_DEFINED
#endif
#include <pthread.h>

// Imports
using TaskID = jenova::TaskID;
using TaskFunction = jenova::TaskFunction;

// Structures
struct Task
{
    pthread_t thread;
    TaskFunction function;
    std::atomic<bool> isDone;
};

// Internal Storage
static std::unordered_map<TaskID, Task> tasks;
static std::mutex taskMutex;
static TaskID nextTaskID = 1;

// Jenova Task System Implementation
TaskID JenovaTaskSystem::InitiateTask(TaskFunction function)
{
    std::lock_guard<std::mutex> lock(taskMutex);
    TaskID taskID = nextTaskID++;
    Task& task = tasks[taskID];
    task.function = function;
    task.isDone = false;
    pthread_create(&task.thread, nullptr, &JenovaTaskSystem::TaskRunner, &task);
    return taskID;
}
bool JenovaTaskSystem::IsTaskComplete(TaskID taskID)
{
    std::lock_guard<std::mutex> lock(taskMutex);
    if (tasks.find(taskID) != tasks.end()) return tasks[taskID].isDone.load();
    return false;
}
void JenovaTaskSystem::ClearTask(TaskID taskID)
{
    std::lock_guard<std::mutex> lock(taskMutex);
    if (tasks.find(taskID) != tasks.end() && tasks[taskID].isDone.load())
    {
        pthread_join(tasks[taskID].thread, nullptr);
        tasks.erase(taskID);
    }
}
void* JenovaTaskSystem::TaskRunner(void* taskPtr)
{
    Task* task = static_cast<Task*>(taskPtr);
    task->function();
    task->isDone.store(true);
    return nullptr;
}