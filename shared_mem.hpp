// This code is from https://github.com/NilsBrause/waylandpp/blob/master/example/shm.cpp
/*
 * Copyright (c) 2014-2019, Nils Christopher Brause, Philipp Kerling, Zsolt Bölöny
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <string.h>
#include <stdexcept>
#include <sys/mman.h>
#include <string>
#include <sstream>
#include "debug.h"

// helper to create a std::function out of a member function and an object
template <typename R, typename T, typename... Args>
std::function<R(Args...)> bind_mem_fn(R(T::* func)(Args...), T *t)
{
return [func, t] (Args... args)
{
  return (t->*func)(args...);
};
}

// shared memory helper class
class shared_mem_t
{
  private:
    std::string name;
    int fd = 0;
    size_t len = 0;
    void *mem = nullptr;

  public:
    shared_mem_t() = default;
    shared_mem_t(const shared_mem_t&) = delete;
    shared_mem_t(shared_mem_t&&) noexcept = delete;
    shared_mem_t& operator=(const shared_mem_t&) = delete;
    shared_mem_t& operator=(shared_mem_t&&) noexcept = delete;

    shared_mem_t(size_t size)
      : len(size)
    {
      // create random filename
      std::stringstream ss;
      std::random_device device;
      std::default_random_engine engine(device());
      std::uniform_int_distribution<unsigned int> distribution(0, std::numeric_limits<unsigned int>::max());
      ss << distribution(engine);
      name = ss.str();
      if(size == 0)
        size = len = 1;

      // open shared memory file
      fd = memfd_create(name.c_str(), 0);
      if(fd < 0) {
        debug << "shm_open failed." << std::endl;
        throw std::runtime_error("shm_open failed.");
      }

      // set size
      if(ftruncate(fd, size) < 0) {
        debug << "ftruncate failed." << std::endl;
        throw std::runtime_error("ftruncate failed.");
      }

      // map memory
      debug << "mmap " << name << std::endl;
      mem = mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
      if(mem == MAP_FAILED) { // NOLINT
        debug << "mmap failed: " << strerror(errno) << " len=" << len << " name=" << name << std::endl; 
        throw std::runtime_error(std::string("[shared_mem::shared_mem] mmap failed: ") + std::string(strerror(errno)));
      }
    }

    ~shared_mem_t() noexcept
    {
      if(fd) {
        debug << "munmap start: " << name << std::endl;
        if(munmap(mem, len) < 0)
          debug << "munmap failed: "  << name << " : " << strerror(errno) << std::endl;
        if(close(fd) < 0)
          debug << "close failed: " << strerror(errno) << std::endl;
        //if(shm_unlink(name.c_str()) < 0)
        //  std::cerr << "shm_unlink failed: (file: " << name << ") " << strerror(errno) << std::endl;
        debug << "munmap end" << std::endl;
      }
    }

    int get_fd() const
    {
      return fd;
    }

    void *get_mem()
    {
      return mem;
    }
};

