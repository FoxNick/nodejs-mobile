#include "crypto/crypto_clienthello-inl.h"
#include "gtest/gtest.h"

// If the test is being compiled with an address sanitizer enabled, it should
// catch the memory violation, so do not use a guard page.
#ifdef __SANITIZE_ADDRESS__
#define NO_GUARD_PAGE
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define NO_GUARD_PAGE
#endif
#endif

// If the test is running without an address sanitizer, see if we can use
// mprotect() or VirtualProtect() to cause a segmentation fault when spatial
// safety is violated.
#if !defined(NO_GUARD_PAGE)
#ifdef __linux__
#include <sys/mman.h>
#include <unistd.h>
#if defined(_SC_PAGE_SIZE) && defined(PROT_NONE) && defined(PROT_READ) &&      \
    defined(PROT_WRITE)
#define USE_MPROTECT
#endif
#elif defined(_WIN32) && defined(_MSC_VER)
#include <Windows.h>
#include <memoryapi.h>
#define USE_VIRTUALPROTECT
#endif
#endif

#if defined(USE_MPROTECT)
size_t GetPageSize() {
  int page_size = sysconf(_SC_PAGE_SIZE);
  CHECK_GE(page_size, 1);
  return page_size;
}
#elif defined(USE_VIRTUALPROTECT)
size_t GetPageSize() {
  SYSTEM_INFO system_info;
  GetSystemInfo(&system_info);
  return system_info.dwPageSize;
}
#endif


// Test that ClientHelloParser::ParseHeader() does not blindly trust the client
// to send a valid frame length and subsequently does not read out-of-bounds.
TEST(NodeCrypto, ClientHelloParserParseHeaderOutOfBoundsRead) {

}
