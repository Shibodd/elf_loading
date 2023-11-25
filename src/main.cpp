#include <cstdint>
#include "output.hpp"
#include "elf_loader.hpp"

int main(int argc, char* argv[]) {
  MY_ASSERT(argc == 2, "2 args, please");
  
  auto elf = ElfLoader::from_file(argv[1]);
  elf.load();

  elf.call<int, int, int>("_Z3mulii", 617, 2);
  elf.call<int, int, int>("_Z3powii", 2, 3);
  elf.call<int, int>("_Z4cubei", 10);

  elf.unload();
}