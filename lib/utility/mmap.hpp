//
// Created by Iwabuchi, Keita on 6/22/18.
//

#ifndef SIMPLE_BFS_MMAP_UTILITY_HPP
#define SIMPLE_BFS_MMAP_UTILITY_HPP

#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <cstddef>

#include "file.hpp"

namespace utility {

/// \brief Get the page size
/// \return The size of page size. Return -1 for error cases.
ssize_t get_page_size() {
  const ssize_t page_size = ::sysconf(_SC_PAGE_SIZE);
  if (page_size == -1) {
    ::perror("sysconf(_SC_PAGE_SIZE)");
    std::cerr << "errno: " << errno << std::endl;
  }

  return page_size;
}

/// \brief Map a file checking errors
/// \param addr Same as mmap(2)
/// \param length Same as mmap(2)
/// \param protection Same as mmap(2)
/// \param flags Same as mmap(2)
/// \param fd Same as mmap(2)
/// \param offset  Same as mmap(2)
/// \return On success, returns a pointer to the mapped area.
/// On error, nullptr is returned.
void *map_file(void *const addr, const size_t length, const int protection, const int flags,
               const int fd, const off_t offset) {
  const ssize_t page_size = get_page_size();
  if (page_size == -1) {
    return nullptr;
  }

  if ((std::ptrdiff_t)addr % page_size != 0) {
    std::cerr << "address (" << addr << ") is not page aligned ("
              << ::sysconf(_SC_PAGE_SIZE) << ")" << std::endl;
    return nullptr;
  }

  if (offset % page_size != 0) {
    std::cerr << "offset (" << offset << ") is not a multiple of the page size (" << ::sysconf(_SC_PAGE_SIZE) << ")"
              << std::endl;
    return nullptr;
  }

  /// ----- Map the file ----- ///
  void *mapped_addr = ::mmap(addr, length, protection, flags, fd, offset);
  if (mapped_addr == MAP_FAILED) {
    ::perror("mmap");
    std::cerr << "errno: " << errno << std::endl;
    return nullptr;
  }

  if ((std::ptrdiff_t)mapped_addr % page_size != 0) {
    std::cerr << "mapped address (" << mapped_addr << ") is not page aligned ("
              << ::sysconf(_SC_PAGE_SIZE) << ")" << std::endl;
    return nullptr;
  }

  return mapped_addr;
}

/// \brief Map a file with read mode
/// \param file_name The name of file to be mapped
/// \param addr Normaly nullptr; if this is not nullptr the kernel takes it as a hint about where to place the mapping
/// \param length The lenght of the map
/// \param offset The offset in the file
/// \return A pair of the file descriptor of the file and the starting address for the map
std::pair<int, void *> map_file_read_mode(const std::string &file_name, void *const addr,
                                          const size_t length, const off_t offset,
                                          const int additional_flags = 0) {
  /// ----- Open the file ----- ///
  const int fd = ::open(file_name.c_str(), O_RDONLY);
  if (fd == -1) {
    ::perror("open");
    std::cerr << "errno: " << errno << std::endl;
    return std::make_pair(-1, nullptr);
  }

  /// ----- Map the file ----- ///
  void *mapped_addr = map_file(addr, length, PROT_READ, MAP_SHARED | additional_flags, fd, offset);
  if (mapped_addr == nullptr) {
    close(fd);
    return std::make_pair(-1, nullptr);
  }

  return std::make_pair(fd, mapped_addr);
}

/// \brief Map a file with write mode
/// \param file_name The name of file to be mapped
/// \param addr Normaly nullptr; if this is not nullptr the kernel takes it as a hint about where to place the mapping
/// \param length The lenght of the map
/// \param offset The offset in the file
/// \return A pair of the file descriptor of the file and the starting address for the map
std::pair<int, void *> map_file_write_mode(const std::string &file_name, void *const addr,
                                           const size_t length, const off_t offset,
                                           const int additional_flags = 0) {
  /// ----- Open the file ----- ///
  const int fd = ::open(file_name.c_str(), O_RDWR);
  if (fd == -1) {
    ::perror("open");
    std::cerr << "errno: " << errno << std::endl;
    return std::make_pair(-1, nullptr);
  }

  /// ----- Map the file ----- ///
  void *mapped_addr = map_file(addr, length, PROT_READ | PROT_WRITE, MAP_SHARED | additional_flags, fd, offset);
  if (mapped_addr == nullptr) {
    close(fd);
    return std::make_pair(-1, nullptr);
  }

  return std::make_pair(fd, mapped_addr);
}

void msync(void *const addr, const size_t length) {
  if (::msync(addr, length, MS_SYNC) != 0) {
    ::perror("msync");
    std::cerr << "errno: " << errno << std::endl;
  }
}

void munmap(void *const addr, const size_t length, const bool call_msync) {
  if (call_msync) msync(addr, length);

  if (::munmap(addr, length) != 0) {
    ::perror("munmap");
    std::cerr << "errno: " << errno << std::endl;
  }
}

} // namespace utility

#endif //SIMPLE_BFS_MMAP_UTILITY_HPP
