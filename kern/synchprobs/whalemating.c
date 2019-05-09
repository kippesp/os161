/*
 * Copyright (c) 2001, 2002, 2009
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
 * Driver code is in kern/tests/synchprobs.c We will
 * replace that file. This file is yours to modify as you see fit.
 *
 * You should implement your solution to the whalemating problem below.
 */

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

/*
 * Called by the driver during initialization.
 */

typedef struct {
  int32_t ready;
  int32_t spent;
} whale_t;

static struct lock* whalelock = NULL;
static struct cv* malecv = NULL;
static struct cv* femalecv = NULL;
static whale_t whale_males = {0, 0};
static whale_t whale_females = {0, 0};
static whale_t whale_matchmakers = {0, 0};

void whalemating_init()
{
  whalelock = lock_create("whalelock");
  malecv = cv_create("malecv");
  femalecv = cv_create("famalecv");

  return;
}

/*
 * Called by the driver during teardown.
 */

void whalemating_cleanup()
{
  lock_destroy(whalelock);
  cv_destroy(malecv);
  cv_destroy(femalecv);

  return;
}

void male(uint32_t index)
{
  (void)index;
  /*
   * Implement this function by calling male_start and male_end when
   * appropriate.
   */

  male_start(index);

  lock_acquire(whalelock);
  whale_males.ready++;

  // Wait for matchmaker to wake
  cv_wait(malecv, whalelock);

  male_end(index);

  whale_males.ready--;
  whale_males.spent++;

  lock_release(whalelock);

  return;
}

void female(uint32_t index)
{
  (void)index;
  /*
   * Implement this function by calling female_start and female_end when
   * appropriate.
   */
  female_start(index);

  lock_acquire(whalelock);
  whale_females.ready++;

  // Wait for matchmaker to wake
  cv_wait(femalecv, whalelock);

  female_end(index);

  whale_females.ready--;
  whale_females.spent++;

  lock_release(whalelock);

  return;
}

void matchmaker(uint32_t index)
{
  (void)index;
  /*
   * Implement this function by calling matchmaker_start and matchmaker_end
   * when appropriate.
   */
  matchmaker_start(index);

  lock_acquire(whalelock);
  whale_matchmakers.ready++;

  KASSERT(whale_males.ready > 0);
  KASSERT(whale_females.ready > 0);

  // Wake a male and female whale
  cv_signal(malecv, whalelock);
  cv_signal(femalecv, whalelock);

  matchmaker_end(index);

  whale_matchmakers.ready--;
  whale_matchmakers.spent++;

  lock_release(whalelock);

  return;
}
