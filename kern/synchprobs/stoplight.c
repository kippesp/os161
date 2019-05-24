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
 * Driver code is in kern/tests/synchprobs.c We will replace that file. This
 * file is yours to modify as you see fit.
 *
 * You should implement your solution to the stoplight problem below. The
 * quadrant and direction mappings for reference: (although the problem is, of
 * course, stable under rotation)
 *
 *   |0 |
 * -     --
 *    01  1
 * 3  32
 * --    --
 *   | 2|
 *
 * As way to think about it, assuming cars drive on the right: a car entering
 * the intersection from direction X will enter intersection quadrant X first.
 * The semantics of the problem are that once a car enters any quadrant it has
 * to be somewhere in the intersection until it calls leaveIntersection(),
 * which it should call while in the final quadrant.
 *
 * As an example, let's say a car approaches the intersection and needs to
 * pass through quadrants 0, 3 and 2. Once you call inQuadrant(0), the car is
 * considered in quadrant 0 until you call inQuadrant(3). After you call
 * inQuadrant(2), the car is considered in quadrant 2 until you call
 * leaveIntersection().
 *
 * You will probably want to write some helper functions to assist with the
 * mappings. Modular arithmetic can help, e.g. a car passing straight through
 * the intersection entering from direction X will leave towards direction
 * (X + 2) % 4, passing through quadrants X and (X + 3) % 4.  Boo-yah.
 *
 * Your solutions below should call the inQuadrant() and leaveIntersection()
 * functions in synchprobs.c to record their progress.
 */

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

#define NUM_QUADRANTS 4
#define UNKNOWN_CAR -1

static struct lock* stoplightlock;
bool quadrant_occupied[NUM_QUADRANTS];

static struct cv* intersectioncv[NUM_QUADRANTS];

// Define how interestion locations will be refered to relative to origin
typedef enum {
  ONCOMING_NEAR = 1,
  ONCOMING_FAR,
  AHEAD_NEAR,
  AHEAD_FAR
} relative_quadrant_from_origin_t;

static uint32_t get_relative_quadrant(
    uint32_t origin, relative_quadrant_from_origin_t relative_quadrant)
{
  uint32_t quadrant = -1;

  switch (relative_quadrant) {
    case ONCOMING_NEAR:
      quadrant = (origin + 1) % 4;
      break;
    case ONCOMING_FAR:
      quadrant = (origin + 2) % 4;
      break;
    case AHEAD_NEAR:
      quadrant = origin;
      break;
    case AHEAD_FAR:
      quadrant = (origin + 3) % 4;
      break;
  }

  return quadrant;
}

/*
 * Called by the driver during initialization.
 */

void stoplight_init()
{
  for (int i = 0; i < NUM_QUADRANTS; i++) {
  }
  quadrant_occupied[0] = false;
  quadrant_occupied[1] = false;
  quadrant_occupied[2] = false;
  quadrant_occupied[3] = false;

  intersectioncv[0] = cv_create("intersection0");
  intersectioncv[1] = cv_create("intersection1");
  intersectioncv[2] = cv_create("intersection2");
  intersectioncv[3] = cv_create("intersection3");

  stoplightlock = lock_create("stoplight");
}

/*
 * Called by the driver during teardown.
 */

void stoplight_cleanup()
{
  lock_destroy(stoplightlock);

  cv_destroy(intersectioncv[0]);
  cv_destroy(intersectioncv[1]);
  cv_destroy(intersectioncv[2]);
  cv_destroy(intersectioncv[3]);
}

void turnright(uint32_t origin, uint32_t index)
{
  /*
   * Implement this function.
   */

  //kprintf_n("%d starting right turn\n", index);

  uint32_t quadrant_t0 = get_relative_quadrant(origin, AHEAD_NEAR);

  uint32_t ahead_near = get_relative_quadrant(origin, AHEAD_NEAR);
  uint32_t ahead_far = get_relative_quadrant(origin, AHEAD_FAR);
  uint32_t oncoming_near = get_relative_quadrant(origin, ONCOMING_NEAR);
  uint32_t oncoming_far = get_relative_quadrant(origin, ONCOMING_FAR);

  lock_acquire(stoplightlock);

  // wait for required locations to be available
  while (quadrant_occupied[quadrant_t0]) {
    cv_wait(intersectioncv[origin], stoplightlock);
  }

  // reserve the space to occupy
  quadrant_occupied[quadrant_t0] = true;

  inQuadrant(quadrant_t0, index);

  cv_broadcast(intersectioncv[ahead_near], stoplightlock);
  cv_broadcast(intersectioncv[ahead_far], stoplightlock);
  cv_broadcast(intersectioncv[oncoming_near], stoplightlock);
  cv_broadcast(intersectioncv[oncoming_far], stoplightlock);

  lock_release(stoplightlock);
  random_yielder(4);
  lock_acquire(stoplightlock);

  leaveIntersection(index);
  quadrant_occupied[quadrant_t0] = false;

  cv_broadcast(intersectioncv[ahead_near], stoplightlock);
  cv_broadcast(intersectioncv[ahead_far], stoplightlock);
  cv_broadcast(intersectioncv[oncoming_near], stoplightlock);
  cv_broadcast(intersectioncv[oncoming_far], stoplightlock);

  lock_release(stoplightlock);
}

