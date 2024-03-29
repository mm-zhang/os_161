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

struct semaphore *
sem_create(const char *name, unsigned initial_count)
{
	struct semaphore *sem;

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

void
sem_destroy(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	/* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&sem->sem_lock);
	wchan_destroy(sem->sem_wchan);
	kfree(sem->sem_name);
	kfree(sem);
}

void
P(struct semaphore *sem)
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

void
V(struct semaphore *sem)
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

struct lock *
lock_create(const char *name)
{
	struct lock *lock;

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
	lock->lk_wchan = wchan_create(lock->lk_name);
	if (lock->lk_wchan == NULL) {
		kfree(lock->lk_name);
		kfree(lock);
		return NULL;
	}

	lock->lk_used = false;
	lock->lk_curThread = NULL;
	spinlock_init(&lock->lk_spinlock);
	return lock;
}

void
lock_destroy(struct lock *lock)
{
	KASSERT(lock != NULL);
	// add stuff here as needed
	spinlock_cleanup(&lock->lk_spinlock);
	wchan_destroy(lock->lk_wchan);
	kfree(lock->lk_name);
  kfree(lock->lk_curThread);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
	/* Call this (atomically) once the lock is acquired */
	//HANGMAN_ACQUIRE(&curthread->t_hangman, &lock->lk_hangman);
	//write this
	// follow P(struct semaphore *sem) function
	KASSERT(lock != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	KASSERT(curthread->t_in_interrupt == false);

	spinlock_acquire(&lock->lk_spinlock);
	/* Call this (atomically) once the lock is acquired */
	HANGMAN_ACQUIRE(&curthread->t_hangman, &lock->lk_hangman);

	// if lock is used, put it to sleep
    	while(lock->lk_used){
		wchan_sleep(lock->lk_wchan,&lock->lk_spinlock);
	}

	lock->lk_used = true;
	lock->lk_curThread = curthread;
	spinlock_release(&lock->lk_spinlock);


	(void)lock;  // suppress warning until code gets written

}

void
lock_release(struct lock *lock)
{
	/* Call this (atomically) when the lock is released */
	//HANGMAN_RELEASE(&curthread->t_hangman, &lock->lk_hangman);

	// Write this
	// follow V(struct semaphore *sem) function
  	KASSERT(lock != NULL);
	KASSERT(lock->lk_curThread != NULL);

	spinlock_acquire(&lock->lk_spinlock);


	wchan_wakeone(lock->lk_wchan,&lock->lk_spinlock);
	lock->lk_used = false;
	lock->lk_curThread = NULL;

	spinlock_release(&lock->lk_spinlock);
	/* Call this (atomically) once the lock is released */
	HANGMAN_ACQUIRE(&curthread->t_hangman, &lock->lk_hangman);

}

bool
lock_do_i_hold(struct lock *lock)
{
	// Write this
	KASSERT(lock != NULL);
	bool hasHold = false;

  	if(lock->lk_curThread == curthread && lock->lk_used){
		hasHold = true;
	}else{
		hasHold = false;
	}

	return hasHold;
  // suppress warning until code gets written

	 // dummy until code gets written
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(*cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->cv_name = kstrdup(name);
	if (cv->cv_name==NULL) {
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

void
cv_destroy(struct cv *cv)
{
	KASSERT(cv != NULL);

	kfree(cv->cv_name);

	wchan_destroy(cv->cv_wchan);
	spinlock_cleanup(&cv->cv_spinlock);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	// Write this
	KASSERT(cv != NULL);
	KASSERT(lock != NULL);
	// must hold a lock
	//KASSERT(lock_do_i_hold(lock));

	// before lease the lock, make sure the lock can release properly
	// spinlock_acquire before lock_release to fill the gap
	spinlock_acquire(&cv->cv_spinlock);
	lock_release(lock);
	//spinlock_acquire(&cv->cv_spinlock);
//	P(cv->cv_semaphore->sem_wchan);
	wchan_sleep(cv->cv_wchan, &cv->cv_spinlock);
	spinlock_release(&cv->cv_spinlock);

	lock_acquire(lock);
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	KASSERT(cv != NULL);
	KASSERT(lock != NULL);
	//KASSERT(lock_do_i_hold(lock));
	spinlock_acquire(&cv->cv_spinlock);
	wchan_wakeone(cv->cv_wchan, &cv->cv_spinlock);
//	V(cv->cv_semaphore);
	spinlock_release(&cv->cv_spinlock);


}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	// Write this
	KASSERT(cv != NULL);
	KASSERT(lock != NULL);
	KASSERT(lock_do_i_hold(lock));

	spinlock_acquire(&cv->cv_spinlock);
	//lock_release(lock);	/*********************after delete lock_do_i_hold*****************************************************/

	wchan_wakeall(cv->cv_wchan, &cv->cv_spinlock);
	spinlock_release(&cv->cv_spinlock);
	//lock_acquire(lock);	/*********************after delete lock_do_i_hold*****************************************************/

}


//////// RW//////////////////////////////////////////////
////////////////////////

struct rwlock *
rwlock_create(const char *name)
{
	struct rwlock *rwlock;

	rwlock = kmalloc(sizeof(*rwlock));
	if (rwlock == NULL) {
		return NULL;
	}

	rwlock->rwlock_name = kstrdup(name);
	if (rwlock->rwlock_name==NULL) {
		kfree(rwlock);
		return NULL;
	}

	// add stuff here as needed
	rwlock->rw_lock = lock_create(rwlock->rwlock_name);
	if (rwlock->rw_lock == NULL) {
		kfree(rwlock->rwlock_name);
		kfree(rwlock);
		return NULL;
	}
	rwlock->rw_to_read = cv_create(rwlock->rwlock_name);
	if (rwlock->rw_to_read == NULL) {
		lock_destroy(rwlock->rw_lock);
			kfree(rwlock->rwlock_name);
		kfree(rwlock);
		return NULL;
	}
	rwlock->rw_to_write = cv_create(rwlock->rwlock_name);
	if (rwlock->rw_to_write == NULL) {
		cv_destroy(rwlock->rw_to_read);
		lock_destroy(rwlock->rw_lock);
		kfree(rwlock->rwlock_name);
		kfree(rwlock);
		return NULL;
	}
	// KASSERT(rwlock->rw_to_write == NULL);
  rwlock->rw_reader_in_queue=0;
	// this reader count is for the reader that waiting in the queue and blocked by writing signal
	rwlock->rw_writer_in_queue=0;
	// same writer counter as reader counter
  rwlock->rw_reader_in_held=0;
	//the reader that successfully acquired into stack
	rwlock->rw_writer_in_held=0;
	//same writer
	return rwlock;

}

void rwlock_destroy(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);
	KASSERT(rwlock->rw_reader_in_held >= 0);
	KASSERT(rwlock->rw_writer_in_queue >= 0);
  KASSERT(rwlock->rw_reader_in_queue >= 0);
	KASSERT(rwlock->rw_writer_in_held >= 0);
	//check the rwlock is existing
	//release all requesting resource
	cv_destroy(	rwlock->rw_to_write);
	cv_destroy(	rwlock->rw_to_read);
	lock_destroy(rwlock->rw_lock);
	kfree(rwlock->rwlock_name);
	kfree(rwlock);
}

void rwlock_acquire_read(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);
	KASSERT(rwlock->rw_reader_in_held >= 0);
	KASSERT(rwlock->rw_writer_in_queue >= 0);
  KASSERT(rwlock->rw_reader_in_queue >= 0);
	KASSERT(rwlock->rw_writer_in_held >= 0);
	lock_acquire(rwlock->rw_lock);
	rwlock->rw_reader_in_queue++;//add the pending queue first
	if(rwlock->rw_writer_in_queue > 0){
		cv_wait(rwlock->rw_to_read,rwlock->rw_lock);
	}
	while(rwlock->rw_writer_in_held > 0){
		// pending process
    cv_wait(rwlock->rw_to_read,rwlock->rw_lock);
	}
	rwlock->rw_reader_in_held++;// passed the while loop,acquire succeed
	rwlock->rw_reader_in_queue--;// out of pending queue
	lock_release(rwlock->rw_lock);
}

void rwlock_release_read(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);
	KASSERT(rwlock->rw_reader_in_held >= 0);
	KASSERT(rwlock->rw_writer_in_queue >= 0);
	KASSERT(rwlock->rw_reader_in_queue >= 0);
	KASSERT(rwlock->rw_writer_in_held >= 0);
	//add
	lock_acquire(rwlock->rw_lock);
	rwlock->rw_reader_in_held--;
	//rwlock->rw_writer_in_queue > 0 &&
	if( rwlock->rw_reader_in_held==0){
		if(rwlock->rw_writer_in_queue > 0){
	  cv_signal(rwlock->rw_to_write,rwlock->rw_lock);
	}else{
		cv_broadcast(rwlock->rw_to_read,rwlock->rw_lock);
	}
}

	lock_release(rwlock->rw_lock);

}

void rwlock_acquire_write(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);
	KASSERT(rwlock->rw_reader_in_held >= 0);
	KASSERT(rwlock->rw_writer_in_queue >= 0);
	KASSERT(rwlock->rw_reader_in_queue >= 0);
	KASSERT(rwlock->rw_writer_in_held >= 0);
	lock_acquire(rwlock->rw_lock);
	rwlock->rw_writer_in_queue++; //add to pending queue
	while(rwlock->rw_writer_in_held > 0 || rwlock->rw_reader_in_held > 0 ){
		  cv_wait(rwlock->rw_to_write,rwlock->rw_lock);
	}
	// rwlock->rw_reader_in_queue > 0
	rwlock->rw_writer_in_queue--;//unqueue
	rwlock->rw_writer_in_held++;//acquire section succed
  lock_release(rwlock->rw_lock);
}

void rwlock_release_write(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);
	KASSERT(rwlock->rw_reader_in_held >= 0);
	KASSERT(rwlock->rw_writer_in_queue >= 0);
	KASSERT(rwlock->rw_reader_in_queue >= 0);
	KASSERT(rwlock->rw_writer_in_held >= 0);
	lock_acquire(rwlock->rw_lock);
	rwlock->rw_writer_in_held--;
	  // if still some writer in queue
	if(rwlock->rw_reader_in_queue > 0){
		cv_broadcast(rwlock->rw_to_read,rwlock->rw_lock);
		// signal for next writer
	}else{
		cv_signal(rwlock->rw_to_write,rwlock->rw_lock);
	}

	lock_release(rwlock->rw_lock);
}
