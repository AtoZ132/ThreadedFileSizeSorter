#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <memory>
#include <chrono>
#include <algorithm>
#include <vector>
#include <filesystem>
#include "TaskObj.hpp"

constexpr auto thread_num = 2;
constexpr auto print_batch = 1;
constexpr auto poll_interval = 3;
bool gDataReady = false;
std::mutex g_tq_mutex;
std::mutex g_ds_mutex;

bool checkDups(const std::shared_ptr<std::queue<TaskObj>> taskQueue,
    const std::shared_ptr<std::vector<TaskObj>> storageVec, std::string s) {
    std::queue<TaskObj> tempQueue = *taskQueue;

    std::scoped_lock lock(g_tq_mutex, g_ds_mutex);
    for (const auto& i : *storageVec) {
            if (i.getName() == s)
                return true;
    }

    while (!tempQueue.empty()) {
        if (tempQueue.front().getName() == s)
            return true;
        tempQueue.pop();
    }

    return false;
}



void pollDirectory(std::shared_ptr<std::queue<TaskObj>> taskQueue,
    std::shared_ptr<std::vector<TaskObj>> storageVec) {
    std::filesystem::path dirPath(".\\bin");
    std::size_t queueSize = 0;

    while (true) {
        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            if (!(checkDups(taskQueue, storageVec, 
                std::filesystem::path(entry.path()).filename().string()))) {

                TaskObj newTask(std::filesystem::path(entry.path()).filename().string(),
                    std::filesystem::file_size(entry.path()));
                std::cout << "adding task:  " << std::filesystem::path(entry.path()).filename().string() << std::endl;
                std::lock_guard<std::mutex> lock(g_tq_mutex);
                taskQueue->push(std::move(newTask));
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(poll_interval));
    }
}

void insertSorted(std::shared_ptr<std::vector<TaskObj>> storageVec,TaskObj&& task) {
    std::lock_guard<std::mutex> lock(g_ds_mutex);
    storageVec->push_back(std::move(task));
    std::sort(storageVec->begin(), storageVec->end(), [](const TaskObj& a, const TaskObj& b) {
        return a.getSize() < b.getSize();
    });

    if (!storageVec->empty() && !(storageVec->size() % print_batch))
        gDataReady = true;
}

void storeFileData(std::shared_ptr<std::queue<TaskObj>> taskQueue,
    std::shared_ptr<std::vector<TaskObj>> storageVec) {
    while (true) {
        std::lock_guard<std::mutex> lock(g_tq_mutex);
        if (!taskQueue->empty()) {
            std::cout << "storing task: " << taskQueue->front().getName() << std::endl;
            TaskObj temp = std::move(taskQueue->front());
            insertSorted(storageVec, std::move(temp));
            taskQueue->pop();
        }
    }
}

int main() {
    std::shared_ptr<std::queue<TaskObj>> taskQueue = std::make_shared<std::queue<TaskObj>>();
    std::shared_ptr<std::vector<TaskObj>> storageVec = std::make_shared<std::vector<TaskObj>>();
    std::vector<std::thread> threads;

    // Run the polling thread detached
    std::thread dPollingThread(pollDirectory, taskQueue, storageVec);
    dPollingThread.detach();

    for (int i = 0; i < thread_num; ++i) {
        threads.push_back(std::thread(storeFileData, taskQueue, storageVec));
        threads.at(i).detach();
    }

    // Main thread
    while (true) {   
        if (gDataReady) {
            std::lock_guard<std::mutex> lock(g_ds_mutex);
            for (const auto& i : *storageVec)
                std::cout << "file name: " << i.getName() << ", size: " << i.getSize() << std::endl;
            gDataReady = false;
        }
    }

    return 0;
}
