// thread.c - BMS Thread System Implementation
#include "thread.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

// ============================================================================
// Internal Structures
// ============================================================================

typedef struct ThreadSystem {
    bool initialized;
    uint32_t threadCounter;
    uint32_t mutexCounter;
    uint32_t condCounter;
    uint32_t semCounter;
    uint32_t poolCounter;
    uint32_t tlsCounter;
    uint32_t barrierCounter;
    
    Thread* threadList;
    Mutex* mutexList;
    Condition* condList;
    Semaphore* semList;
    ThreadPool* poolList;
    ThreadLocal* tlsList;
    
    Thread* mainThread;
    Mutex* systemMutex;
    bool shutdown;
} ThreadSystem;

static ThreadSystem g_threadSystem = {0};

// ============================================================================
// Internal Helper Functions
// ============================================================================

static uint32_t generate_id(void) {
    static uint32_t counter = 0;
    return ++counter;
}

static char* str_dup(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char* result = (char*)malloc(len);
    if (result) {
        memcpy(result, str, len);
    }
    return result;
}

static uint64_t get_time_ms(void) {
#ifdef _WIN32
    return (uint64_t)GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
#endif
}

static void lock_system(void) {
    if (g_threadSystem.systemMutex) {
        mutex_lock(g_threadSystem.systemMutex);
    }
}

static void unlock_system(void) {
    if (g_threadSystem.systemMutex) {
        mutex_unlock(g_threadSystem.systemMutex);
    }
}

// ============================================================================
// Platform-Specific Thread Functions
// ============================================================================

#ifdef _WIN32

typedef struct {
    ThreadEntry entry;
    void* arg;
    Thread* thread;
} WinThreadData;

static DWORD WINAPI win_thread_entry(LPVOID lpParam) {
    WinThreadData* data = (WinThreadData*)lpParam;
    Thread* thread = data->thread;
    
    // Store current thread
    thread->handle = GetCurrentThread();
    thread->status = THREAD_STATUS_RUNNING;
    thread->startTime = get_time_ms();
    
    // Call entry function
    thread->result = data->entry(data->arg);
    
    thread->status = THREAD_STATUS_TERMINATED;
    thread->endTime = get_time_ms();
    
    free(data);
    return 0;
}

static int platform_thread_create(Thread* thread) {
    WinThreadData* data = (WinThreadData*)malloc(sizeof(WinThreadData));
    if (!data) return -1;
    
    data->entry = thread->entry;
    data->arg = thread->arg;
    data->thread = thread;
    
    HANDLE handle = CreateThread(NULL, thread->stackSize, win_thread_entry, data, 0, NULL);
    if (!handle) {
        free(data);
        return -1;
    }
    
    thread->handle = handle;
    return 0;
}

static int platform_thread_join(Thread* thread) {
    if (!thread->handle) return -1;
    DWORD result = WaitForSingleObject(thread->handle, INFINITE);
    if (result != WAIT_OBJECT_0) return -1;
    CloseHandle(thread->handle);
    thread->handle = NULL;
    return 0;
}

static int platform_thread_detach(Thread* thread) {
    if (!thread->handle) return -1;
    CloseHandle(thread->handle);
    thread->handle = NULL;
    thread->detached = true;
    return 0;
}

static int platform_thread_set_priority(Thread* thread, ThreadPriority priority) {
    if (!thread->handle) return -1;
    
    int winPriority;
    switch (priority) {
        case THREAD_PRIORITY_IDLE: winPriority = THREAD_PRIORITY_IDLE; break;
        case THREAD_PRIORITY_LOWEST: winPriority = THREAD_PRIORITY_LOWEST; break;
        case THREAD_PRIORITY_BELOW_NORMAL: winPriority = THREAD_PRIORITY_BELOW_NORMAL; break;
        case THREAD_PRIORITY_NORMAL: winPriority = THREAD_PRIORITY_NORMAL; break;
        case THREAD_PRIORITY_ABOVE_NORMAL: winPriority = THREAD_PRIORITY_ABOVE_NORMAL; break;
        case THREAD_PRIORITY_HIGHEST: winPriority = THREAD_PRIORITY_HIGHEST; break;
        case THREAD_PRIORITY_TIME_CRITICAL: winPriority = THREAD_PRIORITY_TIME_CRITICAL; break;
        default: return -1;
    }
    
    return SetThreadPriority(thread->handle, winPriority) ? 0 : -1;
}

static void platform_sleep(uint32_t ms) {
    Sleep(ms);
}

static void platform_yield(void) {
    SwitchToThread();
}

static uint32_t platform_current_id(void) {
    return (uint32_t)GetCurrentThreadId();
}

#else // POSIX

typedef struct {
    ThreadEntry entry;
    void* arg;
    Thread* thread;
} PThreadData;

static void* pthread_entry(void* arg) {
    PThreadData* data = (PThreadData*)arg;
    Thread* thread = data->thread;
    
    // Store current thread
    thread->handle = pthread_self();
    thread->status = THREAD_STATUS_RUNNING;
    thread->startTime = get_time_ms();
    
    // Call entry function
    thread->result = data->entry(data->arg);
    
    thread->status = THREAD_STATUS_TERMINATED;
    thread->endTime = get_time_ms();
    
    free(data);
    return NULL;
}

static int platform_thread_create(Thread* thread) {
    PThreadData* data = (PThreadData*)malloc(sizeof(PThreadData));
    if (!data) return -1;
    
    data->entry = thread->entry;
    data->arg = thread->arg;
    data->thread = thread;
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    
    if (thread->stackSize > 0) {
        pthread_attr_setstacksize(&attr, thread->stackSize);
    }
    
    if (thread->detached) {
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    }
    
    int result = pthread_create((pthread_t*)&thread->handle, &attr, pthread_entry, data);
    pthread_attr_destroy(&attr);
    
    if (result != 0) {
        free(data);
        return -1;
    }
    
    return 0;
}

