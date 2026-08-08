// thread.cpp - BMS Thread System Implementation (C++)
#include "thread.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <queue>
#include <vector>
#include <atomic>
#include <memory>
#include <functional>
#include <chrono>
#include <unordered_map>

namespace BMS {
namespace Thread {

// ============================================================================
// Thread System Class
// ============================================================================

class ThreadSystem {
public:
    static ThreadSystem* GetInstance() {
        static ThreadSystem instance;
        return &instance;
    }

    bool Initialize() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (initialized_) return true;

        std::cout << "[BMS Thread C++] Initializing thread system..." << std::endl;

        // Initialize main thread
        mainThread_ = std::make_shared<Thread>();
        mainThread_->name = "Main Thread";
        mainThread_->status = ThreadStatus::RUNNING;
        mainThread_->id = GetThreadId(std::this_thread::get_id());

        threads_[mainThread_->id] = mainThread_;

        initialized_ = true;
        std::cout << "[BMS Thread C++] Thread system initialized" << std::endl;

        return true;
    }

    void Shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!initialized_) return;

        std::cout << "[BMS Thread C++] Shutting down thread system..." << std::endl;

        // Shutdown all thread pools
        for (auto& [id, pool] : pools_) {
            pool->Shutdown();
        }
        pools_.clear();

        // Clean up threads
        threads_.clear();

        initialized_ = false;
        std::cout << "[BMS Thread C++] Thread system shut down" << std::endl;
    }

    bool IsInitialized() const { return initialized_; }

    // Thread management
    std::shared_ptr<Thread> CreateThread(std::function<void()> entry, 
                                         const std::string& name = "") {
        std::lock_guard<std::mutex> lock(mutex_);

        auto thread = std::make_shared<Thread>();
        thread->id = GenerateId();
        thread->name = name.empty() ? "Thread-" + std::to_string(thread->id) : name;
        thread->status = ThreadStatus::CREATED;
        thread->entry = entry;

        // Create native thread
        thread->nativeHandle = std::make_unique<std::thread>([this, thread, entry]() {
            this->ThreadEntry(thread, entry);
        });

        threads_[thread->id] = thread;
        std::cout << "[BMS Thread C++] Created thread: " << thread->name << std::endl;

        return thread;
    }

    bool DestroyThread(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = threads_.find(id);
        if (it == threads_.end()) return false;

        auto thread = it->second;
        if (thread->status == ThreadStatus::RUNNING) {
            thread->nativeHandle->detach();
        }

        threads_.erase(it);
        return true;
    }

    std::shared_ptr<Thread> GetThread(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = threads_.find(id);
        return it != threads_.end() ? it->second : nullptr;
    }

    std::shared_ptr<Thread> GetCurrentThread() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto id = GetThreadId(std::this_thread::get_id());
        auto it = threads_.find(id);
        if (it != threads_.end()) {
            return it->second;
        }

        // Create thread object for current thread if not found
        auto thread = std::make_shared<Thread>();
        thread->id = id;
        thread->name = "Thread-" + std::to_string(id);
        thread->status = ThreadStatus::RUNNING;
        thread->nativeHandle = nullptr;

        threads_[id] = thread;
        return thread;
    }

    // Mutex management
    std::shared_ptr<Mutex> CreateMutex(const std::string& name = "") {
        std::lock_guard<std::mutex> lock(mutex_);

        auto mutex = std::make_shared<Mutex>();
        mutex->id = GenerateId();
        mutex->name = name.empty() ? "Mutex-" + std::to_string(mutex->id) : name;
        mutex->nativeMutex = std::make_unique<std::mutex>();

        mutexes_[mutex->id] = mutex;
        return mutex;
    }

    bool DestroyMutex(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = mutexes_.find(id);
        if (it == mutexes_.end()) return false;
        mutexes_.erase(it);
        return true;
    }

    // Condition variable management
    std::shared_ptr<Condition> CreateCondition(const std::string& name = "") {
        std::lock_guard<std::mutex> lock(mutex_);

        auto cond = std::make_shared<Condition>();
        cond->id = GenerateId();
        cond->name = name.empty() ? "Cond-" + std::to_string(cond->id) : name;
        cond->nativeCond = std::make_unique<std::condition_variable>();

        conditions_[cond->id] = cond;
        return cond;
    }

    bool DestroyCondition(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = conditions_.find(id);
        if (it == conditions_.end()) return false;
        conditions_.erase(it);
        return true;
    }

    // Semaphore management
    std::shared_ptr<Semaphore> CreateSemaphore(int32_t initialValue, int32_t maxValue,
                                               const std::string& name = "") {
        std::lock_guard<std::mutex> lock(mutex_);

        auto sem = std::make_shared<Semaphore>();
        sem->id = GenerateId();
        sem->name = name.empty() ? "Sem-" + std::to_string(sem->id) : name;
        sem->value = initialValue;
        sem->maxValue = maxValue;

        semaphores_[sem->id] = sem;
        return sem;
    }

    bool DestroySemaphore(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = semaphores_.find(id);
        if (it == semaphores_.end()) return false;
        semaphores_.erase(it);
        return true;
    }

    // Thread pool management
    std::shared_ptr<ThreadPool> CreateThreadPool(const std::string& name,
                                                 ThreadPoolType type,
                                                 uint32_t minThreads,
                                                 uint32_t maxThreads) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto pool = std::make_shared<ThreadPool>();
        pool->id = GenerateId();
        pool->name = name.empty() ? "Pool-" + std::to_string(pool->id) : name;
        pool->type = type;
        pool->minThreads = minThreads;
        pool->maxThreads = maxThreads;
        pool->running = true;

        // Create initial threads
        for (uint32_t i = 0; i < minThreads; i++) {
            std::string threadName = pool->name + "-Worker-" + std::to_string(i);
            auto thread = CreateThread([pool]() { pool->Worker(); }, threadName);
            pool->threads.push_back(thread);
            thread->status = ThreadStatus::READY;
        }

        pools_[pool->id] = pool;
        std::cout << "[BMS Thread C++] Created thread pool: " << pool->name << std::endl;

        return pool;
    }

    bool DestroyThreadPool(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = pools_.find(id);
        if (it == pools_.end()) return false;
        it->second->Shutdown();
        pools_.erase(it);
        return true;
    }

    // Utility functions
    uint32_t GetThreadId(std::thread::id id) {
        static std::unordered_map<std::thread::id, uint32_t> idMap;
        static std::mutex idMapMutex;
        static uint32_t nextId = 1;

        std::lock_guard<std::mutex> lock(idMapMutex);
        auto it = idMap.find(id);
        if (it != idMap.end()) {
            return it->second;
        }

        uint32_t newId = nextId++;
        idMap[id] = newId;
        return newId;
    }

    void PrintStats() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "\n=== BMS Thread System Statistics (C++) ===" << std::endl;
        std::cout << "Threads: " << threads_.size() << std::endl;
        std::cout << "Mutexes: " << mutexes_.size() << std::endl;
        std::cout << "Conditions: " << conditions_.size() << std::endl;
        std::cout << "Semaphores: " << semaphores_.size() << std::endl;
        std::cout << "Thread Pools: " << pools_.size() << std::endl;
        std::cout << "==========================================" << std::endl;
    }

