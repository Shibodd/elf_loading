#ifndef ELF_VIEW_HPP
#define ELF_VIEW_HPP

#include <cassert>
#include "span.hpp"
#include "elf.h"

// Provides an ELF file view over a range of memory
class ElfView {
  Span<char> data;
  Elf64_Ehdr* header;
  Span<Elf64_Phdr> segment_headers;
  Span<Elf64_Shdr> section_headers;
  Span<char> section_string_table;
  Span<char> symbol_string_table;
  Span<Elf64_Sym> symbol_table;

  template <typename T>
  Span<T> make_span_of_section_data(Elf64_Shdr* shdr) {
    if (shdr->sh_entsize != 0)
      assert(shdr->sh_entsize == sizeof(T) && "Maybe wrong type?");
    assert(shdr->sh_size % sizeof(T) == 0 && "Maybe wrong type?");
    auto begin = (T*)(get_addr() + shdr->sh_offset);
    auto end = (T*)(get_addr() + shdr->sh_offset + shdr->sh_size);
    return { begin, end };
  }

  Span<char> make_symbol_string_table();
  Span<Elf64_Sym> make_symbol_table();
  Span<char> make_section_string_table(uintptr_t elf, Span<Elf64_Shdr> section_headers);

public:
  ElfView(Span<char> data);

  void print_symbol_table();
  Elf64_Sym* resolve_symbol(const char* name);

  inline uintptr_t get_addr() const { return (uintptr_t)data.begin(); }
  inline const Span<char>& get_data() const { return data; };
  inline Elf64_Ehdr* get_header() const { return header; }
  inline const Span<Elf64_Phdr>& get_segment_headers() const { return segment_headers; }
  inline const Span<Elf64_Shdr>& get_section_headers() const { return section_headers; }
  inline const Span<char>& get_section_string_table() const { return section_string_table; }
  inline const Span<char>& get_symbol_string_table() const { return symbol_string_table; }
  inline const Span<Elf64_Sym>& get_symbol_table() const { return symbol_table; }
};

#endif // !ELF_VIEW_HPP