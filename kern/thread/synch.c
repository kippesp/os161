/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Synchronization primitives.
 * The specifications of the functions are in synch.h.
 */

#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <current.h>
#include <synch.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore* sem_create(const char* name, unsigned initial_count)
{
  struct semaphore* sem;

  sem = kmalloc(sizeof(*sem));
  if (sem == NULL) {
    return NULL;
  }

  sem->sem_name = kstrdup(name);
  if (sem->sem_name == NULL) {
    kfree(sem);
    return NULL;
  }

  sem->sem_wchan = wchan_create(sem->sem_name);
  if (sem->sem_wchan == NULL) {
    kfree(sem->sem_name);
    kfree(sem);
    return NULL;
  }

  spinlock_init(&sem->sem_lock);
  sem->sem_count = initial_count;

  return sem;
}

void sem_destroy(struct semaphore* sem)
{
  KASSERT(sem != NULL);

  /* wchan_cleanup will assert if anyone's waiting on it */
  spinlock_cleanup(&sem->sem_lock);
  wchan_destroy(sem->sem_wchan);
  kfree(sem->sem_name);
  kfree(sem);
}

void P(struct semaphore* sem)  // wait(S)
{
  KASSERT(sem != NULL);

  /*
   * May not block in an interrupt handler.
   *
   * For robustness, always check, even if we can actually
   * complete the P without blocking.
   */
  KASSERT(curthread->t_in_interrupt == false);

  /* Use the semaphore spinlock to protect the wchan as well. */
  spinlock_acquire(&sem->sem_lock);
  while (sem->sem_count == 0) {
    /*
     *
     * Note that we don't maintain strict FIFO ordering of
     * threads going through the semaphore; that is, we
     * might "get" it on the first try even if other
     * threads are waiting. Apparently according to some
     * textbooks semaphores must for some reason have
     * strict ordering. Too bad. :-)
     *
     * Exercise: how would you implement strict FIFO
     * ordering?
     */
    wchan_sleep(sem->sem_wchan, &sem->sem_lock);
  }
  KASSERT(sem->sem_count > 0);
  sem->sem_count--;
  spinlock_release(&sem->sem_lock);
}