private:
    ThreadSystem() : initialized_(false), idCounter_(0) {}
    ~ThreadSystem() = default;

    uint32_t GenerateId() {
        return ++idCounter_;
    }

    void ThreadEntry(std::shared_ptr<Thread> thread, std::function<void()> entry) {
        thread->status = ThreadStatus::RUNNING;
        thread->startTime = std::chrono::system_clock::now();

        try {
            entry();
        } catch (const std::exception& e) {
            std::cerr << "[BMS Thread C++] Thread exception: " << e.what() << std::endl;
        }

        thread->status = ThreadStatus::TERMINATED;
        thread->endTime = std::chrono::system_clock::now();
    }

    bool initialized_;
    std::mutex mutex_;
    std::atomic<uint32_t> idCounter_;

    std::unordered_map<uint32_t, std::shared_ptr<Thread>> threads_;
    std::unordered_map<uint32_t, std::shared_ptr<Mutex>> mutexes_;
    std::unordered_map<uint32_t, std::shared_ptr<Condition>> conditions_;
    std::unordered_map<uint32_t, std::shared_ptr<Semaphore>> semaphores_;
    std::unordered_map<uint32_t, std::shared_ptr<ThreadPool>> pools_;

    std::shared_ptr<Thread> mainThread_;
};

// ============================================================================
// ThreadPool Worker
// ============================================================================

void ThreadPool::Worker() {
    while (running) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this]() {
                return !running || !taskQueue.empty();
            });

            if (!running && taskQueue.empty()) {
                break;
            }

            task = std::move(taskQueue.front());
            taskQueue.pop();
        }

        activeThreads++;
        try {
            task();
        } catch (const std::exception& e) {
            std::cerr << "[BMS Thread C++] Pool task exception: " << e.what() << std::endl;
        }
        activeThreads--;
    }
}

// ============================================================================
// C++ API Functions
// ============================================================================

// System functions
bool Initialize() {
    return ThreadSystem::GetInstance()->Initialize();
}

void Shutdown() {
    ThreadSystem::GetInstance()->Shutdown();
}

bool IsInitialized() {
    return ThreadSystem::GetInstance()->IsInitialized();
}

