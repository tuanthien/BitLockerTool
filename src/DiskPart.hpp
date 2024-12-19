#pragma once

#include <string>
#include <boost/asio/readable_pipe.hpp>
#include <boost/asio/writable_pipe.hpp>
#include <boost/asio/awaitable.hpp>

#include "Unit.hpp"

namespace Blt {

enum struct DiskPartState {
  StartUp,
  ListDisk,
  ReadListDisk,
  SelectDisk,
  ReadSelectDisk,
  ListPartition,
  ReadListPartition,
  SelectPartition,
  ReadSelectPartition,
  AssignLetter,
  ReadAssignLetter,
  RemoveLetter,
  ReadRemoveLetter,
  Exit
};

enum struct DiskPartError {
  Success = 0,
  MismatchComputer,
  MismatchDisk,
  MismatchPartition,
  SelectDiskFailed,
  SelectPartitionFailed,
  AssignLetterFailed,
  RemoveLetterFailed,
  ParseFailed,
  IO,
};


auto ReadComputerName(std::u8string &buffer, boost::asio::readable_pipe &diskpartOut)
  -> boost::asio::awaitable<DiskPartError>;

auto ListDisk(std::u8string &buffer, boost::asio::writable_pipe &diskpartIn) -> boost::asio::awaitable<DiskPartError>;

auto ReadListDisk(
  std::u8string &buffer, boost::asio::readable_pipe &diskpartOut, int desireDiskNumber, CapacityBytes desireDiskCapacity)
  -> boost::asio::awaitable<DiskPartError>;

auto SelectDisk(std::u8string &buffer, boost::asio::writable_pipe &diskpartIn, int desireDiskNumber)
  -> boost::asio::awaitable<DiskPartError>;

auto ReadSelectDisk(std::u8string &buffer, boost::asio::readable_pipe &diskpartOut, int desireDiskNumber)
  -> boost::asio::awaitable<DiskPartError>;

auto ListPartition(std::u8string &buffer, boost::asio::writable_pipe &diskpartIn)
  -> boost::asio::awaitable<DiskPartError>;

auto SelectPartition(std::u8string &buffer, boost::asio::writable_pipe &diskpartIn, int desirePartitionNumber)
  -> boost::asio::awaitable<DiskPartError>;

auto ReadListPartition(
  std::u8string &buffer,
  boost::asio::readable_pipe &diskpartOut,
  int desirePartitionNumber,
  CapacityBytes desirePartitionCapacity) -> boost::asio::awaitable<DiskPartError>;

auto SelectPartition(std::u8string &buffer, boost::asio::writable_pipe &diskpartIn, int desirePartitionNumber)
  -> boost::asio::awaitable<DiskPartError>;

auto ReadSelectPartition(std::u8string &buffer, boost::asio::readable_pipe &diskpartOut, int desirePartitionNumber)
  -> boost::asio::awaitable<DiskPartError>;

auto AssignLetter(std::u8string &buffer, boost::asio::writable_pipe &diskpartIn, char assignLetter)
  -> boost::asio::awaitable<DiskPartError>;

auto ReadAssignLetter(std::u8string &buffer, boost::asio::readable_pipe &diskpartOut)
  -> boost::asio::awaitable<DiskPartError>;

auto RemoveLetter(std::u8string &buffer, boost::asio::writable_pipe &diskpartIn, char removeLetter)
  -> boost::asio::awaitable<DiskPartError>;

auto ReadRemoveLetter(std::u8string &buffer, boost::asio::readable_pipe &diskpartOut) -> boost::asio::awaitable<DiskPartError>;

auto Exit(boost::asio::writable_pipe &diskpartIn) -> boost::asio::awaitable<DiskPartError>;

}// namespace Blt