static int platform_thread_join(Thread* thread) {
    if (!thread->handle) return -1;
    int result = pthread_join(*(pthread_t*)&thread->handle, NULL);
    thread->handle = NULL;
    return result == 0 ? 0 : -1;
}

static int platform_thread_detach(Thread* thread) {
    if (!thread->handle) return -1;
    int result = pthread_detach(*(pthread_t*)&thread->handle);
    thread->handle = NULL;
    thread->detached = true;
    return result == 0 ? 0 : -1;
}

static int platform_thread_set_priority(Thread* thread, ThreadPriority priority) {
    if (!thread->handle) return -1;
    
    struct sched_param param;
    int policy;
    int minPriority = sched_get_priority_min(SCHED_OTHER);
    int maxPriority = sched_get_priority_max(SCHED_OTHER);
    
    int priorityValue = minPriority + 
        ((maxPriority - minPriority) * priority) / THREAD_PRIORITY_TIME_CRITICAL;
    
    param.sched_priority = priorityValue;
    return pthread_setschedparam(*(pthread_t*)&thread->handle, SCHED_OTHER, &param);
}

static void platform_sleep(uint32_t ms) {
    usleep(ms * 1000);
}

static void platform_yield(void) {
    sched_yield();
}

static uint32_t platform_current_id(void) {
    return (uint32_t)pthread_self();
}

#endif

// ============================================================================
// Thread System Initialization
// ============================================================================

int thread_system_init(void) {
    if (g_threadSystem.initialized) {
        return 0;
    }
    
    printf("[BMS Thread] Initializing thread system...\n");
    
    memset(&g_threadSystem, 0, sizeof(ThreadSystem));
    
    // Create system mutex
    g_threadSystem.systemMutex = mutex_create();
    if (!g_threadSystem.systemMutex) {
        printf("[BMS Thread] Failed to create system mutex\n");
        return -1;
    }
    
    g_threadSystem.initialized = true;
    
    // Create main thread
    g_threadSystem.mainThread = thread_current();
    if (g_threadSystem.mainThread) {
        g_threadSystem.mainThread->name = str_dup("Main Thread");
    }
    
    printf("[BMS Thread] Thread system initialized\n");
    return 0;
}

void thread_system_shutdown(void) {
    if (!g_threadSystem.initialized) {
        return;
    }
    
    printf("[BMS Thread] Shutting down thread system...\n");
    
    g_threadSystem.shutdown = true;
    
    // Clean up thread pools
    ThreadPool* pool = g_threadSystem.poolList;
    while (pool) {
        ThreadPool* next = pool->next;
        thread_pool_shutdown(pool);
        pool = next;
    }
    
    // Clean up threads
    Thread* thread = g_threadSystem.threadList;
    while (thread) {
        Thread* next = thread->next;
        if (thread != g_threadSystem.mainThread) {
            thread_join(thread);
            thread_destroy(thread);
        }
        thread = next;
    }
    
    // Clean up mutexes
    Mutex* mutex = g_threadSystem.mutexList;
    while (mutex) {
        Mutex* next = mutex->next;
        mutex_destroy(mutex);
        mutex = next;
    }
    
    // Clean up conditions
    Condition* cond = g_threadSystem.condList;
    while (cond) {
        Condition* next = cond->next;
        condition_destroy(cond);
        cond = next;
    }
    
    // Clean up semaphores
    Semaphore* sem = g_threadSystem.semList;
    while (sem) {
        Semaphore* next = sem->next;
        semaphore_destroy(sem);
        sem = next;
    }
    
    // Clean up TLS
    ThreadLocal* tls = g_threadSystem.tlsList;
    while (tls) {
        ThreadLocal* next = tls->next;
        thread_local_destroy(tls);
        tls = next;
    }
    
    // Clean up system mutex
    if (g_threadSystem.systemMutex) {
        mutex_destroy(g_threadSystem.systemMutex);
        g_threadSystem.systemMutex = NULL;
    }
    
    g_threadSystem.initialized = false;
    printf("[BMS Thread] Thread system shut down\n");
}

bool thread_system_is_initialized(void) {
    return g_threadSystem.initialized;
}

const char* thread_get_version(void) {
    return "BMS Thread System v1.0.0";
}

// ============================================================================
// Thread Creation and Management
// ============================================================================

Thread* thread_create(ThreadEntry entry, void* arg) {
    return thread_create_ex(entry, arg, NULL, 0);
}

Thread* thread_create_ex(ThreadEntry entry, void* arg, const char* name, size_t stackSize) {
    if (!entry) return NULL;
    
    Thread* thread = (Thread*)calloc(1, sizeof(Thread));
    if (!thread) return NULL;
    
    thread->id = generate_id();
    thread->entry = entry;
    thread->arg = arg;
    thread->stackSize = stackSize > 0 ? stackSize : 1024 * 1024; // 1MB default
    thread->priority = THREAD_PRIORITY_NORMAL;
    thread->status = THREAD_STATUS_CREATED;
    thread->creationTime = get_time_ms();
    
    if (name) {
        thread->name = str_dup(name);
    } else {
        char defaultName[32];
        snprintf(defaultName, sizeof(defaultName), "Thread-%u", thread->id);
        thread->name = str_dup(defaultName);
    }
    
    // Platform-specific thread creation
    if (platform_thread_create(thread) != 0) {
        free(thread->name);
        free(thread);
        return NULL;
    }
    
    // Add to thread list
    lock_system();
    thread->next = g_threadSystem.threadList;
    g_threadSystem.threadList = thread;
    unlock_system();
    
    printf("[BMS Thread] Created thread: %s (ID: %u)\n", thread->name, thread->id);
    
    return thread;
}

void thread_destroy(Thread* thread) {
    if (!thread) return;
    
    // Remove from list
    lock_system();
    Thread** prev = &g_threadSystem.threadList;
    while (*prev) {
        if (*prev == thread) {
            *prev = thread->next;
            break;
        }
        prev = &(*prev)->next;
    }
    unlock_system();
    
    free(thread->name);
    free(thread);
}

