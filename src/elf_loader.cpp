#include <vector>
#include <cstdint>
#include <limits>
#include <cassert>
#include <cstring>
#include <fstream>
#include "output.hpp"
#include "elf_view.hpp"
#include "elf_loader.hpp"

extern "C" {
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <elf.h>
}

size_t ElfLoader::get_mmap_len() {
  printf("Computing mmap len.\n");

  uintptr_t vaddr_min = std::numeric_limits<uintptr_t>::max();
  uintptr_t vaddr_max = std::numeric_limits<uintptr_t>::min();

  for (auto& phdr : elf.get_program_headers()) {
    if (phdr.p_type == PT_LOAD && phdr.p_memsz > 0) {
      vaddr_min = std::min(vaddr_min, phdr.p_vaddr);
      vaddr_max = std::max(vaddr_max, phdr.p_vaddr + phdr.p_memsz);
    }
  }

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

void ElfLoader::mmap_pages(size_t len) {
  printf("Mmapping %lu bytes\n", len);
  int prot = PROT_READ | PROT_WRITE | PROT_EXEC;
  int flags = MAP_ANONYMOUS | MAP_PRIVATE;

  void* base_ptr = mmap(NULL, len, prot, flags, -1, 0);
  MY_ASSERT_PERROR(base_ptr != MAP_FAILED, "Failed to mmap.");
  mapped_memory = { (char*)base_ptr, (char*)base_ptr + len };
}

void ElfLoader::copy_segments_to_mem() {
  for (auto& phdr : elf.get_program_headers()) {
    if (phdr.p_type == PT_LOAD && phdr.p_memsz > 0) {
      void* dst = (void*)(get_base_addr() + phdr.p_vaddr);
      void* src = (void*)(elf.get_addr() + phdr.p_offset);
      size_t len = phdr.p_memsz;

      assert(len == phdr.p_filesz && "This requires padding, which is not implemented.");

      printf("Copying from %lu bytes from %p to %p.\n", len, src, dst);
      std::memcpy(dst, src, len);
    }
  }
}

ElfLoader::ElfLoader(std::vector<char> data_in)
  : elf_file(data_in), elf({ &*elf_file.begin(), &*elf_file.end() }) { }

ElfLoader ElfLoader::from_file(const char* path) {
  printf("Loading ELF file %s\n", path);
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  MY_ASSERT_PERROR(f.good(), "Failed to open file.");

  std::streamsize size = f.tellg();

  printf("File has length %li.\n", size);
  std::vector<char> data(size);
  f.seekg(0, std::ios::beg);
  f.read(data.data(), size);
  f.close();

  ElfLoader ans(data);

  printf("Succesfully loaded ELF file to memory.\n");
  return ans;
}


void ElfLoader::load() {
  size_t len = get_mmap_len();
  mmap_pages(len);
  copy_segments_to_mem();
  fixup_relocations();
}

void ElfLoader::single_rela_fixup(const Elf64_Shdr& phdr) {
  auto entries = elf.make_span_of_section_data<Elf64_Rela>(phdr);

  for (auto& rela : entries) {
    Elf64_Xword type = ELF64_R_TYPE(rela.r_info);
    Elf64_Xword sym_idx = ELF64_R_SYM(rela.r_info);

    Elf64_Sym& sym = elf.get_symbol_table()[sym_idx];
    char* name = elf.get_symbol_name(sym);

    uintptr_t* fixup_location = (uintptr_t*)(get_base_addr() + rela.r_offset);
    uintptr_t fixup_with = get_base_addr() + sym.st_value;

    switch (type) {
      case R_X86_64_JUMP_SLOT:
      case R_X86_64_GLOB_DAT:
        *fixup_location = fixup_with;
        break;
      case R_X86_64_RELATIVE:
        *fixup_location = fixup_with + rela.r_addend;
        break;
      default:
        MY_ASSERT(false, "Unsupported rela type %lu", type);
    }
  }
}

void ElfLoader::fixup_relocations() {
  for (auto& phdr : elf.get_section_headers()) {
    switch (phdr.sh_type) {
      case SHT_REL:
        assert(false && "REL relocations are not implemented!");
        break;
      case SHT_RELA:
        single_rela_fixup(phdr);
        break;
      default:
        continue;
    }
  }
}

void ElfLoader::unload() {
  
}