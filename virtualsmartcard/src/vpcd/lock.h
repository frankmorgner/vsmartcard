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
#ifndef _LOCK_H_
#define _LOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

int lock(void *io_lock);
int unlock(void *io_lock);
void *create_lock(void);
void free_lock(void *io_lock);
#ifdef  __cplusplus
}
#endif
#endif