int thread_start(Thread* thread) {
    if (!thread) return -1;
    if (thread->status != THREAD_STATUS_CREATED) return -1;
    
    // Platform-specific start already done in create
    thread->status = THREAD_STATUS_READY;
    return 0;
}

int thread_join(Thread* thread) {
    if (!thread) return -1;
    if (thread->detached) return -1;
    if (thread->status == THREAD_STATUS_TERMINATED) return 0;
    
    return platform_thread_join(thread);
}

int thread_detach(Thread* thread) {
    if (!thread) return -1;
    if (thread->status == THREAD_STATUS_TERMINATED) return 0;
    
    return platform_thread_detach(thread);
}

int thread_cancel(Thread* thread) {
    if (!thread) return -1;
    
#ifdef _WIN32
    if (!thread->handle) return -1;
    return TerminateThread(thread->handle, 0) ? 0 : -1;
#else
    return pthread_cancel(*(pthread_t*)&thread->handle);
#endif
}

int thread_terminate(Thread* thread) {
    if (!thread) return -1;
    return thread_cancel(thread);
}

// ============================================================================
// Thread Control Functions
// ============================================================================

void thread_sleep(uint32_t ms) {
    platform_sleep(ms);
}

void thread_yield(void) {
    platform_yield();
}

void thread_exit(void* result) {
    Thread* current = thread_current();
    if (current) {
        current->result = result;
        current->status = THREAD_STATUS_TERMINATED;
    }
#ifdef _WIN32
    ExitThread(0);
#else
    pthread_exit(NULL);
#endif
}

int thread_set_priority(Thread* thread, ThreadPriority priority) {
    if (!thread) return -1;
    if (priority < 0 || priority > THREAD_PRIORITY_TIME_CRITICAL) return -1;
    
    thread->priority = priority;
    return platform_thread_set_priority(thread, priority);
}

ThreadPriority thread_get_priority(Thread* thread) {
    return thread ? thread->priority : THREAD_PRIORITY_NORMAL;
}

int thread_set_affinity(Thread* thread, uint32_t coreMask) {
    if (!thread) return -1;
    
    thread->coreAffinity = coreMask;
    
#ifdef _WIN32
    if (thread->handle) {
        return SetThreadAffinityMask(thread->handle, coreMask) ? 0 : -1;
    }
#else
    if (thread->handle) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        for (int i = 0; i < 32; i++) {
            if (coreMask & (1 << i)) {
                CPU_SET(i, &cpuset);
            }
        }
        return pthread_setaffinity_np(*(pthread_t*)&thread->handle, 
                                      sizeof(cpu_set_t), &cpuset) == 0 ? 0 : -1;
    }
#endif
    return 0;
}

uint32_t thread_get_affinity(Thread* thread) {
    return thread ? thread->coreAffinity : 0;
}

int thread_set_name(Thread* thread, const char* name) {
    if (!thread) return -1;
    
    free(thread->name);
    thread->name = name ? str_dup(name) : NULL;
    return 0;
}

const char* thread_get_name(Thread* thread) {
    return thread ? thread->name : NULL;
}

ThreadStatus thread_get_status(Thread* thread) {
    return thread ? thread->status : THREAD_STATUS_ERROR;
}

uint32_t thread_get_id(Thread* thread) {
    return thread ? thread->id : 0;
}

uint64_t thread_get_cpu_time(Thread* thread) {
    return thread ? thread->cpuTime : 0;
}

// ============================================================================
// Current Thread Functions
// ============================================================================

Thread* thread_current(void) {
    uint32_t currentId = platform_current_id();
    
    lock_system();
    Thread* thread = g_threadSystem.threadList;
    while (thread) {
#ifdef _WIN32
        if (GetThreadId(thread->handle) == currentId) {
#else
        if (*(pthread_t*)&thread->handle == pthread_self()) {
#endif
            unlock_system();
            return thread;
        }
        thread = thread->next;
    }
    unlock_system();
    
    // If not found, create a new thread object for this thread
    Thread* newThread = (Thread*)calloc(1, sizeof(Thread));
    if (newThread) {
        newThread->id = generate_id();
        newThread->status = THREAD_STATUS_RUNNING;
        newThread->name = str_dup("Current Thread");
#ifdef _WIN32
        newThread->handle = GetCurrentThread();
#else
        newThread->handle = (void*)pthread_self();
#endif
        
        lock_system();
        newThread->next = g_threadSystem.threadList;
        g_threadSystem.threadList = newThread;
        unlock_system();
        
        return newThread;
    }
    
    return NULL;
}

uint32_t thread_current_id(void) {
    return platform_current_id();
}

void thread_current_exit(void* result) {
    Thread* current = thread_current();
    if (current) {
        current->result = result;
        current->status = THREAD_STATUS_TERMINATED;
    }
#ifdef _WIN32
    ExitThread(0);
#else
    pthread_exit(NULL);
#endif
}

int thread_current_set_priority(ThreadPriority priority) {
    Thread* current = thread_current();
    return current ? thread_set_priority(current, priority) : -1;
}

ThreadPriority thread_current_get_priority(void) {
    Thread* current = thread_current();
    return current ? thread_get_priority(current) : THREAD_PRIORITY_NORMAL;
}

int thread_current_set_name(const char* name) {
    Thread* current = thread_current();
    return current ? thread_set_name(current, name) : -1;
}

const char* thread_current_get_name(void) {
    Thread* current = thread_current();
    return current ? thread_get_name(current) : NULL;
}

// ============================================================================
// Mutex Functions
// ============================================================================

Mutex* mutex_create(void) {
    return mutex_create_ex(MUTEX_TYPE_NORMAL, NULL);
}

Mutex* mutex_create_ex(MutexType type, const char* name) {
    Mutex* mutex = (Mutex*)calloc(1, sizeof(Mutex));
    if (!mutex) return NULL;
    
    mutex->id = generate_id();
    mutex->type = type;
    
    if (name) {
        mutex->name = str_dup(name);
    } else {
        char defaultName[32];
        snprintf(defaultName, sizeof(defaultName), "Mutex-%u", mutex->id);
        mutex->name = str_dup(defaultName);
    }
    
#ifdef _WIN32
    InitializeCriticalSection(&mutex->handle);
#else
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    
    switch (type) {
        case MUTEX_TYPE_RECURSIVE:
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
            break;
        case MUTEX_TYPE_ERROR_CHECK:
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
            break;
        case MUTEX_TYPE_ADAPTIVE:
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ADAPTIVE_NP);
            break;
        default:
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
            break;
    }
    
    pthread_mutex_init(&mutex->handle, &attr);
    pthread_mutexattr_destroy(&attr);
#endif
    
    lock_system();
    mutex->next = g_threadSystem.mutexList;
    g_threadSystem.mutexList = mutex;
    unlock_system();
    
    return mutex;
}

