#include <fstream>
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
#include <functional>
#include <limits>
#include <cstring>
#include <algorithm>
#include <iostream>
#include "output.hpp"
#include "shm_support.hpp"

extern "C" {
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <elf.h>
}

std::vector<char> read_image(const char* path) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  MY_ASSERT_PERROR(f.good(), "Failed to open image file.");

  std::streamsize size = f.tellg();
  f.seekg(0, std::ios::beg);

  std::vector<char> ans(size);
  f.read(ans.data(), size);
  return ans;
}

template <typename T>
class Span {
  T* begin_;
  T* end_;

public:
  Span(T* begin, T* end) : begin_(begin), end_(end) { }

  inline T* begin() const { return begin_; }
  inline T* end() const { return end_; }

  inline T* begin() { return begin_; }
  inline T* end() { return end_; }

  inline size_t len() { return end() - begin(); }
  T& operator [] (size_t index) { return begin()[index]; }
};

template <typename T>
void iter_span_if(Span<T> span, std::function<bool(T&)> predicate, std::function<void(T&)> action) {
  for (T& t : span) {
    if (predicate(t)) {
      action(t);
    }
  }
}

#define ELFHDR(elf) ((Elf64_Ehdr*)elf)

Span<Elf64_Phdr> get_segment_headers(uintptr_t elf) {
  auto begin = (Elf64_Phdr*)(elf + ELFHDR(elf)->e_phoff);
  auto end = begin + ELFHDR(elf)->e_phnum;
  return { begin, end };
}

Span<Elf64_Shdr> get_section_headers(uintptr_t elf) {
  auto begin = (Elf64_Shdr*)(elf + ELFHDR(elf)->e_shoff);
  auto end = begin + ELFHDR(elf)->e_shnum;
  return { begin, end };
}

template <typename T>
Span<T> get_section_data_as_span(uintptr_t elf, Elf64_Shdr* shdr) {
  if (shdr->sh_entsize != 0)
    assert(shdr->sh_entsize == sizeof(T) && "Maybe wrong type?");
  assert(shdr->sh_size % sizeof(T) == 0 && "Maybe wrong type?");
  auto begin = (T*)(elf + shdr->sh_offset);
  auto end = (T*)(elf + shdr->sh_offset + shdr->sh_size);
  return { begin, end };
}

void iter_segments_if(uintptr_t elf, std::function<bool(const Elf64_Phdr&)> predicate, std::function<void(const Elf64_Phdr&)> action) {
  iter_span_if<Elf64_Phdr>(get_segment_headers(elf), predicate, action);
}
void iter_sections_if(uintptr_t elf, std::function<bool(const Elf64_Shdr&)> predicate, std::function<void(const Elf64_Shdr&)> action) {
  iter_span_if<Elf64_Shdr>(get_section_headers(elf), predicate, action);
}

template <typename T>
T* find_single(Span<T> ts, std::function<bool(const T&)> predicate) {
  T* ans = nullptr;
  iter_span_if<T>(ts, predicate, [&ans] (T& t) {
    assert(ans == nullptr && "Multiple items match the criteria!");
    ans = &t;
  });
  assert(ans != nullptr && "No section matches the criteria!");
  return ans;
}



Span<char> get_section_string_table(uintptr_t elf) {
  Elf64_Half index = ELFHDR(elf)->e_shstrndx;
  assert(index != SHN_UNDEF && "File has no section name string table!");

  Elf64_Shdr* shdr = &get_section_headers(elf)[index];
  return get_section_data_as_span<char>(elf, shdr);
}

Span<char> get_symbol_string_table(uintptr_t elf) {
  Span<char> section_string_table = get_section_string_table(elf);
  Elf64_Shdr* shdr = find_single<Elf64_Shdr>(get_section_headers(elf), [section_string_table] (const Elf64_Shdr& shdr) { 
    return strcmp(section_string_table.begin() + shdr.sh_name, ".strtab") == 0; 
  });

  assert(shdr->sh_type == SHT_STRTAB && "Not a string table.");
  return get_section_data_as_span<char>(elf, shdr);
}

Span<Elf64_Sym> get_symbol_table(uintptr_t elf) {
  Elf64_Shdr* shdr = find_single<Elf64_Shdr>(get_section_headers(elf), [] (const Elf64_Shdr& shdr) { 
    return shdr.sh_type == SHT_SYMTAB;
  });
  return get_section_data_as_span<Elf64_Sym>(elf, shdr);
}



bool is_segment_loadable(const Elf64_Phdr& seg) {
  return seg.p_type == PT_LOAD && seg.p_memsz > 0;
}

