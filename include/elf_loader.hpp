#ifndef ELF_LOADER_HPP
#define ELF_LOADER_HPP

#include <vector>
#include <iostream>
#include <cstdint>
#include "elf_view.hpp"

// Provides a way to load an ELF in memory and execute symbols
class ElfLoader {
  Span<char> mapped_memory;
  std::vector<char> elf_file;
  ElfView elf;

  static bool is_segment_loadable(const Elf64_Phdr& seg);
  size_t get_mmap_len();
  void mmap_pages(size_t len);
  void copy_segments_to_mem();
  void relocate();

  inline uintptr_t get_base_addr() { return (uintptr_t)mapped_memory.begin(); }

  template <typename T>
  T get_symbol_addr(const char* name) {
    Elf64_Sym* sym = elf.resolve_symbol(name);
    uintptr_t addr = get_base_addr() + sym->st_value;
    return (T)addr;
  }

public:
  ElfLoader(std::vector<char> data_in);

  static ElfLoader from_file(const char* path);

  void load();
  void unload();

  template<typename RType, typename... Args>
  RType call(const char* name, Args... args) {
    using FType = RType(*)(Args...);
    FType fptr = get_symbol_addr<FType>(name);
    
    // This is unreadable, but it just prints the arguments.
    printf("Calling %s(", name);
    using expander = int[];
    (void)expander{0, (void(std::cout << ',' << std::forward<Args>(args)), 0)...};
    printf(")... ");
    fflush(stdout);

    RType ans = fptr(args...);
    std::cout << "Returned " << ans << std::endl;
    return ans;
  }
};

#endif // !ELF_LOADER_HPP