void mutex_destroy(Mutex* mutex) {
    if (!mutex) return;
    
    // Remove from list
    lock_system();
    Mutex** prev = &g_threadSystem.mutexList;
    while (*prev) {
        if (*prev == mutex) {
            *prev = mutex->next;
            break;
        }
        prev = &(*prev)->next;
    }
    unlock_system();
    
#ifdef _WIN32
    DeleteCriticalSection(&mutex->handle);
#else
    pthread_mutex_destroy(&mutex->handle);
#endif
    
    free(mutex->name);
    free(mutex);
}

int mutex_lock(Mutex* mutex) {
    if (!mutex) return -1;
    
#ifdef _WIN32
    EnterCriticalSection(&mutex->handle);
    mutex->owner = thread_current();
    mutex->recursionCount++;
#else
    int result = pthread_mutex_lock(&mutex->handle);
    if (result == 0) {
        mutex->owner = thread_current();
        mutex->recursionCount++;
    }
    return result;
#endif
    return 0;
}

int mutex_trylock(Mutex* mutex) {
    if (!mutex) return -1;
    
#ifdef _WIN32
    if (TryEnterCriticalSection(&mutex->handle)) {
        mutex->owner = thread_current();
        mutex->recursionCount++;
        return 0;
    }
    return -1;
#else
    int result = pthread_mutex_trylock(&mutex->handle);
    if (result == 0) {
        mutex->owner = thread_current();
        mutex->recursionCount++;
        return 0;
    }
    return -1;
#endif
}

int mutex_unlock(Mutex* mutex) {
    if (!mutex) return -1;
    
    mutex->recursionCount--;
    mutex->owner = NULL;
    
#ifdef _WIN32
    LeaveCriticalSection(&mutex->handle);
#else
    return pthread_mutex_unlock(&mutex->handle);
#endif
    return 0;
}

int mutex_timedlock(Mutex* mutex, uint32_t timeoutMs) {
    if (!mutex) return -1;
    
    uint64_t startTime = get_time_ms();
    while (get_time_ms() - startTime < timeoutMs) {
        if (mutex_trylock(mutex) == 0) {
            return 0;
        }
        thread_sleep(1);
    }
    
    return -1;
}

bool mutex_is_locked(Mutex* mutex) {
    return mutex ? mutex->recursionCount > 0 : false;
}

Thread* mutex_get_owner(Mutex* mutex) {
    return mutex ? mutex->owner : NULL;
}

uint32_t mutex_get_wait_count(Mutex* mutex) {
    return mutex ? mutex->waitCount : 0;
}

// ============================================================================
// Condition Variable Functions
// ============================================================================

Condition* condition_create(void) {
    return condition_create_ex(CONDITION_TYPE_NORMAL, NULL);
}

Condition* condition_create_ex(ConditionType type, const char* name) {
    Condition* cond = (Condition*)calloc(1, sizeof(Condition));
    if (!cond) return NULL;
    
    cond->id = generate_id();
    cond->type = type;
    
    if (name) {
        cond->name = str_dup(name);
    } else {
        char defaultName[32];
        snprintf(defaultName, sizeof(defaultName), "Cond-%u", cond->id);
        cond->name = str_dup(defaultName);
    }
    
#ifdef _WIN32
    InitializeConditionVariable(&cond->handle);
#else
    pthread_cond_init(&cond->handle, NULL);
#endif
    
    lock_system();
    cond->next = g_threadSystem.condList;
    g_threadSystem.condList = cond;
    unlock_system();
    
    return cond;
}

void condition_destroy(Condition* cond) {
    if (!cond) return;
    
    // Remove from list
    lock_system();
    Condition** prev = &g_threadSystem.condList;
    while (*prev) {
        if (*prev == cond) {
            *prev = cond->next;
            break;
        }
        prev = &(*prev)->next;
    }
    unlock_system();
    
#ifdef _WIN32
    // No explicit destroy needed for CONDITION_VARIABLE
#else
    pthread_cond_destroy(&cond->handle);
#endif
    
    free(cond->name);
    free(cond);
}

int condition_wait(Condition* cond, Mutex* mutex) {
    if (!cond || !mutex) return -1;
    
    cond->waitCount++;
#ifdef _WIN32
    SleepConditionVariableCS(&cond->handle, &mutex->handle, INFINITE);
#else
    pthread_cond_wait(&cond->handle, &mutex->handle);
#endif
    cond->waitCount--;
    
    return 0;
}

int condition_timedwait(Condition* cond, Mutex* mutex, uint32_t timeoutMs) {
    if (!cond || !mutex) return -1;
    
#ifdef _WIN32
    if (SleepConditionVariableCS(&cond->handle, &mutex->handle, timeoutMs)) {
        return 0;
    }
    return -1;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeoutMs / 1000;
    ts.tv_nsec += (timeoutMs % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000;
    }
    return pthread_cond_timedwait(&cond->handle, &mutex->handle, &ts);
#endif
}

