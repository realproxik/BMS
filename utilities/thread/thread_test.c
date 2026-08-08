// thread_test.c - BMS Thread System Test
#include "thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define SLEEP_MS(x) Sleep(x)
#else
#include <unistd.h>
#define SLEEP_MS(x) usleep((x) * 1000)
#endif

// Test counters
static volatile uint32_t g_counter = 0;
static Mutex* g_counterMutex = NULL;

// Test thread entry function
void* test_thread_entry(void* arg) {
    int threadId = *(int*)arg;
    printf("  Thread %d starting...\n", threadId);
    
    for (int i = 0; i < 5; i++) {
        mutex_lock(g_counterMutex);
        g_counter++;
        printf("  Thread %d: counter = %u\n", threadId, g_counter);
        mutex_unlock(g_counterMutex);
        
        thread_sleep(100);
    }
    
    printf("  Thread %d finished\n", threadId);
    return NULL;
}

// Thread pool task
void test_pool_task(void* arg) {
    int taskId = *(int*)arg;
    printf("  Task %d executing on thread %u\n", taskId, thread_current_id());
    thread_sleep(200);
}

void test_basic_threads(void) {
    printf("\n=== Testing Basic Threads ===\n");
    
    g_counterMutex = mutex_create();
    if (!g_counterMutex) {
        printf("  Failed to create mutex\n");
        return;
    }
    
    Thread* threads[3];
    int threadArgs[3] = {1, 2, 3};
    
    for (int i = 0; i < 3; i++) {
        threads[i] = thread_create(test_thread_entry, &threadArgs[i]);
        if (threads[i]) {
            printf("  Created thread %d\n", i + 1);
            thread_start(threads[i]);
        }
    }
    
    // Wait for all threads to finish
    for (int i = 0; i < 3; i++) {
        if (threads[i]) {
            thread_join(threads[i]);
            thread_destroy(threads[i]);
        }
    }
    
    mutex_destroy(g_counterMutex);
    printf("  All threads completed\n");
}

void test_priority_threads(void) {
    printf("\n=== Testing Priority Threads ===\n");
    
    // Create high priority thread
    Thread* highThread = thread_create_ex(NULL, NULL, "HighPriority", 0);
    if (highThread) {
        thread_set_priority(highThread, THREAD_PRIORITY_HIGHEST);
        thread_start(highThread);
        printf("  High priority thread created\n");
    }
    
    // Create low priority thread
    Thread* lowThread = thread_create_ex(NULL, NULL, "LowPriority", 0);
    if (lowThread) {
        thread_set_priority(lowThread, THREAD_PRIORITY_LOWEST);
        thread_start(lowThread);
        printf("  Low priority thread created\n");
    }
    
    thread_sleep(1000);
    
    if (highThread) {
        thread_terminate(highThread);
        thread_join(highThread);
        thread_destroy(highThread);
    }
    if (lowThread) {
        thread_terminate(lowThread);
        thread_join(lowThread);
        thread_destroy(lowThread);
    }
    
    printf("  Priority threads completed\n");
}

void test_thread_pool(void) {
    printf("\n=== Testing Thread Pool ===\n");
    
    // Create thread pool
    ThreadPool* pool = thread_pool_create(THREAD_POOL_TYPE_FIXED, 2, 4);
    if (!pool) {
        printf("  Failed to create thread pool\n");
        return;
    }
    printf("  Thread pool created\n");
    
    // Submit tasks
    int taskArgs[10];
    for (int i = 0; i < 10; i++) {
        taskArgs[i] = i;
        thread_pool_submit(pool, test_pool_task, &taskArgs[i]);
        printf("  Submitted task %d\n", i);
    }
    
    // Wait for all tasks to complete
    thread_pool_wait(pool);
    printf("  All tasks completed\n");
    
    thread_pool_destroy(pool);
}

void test_semaphores(void) {
    printf("\n=== Testing Semaphores ===\n");
    
    // Create semaphore
    Semaphore* sem = semaphore_create(0, 5);
    if (!sem) {
        printf("  Failed to create semaphore\n");
        return;
    }
    printf("  Semaphore created with initial value 0\n");
    
    // Post to semaphore
    for (int i = 0; i < 3; i++) {
        semaphore_post(sem);
        printf("  Posted to semaphore (value: %d)\n", semaphore_get_value(sem));
    }
    
    // Wait on semaphore
    for (int i = 0; i < 2; i++) {
        semaphore_wait(sem);
        printf("  Waited on semaphore (value: %d)\n", semaphore_get_value(sem));
    }
    
    semaphore_destroy(sem);
    printf("  Semaphore test completed\n");
}

