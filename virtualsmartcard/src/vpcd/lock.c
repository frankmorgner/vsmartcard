/*
 * Copyright (C) 2014 Frank Morgner
 *
 * This file is part of virtualsmartcard.
 *
 * virtualsmartcard is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * virtualsmartcard is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * virtualsmartcard.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>

int lock(void *io_lock)
{
    EnterCriticalSection(io_lock);
    return 1;
}

int unlock(void *io_lock)
{
	LeaveCriticalSection(io_lock);
    return 1;
}

void *create_lock(void)
{
    CRITICAL_SECTION *io_lock = malloc(sizeof *io_lock);
    InitializeCriticalSection(io_lock);
    /* FIXME error handling */
    return io_lock;
}

void free_lock(void *io_lock)
{
    DeleteCriticalSection(io_lock);
    free(io_lock);
}

#else

#ifdef HAVE_PTHREAD
#include <pthread.h>

int lock(void *io_lock)
{
    r = 0;
    if (0 == pthread_mutex_lock(io_lock))
        r = 1;
    return r;
}

int unlock(void *io_lock)
{
    r = 0;
    if (0 == pthread_mutex_unlock(io_lock))
        r = 1;
    return r;
}

void *create_lock(void)
{
    pthread_mutex_t *io_lock = malloc(sizeof *io_lock);
    if (io_lock) {
        *io_lock = PTHREAD_MUTEX_INITIALIZER;
    }
    return io_lock;
}

void free_lock(void *io_lock)
{
    pthread_mutex_destroy(io_lock);
    free(io_lock);
}

#else

int lock(void *io_lock)
{
    return 1;
}

int unlock(void *io_lock)
{
    return 1;
}

void *create_lock(void)
{
    return (void *) 1;
}

void free_lock(void *io_lock)
{
}

#endif

#endif