// Thread functions
std::shared_ptr<Thread> CreateThread(std::function<void()> entry, const std::string& name) {
    return ThreadSystem::GetInstance()->CreateThread(entry, name);
}

bool DestroyThread(uint32_t id) {
    return ThreadSystem::GetInstance()->DestroyThread(id);
}

std::shared_ptr<Thread> GetThread(uint32_t id) {
    return ThreadSystem::GetInstance()->GetThread(id);
}

std::shared_ptr<Thread> GetCurrentThread() {
    return ThreadSystem::GetInstance()->GetCurrentThread();
}

void Sleep(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void Yield() {
    std::this_thread::yield();
}

// Mutex functions
std::shared_ptr<Mutex> CreateMutex(const std::string& name) {
    return ThreadSystem::GetInstance()->CreateMutex(name);
}

bool DestroyMutex(uint32_t id) {
    return ThreadSystem::GetInstance()->DestroyMutex(id);
}

void LockMutex(std::shared_ptr<Mutex> mutex) {
    if (mutex && mutex->nativeMutex) {
        mutex->nativeMutex->lock();
        mutex->owner = GetCurrentThread();
        mutex->recursionCount++;
    }
}

bool TryLockMutex(std::shared_ptr<Mutex> mutex) {
    if (mutex && mutex->nativeMutex) {
        if (mutex->nativeMutex->try_lock()) {
            mutex->owner = GetCurrentThread();
            mutex->recursionCount++;
            return true;
        }
    }
    return false;
}

void UnlockMutex(std::shared_ptr<Mutex> mutex) {
    if (mutex && mutex->nativeMutex) {
        mutex->recursionCount--;
        if (mutex->recursionCount == 0) {
            mutex->owner = nullptr;
        }
        mutex->nativeMutex->unlock();
    }
}

// Condition functions
std::shared_ptr<Condition> CreateCondition(const std::string& name) {
    return ThreadSystem::GetInstance()->CreateCondition(name);
}

bool DestroyCondition(uint32_t id) {
    return ThreadSystem::GetInstance()->DestroyCondition(id);
}

void WaitCondition(std::shared_ptr<Condition> cond, std::shared_ptr<Mutex> mutex) {
    if (cond && cond->nativeCond && mutex && mutex->nativeMutex) {
        std::unique_lock<std::mutex> lock(*mutex->nativeMutex, std::adopt_lock);
        cond->nativeCond->wait(lock);
        lock.release();
    }
}

bool TimedWaitCondition(std::shared_ptr<Condition> cond, std::shared_ptr<Mutex> mutex,
                        uint32_t timeoutMs) {
    if (cond && cond->nativeCond && mutex && mutex->nativeMutex) {
        std::unique_lock<std::mutex> lock(*mutex->nativeMutex, std::adopt_lock);
        auto status = cond->nativeCond->wait_for(lock, 
            std::chrono::milliseconds(timeoutMs));
        lock.release();
        return status == std::cv_status::no_timeout;
    }
    return false;
}

void SignalCondition(std::shared_ptr<Condition> cond) {
    if (cond && cond->nativeCond) {
        cond->nativeCond->notify_one();
    }
}

void BroadcastCondition(std::shared_ptr<Condition> cond) {
    if (cond && cond->nativeCond) {
        cond->nativeCond->notify_all();
    }
}

// Semaphore functions
std::shared_ptr<Semaphore> CreateSemaphore(int32_t initialValue, int32_t maxValue,
                                           const std::string& name) {
    return ThreadSystem::GetInstance()->CreateSemaphore(initialValue, maxValue, name);
}

bool DestroySemaphore(uint32_t id) {
    return ThreadSystem::GetInstance()->DestroySemaphore(id);
}

void WaitSemaphore(std::shared_ptr<Semaphore> sem) {
    if (sem) {
        std::unique_lock<std::mutex> lock(sem->mutex);
        sem->condition.wait(lock, [sem]() { return sem->value > 0; });
        sem->value--;
    }
}

bool TryWaitSemaphore(std::shared_ptr<Semaphore> sem) {
    if (sem) {
        std::lock_guard<std::mutex> lock(sem->mutex);
        if (sem->value > 0) {
            sem->value--;
            return true;
        }
    }
    return false;
}

void PostSemaphore(std::shared_ptr<Semaphore> sem) {
    if (sem) {
        std::lock_guard<std::mutex> lock(sem->mutex);
        if (sem->value < sem->maxValue) {
            sem->value++;
            sem->condition.notify_one();
        }
    }
}

int32_t GetSemaphoreValue(std::shared_ptr<Semaphore> sem) {
    if (sem) {
        std::lock_guard<std::mutex> lock(sem->mutex);
        return sem->value;
    }
    return -1;
}

// Thread pool functions
std::shared_ptr<ThreadPool> CreateThreadPool(const std::string& name,
                                             ThreadPoolType type,
                                             uint32_t minThreads,
                                             uint32_t maxThreads) {
    return ThreadSystem::GetInstance()->CreateThreadPool(name, type, minThreads, maxThreads);
}

bool DestroyThreadPool(uint32_t id) {
    return ThreadSystem::GetInstance()->DestroyThreadPool(id);
}

void SubmitTask(std::shared_ptr<ThreadPool> pool, std::function<void()> task) {
    if (pool && pool->running) {
        std::lock_guard<std::mutex> lock(pool->queueMutex);
        pool->taskQueue.push(task);
        pool->condition.notify_one();
    }
}

void WaitForPool(std::shared_ptr<ThreadPool> pool) {
    if (pool) {
        while (true) {
            bool done;
            {
                std::lock_guard<std::mutex> lock(pool->queueMutex);
                done = pool->taskQueue.empty() && pool->activeThreads == 0;
            }
            if (done) break;
            Sleep(10);
        }
    }
}

// BMS Browser specific functions
void StartBrowserThreads() {
    auto* system = ThreadSystem::GetInstance();

    // Create browser thread
    system->CreateThread([]() {
        std::cout << "[BMS Thread C++] Browser thread running" << std::endl;
        while (true) {
            Sleep(1000);
        }
    }, "BMS-Browser");

    // Create renderer thread
    system->CreateThread([]() {
        std::cout << "[BMS Thread C++] Renderer thread running" << std::endl;
        while (true) {
            Sleep(100);
        }
    }, "BMS-Renderer");

    // Create network thread
    system->CreateThread([]() {
        std::cout << "[BMS Thread C++] Network thread running" << std::endl;
        while (true) {
            Sleep(500);
        }
    }, "BMS-Network");

    // Create audio thread
    system->CreateThread([]() {
        std::cout << "[BMS Thread C++] Audio thread running" << std::endl;
        while (true) {
            Sleep(100);
        }
    }, "BMS-Audio");

    // Create IO thread
    system->CreateThread([]() {
        std::cout << "[BMS Thread C++] IO thread running" << std::endl;
        while (true) {
            Sleep(100);
        }
    }, "BMS-IO");

    // Create thread pools
    CreateThreadPool("RenderPool", ThreadPoolType::FIXED, 2, 4);
    CreateThreadPool("NetworkPool", ThreadPoolType::CACHED, 4, 16);
    CreateThreadPool("AudioPool", ThreadPoolType::FIXED, 2, 4);
    CreateThreadPool("IOPool", ThreadPoolType::CACHED, 2, 8);
    CreateThreadPool("DefaultPool", ThreadPoolType::CACHED, 4, 16);
}

void StopBrowserThreads() {
    ThreadSystem::GetInstance()->Shutdown();
}

} // namespace Thread
} // namespace BMS