int condition_signal(Condition* cond) {
    if (!cond) return -1;
    
#ifdef _WIN32
    WakeConditionVariable(&cond->handle);
#else
    pthread_cond_signal(&cond->handle);
#endif
    return 0;
}

int condition_broadcast(Condition* cond) {
    if (!cond) return -1;
    
#ifdef _WIN32
    WakeAllConditionVariable(&cond->handle);
#else
    pthread_cond_broadcast(&cond->handle);
#endif
    return 0;
}

bool condition_has_waiters(Condition* cond) {
    return cond ? cond->waitCount > 0 : false;
}

uint32_t condition_get_wait_count(Condition* cond) {
    return cond ? cond->waitCount : 0;
}

// ============================================================================
// Semaphore Functions
// ============================================================================

Semaphore* semaphore_create(int32_t initialValue, int32_t maxValue) {
    return semaphore_create_ex(SEMAPHORE_TYPE_COUNTING, NULL, initialValue, maxValue);
}

Semaphore* semaphore_create_ex(SemaphoreType type, const char* name, 
                               int32_t initialValue, int32_t maxValue) {
    Semaphore* sem = (Semaphore*)calloc(1, sizeof(Semaphore));
    if (!sem) return NULL;
    
    sem->id = generate_id();
    sem->type = type;
    sem->value = initialValue;
    sem->maxValue = maxValue;
    
    if (name) {
        sem->name = str_dup(name);
    } else {
        char defaultName[32];
        snprintf(defaultName, sizeof(defaultName), "Sem-%u", sem->id);
        sem->name = str_dup(defaultName);
    }
    
#ifdef _WIN32
    sem->handle = CreateSemaphore(NULL, initialValue, maxValue, NULL);
#else
    sem_init(&sem->handle, 0, initialValue);
#endif
    
    lock_system();
    sem->next = g_threadSystem.semList;
    g_threadSystem.semList = sem;
    unlock_system();
    
    return sem;
}

void semaphore_destroy(Semaphore* sem) {
    if (!sem) return;
    
    // Remove from list
    lock_system();
    Semaphore** prev = &g_threadSystem.semList;
    while (*prev) {
        if (*prev == sem) {
            *prev = sem->next;
            break;
        }
        prev = &(*prev)->next;
    }
    unlock_system();
    
#ifdef _WIN32
    CloseHandle(sem->handle);
#else
    sem_destroy(&sem->handle);
#endif
    
    free(sem->name);
    free(sem);
}

int semaphore_wait(Semaphore* sem) {
    if (!sem) return -1;
    
#ifdef _WIN32
    return WaitForSingleObject(sem->handle, INFINITE) == WAIT_OBJECT_0 ? 0 : -1;
#else
    return sem_wait(&sem->handle);
#endif
}

int semaphore_trywait(Semaphore* sem) {
    if (!sem) return -1;
    
#ifdef _WIN32
    return WaitForSingleObject(sem->handle, 0) == WAIT_OBJECT_0 ? 0 : -1;
#else
    return sem_trywait(&sem->handle);
#endif
}

int semaphore_timedwait(Semaphore* sem, uint32_t timeoutMs) {
    if (!sem) return -1;
    
#ifdef _WIN32
    return WaitForSingleObject(sem->handle, timeoutMs) == WAIT_OBJECT_0 ? 0 : -1;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeoutMs / 1000;
    ts.tv_nsec += (timeoutMs % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000;
    }
    return sem_timedwait(&sem->handle, &ts);
#endif
}

int semaphore_post(Semaphore* sem) {
    if (!sem) return -1;
    
#ifdef _WIN32
    return ReleaseSemaphore(sem->handle, 1, NULL) ? 0 : -1;
#else
    return sem_post(&sem->handle);
#endif
}

int semaphore_get_value(Semaphore* sem) {
    if (!sem) return -1;
    
#ifdef _WIN32
    LONG currentCount;
    ReleaseSemaphore(sem->handle, 0, &currentCount);
    return currentCount;
#else
    int sval;
    sem_getvalue(&sem->handle, &sval);
    return sval;
#endif
}

uint32_t semaphore_get_wait_count(Semaphore* sem) {
    return sem ? sem->waitCount : 0;
}

// ============================================================================
// Thread Pool Functions
// ============================================================================

typedef struct {
    ThreadPoolTask task;
    void* arg;
    uint32_t priority;
} PoolTask;

ThreadPool* thread_pool_create(ThreadPoolType type, uint32_t minThreads, uint32_t maxThreads) {
    return thread_pool_create_ex(NULL, type, minThreads, maxThreads);
}

ThreadPool* thread_pool_create_ex(const char* name, ThreadPoolType type, 
                                  uint32_t minThreads, uint32_t maxThreads) {
    ThreadPool* pool = (ThreadPool*)calloc(1, sizeof(ThreadPool));
    if (!pool) return NULL;
    
    pool->id = generate_id();
    pool->type = type;
    pool->minThreads = minThreads > 0 ? minThreads : 1;
    pool->maxThreads = maxThreads > pool->minThreads ? maxThreads : pool->minThreads * 2;
    pool->running = true;
    pool->maxQueueSize = 10000;
    
    if (name) {
        pool->name = str_dup(name);
    } else {
        char defaultName[32];
        snprintf(defaultName, sizeof(defaultName), "Pool-%u", pool->id);
        pool->name = str_dup(defaultName);
    }
    
    // Create queue mutex and condition
    pool->queueMutex = mutex_create_ex(MUTEX_TYPE_NORMAL, "PoolQueueMutex");
    pool->queueCond = condition_create_ex(CONDITION_TYPE_NORMAL, "PoolQueueCond");
    
    if (!pool->queueMutex || !pool->queueCond) {
        free(pool->name);
        free(pool);
        return NULL;
    }
    
    // Allocate queue
    pool->queue = (void**)calloc(pool->maxQueueSize, sizeof(void*));
    pool->queueArgs = (void**)calloc(pool->maxQueueSize, sizeof(void*));
    
    if (!pool->queue || !pool->queueArgs) {
        free(pool->queue);
        free(pool->queueArgs);
        free(pool->name);
        free(pool);
        return NULL;
    }
    
    // Create initial threads
    pool->threads = (Thread**)calloc(pool->minThreads, sizeof(Thread*));
    if (!pool->threads) {
        free(pool->queue);
        free(pool->queueArgs);
        free(pool->name);
        free(pool);
        return NULL;
    }
    
    for (uint32_t i = 0; i < pool->minThreads; i++) {
        char threadName[64];
        snprintf(threadName, sizeof(threadName), "%s-Worker-%u", pool->name, i);
        
        Thread* thread = thread_create_ex(thread_pool_worker, pool, threadName, 0);
        if (thread) {
            pool->threads[pool->threadCount++] = thread;
            thread_start(thread);
        }
    }
    
    lock_system();
    pool->next = g_threadSystem.poolList;
    g_threadSystem.poolList = pool;
    unlock_system();
    
    printf("[BMS Thread] Created thread pool: %s (min: %u, max: %u)\n", 
           pool->name, pool->minThreads, pool->maxThreads);
    
    return pool;
}

