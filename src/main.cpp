#include <cstdint>
#include "output.hpp"
#include "elf_loader.hpp"


/*
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

*/

int main(int argc, char* argv[]) {
  MY_ASSERT(argc == 2, "2 args, please");
  
  auto elf = ElfLoader::from_file(argv[1]);
  elf.load();

  elf.call<int, int, int>("_Z3mulii", 617, 2);
  elf.call<int, int, int>("_Z3powii", 2, 3);
  elf.call<int, int>("_Z4cubei", 5);

  elf.unload();
  
/*
  // Read the image
  std::vector<char> img = read_entire_file(argv[1]);
  ElfView elf({ &*img.begin(), &*img.end() });

  // Create pages on which to map our library
  uintptr_t base_addr = create_pages_for_mapping(elf);

  // Copy the ELF segments to the pages
  copy_segments_to_mem(elf, base_addr);

  // Print symbol table
  // print_symbol_table(elf);

  relocate(elf, base_addr);

  print_call
  print_call
  print_call
*/
}