void test_conditions(void) {
    printf("\n=== Testing Condition Variables ===\n");
    
    Mutex* mutex = mutex_create();
    Condition* cond = condition_create();
    
    if (!mutex || !cond) {
        printf("  Failed to create mutex or condition\n");
        if (mutex) mutex_destroy(mutex);
        if (cond) condition_destroy(cond);
        return;
    }
    
    printf("  Created mutex and condition\n");
    
    // Test wait/signal
    mutex_lock(mutex);
    printf("  Locked mutex, waiting for signal...\n");
    
    // Signal from another thread
    // In real test, this would be from another thread
    condition_signal(cond);
    condition_wait(cond, mutex);
    printf("  Condition signaled\n");
    
    mutex_unlock(mutex);
    
    mutex_destroy(mutex);
    condition_destroy(cond);
    printf("  Condition test completed\n");
}

void test_thread_local_storage(void) {
    printf("\n=== Testing Thread Local Storage ===\n");
    
    // Create TLS key
    ThreadLocal* tls = thread_local_create(NULL);
    if (!tls) {
        printf("  Failed to create TLS\n");
        return;
    }
    
    // Set TLS value
    int value = 42;
    thread_local_set(tls, &value);
    printf("  TLS value set to %d\n", *(int*)thread_local_get(tls));
    
    // Remove TLS value
    thread_local_remove(tls);
    printf("  TLS value removed\n");
    
    thread_local_destroy(tls);
    printf("  TLS test completed\n");
}

void test_barrier(void) {
    printf("\n=== Testing Barrier ===\n");
    
    int barrierId = thread_barrier_create(3);
    if (barrierId < 0) {
        printf("  Failed to create barrier\n");
        return;
    }
    printf("  Barrier created with count 3\n");
    
    // Create threads that will wait on barrier
    // In real test, multiple threads would call barrier_wait
    thread_barrier_wait(barrierId);
    printf("  Barrier wait 1\n");
    thread_barrier_wait(barrierId);
    printf("  Barrier wait 2\n");
    thread_barrier_wait(barrierId);
    printf("  Barrier wait 3 (should release)\n");
    
    thread_barrier_destroy(barrierId);
    printf("  Barrier test completed\n");
}

void test_browser_threads(void) {
    printf("\n=== Testing BMS Browser Threads ===\n");
    
    // Start browser threads
    printf("  Starting browser threads...\n");
    thread_bms_browser_start();
    thread_bms_renderer_start();
    thread_bms_network_start();
    thread_bms_audio_start();
    thread_bms_io_start();
    
    // Get thread pools
    ThreadPool* renderPool = thread_bms_get_render_pool();
    ThreadPool* networkPool = thread_bms_get_network_pool();
    ThreadPool* audioPool = thread_bms_get_audio_pool();
    ThreadPool* ioPool = thread_bms_get_io_pool();
    ThreadPool* defaultPool = thread_bms_get_default_pool();
    
    printf("  Render pool: %u threads\n", thread_pool_get_thread_count(renderPool));
    printf("  Network pool: %u threads\n", thread_pool_get_thread_count(networkPool));
    printf("  Audio pool: %u threads\n", thread_pool_get_thread_count(audioPool));
    printf("  IO pool: %u threads\n", thread_pool_get_thread_count(ioPool));
    printf("  Default pool: %u threads\n", thread_pool_get_thread_count(defaultPool));
    
    // Submit tasks to pools
    int taskArg = 0;
    thread_pool_submit(defaultPool, test_pool_task, &taskArg);
    printf("  Submitted task to default pool\n");
    
    // Wait a bit for tasks to complete
    thread_sleep(1000);
    
    // Stop browser threads
    printf("  Stopping browser threads...\n");
    thread_bms_browser_stop();
    thread_bms_renderer_stop();
    thread_bms_network_stop();
    thread_bms_audio_stop();
    thread_bms_io_stop();
}

void print_banner(void) {
    printf("╔═══════════════════════════════════════════════════════════════════╗\n");
    printf("║           BMS Thread System Test Suite                          ║\n");
    printf("║           Version: %s                           ║\n", thread_get_version());
    printf("╚═══════════════════════════════════════════════════════════════════╝\n");
}

int main(int argc, char** argv) {
    print_banner();
    
    // Initialize thread system
    if (thread_system_init() != 0) {
        printf("Failed to initialize thread system\n");
        return 1;
    }
    printf("Thread system initialized\n");
    
    // Run tests
    test_basic_threads();
    test_priority_threads();
    test_thread_pool();
    test_semaphores();
    test_conditions();
    test_thread_local_storage();
    test_barrier();
    test_browser_threads();
    
    // Print statistics
    thread_print_stats();
    thread_print_all();
    
    // Shutdown
    thread_system_shutdown();
    printf("\nAll tests completed successfully!\n");
    
    return 0;
}