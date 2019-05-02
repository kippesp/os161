/*
 * All the contents of this file are overwritten during automated
 * testing. Please consider this before changing anything in this file.
 */

#include <types.h>
#include <lib.h>
#include <clock.h>
#include <thread.h>
#include <synch.h>
#include <test.h>
#include <kern/test161.h>
#include <spinlock.h>

/*
 * Use these stubs to test your reader-writer locks.
 */

#define CREATELOOPS 8
#define NTHREADS_R 50
#define NTHREADS_W 10
#define NREADLOOPS 500
#define NWRITELOOPS 20

static bool test_status = TEST161_FAIL;
static struct rwlock* testrwlock = NULL;
static struct semaphore* donesem = NULL;
static struct semaphore* donewriter = NULL;

struct spinlock status_lock;

typedef struct {
  int values[NTHREADS_R];
  int sum;
} rwtest_s1_t;

static rwtest_s1_t rwtest_s1;

const char* test_message;

static bool failif(bool condition)
{
  if (condition) {
    spinlock_acquire(&status_lock);
    test_status = TEST161_FAIL;
    spinlock_release(&status_lock);
  }
  return condition;
}

static void rwlocktestthread_r(void* junk, unsigned long num)
{
  (void)junk;
  (void)num;

  int j;

  for (j = 0; j < NREADLOOPS; j++) {
    kprintf_t(".");
    rwlock_acquire_read(testrwlock);

    random_yielder(4);

    int i;
    int sum = 0;

    for (i = 0; i < NTHREADS_R; i++) {
      sum += rwtest_s1.values[i];
    }

    if (sum != rwtest_s1.sum) {
      goto fail;
    }

    rwlock_release_read(testrwlock);
  }

  V(donesem);
  return;

fail:
  rwlock_release_read(testrwlock);
#if 0
fail2:
#endif
  failif(true);
  V(donesem);
  return;
}

static void rwlocktestthread_w(void* junk, unsigned long num)
{
  (void)junk;
  (void)num;

  int j;
  for (j = 0; j < NWRITELOOPS; j++) {
    rwlock_acquire_write(testrwlock);

    int i;
    int sum = 0;

    random_yielder(4);

    // Verify existing
    for (i = 0; i < NTHREADS_R; i++) {
      sum += rwtest_s1.values[i];
    }

    if (sum != rwtest_s1.sum) {
      goto fail;
    }

    // Update data
    rwtest_s1.sum = 0;
    for (i = 0; i < NTHREADS_R; i++) {
      rwtest_s1.values[i] = j + num + i;
      rwtest_s1.sum += rwtest_s1.values[i];
    }

    rwlock_release_write(testrwlock);
  }

  V(donewriter);
  V(donesem);
  return;

fail:
  rwlock_release_write(testrwlock);
#if 0
fail2:
#endif
  failif(true);
  V(donesem);
  return;
}

// multiple readers
// single writer
int rwtest(int nargs, char** args)
{
  (void)nargs;
  (void)args;

  int i, result;

  kprintf_n("Starting rwt1...\n");
  for (i = 0; i < CREATELOOPS; i++) {
    kprintf_t(".");
    testrwlock = rwlock_create("testrwlock");
    if (testrwlock == NULL) {
      panic("rwt1: lock_create failed\n");
    }
    donesem = sem_create("donesem", 0);
    if (donesem == NULL) {
      panic("rwt1: sem_create failed\n");
    }
    if (i != CREATELOOPS - 1) {
      rwlock_destroy(testrwlock);
      sem_destroy(donesem);
    }
  }
  spinlock_init(&status_lock);
  test_status = TEST161_SUCCESS;

  // Create NTHREADS_R readers
  for (i = 0; i < NTHREADS_R; i++) {
    kprintf_t(".");
    result =
        thread_fork("synchtest", NULL, rwlocktestthread_r, &rwtest_s1, i + 1);
    if (result) {
      panic("rwt1: thread_fork failed: %s\n", strerror(result));
    }
  }

  donewriter = sem_create("donewriter", 0);
  if (donewriter == NULL) {
    panic("rwt2: sem_create failed\n");
  }

  // Create a single writer
  kprintf_t(".");
  result = thread_fork("synchtest", NULL, rwlocktestthread_w, &rwtest_s1, 5);
  if (result) {
    panic("rwt1: thread_fork failed: %s\n", strerror(result));
  }

  P(donewriter);

  for (i = 0; i < NTHREADS_R; i++) {
    kprintf_t(".");
    P(donesem);
  }

  P(donesem);

  rwlock_destroy(testrwlock);
  sem_destroy(donesem);
  testrwlock = NULL;
  donesem = NULL;

  sem_destroy(donewriter);
  donewriter = NULL;

  kprintf_t("\n");
  success(test_status, SECRET, "rwt1");

  return 0;
}

