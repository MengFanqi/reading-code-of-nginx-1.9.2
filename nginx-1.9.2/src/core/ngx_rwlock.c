
/*
 * Copyright (C) Ruslan Ermilov
 * Copyright (C) Nginx, Inc.
 */

/**

操作系统支持原子操作的情况下的自旋锁的实现, 自旋锁是非睡眠锁  http://www.cnblogs.com/sdphome/archive/2011/10/11/2206833.html

**/

#include <ngx_config.h>
#include <ngx_core.h>


#if (NGX_HAVE_ATOMIC_OPS)


#define NGX_RWLOCK_SPIN   2048
#define NGX_RWLOCK_WLOCK  ((ngx_atomic_uint_t) -1)


//获取互斥锁（非睡眠锁）
void
ngx_rwlock_wlock(ngx_atomic_t *lock)  //http://blog.csdn.net/zgrjkflmkyc/article/details/41649527
{
    ngx_uint_t  i, n;

    for ( ;; ) {

        //lock为0，表示锁没有被其他进程持有，这时将lock值设为 NGX_RWLOCK_WLOCK，表示该进程占用了锁
        if (*lock == 0 && ngx_atomic_cmp_set(lock, 0, NGX_RWLOCK_WLOCK)) {
            return;
        }

        if (ngx_ncpu > 1) {

            for (n = 1; n < NGX_RWLOCK_SPIN; n <<= 1) {

                for (i = 0; i < n; i++) {
                    ngx_cpu_pause();
                }

                if (*lock == 0
                    && ngx_atomic_cmp_set(lock, 0, NGX_RWLOCK_WLOCK))
                {
                    return;
                }
            }
        }

        ngx_sched_yield();  //http://blog.csdn.net/magod/article/details/7265555
    }
}


//获取共享锁（非睡眠）
void
ngx_rwlock_rlock(ngx_atomic_t *lock)
{
    ngx_uint_t         i, n;
    ngx_atomic_uint_t  readers;

    for ( ;; ) {
        readers = *lock;

        if (readers != NGX_RWLOCK_WLOCK
            && ngx_atomic_cmp_set(lock, readers, readers + 1))
        {
            return;
        }

        if (ngx_ncpu > 1) {

            for (n = 1; n < NGX_RWLOCK_SPIN; n <<= 1) {

                for (i = 0; i < n; i++) {
                    ngx_cpu_pause();
                }

                readers = *lock;

                if (readers != NGX_RWLOCK_WLOCK
                    && ngx_atomic_cmp_set(lock, readers, readers + 1))
                {
                    return;
                }
            }
        }

        ngx_sched_yield();
    }
}

//释放锁（包括互斥锁和共享锁）
void
ngx_rwlock_unlock(ngx_atomic_t *lock)
{
    ngx_atomic_uint_t  readers;

    readers = *lock;

    if (readers == NGX_RWLOCK_WLOCK) {
        *lock = 0;
        return;
    }

    for ( ;; ) {

        if (ngx_atomic_cmp_set(lock, readers, readers - 1)) {
            return;
        }

        readers = *lock;
    }
}


#else

#if (NGX_HTTP_UPSTREAM_ZONE)

#error ngx_atomic_cmp_set() is not defined!

#endif

#endif
