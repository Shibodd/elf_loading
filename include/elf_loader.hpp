#ifndef ELF_LOADER_HPP
#define ELF_LOADER_HPP

#include <vector>
#include <iostream>
#include <cstdint>
#include <functional>
#include "elf_view.hpp"

struct Mapper {
  virtual Span<char> map(size_t size) = 0;
  virtual void unmap(Span<char> mapped_area) = 0;
};

// Provides a way to load an ELF in memory and execute symbols
class ElfLoader {
  Span<char> mapped_memory;
  std::vector<char> elf_file;
  ElfView elf;
  Mapper& mapper;

  size_t get_mmap_len();
  void copy_segments_to_mem();
  void fixup_relocations();
  void single_rela_fixup(const Elf64_Shdr& phdr);

  inline uintptr_t get_base_addr() { return (uintptr_t)mapped_memory.begin(); }

  template <typename T>
  T get_symbol_addr(const char* name) {
    Elf64_Sym* sym = elf.resolve_symbol(name);
    uintptr_t addr = get_base_addr() + sym->st_value;
    return (T)addr;
  }

public:
  ElfLoader(Span<char> elf_file, Mapper& mapper);

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