#include <cstring>
#include <cassert>

#include "span.hpp"
#include "elf.h"
#include "elf_view.hpp"


Span<char> ElfView::make_symbol_string_table() {
  Elf64_Shdr* phdr = find_single_in_span<Elf64_Shdr>(section_headers, [this] (const Elf64_Shdr& phdr) { 
    return strcmp(section_string_table.begin() + phdr.sh_name, ".dynstr") == 0; 
  });
  assert(phdr->sh_type == SHT_STRTAB && "Not a string table.");
  return make_span_of_section_data<char>(*phdr);
}

Span<Elf64_Sym> ElfView::make_symbol_table() {
  Elf64_Shdr* phdr = find_single_in_span<Elf64_Shdr>(section_headers, [] (const Elf64_Shdr& phdr) { 
    return phdr.sh_type == SHT_DYNSYM;
  });
  return make_span_of_section_data<Elf64_Sym>(*phdr);
}

Span<char> ElfView::make_section_string_table() {
  Elf64_Half index = header->e_shstrndx;
  assert(index != SHN_UNDEF && "File has no section name string table!");

  Elf64_Shdr* phdr = &section_headers[index];
  return make_span_of_section_data<char>(*phdr);
}

ElfView::ElfView(Span<char> data) : 
  data(data),
  header((Elf64_Ehdr*)data.begin()),
  program_headers((Elf64_Phdr*)(get_addr() + header->e_phoff), header->e_phnum),
  section_headers((Elf64_Shdr*)(get_addr() + header->e_shoff), header->e_shnum),
  section_string_table(make_section_string_table()),
  symbol_string_table(make_symbol_string_table()),
  symbol_table(make_symbol_table())
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

  printf("Resolving %s... ", name);
  fflush(stdout);
  Elf64_Sym* s = find_single_in_span<Elf64_Sym>(symbol_table, [name, &symbol_strtab] (const Elf64_Sym& sym) {
    const char* sym_name = symbol_strtab.begin() + sym.st_name;
    bool ans = strcmp(sym_name, name) == 0;
    return ans;
  });
  printf("Symbol resolved!\n");
  return s;
}