// Worker thread function
static void* thread_pool_worker(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    
    while (pool->running) {
        PoolTask* task = NULL;
        
        mutex_lock(pool->queueMutex);
        
        // Wait for tasks
        while (pool->running && pool->queueSize == 0) {
            condition_wait(pool->queueCond, pool->queueMutex);
        }
        
        if (!pool->running) {
            mutex_unlock(pool->queueMutex);
            break;
        }
        
        // Get task from queue
        if (pool->queueSize > 0) {
            task = (PoolTask*)pool->queue[pool->queueHead];
            pool->queueHead = (pool->queueHead + 1) % pool->maxQueueSize;
            pool->queueSize--;
        }
        
        mutex_unlock(pool->queueMutex);
        
        // Execute task
        if (task) {
            pool->activeThreads++;
            task->task(task->arg);
            pool->activeThreads--;
            free(task);
        }
    }
    
    return NULL;
}

int thread_pool_submit(ThreadPool* pool, ThreadPoolTask task, void* arg) {
    return thread_pool_submit_ex(pool, task, arg, 0);
}

int thread_pool_submit_ex(ThreadPool* pool, ThreadPoolTask task, void* arg, uint32_t priority) {
    if (!pool || !task || !pool->running) return -1;
    
    PoolTask* poolTask = (PoolTask*)malloc(sizeof(PoolTask));
    if (!poolTask) return -1;
    
    poolTask->task = task;
    poolTask->arg = arg;
    poolTask->priority = priority;
    
    mutex_lock(pool->queueMutex);
    
    // Check if queue is full
    if (pool->queueSize >= pool->maxQueueSize) {
        mutex_unlock(pool->queueMutex);
        free(poolTask);
        return -1;
    }
    
    // Add to queue
    pool->queue[pool->queueTail] = poolTask;
    pool->queueTail = (pool->queueTail + 1) % pool->maxQueueSize;
    pool->queueSize++;
    
    condition_signal(pool->queueCond);
    mutex_unlock(pool->queueMutex);
    
    return 0;
}

int thread_pool_wait(ThreadPool* pool) {
    if (!pool) return -1;
    
    while (true) {
        mutex_lock(pool->queueMutex);
        bool hasTasks = pool->queueSize > 0 || pool->activeThreads > 0;
        mutex_unlock(pool->queueMutex);
        
        if (!hasTasks) break;
        thread_sleep(10);
    }
    
    return 0;
}

int thread_pool_shutdown(ThreadPool* pool) {
    if (!pool) return -1;
    
    pool->running = false;
    pool->shutdown = true;
    
    // Wake up all workers
    condition_broadcast(pool->queueCond);
    
    // Wait for all threads to finish
    for (uint32_t i = 0; i < pool->threadCount; i++) {
        if (pool->threads[i]) {
            thread_join(pool->threads[i]);
            thread_destroy(pool->threads[i]);
        }
    }
    
    // Clean up queue
    while (pool->queueSize > 0) {
        PoolTask* task = (PoolTask*)pool->queue[pool->queueHead];
        if (task) {
            free(task);
        }
        pool->queueHead = (pool->queueHead + 1) % pool->maxQueueSize;
        pool->queueSize--;
    }
    
    // Clean up resources
    free(pool->threads);
    free(pool->queue);
    free(pool->queueArgs);
    
    if (pool->queueMutex) {
        mutex_destroy(pool->queueMutex);
    }
    if (pool->queueCond) {
        condition_destroy(pool->queueCond);
    }
    
    // Remove from list
    lock_system();
    ThreadPool** prev = &g_threadSystem.poolList;
    while (*prev) {
        if (*prev == pool) {
            *prev = pool->next;
            break;
        }
        prev = &(*prev)->next;
    }
    unlock_system();
    
    free(pool->name);
    free(pool);
    
    return 0;
}

int thread_pool_resize(ThreadPool* pool, uint32_t minThreads, uint32_t maxThreads) {
    if (!pool) return -1;
    
    pool->minThreads = minThreads;
    pool->maxThreads = maxThreads;
    
    // Create new threads if needed
    while (pool->threadCount < pool->minThreads) {
        char threadName[64];
        snprintf(threadName, sizeof(threadName), "%s-Worker-%u", pool->name, pool->threadCount);
        
        Thread* thread = thread_create_ex(thread_pool_worker, pool, threadName, 0);
        if (thread) {
            pool->threads = (Thread**)realloc(pool->threads, 
                                              (pool->threadCount + 1) * sizeof(Thread*));
            if (pool->threads) {
                pool->threads[pool->threadCount++] = thread;
                thread_start(thread);
            } else {
                thread_destroy(thread);
                break;
            }
        } else {
            break;
        }
    }
    
    return 0;
}

uint32_t thread_pool_get_thread_count(ThreadPool* pool) {
    return pool ? pool->threadCount : 0;
}