// single writer
// multiple readers
int rwtest2(int nargs, char** args)
{
  (void)nargs;
  (void)args;

  int i, result;

  kprintf_n("Starting rwt2...\n");
  for (i = 0; i < CREATELOOPS; i++) {
    kprintf_t(".");
    testrwlock = rwlock_create("testrwlock");
    if (testrwlock == NULL) {
      panic("rwt2: lock_create failed\n");
    }
    donesem = sem_create("donesem", 0);
    if (donesem == NULL) {
      panic("rwt2: sem_create failed\n");
    }
    if (i != CREATELOOPS - 1) {
      rwlock_destroy(testrwlock);
      sem_destroy(donesem);
    }
  }
  spinlock_init(&status_lock);
  test_status = TEST161_SUCCESS;

  donewriter = sem_create("donewriter", 0);
  if (donewriter == NULL) {
    panic("rwt2: sem_create failed\n");
  }

  // Create a single writer
  kprintf_t(".");
  result = thread_fork("synchtest", NULL, rwlocktestthread_w, &rwtest_s1, 5);
  if (result) {
    panic("rwt1: thread_fork failed: %s\n", strerror(result));
  }

  // Create NTHREADS_R readers
  for (i = 0; i < NTHREADS_R; i++) {
    kprintf_t(".");
    result =
        thread_fork("synchtest", NULL, rwlocktestthread_r, &rwtest_s1, i + 1);
    if (result) {
      panic("rwt1: thread_fork failed: %s\n", strerror(result));
    }
  }

  P(donewriter);

  for (i = 0; i < NTHREADS_R; i++) {
    kprintf_t(".");
    P(donesem);
  }

  P(donesem);

  rwlock_destroy(testrwlock);
  sem_destroy(donesem);
  testrwlock = NULL;
  donesem = NULL;

  sem_destroy(donewriter);
  donewriter = NULL;

  kprintf_t("\n");
  success(test_status, SECRET, "rwt2");

  return 0;
}

int rwtest3(int nargs, char** args)
{
  (void)nargs;
  (void)args;

  //kprintf_n("rwt3 unimplemented\n");
  //success(TEST161_SUCCESS, SECRET, "rwt3");
  panic("rwt3: NOOP test\n");

  success(test_status, SECRET, "rwt3");

  return 0;
}

int rwtest4(int nargs, char** args)
{
  (void)nargs;
  (void)args;

  kprintf_n("Starting rwt4...\n");

  //kprintf_n("rwt4 unimplemented\n");
  //success(TEST161_SUCCESS, SECRET, "rwt4");
  KASSERT(0 == 1);
  panic("rwt4: NOOP test\n");

  success(test_status, SECRET, "rwt4");

  return 0;
}

int rwtest5(int nargs, char** args)
{
  (void)nargs;
  (void)args;

  //kprintf_n("rwt5 unimplemented\n");
  //success(TEST161_SUCCESS, SECRET, "rwt5");
  panic("rwt5: NOOP test failed\n");

  success(test_status, SECRET, "rwt5");

  return 0;
}
