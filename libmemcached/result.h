/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Libmemcached library
 *
 *  Copyright (C) 2011-2012 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2006-2009 Brian Aker All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#pragma once

#include "libmemcached/string.hpp"

struct Result {
  struct {
    bool is_allocated;
    bool is_initialized;
  } options;
  uint32_t item_flags;
  time_t item_expiration;
  size_t key_length;
  uint64_t item_cas;
  struct memcached_st *root;
  memcached_string_st value;
  uint64_t numeric_value;
  uint64_t count;
  char item_key[MEMCACHED_MAX_KEY];
  /* Add result callback function */

  Result(memcached_result_st* shell_, const struct memcached_st* memc_) :
    item_flags(0),
    item_expiration(0),
    key_length(0),
    item_cas(0),
    root(const_cast<memcached_st*>(memc_)),
    numeric_value(UINT64_MAX),
    count(0),
    _shell(shell_)
  {
    item_key[0]= 0;

    if (shell_)
    {
      memcached_set_allocated(_shell, false);
    }
    else
    {
      _shell= &_owned_shell;
      memcached_set_allocated(_shell, true);
    }

    _shell->impl(this);
    memcached_set_initialized(_shell, true);

    memcached_string_create((memcached_st*)root, &value, 0);
  }

  ~Result()
  {
  }

  memcached_result_st* shell()
  {
    return _shell;
  }

private:
  memcached_result_st* _shell;
  memcached_result_st _owned_shell;
};

void memcached_result_reset_value(memcached_result_st *ptr);