void V(struct semaphore* sem)  // signal(S)
{
  KASSERT(sem != NULL);

  spinlock_acquire(&sem->sem_lock);

  sem->sem_count++;
  KASSERT(sem->sem_count > 0);
  wchan_wakeone(sem->sem_wchan, &sem->sem_lock);

  spinlock_release(&sem->sem_lock);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock* lock_create(const char* name)
{
  struct lock* lock;

  KASSERT(name != NULL);

  lock = kmalloc(sizeof(*lock));
  if (lock == NULL) {
    return NULL;
  }

  lock->lk_name = kstrdup(name);
  if (lock->lk_name == NULL) {
    kfree(lock);
    return NULL;
  }

  HANGMAN_LOCKABLEINIT(&lock->lk_hangman, lock->lk_name);

  // add stuff here as needed

  lock->owning_thread = NULL;
  lock->is_owned = 0;

  lock->lk_wchan = wchan_create(lock->lk_name);
  if (lock->lk_wchan == NULL) {
    kfree(lock->lk_name);
    kfree(lock);
    return NULL;
  }

  spinlock_init(&lock->lk_spinlock);

  return lock;
}

void lock_destroy(struct lock* lock)
{
  KASSERT(lock != NULL);

  // add stuff here as needed

  spinlock_acquire(&lock->lk_spinlock);

  KASSERT(wchan_isempty(lock->lk_wchan, &lock->lk_spinlock));
  KASSERT(lock->lk_name != NULL);
  KASSERT(lock->is_owned == 0);
  KASSERT(lock->owning_thread == NULL);

  wchan_destroy(lock->lk_wchan);

  spinlock_release(&lock->lk_spinlock);

  spinlock_cleanup(&lock->lk_spinlock);

  kfree(lock->lk_name);
  kfree(lock);
}

void lock_acquire(struct lock* lock)
{
  KASSERT(lock != NULL);
  KASSERT(curthread->t_in_interrupt == false);

  spinlock_acquire(&lock->lk_spinlock);

  /* Call this (atomically) before waiting for a lock */
  HANGMAN_WAIT(&curthread->t_hangman, &lock->lk_hangman);

  KASSERT(!((lock->is_owned == 1) && (lock->owning_thread == curthread)));

  while (lock->is_owned) {
    wchan_sleep(lock->lk_wchan, &lock->lk_spinlock);
  }

  KASSERT(lock->is_owned == 0);
  KASSERT(lock->owning_thread == NULL);

  lock->owning_thread = curthread;
  lock->is_owned = 1;

  /* Call this (atomically) once the lock is acquired */
  HANGMAN_ACQUIRE(&curthread->t_hangman, &lock->lk_hangman);

  spinlock_release(&lock->lk_spinlock);
}

void lock_release(struct lock* lock)
{
  KASSERT(lock != NULL);

  // Write this

  spinlock_acquire(&lock->lk_spinlock);

  KASSERT(lock->is_owned == 1);
  KASSERT(lock->owning_thread == curthread);

  wchan_wakeone(lock->lk_wchan, &lock->lk_spinlock);

  lock->owning_thread = NULL;
  lock->is_owned = 0;

  spinlock_release(&lock->lk_spinlock);

  /* Call this (atomically) when the lock is released */
  HANGMAN_RELEASE(&curthread->t_hangman, &lock->lk_hangman);
}

bool lock_do_i_hold(struct lock* lock)
{
  // Write this

  KASSERT(lock != NULL);
  KASSERT((lock->is_owned == 0) || (lock->is_owned == 1));

  /* Assume we can read splk_holder atomically enough for this to work */
  return ((lock->owning_thread == curthread) && (lock->is_owned == 1));
}

////////////////////////////////////////////////////////////
//
// CV (using Mesa semantics) the thread that calls cv_signal will awaken one
// thread waiting on the condition variable but will not block.

struct cv* cv_create(const char* name)
{
  struct cv* cv;

  cv = kmalloc(sizeof(*cv));
  if (cv == NULL) {
    return NULL;
  }

  cv->cv_name = kstrdup(name);
  if (cv->cv_name == NULL) {
    kfree(cv);
    return NULL;
  }

  // add stuff here as needed

  cv->cv_wchan = wchan_create(cv->cv_name);
  if (cv->cv_wchan == NULL) {
    kfree(cv->cv_name);
    kfree(cv);
    return NULL;
  }

  spinlock_init(&cv->cv_spinlock);

  return cv;
}

void cv_destroy(struct cv* cv)
{
  KASSERT(cv != NULL);

  // add stuff here as needed

  spinlock_acquire(&cv->cv_spinlock);

  KASSERT(wchan_isempty(cv->cv_wchan, &cv->cv_spinlock));
  wchan_destroy(cv->cv_wchan);
  kfree(cv->cv_name);

  spinlock_release(&cv->cv_spinlock);
  spinlock_cleanup(&cv->cv_spinlock);

  kfree(cv);
}

void cv_wait(struct cv* cv, struct lock* lock)
{
  // Write this

  KASSERT(cv != NULL);
  KASSERT(lock_do_i_hold(lock));

  // Atomic: release-sleep-acquire

  spinlock_acquire(&cv->cv_spinlock);
  lock_release(lock);
  wchan_sleep(cv->cv_wchan, &cv->cv_spinlock);
  spinlock_release(&cv->cv_spinlock);

  lock_acquire(lock);
}

void cv_signal(struct cv* cv, struct lock* lock)
{
  // Write this

  KASSERT(cv != NULL);
  KASSERT(lock_do_i_hold(lock));

  spinlock_acquire(&cv->cv_spinlock);
  wchan_wakeone(cv->cv_wchan, &cv->cv_spinlock);
  spinlock_release(&cv->cv_spinlock);
}

void cv_broadcast(struct cv* cv, struct lock* lock)
{
  // Write this

  KASSERT(cv != NULL);
  KASSERT(lock_do_i_hold(lock));

  spinlock_acquire(&cv->cv_spinlock);
  wchan_wakeall(cv->cv_wchan, &cv->cv_spinlock);
  spinlock_release(&cv->cv_spinlock);
}

struct rwlock* rwlock_create(const char* name)
{
  struct rwlock* rwlock;

  rwlock = kmalloc(sizeof(*rwlock));
  if (rwlock == NULL) {
    return NULL;
  }

  rwlock->rwlk_name = kstrdup(name);
  if (rwlock->rwlk_name == NULL) {
    kfree(rwlock);
    return NULL;
  }

  rwlock->rwlk_lock = lock_create(rwlock->rwlk_name);
  if (rwlock->rwlk_lock == NULL) {
    kfree(rwlock->rwlk_name);
    kfree(rwlock);
    return NULL;
  }

  rwlock->rwlk_semaphore = sem_create(rwlock->rwlk_name, MAX_RWLOCK_READERS);
  if (rwlock->rwlk_semaphore == NULL) {
    lock_destroy(rwlock->rwlk_lock);
    kfree(rwlock->rwlk_name);
    kfree(rwlock);
    return NULL;
  }

  return rwlock;
}

void rwlock_destroy(struct rwlock* rwlock)
{
  KASSERT(rwlock != NULL);

  lock_acquire(rwlock->rwlk_lock);

  kfree(rwlock->rwlk_name);
  sem_destroy(rwlock->rwlk_semaphore);

  lock_release(rwlock->rwlk_lock);
  lock_destroy(rwlock->rwlk_lock);

  kfree(rwlock);
}

void rwlock_acquire_read(struct rwlock* rwlock)
{
  KASSERT(rwlock != NULL);
  lock_acquire(rwlock->rwlk_lock);
  P(rwlock->rwlk_semaphore);
  lock_release(rwlock->rwlk_lock);
}

void rwlock_release_read(struct rwlock* rwlock)
{
  KASSERT(rwlock != NULL);
  V(rwlock->rwlk_semaphore);
}

void rwlock_acquire_write(struct rwlock* rwlock)
{
  KASSERT(rwlock != NULL);

  // acquire all the semaphores to write to block future readers
  lock_acquire(rwlock->rwlk_lock);
  int i;
  for (i = 0; i < MAX_RWLOCK_READERS; i++) {
    P(rwlock->rwlk_semaphore);
  }
  lock_release(rwlock->rwlk_lock);
}

void rwlock_release_write(struct rwlock* rwlock)
{
  KASSERT(rwlock != NULL);

  int i;
  for (i = 0; i < MAX_RWLOCK_READERS; i++) {
    V(rwlock->rwlk_semaphore);
  }
}
