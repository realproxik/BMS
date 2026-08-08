// thread.h - BMS Thread System Header
#ifndef BMS_THREAD_H
#define BMS_THREAD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Thread Enumerations
// ============================================================================

typedef enum {
    THREAD_PRIORITY_IDLE = 0,
    THREAD_PRIORITY_LOWEST = 1,
    THREAD_PRIORITY_BELOW_NORMAL = 2,
    THREAD_PRIORITY_NORMAL = 3,
    THREAD_PRIORITY_ABOVE_NORMAL = 4,
    THREAD_PRIORITY_HIGHEST = 5,
    THREAD_PRIORITY_TIME_CRITICAL = 6
} ThreadPriority;

typedef enum {
    THREAD_STATUS_CREATED = 0,
    THREAD_STATUS_READY = 1,
    THREAD_STATUS_RUNNING = 2,
    THREAD_STATUS_BLOCKED = 3,
    THREAD_STATUS_WAITING = 4,
    THREAD_STATUS_SLEEPING = 5,
    THREAD_STATUS_TERMINATED = 6,
    THREAD_STATUS_ERROR = 7
} ThreadStatus;

typedef enum {
    MUTEX_TYPE_NORMAL = 0,
    MUTEX_TYPE_RECURSIVE = 1,
    MUTEX_TYPE_ERROR_CHECK = 2,
    MUTEX_TYPE_ADAPTIVE = 3
} MutexType;

typedef enum {
    CONDITION_TYPE_NORMAL = 0,
    CONDITION_TYPE_TIMED = 1,
    CONDITION_TYPE_SIGNAL = 2
} ConditionType;

typedef enum {
    SEMAPHORE_TYPE_BINARY = 0,
    SEMAPHORE_TYPE_COUNTING = 1,
    SEMAPHORE_TYPE_FAIR = 2
} SemaphoreType;

typedef enum {
    THREAD_POOL_TYPE_FIXED = 0,
    THREAD_POOL_TYPE_CACHED = 1,
    THREAD_POOL_TYPE_SCHEDULED = 2
} ThreadPoolType;

// ============================================================================
// Thread Structures
// ============================================================================

typedef struct Thread {
    uint32_t id;
    char* name;
    ThreadPriority priority;
    ThreadStatus status;
    void* handle;
    void* (*entry)(void* arg);
    void* arg;
    void* result;
    size_t stackSize;
    uint64_t creationTime;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t cpuTime;
    uint32_t coreAffinity;
    bool detached;
    bool joinable;
    void* userData;
    struct Thread* next;
} Thread;

typedef struct Mutex {
    uint32_t id;
    char* name;
    MutexType type;
    void* handle;
    Thread* owner;
    uint32_t recursionCount;
    uint32_t waitCount;
    struct Mutex* next;
} Mutex;

typedef struct Condition {
    uint32_t id;
    char* name;
    ConditionType type;
    void* handle;
    Mutex* mutex;
    uint32_t waitCount;
    struct Condition* next;
} Condition;

typedef struct Semaphore {
    uint32_t id;
    char* name;
    SemaphoreType type;
    void* handle;
    int32_t value;
    int32_t maxValue;
    uint32_t waitCount;
    struct Semaphore* next;
} Semaphore;

typedef struct ThreadPool {
    uint32_t id;
    char* name;
    ThreadPoolType type;
    Thread** threads;
    uint32_t threadCount;
    uint32_t minThreads;
    uint32_t maxThreads;
    uint32_t idleThreads;
    uint32_t activeThreads;
    uint32_t queueSize;
    uint32_t maxQueueSize;
    void** queue;
    void** queueArgs;
    uint32_t queueHead;
    uint32_t queueTail;
    bool running;
    bool shutdown;
    Mutex* queueMutex;
    Condition* queueCond;
    struct ThreadPool* next;
} ThreadPool;

typedef struct ThreadLocal {
    uint32_t id;
    char* name;
    void* value;
    void (*destructor)(void*);
    struct ThreadLocal* next;
} ThreadLocal;

// ============================================================================
// Thread Callback Types
// ============================================================================

typedef void* (*ThreadEntry)(void* arg);
typedef void (*ThreadExitHandler)(void* arg);
typedef void (*ThreadCleanupHandler)(void* arg);
typedef void (*ThreadPoolTask)(void* arg);

// ============================================================================
// Thread System Functions
// ============================================================================

// System initialization
int thread_system_init(void);
void thread_system_shutdown(void);
bool thread_system_is_initialized(void);
const char* thread_get_version(void);

// Thread creation and management
Thread* thread_create(ThreadEntry entry, void* arg);
Thread* thread_create_ex(ThreadEntry entry, void* arg, const char* name, size_t stackSize);
void thread_destroy(Thread* thread);
int thread_start(Thread* thread);
int thread_join(Thread* thread);
int thread_detach(Thread* thread);
int thread_cancel(Thread* thread);
int thread_terminate(Thread* thread);

// Thread control
void thread_sleep(uint32_t ms);
void thread_yield(void);
void thread_exit(void* result);
int thread_set_priority(Thread* thread, ThreadPriority priority);
ThreadPriority thread_get_priority(Thread* thread);
int thread_set_affinity(Thread* thread, uint32_t coreMask);
uint32_t thread_get_affinity(Thread* thread);
int thread_set_name(Thread* thread, const char* name);
const char* thread_get_name(Thread* thread);
ThreadStatus thread_get_status(Thread* thread);
uint32_t thread_get_id(Thread* thread);
uint64_t thread_get_cpu_time(Thread* thread);

