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

//Added wchan functions for calling spinlock
//Edit: MOVED!
/*void wchan_lock(struct wchan *wch){
	spinlock_acquire(&wch->wc_lock);
}

void wchan_unlock(struct wchan *wch){
	spinlock_release(&wc->wc_lock);
}*/

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

	/* Massive change here:
	 * Kernel was panicking so I changed things
	 * Also made names more clear with thsi new implementation
	 */
        // add stuff here as needed

	/*//wait channel
	lock->l_wch = wchan_create(lock->lk_name);

	//check if null
	if(lock->l_wch == NULL){

		//modify the lock name *char first
		kfree(lock->lk_name);

		//then the lock
		kfree(lock);

		return NULL;
	}

	//initialize the spinlock 		//note:
	//spinlock_init(lock->splock); 		//spinlock_init needs &lock!
	//spinlock_init(*lock->splock);
	spinlock_init(&lock->splock);
        
	//No thread owning this lock
	lock->th_holder = NULL;

	//Set locked value
	lock->locked = 1;

	//Everything initialized!
	*/

	//Create wait channel and set lock to not busy
	lock->lk_wchan = wchan_create(name);
	lock->lk_busy = false;

	//create spinlock
	spinlock_init(&lock->lk_spinlock);

	//All initialized!

	return lock;
}

void
lock_destroy(struct lock *lock)
{
        KASSERT(lock != NULL);

        // add stuff here as needed

	//remove wait channel
	/*wchan_destroy(lock->l_wch);

	//remove spinlock
	//spinlock_release(lock->splock);
	spinlock_cleanup(&lock->splock);
	*/

	//Much cleaner and easily understood variables
	//Old method commented for reference
        kfree(lock->lk_name);
        wchan_destroy(lock->lk_wchan);
	spinlock_cleanup(&lock->lk_spinlock);
	kfree(lock);

}

void
lock_acquire(struct lock *lock)
{
        // Write this

	//OLD
	/*
        //(void)lock;  // suppress warning until code gets written

	//important:
	KASSERT(lock != NULL); 		//call into lib.h code

	//code from current.h - check machine dependent variance
	if(CURCPU_EXISTS())
		KASSERT(lock->th_holder != curthread);


	spinlock_acquire(&lock->splock);

	while(lock->locked == 0){
		
		wchan_lock(lock->l_wch);

		//spinlock_release(lock->splock);
		spinlock_release(&lock->splock);

		wchan_sleep(lock->l_wch,&lock->splock);
		spinlock_acquire(&lock->splock);
		//wchan_wakeone(lock->l_wch);
	}

	//KASSERT(lock->locked == 0);
	KASSERT(lock->locked > 0);

	//NOW make it zero
	lock->locked = 0;

	//depending on which is less expensive:
	if(CURCPU_EXISTS())
		lock->th_holder = curthread; //the current thread is using this lock
	else
		lock->th_holder = NULL; 	//Nobody owns this lock

	spinlock_release(&lock->splock);
	*/

	spinlock_acquire(&lock->lk_spinlock);
	while(lock->lk_busy)
		wchan_sleep(lock->lk_wchan, &lock->lk_spinlock);

	lock->lk_busy = true;
	lock->lk_thread = curthread;

	spinlock_release(&lock->lk_spinlock);

}

void
lock_release(struct lock *lock)
{
        // Write this

        //(void)lock;  // suppress warning until code gets written

	//OLD - kept for reference
	/*
	KASSERT(lock != NULL);
	spinlock_acquire(&lock->splock); 	//Actually release the lock

	//Just like in lock_acquire
	if(CURCPU_EXISTS())
		KASSERT(lock->th_holder == curthread); 		//Current thread holds lock?
	//no thread holder now
	lock->th_holder = NULL;

	//locked is now > 0
	lock->locked++;
	
	//KASSERT(lock->locked == 0);
	KASSERT(lock->locked > 0);

	//wchan_wakeone(lock->splock);
	wchan_wakeone(lock->l_wch, &lock->splock);
	spinlock_release(&lock->splock);
	*/

	spinlock_acquire(&lock->lk_spinlock);

	//KASSERT section
	KASSERT(lock->lk_thread == curthread); 	//This gave me so much trouble
	KASSERT(lock->lk_busy);

	lock->lk_busy = false;
	lock->lk_thread = NULL;

	wchan_wakeone(lock->lk_wchan, &lock->lk_spinlock);
	spinlock_release(&lock->lk_spinlock);

}

bool
lock_do_i_hold(struct lock *lock)
{
        // Write this

        /*(void)lock;  // suppress warning until code gets written

        return true; // dummy until code gets written
	*/

	//easier
	return lock->lk_thread == curthread;

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

	//Need to add support for the conditional variable wait channel I made:
	cv->cv_wchan = wchan_create(cv->cv_name);

	//is wait channel we just created null?
	if(cv->cv_wchan == NULL){
		//we don't need to return the null cv if it's null.
		kfree(cv->cv_name);
		kfree(cv);
		return NULL;
	}

        return cv;
}

void
cv_destroy(struct cv *cv)
{
        KASSERT(cv != NULL);

        // add stuff here as needed

	//All we need to add is a line to delete the wait channel of the cv
	//that I added
	spinlock_cleanup(&cv->cv_lock);
	wchan_destroy(cv->cv_wchan);

        kfree(cv->cv_name);
        kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
        // Write this
        //(void)cv;    // suppress warning until code gets written
        //(void)lock;  // suppress warning until code gets written

	//KASSERT(cv == NULL);  		//TEST
	//KASSERT(lock == NULL); 		//TEST
	//KEPT
	KASSERT(cv != NULL);
	KASSERT(lock != NULL);
	//newish:
	KASSERT(curthread->t_in_interrupt == false);
	KASSERT(lock_do_i_hold(lock));

	spinlock_acquire(&cv->cv_lock);
	lock_release(lock);
	
	//NOW it should give the opposite
	KASSERT(!lock_do_i_hold(lock));
	wchan_sleep(cv->cv_wchan, &cv->cv_lock);

	spinlock_release(&cv->cv_lock);
	lock_acquire(lock);
	KASSERT(lock_do_i_hold(lock));

	//OLD
	/*wchan_lock(cv->cv_wch);
	//unlock
	lock_release(lock);
	wchan_sleep(cv->cv_wch, &lock->splock);
	//relock
	lock_acquire(lock);
	*/
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
        // Write this
	//(void)cv;    // suppress warning until code gets written
	//(void)lock;  // suppress warning until code gets written

	//kept
	KASSERT(cv != NULL);
	KASSERT(lock != NULL);

	KASSERT(lock_do_i_hold(lock)); 		//Well, do we?
	//Signal one in the wait channel
	wchan_wakeone(cv->cv_wchan,&lock->lk_spinlock);

	//IMPORTANT - almost forgot
	spinlock_release(&cv->cv_lock);

}


//Identical to cv_signal, except signals ALL
void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	// Write this
	//(void)cv;    // suppress warning until code gets written
	//(void)lock;  // suppress warning until code gets written

	KASSERT(cv != NULL);
	KASSERT(lock != NULL);

	KASSERT(lock_do_i_hold(lock)); 		//Well, do we?
	
	//important:
	spinlock_acquire(&cv->cv_lock);
	
	//Signal one in the wait channel
	wchan_wakeall(cv->cv_wchan, &lock->lk_spinlock);

	//important
	spinlock_release(&cv->cv_lock);

}