uint32_t thread_pool_get_active_count(ThreadPool* pool) {
    return pool ? pool->activeThreads : 0;
}

uint32_t thread_pool_get_queue_size(ThreadPool* pool) {
    return pool ? pool->queueSize : 0;
}

// ============================================================================
// Thread Local Storage
// ============================================================================

ThreadLocal* thread_local_create(void (*destructor)(void*)) {
    return thread_local_create_ex(NULL, destructor);
}

ThreadLocal* thread_local_create_ex(const char* name, void (*destructor)(void*)) {
    ThreadLocal* tls = (ThreadLocal*)calloc(1, sizeof(ThreadLocal));
    if (!tls) return NULL;
    
    tls->id = generate_id();
    tls->destructor = destructor;
    
    if (name) {
        tls->name = str_dup(name);
    } else {
        char defaultName[32];
        snprintf(defaultName, sizeof(defaultName), "TLS-%u", tls->id);
        tls->name = str_dup(defaultName);
    }
    
    lock_system();
    tls->next = g_threadSystem.tlsList;
    g_threadSystem.tlsList = tls;
    unlock_system();
    
    return tls;
}

void thread_local_destroy(ThreadLocal* tls) {
    if (!tls) return;
    
    // Call destructor if value exists
    if (tls->value && tls->destructor) {
        tls->destructor(tls->value);
    }
    
    // Remove from list
    lock_system();
    ThreadLocal** prev = &g_threadSystem.tlsList;
    while (*prev) {
        if (*prev == tls) {
            *prev = tls->next;
            break;
        }
        prev = &(*prev)->next;
    }
    unlock_system();
    
    free(tls->name);
    free(tls);
}

int thread_local_set(ThreadLocal* tls, void* value) {
    if (!tls) return -1;
    
    // Clean up old value
    if (tls->value && tls->destructor) {
        tls->destructor(tls->value);
    }
    
    tls->value = value;
    return 0;
}

void* thread_local_get(ThreadLocal* tls) {
    return tls ? tls->value : NULL;
}

int thread_local_remove(ThreadLocal* tls) {
    if (!tls) return -1;
    
    if (tls->value && tls->destructor) {
        tls->destructor(tls->value);
    }
    tls->value = NULL;
    
    return 0;
}

// ============================================================================
// Thread Barrier
// ============================================================================

typedef struct Barrier {
    uint32_t id;
    uint32_t count;
    uint32_t waiting;
    Mutex* mutex;
    Condition* cond;
    bool reset;
} Barrier;

static Barrier* g_barriers = NULL;

int thread_barrier_create(uint32_t count) {
    Barrier* barrier = (Barrier*)calloc(1, sizeof(Barrier));
    if (!barrier) return -1;
    
    barrier->id = generate_id();
    barrier->count = count;
    barrier->waiting = 0;
    barrier->mutex = mutex_create();
    barrier->cond = condition_create();
    
    if (!barrier->mutex || !barrier->cond) {
        free(barrier);
        return -1;
    }
    
    barrier->next = g_barriers;
    g_barriers = barrier;
    
    return barrier->id;
}

void thread_barrier_destroy(int barrierId) {
    Barrier** prev = &g_barriers;
    while (*prev) {
        if ((*prev)->id == barrierId) {
            Barrier* barrier = *prev;
            *prev = barrier->next;
            mutex_destroy(barrier->mutex);
            condition_destroy(barrier->cond);
            free(barrier);
            return;
        }
        prev = &(*prev)->next;
    }
}

int thread_barrier_wait(int barrierId) {
    Barrier* barrier = g_barriers;
    while (barrier) {
        if (barrier->id == barrierId) {
            mutex_lock(barrier->mutex);
            barrier->waiting++;
            
            if (barrier->waiting >= barrier->count) {
                barrier->waiting = 0;
                condition_broadcast(barrier->cond);
                mutex_unlock(barrier->mutex);
                return 1;
            } else {
                condition_wait(barrier->cond, barrier->mutex);
                mutex_unlock(barrier->mutex);
                return 0;
            }
        }
        barrier = barrier->next;
    }
    return -1;
}

// ============================================================================
// Thread Debugging
// ============================================================================

void thread_print_stats(void) {
    printf("\n=== BMS Thread System Statistics ===\n");
    printf("Threads: %u\n", thread_pool_get_thread_count(NULL));
    printf("Threads: %u\n", g_threadSystem.threadCounter);
    printf("Mutexes: %u\n", g_threadSystem.mutexCounter);
    printf("Conditions: %u\n", g_threadSystem.condCounter);
    printf("Semaphores: %u\n", g_threadSystem.semCounter);
    printf("Thread Pools: %u\n", g_threadSystem.poolCounter);
    printf("TLS Keys: %u\n", g_threadSystem.tlsCounter);
    printf("====================================\n");
}

