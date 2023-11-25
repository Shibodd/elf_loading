#include <cstdint>
#include <fstream>
#include "output.hpp"
#include "elf_loader.hpp"

std::vector<char> load_entire_file(const char* path) {
  printf("Loading ELF file %s\n", path);
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  MY_ASSERT_PERROR(f.good(), "Failed to open file.");

  std::streamsize size = f.tellg();
  std::vector<char> ans(size);

  printf("File has length %li.\n", size);
  f.seekg(0, std::ios::beg);
  f.read(ans.data(), size);
  f.close();

  printf("Succesfully loaded ELF file to memory.\n");
  return ans;
}

extern "C" {
#include <sys/mman.h>
}


Span<char> mapper(size_t len) {
  printf("Mmapping %lu bytes\n", len);
  int prot = PROT_READ | PROT_WRITE | PROT_EXEC;
  int flags = MAP_ANONYMOUS | MAP_PRIVATE;

  void* base_ptr = mmap(NULL, len, prot, flags, -1, 0);
  MY_ASSERT_PERROR(base_ptr != MAP_FAILED, "Failed to mmap.");
  return { (char*)base_ptr, (char*)base_ptr + len };
}

void unmapper(Span<char> span) {
  munmap(span.begin(), span.len());
}


int main(int argc, char* argv[]) {
  MY_ASSERT(argc == 2, "2 args, please");

  std::vector<char> elf_file = load_entire_file(argv[1]);

  auto elf = ElfLoader({ &*elf_file.begin(), &*elf_file.end() }, mapper, unmapper);
  elf.load();

  elf.call<int, int, int>("_Z3mulii", 617, 2);
  elf.call<int, int, int>("_Z3powii", 2, 3);
  elf.call<int, int>("_Z4cubei", 10);

  elf.unload();
}