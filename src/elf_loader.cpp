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

bool ElfLoader::is_segment_loadable(const Elf64_Phdr& seg) {
  return seg.p_type == PT_LOAD && seg.p_memsz > 0;
}

size_t ElfLoader::get_mmap_len() {
  printf("Computing mmap len.\n");

  uintptr_t vaddr_min = std::numeric_limits<uintptr_t>::max();
  uintptr_t vaddr_max = std::numeric_limits<uintptr_t>::min();

  for (auto& seg : elf.get_segment_headers()) {
    if (is_segment_loadable(seg)) {
      vaddr_min = std::min(vaddr_min, seg.p_vaddr);
      vaddr_max = std::max(vaddr_max, seg.p_vaddr + seg.p_memsz);
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
  for (auto& seg : elf.get_segment_headers()) {
    if (is_segment_loadable(seg)) {
      void* dst = (void*)(get_base_addr() + seg.p_vaddr);
      void* src = (void*)(elf.get_addr() + seg.p_offset);
      size_t len = seg.p_memsz;

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
}

void ElfLoader::relocate() {
  /*
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
  */
}

void ElfLoader::unload() {
  
}