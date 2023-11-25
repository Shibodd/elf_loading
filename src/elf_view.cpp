#include <cstring>
#include <cassert>

#include "span.hpp"
#include "elf.h"
#include "elf_view.hpp"


Span<char> ElfView::make_symbol_string_table() {
  Elf64_Shdr* shdr = find_single_in_span<Elf64_Shdr>(section_headers, [this] (const Elf64_Shdr& shdr) { 
    return strcmp(section_string_table.begin() + shdr.sh_name, ".strtab") == 0; 
  });
  assert(shdr->sh_type == SHT_STRTAB && "Not a string table.");
  return make_span_of_section_data<char>(shdr);
}

Span<Elf64_Sym> ElfView::make_symbol_table() {
  Elf64_Shdr* shdr = find_single_in_span<Elf64_Shdr>(section_headers, [] (const Elf64_Shdr& shdr) { 
    return shdr.sh_type == SHT_SYMTAB;
  });
  return make_span_of_section_data<Elf64_Sym>(shdr);
}

Span<char> ElfView::make_section_string_table(uintptr_t elf, Span<Elf64_Shdr> section_headers) {
  Elf64_Half index = ((Elf64_Ehdr*)elf)->e_shstrndx;
  assert(index != SHN_UNDEF && "File has no section name string table!");

  Elf64_Shdr* shdr = &section_headers[index];
  return make_span_of_section_data<char>(shdr);
}

ElfView::ElfView(Span<char> data) : 
  data(data),
  header((Elf64_Ehdr*)data.begin()),
  segment_headers((Elf64_Phdr*)(get_addr() + header->e_phoff), header->e_phnum),
  section_headers((Elf64_Shdr*)(get_addr() + header->e_shoff), header->e_shnum),
  section_string_table(get_section_string_table()),
  symbol_string_table(get_symbol_string_table()),
  symbol_table(get_symbol_table())
{
  
}

void ElfView::print_symbol_table() {
  printf("\n\nBEGIN SYMBOL TABLE\n");
  for (auto &sym : symbol_table) {
    if (sym.st_name != 0) {
      const char* name = symbol_string_table.begin() + sym.st_name;
      printf("%s\n", name);
    }
  }
  printf("END SYMBOL TABLE\n\n");
}

Elf64_Sym* ElfView::resolve_symbol(const char* name) {
  auto& symbol_strtab = symbol_string_table;

  printf("Resolving %s\n", name);
  Elf64_Sym* s = find_single_in_span<Elf64_Sym>(symbol_table, [name, &symbol_strtab] (const Elf64_Sym& sym) {
    const char* sym_name = symbol_strtab.begin() + sym.st_name;
    bool ans = strcmp(sym_name, name) == 0;
    return ans;
  });
  return s;
}