void gostraight(uint32_t origin, uint32_t index)
{
  /*
   * Implement this function.
   */

  //kprintf_n("%d starting straight thru\n", index);

  uint32_t quadrant_t0 = get_relative_quadrant(origin, AHEAD_NEAR);
  uint32_t quadrant_t1 = get_relative_quadrant(origin, AHEAD_FAR);

  uint32_t ahead_near = get_relative_quadrant(origin, AHEAD_NEAR);
  uint32_t ahead_far = get_relative_quadrant(origin, AHEAD_FAR);
  uint32_t oncoming_near = get_relative_quadrant(origin, ONCOMING_NEAR);
  uint32_t oncoming_far = get_relative_quadrant(origin, ONCOMING_FAR);

  lock_acquire(stoplightlock);

  // wait for required locations to be available
  while (quadrant_occupied[quadrant_t0] || quadrant_occupied[quadrant_t1]) {
    cv_wait(intersectioncv[origin], stoplightlock);
  }

  // reserve the space to occupy
  quadrant_occupied[quadrant_t0] = true;
  quadrant_occupied[quadrant_t1] = true;

  inQuadrant(quadrant_t0, index);

  cv_broadcast(intersectioncv[ahead_near], stoplightlock);
  cv_broadcast(intersectioncv[ahead_far], stoplightlock);
  cv_broadcast(intersectioncv[oncoming_near], stoplightlock);
  cv_broadcast(intersectioncv[oncoming_far], stoplightlock);

  lock_release(stoplightlock);
  random_yielder(4);
  lock_acquire(stoplightlock);

  inQuadrant(quadrant_t1, index);
  quadrant_occupied[quadrant_t0] = false;

  cv_broadcast(intersectioncv[ahead_near], stoplightlock);
  cv_broadcast(intersectioncv[ahead_far], stoplightlock);
  cv_broadcast(intersectioncv[oncoming_near], stoplightlock);
  cv_broadcast(intersectioncv[oncoming_far], stoplightlock);

  lock_release(stoplightlock);
  random_yielder(4);
  lock_acquire(stoplightlock);

  leaveIntersection(index);
  quadrant_occupied[quadrant_t1] = false;

  cv_broadcast(intersectioncv[ahead_near], stoplightlock);
  cv_broadcast(intersectioncv[ahead_far], stoplightlock);
  cv_broadcast(intersectioncv[oncoming_near], stoplightlock);
  cv_broadcast(intersectioncv[oncoming_far], stoplightlock);

  lock_release(stoplightlock);
}

void turnleft(uint32_t origin, uint32_t index)
{
  /*
   * Implement this function.
   */

  //kprintf_n("%d starting left turn\n", index);

  uint32_t quadrant_t0 = get_relative_quadrant(origin, AHEAD_NEAR);
  uint32_t quadrant_t1 = get_relative_quadrant(origin, AHEAD_FAR);
  uint32_t quadrant_t2 = get_relative_quadrant(origin, ONCOMING_FAR);

  uint32_t ahead_near = get_relative_quadrant(origin, AHEAD_NEAR);
  uint32_t ahead_far = get_relative_quadrant(origin, AHEAD_FAR);
  uint32_t oncoming_near = get_relative_quadrant(origin, ONCOMING_NEAR);
  uint32_t oncoming_far = get_relative_quadrant(origin, ONCOMING_FAR);

  lock_acquire(stoplightlock);

  // wait for required locations to be available
  while (quadrant_occupied[quadrant_t0] || quadrant_occupied[quadrant_t1] ||
         quadrant_occupied[quadrant_t2]) {
    cv_wait(intersectioncv[origin], stoplightlock);
  }

  // reserve the space to occupy
  quadrant_occupied[quadrant_t0] = true;
  quadrant_occupied[quadrant_t1] = true;
  quadrant_occupied[quadrant_t2] = true;

  inQuadrant(quadrant_t0, index);

  cv_broadcast(intersectioncv[ahead_near], stoplightlock);
  cv_broadcast(intersectioncv[ahead_far], stoplightlock);
  cv_broadcast(intersectioncv[oncoming_near], stoplightlock);
  cv_broadcast(intersectioncv[oncoming_far], stoplightlock);

  lock_release(stoplightlock);
  random_yielder(4);
  lock_acquire(stoplightlock);

  inQuadrant(quadrant_t1, index);
  quadrant_occupied[quadrant_t0] = false;

  cv_broadcast(intersectioncv[ahead_near], stoplightlock);
  cv_broadcast(intersectioncv[ahead_far], stoplightlock);
  cv_broadcast(intersectioncv[oncoming_near], stoplightlock);
  cv_broadcast(intersectioncv[oncoming_far], stoplightlock);

  lock_release(stoplightlock);
  random_yielder(4);
  lock_acquire(stoplightlock);

  inQuadrant(quadrant_t2, index);
  quadrant_occupied[quadrant_t1] = false;

  cv_broadcast(intersectioncv[ahead_near], stoplightlock);
  cv_broadcast(intersectioncv[ahead_far], stoplightlock);
  cv_broadcast(intersectioncv[oncoming_near], stoplightlock);
  cv_broadcast(intersectioncv[oncoming_far], stoplightlock);

  lock_release(stoplightlock);
  random_yielder(4);
  lock_acquire(stoplightlock);

  leaveIntersection(index);
  quadrant_occupied[quadrant_t2] = false;

  cv_broadcast(intersectioncv[ahead_near], stoplightlock);
  cv_broadcast(intersectioncv[ahead_far], stoplightlock);
  cv_broadcast(intersectioncv[oncoming_near], stoplightlock);
  cv_broadcast(intersectioncv[oncoming_far], stoplightlock);

  lock_release(stoplightlock);
}