void thread_print_all(void) {
    printf("\n=== BMS Thread List ===\n");
    
    lock_system();
    Thread* thread = g_threadSystem.threadList;
    while (thread) {
        printf("Thread %u: %s (Status: %s, Priority: %s, ID: %u)\n",
               thread->id,
               thread->name ? thread->name : "Unnamed",
               thread_status_to_string(thread->status),
               thread_priority_to_string(thread->priority),
               thread_get_id(thread));
        thread = thread->next;
    }
    unlock_system();
    
    printf("=======================\n");
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

// ============================================================================
// Thread Safety Utilities
// ============================================================================

void thread_safe_increment(volatile uint32_t* value) {
#ifdef _WIN32
    InterlockedIncrement(value);
#else
    __sync_add_and_fetch(value, 1);
#endif
}

uint32_t thread_safe_decrement(volatile uint32_t* value) {
#ifdef _WIN32
    return InterlockedDecrement(value);
#else
    return __sync_sub_and_fetch(value, 1);
#endif
}

uint32_t thread_safe_exchange(volatile uint32_t* target, uint32_t value) {
#ifdef _WIN32
    return InterlockedExchange(target, value);
#else
    return __sync_lock_test_and_set(target, value);
#endif
}

uint32_t thread_safe_compare_exchange(volatile uint32_t* target, uint32_t expected, uint32_t desired) {
#ifdef _WIN32
    return InterlockedCompareExchange(target, desired, expected);
#else
    return __sync_val_compare_and_swap(target, expected, desired);
#endif
}

// ============================================================================
// BMS Browser Specific Thread Functions
// ============================================================================

// Browser thread management
static Thread* g_browserThread = NULL;
static Thread* g_rendererThread = NULL;
static Thread* g_networkThread = NULL;
static Thread* g_audioThread = NULL;
static Thread* g_ioThread = NULL;

static ThreadPool* g_renderPool = NULL;
static ThreadPool* g_networkPool = NULL;
static ThreadPool* g_audioPool = NULL;
static ThreadPool* g_ioPool = NULL;
static ThreadPool* g_defaultPool = NULL;

int thread_bms_browser_start(void) {
    if (g_browserThread) return 0;
    
    g_browserThread = thread_create_ex(NULL, NULL, "BMS-Browser", 0);
    if (!g_browserThread) return -1;
    
    thread_set_priority(g_browserThread, THREAD_PRIORITY_ABOVE_NORMAL);
    thread_start(g_browserThread);
    
    printf("[BMS Thread] Browser thread started\n");
    return 0;
}

void thread_bms_browser_stop(void) {
    if (g_browserThread) {
        thread_terminate(g_browserThread);
        thread_join(g_browserThread);
        thread_destroy(g_browserThread);
        g_browserThread = NULL;
        printf("[BMS Thread] Browser thread stopped\n");
    }
}

int thread_bms_renderer_start(void) {
    if (g_rendererThread) return 0;
    
    g_rendererThread = thread_create_ex(NULL, NULL, "BMS-Renderer", 0);
    if (!g_rendererThread) return -1;
    
    thread_set_priority(g_rendererThread, THREAD_PRIORITY_HIGHEST);
    thread_start(g_rendererThread);
    
    printf("[BMS Thread] Renderer thread started\n");
    return 0;
}

void thread_bms_renderer_stop(void) {
    if (g_rendererThread) {
        thread_terminate(g_rendererThread);
        thread_join(g_rendererThread);
        thread_destroy(g_rendererThread);
        g_rendererThread = NULL;
        printf("[BMS Thread] Renderer thread stopped\n");
    }
}

int thread_bms_network_start(void) {
    if (g_networkThread) return 0;
    
    g_networkThread = thread_create_ex(NULL, NULL, "BMS-Network", 0);
    if (!g_networkThread) return -1;
    
    thread_set_priority(g_networkThread, THREAD_PRIORITY_ABOVE_NORMAL);
    thread_start(g_networkThread);
    
    printf("[BMS Thread] Network thread started\n");
    return 0;
}

void thread_bms_network_stop(void) {
    if (g_networkThread) {
        thread_terminate(g_networkThread);
        thread_join(g_networkThread);
        thread_destroy(g_networkThread);
        g_networkThread = NULL;
        printf("[BMS Thread] Network thread stopped\n");
    }
}

int thread_bms_audio_start(void) {
    if (g_audioThread) return 0;
    
    g_audioThread = thread_create_ex(NULL, NULL, "BMS-Audio", 0);
    if (!g_audioThread) return -1;
    
    thread_set_priority(g_audioThread, THREAD_PRIORITY_ABOVE_NORMAL);
    thread_start(g_audioThread);
    
    printf("[BMS Thread] Audio thread started\n");
    return 0;
}

void thread_bms_audio_stop(void) {
    if (g_audioThread) {
        thread_terminate(g_audioThread);
        thread_join(g_audioThread);
        thread_destroy(g_audioThread);
        g_audioThread = NULL;
        printf("[BMS Thread] Audio thread stopped\n");
    }
}

int thread_bms_io_start(void) {
    if (g_ioThread) return 0;
    
    g_ioThread = thread_create_ex(NULL, NULL, "BMS-IO", 0);
    if (!g_ioThread) return -1;
    
    thread_set_priority(g_ioThread, THREAD_PRIORITY_BELOW_NORMAL);
    thread_start(g_ioThread);
    
    printf("[BMS Thread] IO thread started\n");
    return 0;
}

void thread_bms_io_stop(void) {
    if (g_ioThread) {
        thread_terminate(g_ioThread);
        thread_join(g_ioThread);
        thread_destroy(g_ioThread);
        g_ioThread = NULL;
        printf("[BMS Thread] IO thread stopped\n");
    }
}

// Worker thread pools
ThreadPool* thread_bms_get_render_pool(void) {
    if (!g_renderPool) {
        g_renderPool = thread_pool_create_ex("RenderPool", THREAD_POOL_TYPE_FIXED, 2, 4);
    }
    return g_renderPool;
}

ThreadPool* thread_bms_get_network_pool(void) {
    if (!g_networkPool) {
        g_networkPool = thread_pool_create_ex("NetworkPool", THREAD_POOL_TYPE_CACHED, 4, 16);
    }
    return g_networkPool;
}

ThreadPool* thread_bms_get_audio_pool(void) {
    if (!g_audioPool) {
        g_audioPool = thread_pool_create_ex("AudioPool", THREAD_POOL_TYPE_FIXED, 2, 4);
    }
    return g_audioPool;
}

ThreadPool* thread_bms_get_io_pool(void) {
    if (!g_ioPool) {
        g_ioPool = thread_pool_create_ex("IOPool", THREAD_POOL_TYPE_CACHED, 2, 8);
    }
    return g_ioPool;
}

ThreadPool* thread_bms_get_default_pool(void) {
    if (!g_defaultPool) {
        g_defaultPool = thread_pool_create_ex("DefaultPool", THREAD_POOL_TYPE_CACHED, 4, 16);
    }
    return g_defaultPool;
}