size_t get_mmap_len(uintptr_t elf) {
  uintptr_t vaddr_min = std::numeric_limits<uintptr_t>::max();
  uintptr_t vaddr_max = std::numeric_limits<uintptr_t>::min();

  iter_segments_if(elf, is_segment_loadable, [&vaddr_min, &vaddr_max](const Elf64_Phdr& prg) {
    vaddr_min = std::min(vaddr_min, prg.p_vaddr);
    vaddr_max = std::max(vaddr_max, prg.p_vaddr + prg.p_memsz);
  });
 
  // Get the number of pages that include the [vaddr_min, vaddr_max] range
  int page_size = getpagesize();
  uintptr_t base = vaddr_min - (vaddr_min % page_size); // floor to the nearest page boundary
  size_t len = vaddr_max - base;
  len += page_size - (len % page_size);

  // Check the result
  ptrdiff_t ori_size = vaddr_max - vaddr_min;
  assert((len < ori_size + (2 * page_size)) && "Exagerated size.");
  assert(len > ori_size && "Length is smaller than the original vaddr range.");
  assert((base % page_size) == 0 && "Base is not on a page boundary.");
  assert((len % page_size) == 0 && "Length is not a multiple of page size.");
  return len;
}

void copy_segments_to_mem(uintptr_t elf, uintptr_t base) {
  iter_segments_if(elf, is_segment_loadable, [elf, base](const Elf64_Phdr& prg) {
    void* dst = (void*)(base + prg.p_vaddr);
    void* src = (void*)(elf + prg.p_offset);
    size_t len = prg.p_memsz;

    printf("Copying from %lu bytes from %p to %p.\n", len, src, dst);
    std::memcpy(dst, src, len);
  });
}

uintptr_t create_pages_for_mapping(uintptr_t elf) {
  size_t mmap_len = get_mmap_len(elf);
  printf("Mmapping %lu bytes\n", mmap_len);
  int prot = PROT_READ | PROT_WRITE | PROT_EXEC;
  int flags = MAP_ANONYMOUS | MAP_PRIVATE;

  void* base_addr_ = mmap(NULL, mmap_len, prot, flags, -1, 0);
  MY_ASSERT_PERROR(base_addr_ != MAP_FAILED, "Failed to mmap.");
  return (uintptr_t)base_addr_;
}

void relocate(uintptr_t elf, uintptr_t base_address) {
  for (auto& shdr : get_section_headers(elf)) {
    switch (shdr.sh_type) {
      case SHT_REL:
        printf("REL section\n");
        break;
      case SHT_RELA:

        printf("RELA section\n");
        break;
      default:
        continue;
    }
  }
}


Elf64_Sym* resolve_symbol(uintptr_t elf, const char* name) {
  auto symbol_strtab = get_symbol_string_table(elf);
  auto symbol_table = get_symbol_table(elf);

  printf("Resolving %s\n", name);
  Elf64_Sym* s = find_single<Elf64_Sym>(symbol_table, [name, symbol_strtab] (const Elf64_Sym& sym) {
    const char* sym_name = symbol_strtab.begin() + sym.st_name;
    bool ans = strcmp(sym_name, name) == 0;
    return ans;
  });
  return s;
}

template <typename T>
T resolve_callable_symbol(uintptr_t elf, uintptr_t base_addr, const char* name) {
  Elf64_Sym* sym = resolve_symbol(elf, name);
  uintptr_t addr = base_addr + sym->st_value;
  return (T)addr;
}


// HAHAHAH this is unreadable, have fun
// just prints "Calling(args)... Returned retval"
template <typename RType, typename... Args>
void print_call(uintptr_t elf, uintptr_t base_addr, const char* name, Args... args) {
  using FType = RType(*)(Args...);

  FType fptr = resolve_callable_symbol<FType>(elf, base_addr, name);
  
  printf("Calling %s(", name);
  using expander = int[];
  (void)expander{0, (void(std::cout << ',' << std::forward<Args>(args)), 0)...};
  printf(")... ");
  fflush(stdout);

  RType ans = fptr(args...);
  std::cout << "Returned " << ans << std::endl;
}

void print_symbol_table(uintptr_t elf) {
  auto symbol_strtab = get_symbol_string_table(elf);
  auto symbol_table = get_symbol_table(elf);
  printf("\n\nBEGIN SYMBOL TABLE\n");
  for (auto &sym : symbol_table) {
    if (sym.st_name != 0) {
      const char* name = symbol_strtab.begin() + sym.st_name;
      printf("%s\n", name);
    }
  }
  printf("END SYMBOL TABLE\n\n");
}


int main(int argc, char* argv[]) {
  MY_ASSERT(argc == 2, "2 args, please");

  // Read the image
  std::vector<char> img = read_image(argv[1]);
  uintptr_t elf = (uintptr_t)img.data();

  // Create pages on which to map our library
  uintptr_t base_addr = create_pages_for_mapping(elf);

  // Copy the ELF segments to the pages
  copy_segments_to_mem(elf, base_addr);

  // Print symbol table
  // print_symbol_table(elf);

  relocate(elf, base_addr);

  print_call<int, int, int>(elf, base_addr, "_Z3mulii", 617, 2);
  print_call<int, int, int>(elf, base_addr, "_Z3powii", 2, 3);
  print_call<int, int>(elf, base_addr, "_Z4cubei", 5);
}