// Current thread functions
Thread* thread_current(void);
uint32_t thread_current_id(void);
void thread_current_exit(void* result);
int thread_current_set_priority(ThreadPriority priority);
ThreadPriority thread_current_get_priority(void);
int thread_current_set_name(const char* name);
const char* thread_current_get_name(void);

// Mutex functions
Mutex* mutex_create(void);
Mutex* mutex_create_ex(MutexType type, const char* name);
void mutex_destroy(Mutex* mutex);
int mutex_lock(Mutex* mutex);
int mutex_trylock(Mutex* mutex);
int mutex_unlock(Mutex* mutex);
int mutex_timedlock(Mutex* mutex, uint32_t timeoutMs);
bool mutex_is_locked(Mutex* mutex);
Thread* mutex_get_owner(Mutex* mutex);
uint32_t mutex_get_wait_count(Mutex* mutex);

// Condition variable functions
Condition* condition_create(void);
Condition* condition_create_ex(ConditionType type, const char* name);
void condition_destroy(Condition* cond);
int condition_wait(Condition* cond, Mutex* mutex);
int condition_timedwait(Condition* cond, Mutex* mutex, uint32_t timeoutMs);
int condition_signal(Condition* cond);
int condition_broadcast(Condition* cond);
bool condition_has_waiters(Condition* cond);
uint32_t condition_get_wait_count(Condition* cond);

// Semaphore functions
Semaphore* semaphore_create(int32_t initialValue, int32_t maxValue);
Semaphore* semaphore_create_ex(SemaphoreType type, const char* name, int32_t initialValue, int32_t maxValue);
void semaphore_destroy(Semaphore* sem);
int semaphore_wait(Semaphore* sem);
int semaphore_trywait(Semaphore* sem);
int semaphore_timedwait(Semaphore* sem, uint32_t timeoutMs);
int semaphore_post(Semaphore* sem);
int semaphore_get_value(Semaphore* sem);
uint32_t semaphore_get_wait_count(Semaphore* sem);

// Thread pool functions
ThreadPool* thread_pool_create(ThreadPoolType type, uint32_t minThreads, uint32_t maxThreads);
ThreadPool* thread_pool_create_ex(const char* name, ThreadPoolType type, uint32_t minThreads, uint32_t maxThreads);
void thread_pool_destroy(ThreadPool* pool);
int thread_pool_submit(ThreadPool* pool, ThreadPoolTask task, void* arg);
int thread_pool_submit_ex(ThreadPool* pool, ThreadPoolTask task, void* arg, uint32_t priority);
int thread_pool_wait(ThreadPool* pool);
int thread_pool_shutdown(ThreadPool* pool);
int thread_pool_resize(ThreadPool* pool, uint32_t minThreads, uint32_t maxThreads);
uint32_t thread_pool_get_thread_count(ThreadPool* pool);
uint32_t thread_pool_get_active_count(ThreadPool* pool);
uint32_t thread_pool_get_queue_size(ThreadPool* pool);

// Thread local storage
ThreadLocal* thread_local_create(void (*destructor)(void*));
ThreadLocal* thread_local_create_ex(const char* name, void (*destructor)(void*));
void thread_local_destroy(ThreadLocal* tls);
int thread_local_set(ThreadLocal* tls, void* value);
void* thread_local_get(ThreadLocal* tls);
int thread_local_remove(ThreadLocal* tls);

// Thread synchronization utilities
int thread_barrier_create(uint32_t count);
void thread_barrier_destroy(int barrierId);
int thread_barrier_wait(int barrierId);

// Thread debugging
void thread_print_stats(void);
void thread_print_all(void);
const char* thread_status_to_string(ThreadStatus status);
const char* thread_priority_to_string(ThreadPriority priority);

// ============================================================================
// BMS Browser Specific Thread Functions
// ============================================================================

// Browser thread management
int thread_bms_browser_start(void);
void thread_bms_browser_stop(void);
int thread_bms_renderer_start(void);
void thread_bms_renderer_stop(void);
int thread_bms_network_start(void);
void thread_bms_network_stop(void);
int thread_bms_audio_start(void);
void thread_bms_audio_stop(void);
int thread_bms_io_start(void);
void thread_bms_io_stop(void);

// Worker thread pools
ThreadPool* thread_bms_get_render_pool(void);
ThreadPool* thread_bms_get_network_pool(void);
ThreadPool* thread_bms_get_audio_pool(void);
ThreadPool* thread_bms_get_io_pool(void);
ThreadPool* thread_bms_get_default_pool(void);

// Thread safety utilities
void thread_safe_increment(volatile uint32_t* value);
uint32_t thread_safe_decrement(volatile uint32_t* value);
uint32_t thread_safe_exchange(volatile uint32_t* target, uint32_t value);
uint32_t thread_safe_compare_exchange(volatile uint32_t* target, uint32_t expected, uint32_t desired);

#ifdef __cplusplus
}
#endif

#endif // BMS_THREAD_H