#ifndef SHM_API_WRAP_HPP
#define SHM_API_WRAP_HPP

// Linux C API wrap

extern "C" {
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
}

#include "shm_support.hpp"
#include "shm_errs.hpp"

static llvm::Error wrap_shm_open(const std::string& name, int& file_out, bool create) {
  int flags = O_RDWR;
  if (create) {
    flags |= O_CREAT | O_TRUNC;
  }

  int file = shm_open(name.c_str(), flags, S_IREAD | S_IWRITE | S_IEXEC);
  if (file < 0)
    return make_err_withno("Could not open the shared memory object.");

  file_out = file;
  return make_success();
}

static llvm::Error wrap_shm_mmap(int file, size_t size, void*& address_out) {
  void* address = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, file, 0);
  if (address == MAP_FAILED)
    return make_err_withno("Could not mmap the shared memory object.");

  address_out = address;
  return make_success();
}

static llvm::Error wrap_ftruncate(int file, size_t size) {
  if (ftruncate(file, size) != 0)
    return make_err_withno("Could not resize the shared memory object.");
  return make_success();
}

static llvm::Error wrap_munmap(void* address, size_t size) {
  if (munmap(address, size) != 0)
    return make_err_withno("Could not munmap the shared memory object.");
  return make_success();
}

static llvm::Error wrap_close(int file) {
  if (close(file) != 0)
    return make_err_withno("Could not close the shared memory object.");
  return make_success();
}

static llvm::Error wrap_fstat_size(int file, size_t& size) {
  struct stat buf;
  if (fstat(file, &buf) != 0)
    return make_err_withno("Could not get shared memory object length.");
    
  size = buf.st_size;
  return make_success();
}

static llvm::Error wrap_shm_unlink(const std::string& name) {
  if (shm_unlink(name.c_str()) != 0)
    return make_err_withno("Couldn't unlink shared memory object.");
  
  return make_success();
}

#endif // !SHM_API_WRAP_HPP