#ifndef SHM_SUPPORT_HPP
#define SHM_SUPPORT_HPP

#include <string>
#include "shm_errs.hpp"

class ShmObject {
  std::string name;
  size_t size;
  void* address;
  int file;
  bool opened;

public:
  ShmObject(const std::string& name) : name(name), size(0), address(nullptr), file(-1), opened(false)  { }
  ShmObject(const std::string& name, size_t size, void* address, int file)
    : name(name), size(size), address(address), file(file), opened(false) { }

  llvm::Error open();
  llvm::Error create(size_t size);
  void close();
  void unlink();

  inline bool is_open() { return opened; }
  inline void* get_address() { return address; }
};

#endif // !SHM_SUPPORT_HPP