// ============================================================================
// C Compatibility Wrappers
// ============================================================================

extern "C" {

int thread_system_init(void) {
    return BMS::Thread::Initialize() ? 0 : -1;
}

void thread_system_shutdown(void) {
    BMS::Thread::Shutdown();
}

bool thread_system_is_initialized(void) {
    return BMS::Thread::IsInitialized();
}

const char* thread_get_version(void) {
    static std::string version = "BMS Thread System v1.0.0";
    return version.c_str();
}

// Thread wrappers
Thread* thread_create(ThreadEntry entry, void* arg) {
    auto thread = BMS::Thread::CreateThread([entry, arg]() {
        entry(arg);
    });
    if (!thread) return nullptr;

    Thread* cThread = (Thread*)malloc(sizeof(Thread));
    if (cThread) {
        cThread->id = thread->id;
        cThread->name = strdup(thread->name.c_str());
        cThread->status = thread->status;
        cThread->userData = thread.get();
    }
    return cThread;
}

Thread* thread_create_ex(ThreadEntry entry, void* arg, const char* name, size_t stackSize) {
    return thread_create(entry, arg);
}

void thread_destroy(Thread* thread) {
    if (!thread) return;
    free(thread->name);
    free(thread);
}

int thread_start(Thread* thread) {
    if (!thread) return -1;
    auto cppThread = static_cast<BMS::Thread::Thread*>(thread->userData);
    if (cppThread) {
        cppThread->status = BMS::Thread::ThreadStatus::READY;
        return 0;
    }
    return -1;
}

int thread_join(Thread* thread) {
    if (!thread) return -1;
    auto cppThread = static_cast<BMS::Thread::Thread*>(thread->userData);
    if (cppThread && cppThread->nativeHandle) {
        cppThread->nativeHandle->join();
        return 0;
    }
    return -1;
}

int thread_detach(Thread* thread) {
    if (!thread) return -1;
    auto cppThread = static_cast<BMS::Thread::Thread*>(thread->userData);
    if (cppThread && cppThread->nativeHandle) {
        cppThread->nativeHandle->detach();
        thread->detached = true;
        return 0;
    }
    return -1;
}

int thread_cancel(Thread* thread) {
    // C++ threads don't have cancel, use terminate
    return thread_terminate(thread);
}

int thread_terminate(Thread* thread) {
    if (!thread) return -1;
    // In C++, we can't forcibly terminate threads
    // We'll mark it as terminated
    thread->status = THREAD_STATUS_TERMINATED;
    return 0;
}

void thread_sleep(uint32_t ms) {
    BMS::Thread::Sleep(ms);
}

void thread_yield(void) {
    BMS::Thread::Yield();
}

void thread_exit(void* result) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

int thread_set_priority(Thread* thread, ThreadPriority priority) {
    if (!thread) return -1;
    thread->priority = priority;
    return 0;
}

ThreadPriority thread_get_priority(Thread* thread) {
    return thread ? thread->priority : THREAD_PRIORITY_NORMAL;
}

int thread_set_affinity(Thread* thread, uint32_t coreMask) {
    if (!thread) return -1;
    thread->coreAffinity = coreMask;
    return 0;
}

uint32_t thread_get_affinity(Thread* thread) {
    return thread ? thread->coreAffinity : 0;
}

int thread_set_name(Thread* thread, const char* name) {
    if (!thread) return -1;
    free(thread->name);
    thread->name = strdup(name);
    return 0;
}

const char* thread_get_name(Thread* thread) {
    return thread ? thread->name : NULL;
}

ThreadStatus thread_get_status(Thread* thread) {
    if (!thread) return THREAD_STATUS_ERROR;
    auto cppThread = static_cast<BMS::Thread::Thread*>(thread->userData);
    if (cppThread) {
        return (ThreadStatus)cppThread->status;
    }
    return thread->status;
}

uint32_t thread_get_id(Thread* thread) {
    return thread ? thread->id : 0;
}

uint64_t thread_get_cpu_time(Thread* thread) {
    if (!thread) return 0;
    auto cppThread = static_cast<BMS::Thread::Thread*>(thread->userData);
    if (cppThread && cppThread->nativeHandle) {
        // Not easily available in C++ standard library
        return 0;
    }
    return 0;
}

Thread* thread_current(void) {
    auto cppThread = BMS::Thread::GetCurrentThread();
    if (!cppThread) return nullptr;

    Thread* cThread = (Thread*)calloc(1, sizeof(Thread));
    if (cThread) {
        cThread->id = cppThread->id;
        cThread->name = strdup(cppThread->name.c_str());
        cThread->status = (ThreadStatus)cppThread->status;
        cThread->userData = cppThread.get();
    }
    return cThread;
}

uint32_t thread_current_id(void) {
    return BMS::Thread::ThreadSystem::GetInstance()->GetThreadId(std::this_thread::get_id());
}

void thread_current_exit(void* result) {
    thread_exit(result);
}

int thread_current_set_priority(ThreadPriority priority) {
    return thread_set_priority(thread_current(), priority);
}

ThreadPriority thread_current_get_priority(void) {
    return thread_get_priority(thread_current());
}

int thread_current_set_name(const char* name) {
    return thread_set_name(thread_current(), name);
}

const char* thread_current_get_name(void) {
    return thread_get_name(thread_current());
}

// Mutex wrappers
Mutex* mutex_create(void) {
    auto cppMutex = BMS::Thread::CreateMutex();
    if (!cppMutex) return nullptr;

    Mutex* cMutex = (Mutex*)calloc(1, sizeof(Mutex));
    if (cMutex) {
        cMutex->id = cppMutex->id;
        cMutex->name = strdup(cppMutex->name.c_str());
        cMutex->userData = cppMutex.get();
    }
    return cMutex;
}

Mutex* mutex_create_ex(MutexType type, const char* name) {
    return mutex_create();
}

void mutex_destroy(Mutex* mutex) {
    if (!mutex) return;
    free(mutex->name);
    free(mutex);
}

int mutex_lock(Mutex* mutex) {
    if (!mutex) return -1;
    auto cppMutex = static_cast<BMS::Thread::Mutex*>(mutex->userData);
    if (cppMutex) {
        cppMutex->nativeMutex->lock();
        return 0;
    }
    return -1;
}

int mutex_trylock(Mutex* mutex) {
    if (!mutex) return -1;
    auto cppMutex = static_cast<BMS::Thread::Mutex*>(mutex->userData);
    if (cppMutex) {
        return cppMutex->nativeMutex->try_lock() ? 0 : -1;
    }
    return -1;
}

int mutex_unlock(Mutex* mutex) {
    if (!mutex) return -1;
    auto cppMutex = static_cast<BMS::Thread::Mutex*>(mutex->userData);
    if (cppMutex) {
        cppMutex->nativeMutex->unlock();
        return 0;
    }
    return -1;
}

int mutex_timedlock(Mutex* mutex, uint32_t timeoutMs) {
    if (!mutex) return -1;
    // C++ mutex doesn't have timed lock
    // Use try_lock in a loop
    uint64_t start = get_time_ms();
    while (get_time_ms() - start < timeoutMs) {
        if (mutex_trylock(mutex) == 0) {
            return 0;
        }
        thread_sleep(1);
    }
    return -1;
}

bool mutex_is_locked(Mutex* mutex) {
    if (!mutex) return false;
    auto cppMutex = static_cast<BMS::Thread::Mutex*>(mutex->userData);
    if (cppMutex && cppMutex->nativeMutex) {
        return cppMutex->recursionCount > 0;
    }
    return false;
}

Thread* mutex_get_owner(Mutex* mutex) {
    // Not easily available in C++
    return NULL;
}

uint32_t mutex_get_wait_count(Mutex* mutex) {
    return mutex ? mutex->waitCount : 0;
}

// Condition wrappers
Condition* condition_create(void) {
    auto cppCond = BMS::Thread::CreateCondition();
    if (!cppCond) return nullptr;

    Condition* cCond = (Condition*)calloc(1, sizeof(Condition));
    if (cCond) {
        cCond->id = cppCond->id;
        cCond->name = strdup(cppCond->name.c_str());
        cCond->userData = cppCond.get();
    }
    return cCond;
}

Condition* condition_create_ex(ConditionType type, const char* name) {
    return condition_create();
}

void condition_destroy(Condition* cond) {
    if (!cond) return;
    free(cond->name);
    free(cond);
}

int condition_wait(Condition* cond, Mutex* mutex) {
    if (!cond || !mutex) return -1;
    auto cppCond = static_cast<BMS::Thread::Condition*>(cond->userData);
    auto cppMutex = static_cast<BMS::Thread::Mutex*>(mutex->userData);
    if (cppCond && cppMutex) {
        std::unique_lock<std::mutex> lock(*cppMutex->nativeMutex);
        cppCond->nativeCond->wait(lock);
        return 0;
    }
    return -1;
}

int condition_timedwait(Condition* cond, Mutex* mutex, uint32_t timeoutMs) {
    if (!cond || !mutex) return -1;
    auto cppCond = static_cast<BMS::Thread::Condition*>(cond->userData);
    auto cppMutex = static_cast<BMS::Thread::Mutex*>(mutex->userData);
    if (cppCond && cppMutex) {
        std::unique_lock<std::mutex> lock(*cppMutex->nativeMutex);
        auto status = cppCond->nativeCond->wait_for(lock, 
            std::chrono::milliseconds(timeoutMs));
        return status == std::cv_status::no_timeout ? 0 : -1;
    }
    return -1;
}

int condition_signal(Condition* cond) {
    if (!cond) return -1;
    auto cppCond = static_cast<BMS::Thread::Condition*>(cond->userData);
    if (cppCond) {
        cppCond->nativeCond->notify_one();
        return 0;
    }
    return -1;
}

int condition_broadcast(Condition* cond) {
    if (!cond) return -1;
    auto cppCond = static_cast<BMS::Thread::Condition*>(cond->userData);
    if (cppCond) {
        cppCond->nativeCond->notify_all();
        return 0;
    }
    return -1;
}

bool condition_has_waiters(Condition* cond) {
    return cond ? cond->waitCount > 0 : false;
}

uint32_t condition_get_wait_count(Condition* cond) {
    return cond ? cond->waitCount : 0;
}

// Semaphore wrappers
Semaphore* semaphore_create(int32_t initialValue, int32_t maxValue) {
    auto cppSem = BMS::Thread::CreateSemaphore(initialValue, maxValue);
    if (!cppSem) return nullptr;

    Semaphore* cSem = (Semaphore*)calloc(1, sizeof(Semaphore));
    if (cSem) {
        cSem->id = cppSem->id;
        cSem->name = strdup(cppSem->name.c_str());
        cSem->value = initialValue;
        cSem->maxValue = maxValue;
        cSem->userData = cppSem.get();
    }
    return cSem;
}

Semaphore* semaphore_create_ex(SemaphoreType type, const char* name,
                               int32_t initialValue, int32_t maxValue) {
    return semaphore_create(initialValue, maxValue);
}

void semaphore_destroy(Semaphore* sem) {
    if (!sem) return;
    free(sem->name);
    free(sem);
}

int semaphore_wait(Semaphore* sem) {
    if (!sem) return -1;
    auto cppSem = static_cast<BMS::Thread::Semaphore*>(sem->userData);
    if (cppSem) {
        std::unique_lock<std::mutex> lock(cppSem->mutex);
        cppSem->condition.wait(lock, [cppSem]() { return cppSem->value > 0; });
        cppSem->value--;
        return 0;
    }
    return -1;
}

int semaphore_trywait(Semaphore* sem) {
    if (!sem) return -1;
    auto cppSem = static_cast<BMS::Thread::Semaphore*>(sem->userData);
    if (cppSem) {
        std::lock_guard<std::mutex> lock(cppSem->mutex);
        if (cppSem->value > 0) {
            cppSem->value--;
            return 0;
        }
    }
    return -1;
}

int semaphore_timedwait(Semaphore* sem, uint32_t timeoutMs) {
    if (!sem) return -1;
    auto cppSem = static_cast<BMS::Thread::Semaphore*>(sem->userData);
    if (cppSem) {
        std::unique_lock<std::mutex> lock(cppSem->mutex);
        auto status = cppSem->condition.wait_for(lock,
            std::chrono::milliseconds(timeoutMs),
            [cppSem]() { return cppSem->value > 0; });
        if (status) {
            cppSem->value--;
            return 0;
        }
    }
    return -1;
}

int semaphore_post(Semaphore* sem) {
    if (!sem) return -1;
    auto cppSem = static_cast<BMS::Thread::Semaphore*>(sem->userData);
    if (cppSem) {
        std::lock_guard<std::mutex> lock(cppSem->mutex);
        if (cppSem->value < cppSem->maxValue) {
            cppSem->value++;
            cppSem->condition.notify_one();
            return 0;
        }
    }
    return -1;
}

int semaphore_get_value(Semaphore* sem) {
    if (!sem) return -1;
    auto cppSem = static_cast<BMS::Thread::Semaphore*>(sem->userData);
    if (cppSem) {
        std::lock_guard<std::mutex> lock(cppSem->mutex);
        return cppSem->value;
    }
    return -1;
}

uint32_t semaphore_get_wait_count(Semaphore* sem) {
    return sem ? sem->waitCount : 0;
}

// Thread pool wrappers
ThreadPool* thread_pool_create(ThreadPoolType type, uint32_t minThreads, uint32_t maxThreads) {
    return thread_pool_create_ex(NULL, type, minThreads, maxThreads);
}

ThreadPool* thread_pool_create_ex(const char* name, ThreadPoolType type,
                                  uint32_t minThreads, uint32_t maxThreads) {
    std::string poolName = name ? name : "";
    auto cppPool = BMS::Thread::CreateThreadPool(poolName, 
        (BMS::Thread::ThreadPoolType)type, minThreads, maxThreads);
    if (!cppPool) return nullptr;

    ThreadPool* cPool = (ThreadPool*)calloc(1, sizeof(ThreadPool));
    if (cPool) {
        cPool->id = cppPool->id;
        cPool->name = strdup(cppPool->name.c_str());
        cPool->type = type;
        cPool->minThreads = minThreads;
        cPool->maxThreads = maxThreads;
        cPool->userData = cppPool.get();
    }
    return cPool;
}

void thread_pool_destroy(ThreadPool* pool) {
    if (!pool) return;
    auto cppPool = static_cast<BMS::Thread::ThreadPool*>(pool->userData);
    if (cppPool) {
        cppPool->Shutdown();
    }
    free(pool->name);
    free(pool);
}

int thread_pool_submit(ThreadPool* pool, ThreadPoolTask task, void* arg) {
    if (!pool || !task) return -1;
    auto cppPool = static_cast<BMS::Thread::ThreadPool*>(pool->userData);
    if (cppPool && cppPool->running) {
        BMS::Thread::SubmitTask(cppPool, [task, arg]() { task(arg); });
        return 0;
    }
    return -1;
}

int thread_pool_submit_ex(ThreadPool* pool, ThreadPoolTask task, void* arg, uint32_t priority) {
    return thread_pool_submit(pool, task, arg);
}

int thread_pool_wait(ThreadPool* pool) {
    if (!pool) return -1;
    auto cppPool = static_cast<BMS::Thread::ThreadPool*>(pool->userData);
    if (cppPool) {
        BMS::Thread::WaitForPool(cppPool);
        return 0;
    }
    return -1;
}

int thread_pool_shutdown(ThreadPool* pool) {
    if (!pool) return -1;
    auto cppPool = static_cast<BMS::Thread::ThreadPool*>(pool->userData);
    if (cppPool) {
        cppPool->Shutdown();
        return 0;
    }
    return -1;
}

int thread_pool_resize(ThreadPool* pool, uint32_t minThreads, uint32_t maxThreads) {
    if (!pool) return -1;
    auto cppPool = static_cast<BMS::Thread::ThreadPool*>(pool->userData);
    if (cppPool) {
        pool->minThreads = minThreads;
        pool->maxThreads = maxThreads;
        // Resize implementation would go here
        return 0;
    }
    return -1;
}

uint32_t thread_pool_get_thread_count(ThreadPool* pool) {
    if (!pool) return 0;
    auto cppPool = static_cast<BMS::Thread::ThreadPool*>(pool->userData);
    return cppPool ? cppPool->threads.size() : 0;
}

uint32_t thread_pool_get_active_count(ThreadPool* pool) {
    if (!pool) return 0;
    auto cppPool = static_cast<BMS::Thread::ThreadPool*>(pool->userData);
    return cppPool ? cppPool->activeThreads : 0;
}

uint32_t thread_pool_get_queue_size(ThreadPool* pool) {
    if (!pool) return 0;
    auto cppPool = static_cast<BMS::Thread::ThreadPool*>(pool->userData);
    if (cppPool) {
        std::lock_guard<std::mutex> lock(cppPool->queueMutex);
        return cppPool->taskQueue.size();
    }
    return 0;
}

// BMS Browser specific functions
int thread_bms_browser_start(void) {
    return 0;
}

void thread_bms_browser_stop(void) {}

int thread_bms_renderer_start(void) {
    return 0;
}

void thread_bms_renderer_stop(void) {}

int thread_bms_network_start(void) {
    return 0;
}

void thread_bms_network_stop(void) {}

int thread_bms_audio_start(void) {
    return 0;
}

void thread_bms_audio_stop(void) {}

int thread_bms_io_start(void) {
    return 0;
}

void thread_bms_io_stop(void) {}

ThreadPool* thread_bms_get_render_pool(void) {
    static ThreadPool* pool = NULL;
    if (!pool) {
        pool = thread_pool_create_ex("RenderPool", THREAD_POOL_TYPE_FIXED, 2, 4);
    }
    return pool;
}

ThreadPool* thread_bms_get_network_pool(void) {
    static ThreadPool* pool = NULL;
    if (!pool) {
        pool = thread_pool_create_ex("NetworkPool", THREAD_POOL_TYPE_CACHED, 4, 16);
    }
    return pool;
}

ThreadPool* thread_bms_get_audio_pool(void) {
    static ThreadPool* pool = NULL;
    if (!pool) {
        pool = thread_pool_create_ex("AudioPool", THREAD_POOL_TYPE_FIXED, 2, 4);
    }
    return pool;
}

ThreadPool* thread_bms_get_io_pool(void) {
    static ThreadPool* pool = NULL;
    if (!pool) {
        pool = thread_pool_create_ex("IOPool", THREAD_POOL_TYPE_CACHED, 2, 8);
    }
    return pool;
}

ThreadPool* thread_bms_get_default_pool(void) {
    static ThreadPool* pool = NULL;
    if (!pool) {
        pool = thread_pool_create_ex("DefaultPool", THREAD_POOL_TYPE_CACHED, 4, 16);
    }
    return pool;
}

void thread_print_stats(void) {
    BMS::Thread::ThreadSystem::GetInstance()->PrintStats();
}

void thread_print_all(void) {
    thread_print_stats();
}

const char* thread_status_to_string(ThreadStatus status) {
    switch (status) {
        case THREAD_STATUS_CREATED: return "Created";
        case THREAD_STATUS_READY: return "Ready";
        case THREAD_STATUS_RUNNING: return "Running";
        case THREAD_STATUS_BLOCKED: return "Blocked";
        case THREAD_STATUS_WAITING: return "Waiting";
        case THREAD_STATUS_SLEEPING: return "Sleeping";
        case THREAD_STATUS_TERMINATED: return "Terminated";
        case THREAD_STATUS_ERROR: return "Error";
        default: return "Unknown";
    }
}

const char* thread_priority_to_string(ThreadPriority priority) {
    switch (priority) {
        case THREAD_PRIORITY_IDLE: return "Idle";
        case THREAD_PRIORITY_LOWEST: return "Lowest";
        case THREAD_PRIORITY_BELOW_NORMAL: return "Below Normal";
        case THREAD_PRIORITY_NORMAL: return "Normal";
        case THREAD_PRIORITY_ABOVE_NORMAL: return "Above Normal";
        case THREAD_PRIORITY_HIGHEST: return "Highest";
        case THREAD_PRIORITY_TIME_CRITICAL: return "Time Critical";
        default: return "Unknown";
    }
}

// Helper function
static uint64_t get_time_ms(void) {
#ifdef _WIN32
    return (uint64_t)GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
#endif
}

} // extern "C"