#ifndef SHM_ERRS_HPP
#define SHM_ERRS_HPP

#include <string>
#include <string.h>
#include <errno.h>
#include "output.hpp"

namespace llvm {
  typedef bool Error;
};

static inline llvm::Error make_success() { return false; }
static inline llvm::Error make_err_msg(std::string err_msg) { MY_ASSERT(false, "%s", err_msg.c_str()); }
static inline llvm::Error make_err_withno(std::string err_msg) { return make_err_msg(err_msg + " (" + strerror(errno) + ")"); }

#endif // !SHM_ERRS_HPP