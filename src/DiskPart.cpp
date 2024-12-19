#include "DiskPart.hpp"

#include <fmt/format.h>
#include <boost/asio.hpp>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <ctre-unicode.hpp>

#include "Common.hpp"
#include "Unit.hpp"

namespace Blt {

namespace asio = boost::asio;

auto ReadComputerName(std::u8string &buffer, asio::readable_pipe &diskpartOut) -> asio::awaitable<DiskPartError>
{
  auto [ec, size] = co_await asio::async_read_until(
    diskpartOut, asio::dynamic_buffer(buffer, 1024), "DISKPART>", asio::as_tuple(asio::use_awaitable));
  if (ec == boost::system::errc::success) {
    auto [_, computerName] = ctre::search<"On computer: (.*?)\r\n">(buffer);
    fmt::println("Computer: {}", toCompatView(computerName));
    co_return DiskPartError::Success;
  } else {
    fmt::println("{}", ec.what());
    co_return DiskPartError::IO;
  }
}

auto ListDisk(std::u8string &buffer, asio::writable_pipe &diskpartIn) -> asio::awaitable<DiskPartError>
{
  auto [ec, size] =
    co_await asio::async_write(diskpartIn, asio::buffer("list disk"), asio::as_tuple(asio::use_awaitable));
  if (ec == boost::system::errc::success) {
    fmt::println("listing disk");
    co_return DiskPartError::Success;
  } else {
    fmt::println("{}", ec.what());
    co_return DiskPartError::IO;
  }
}

auto ReadListDisk(
  std::u8string &buffer, asio::readable_pipe &diskpartOut, int desireDiskNumber, CapacityBytes desireDiskCapacity)
  -> asio::awaitable<DiskPartError>
{
  if (auto [read_ec, size] = co_await asio::async_read_until(
        diskpartOut, asio::dynamic_buffer(buffer, 1024), "DISKPART>", asio::as_tuple(asio::use_awaitable));
      read_ec != boost::system::errc::success) {
    fmt::println("{}", read_ec.what());
    co_return DiskPartError::IO;
  }

  // should we assume GiB or should we parse this
  auto diskRange = ctre::multiline_search_all<"Disk\\h+(\\d+)\\h+.+?\\h+(\\d+)\\h(.+?)\\h+.+">(buffer);
  auto foundDisk = false;
  for (auto disk : diskRange) {
    int diskNumber;
    {
      auto compatView = toCompatView(disk.get<1>());
      if (auto [_, convert_ec] =
            std::from_chars(compatView.data(), compatView.data() + compatView.size(), diskNumber, 10);
          convert_ec != std::error_code()) {
        // assert(false && "unable to parse diskpart output for diskNumber");
        co_return DiskPartError::ParseFailed;
      }
    }

    int diskCapacityValue;
    {
      auto compatView = toCompatView(disk.get<2>());
      if (auto [_, ec] =
            std::from_chars(compatView.data(), compatView.data() + compatView.size(), diskCapacityValue, 10);
          ec != std::error_code()) {
        // assert(false && "unable to parse diskpart output for diskNumber");
        co_return DiskPartError::ParseFailed;
      }
    }

    CapacityBytes diskCapacity;
    {
      auto capacityStr = toCompatView(disk.get<3>());
      if (capacityStr == "KB") {
        diskCapacity = capacityCast<CapacityBytes>(Kibibytes(diskCapacityValue));
      } else if (capacityStr == "MB") {
        diskCapacity = capacityCast<CapacityBytes>(Mebibytes(diskCapacityValue));
      } else if (capacityStr == "GB") {
        diskCapacity = capacityCast<CapacityBytes>(Gibibytes(diskCapacityValue));
      } else {
        co_return DiskPartError::ParseFailed;
      }
    }

    if (diskNumber == desireDiskNumber and diskCapacity == desireDiskCapacity) {
      fmt::println("Found desire disk: #{}", diskNumber);
      foundDisk = true;
    }
  }
  if (not foundDisk) {
    // assert(false);
    co_return DiskPartError::MismatchDisk;
  }
  co_return DiskPartError::Success;
}

auto SelectDisk(std::u8string &buffer, asio::writable_pipe &diskpartIn, int desireDiskNumber)
  -> asio::awaitable<DiskPartError>
{
  fmt::format_to(std::back_inserter(buffer), "select disk {}\n", desireDiskNumber);
  auto [ec, size] = co_await asio::async_write(diskpartIn, asio::buffer(buffer), asio::as_tuple(asio::use_awaitable));
  if (ec == boost::system::errc::success) {
    fmt::println("selecting disk #{}", desireDiskNumber);
    co_return DiskPartError::Success;
  } else {
    fmt::println("{}", ec.what());
    co_return DiskPartError::IO;
  }
}

auto ReadSelectDisk(std::u8string &buffer, asio::readable_pipe &diskpartOut, int desireDiskNumber)
  -> asio::awaitable<DiskPartError>
{
  if (auto [read_ec, size] = co_await asio::async_read_until(
        diskpartOut, asio::dynamic_buffer(buffer, 1024), "DISKPART>", asio::as_tuple(asio::use_awaitable));
      read_ec != boost::system::errc::success) {
    fmt::println("{}", read_ec.what());
    co_return DiskPartError::IO;
  }

  auto [_, diskNumberCapture] = ctre::search<"Disk (\\d+) is now the selected disk.\r\n">(buffer);
  int diskNumber;
  {
    auto compatView = toCompatView(diskNumberCapture);

    if (auto [_, ec] = std::from_chars(compatView.data(), compatView.data() + compatView.size(), diskNumber, 10);
        ec != std::error_code()) {
      // assert(false && "unable to parse diskpart output for diskNumber");
      co_return DiskPartError::ParseFailed;
    }
  }
  if (diskNumber == desireDiskNumber) {
    fmt::println("disk #{} selected", diskNumber);
    co_return DiskPartError::Success;
  } else {
    // assert(false && "selected disk is different from desired disk");
    co_return DiskPartError::ParseFailed;
  }
}

auto ListPartition(std::u8string &buffer, asio::writable_pipe &diskpartIn) -> asio::awaitable<DiskPartError>
{
  auto [ec, size] =
    co_await asio::async_write(diskpartIn, asio::buffer("list partition"), asio::as_tuple(asio::use_awaitable));
  if (ec == boost::system::errc::success) {
    fmt::println("listing partition");
    co_return DiskPartError::Success;
  } else {
    fmt::println("{}", ec.what());
    co_return DiskPartError::IO;
  }
}

auto ReadListPartition(
  std::u8string &buffer,
  asio::readable_pipe &diskpartOut,
  int desirePartitionNumber,
  CapacityBytes desirePartitionCapacity) -> asio::awaitable<DiskPartError>
{
  if (auto [read_ec, size] = co_await asio::async_read_until(
        diskpartOut, asio::dynamic_buffer(buffer, 1024 * 2), "DISKPART>", asio::as_tuple(asio::use_awaitable));
      read_ec != boost::system::errc::success) {
    fmt::println("{}", read_ec.what());
    co_return DiskPartError::IO;
  }

  // should we assume GiB or should we parse this
  auto partitionRange = ctre::multiline_search_all<"Partition\\h+(\\d+)\\h+.+?\\h+(\\d+)\\h(.+?)\\h+.+">(buffer);
  auto foundPartition = false;
  for (auto partition : partitionRange) {
    int partitionNumber;
    {
      auto compatView = toCompatView(partition.get<1>());
      if (auto [_, ec] = std::from_chars(compatView.data(), compatView.data() + compatView.size(), partitionNumber, 10);
          ec != std::error_code()) {
        // assert(false && "unable to parse partitionpart output for partitionNumber");
        co_return DiskPartError::ParseFailed;
      }
    }

    int partitionCapacityValue;
    {
      auto compatView = toCompatView(partition.get<2>());
      auto [_, ec] =
        std::from_chars(compatView.data(), compatView.data() + compatView.size(), partitionCapacityValue, 10);
      if (ec != std::error_code()) {
        // assert(false && "unable to parse partitionpart output for partitionNumber");
        co_return DiskPartError::ParseFailed;
      }
    }

    CapacityBytes partitionCapacity;
    {
      auto capacityStr = toCompatView(partition.get<3>());
      if (capacityStr == "KB") {
        partitionCapacity = capacityCast<CapacityBytes>(Kibibytes(partitionCapacityValue));
      } else if (capacityStr == "MB") {
        partitionCapacity = capacityCast<CapacityBytes>(Mebibytes(partitionCapacityValue));
      } else if (capacityStr == "GB") {
        partitionCapacity = capacityCast<CapacityBytes>(Gibibytes(partitionCapacityValue));
      } else {
        co_return DiskPartError::ParseFailed;
      }
    }
    if (desirePartitionNumber == partitionNumber and desirePartitionCapacity == partitionCapacity) {
      fmt::println("found desired partition #{}", partitionNumber);
      foundPartition = true;
      co_return DiskPartError::Success;
    }
  }
  if (not foundPartition) {
    assert(false);
    co_return DiskPartError::MismatchPartition;
  }
}

auto SelectPartition(std::u8string &buffer, asio::writable_pipe &diskpartIn, int desirePartitionNumber)
  -> asio::awaitable<DiskPartError>
{
  fmt::format_to(std::back_inserter(buffer), "select partition {}\n", desirePartitionNumber);
  auto [ec, size] = co_await asio::async_write(diskpartIn, asio::buffer(buffer), asio::as_tuple(asio::use_awaitable));
  if (ec == boost::system::errc::success) {
    fmt::println("selecting partition #{}", desirePartitionNumber);
    co_return DiskPartError::Success;
  } else {
    fmt::println("{}", ec.what());
    co_return DiskPartError::IO;
  }
}

auto ReadSelectPartition(std::u8string &buffer, asio::readable_pipe &diskpartOut, int desirePartitionNumber)
  -> asio::awaitable<DiskPartError>
{
  if (auto [read_ec, size] = co_await asio::async_read_until(
        diskpartOut, asio::dynamic_buffer(buffer, 1024 * 5), "DISKPART>", asio::as_tuple(asio::use_awaitable));
      read_ec != boost::system::errc::success) {
    fmt::println("{}", read_ec.what());
    co_return DiskPartError::IO;
  }

  auto [_, selectedPartitionCapture] = ctre::search<"Partition (\\d+) is now the selected partition">(buffer);
  int partitionNumber;
  {
    auto compatView = toCompatView(selectedPartitionCapture);
    if (auto [_, ec] = std::from_chars(compatView.data(), compatView.data() + compatView.size(), partitionNumber, 10);
        ec != std::error_code()) {
      // assert(false && "unable to parse diskpart output for selected partition number");
      co_return DiskPartError::ParseFailed;
    }
  }
  if (partitionNumber == desirePartitionNumber) {
    fmt::println("partition #{} selected", partitionNumber);
    co_return DiskPartError::Success;
  } else {
    // assert(false && "unspected diskpart select undesirable partion number");
    co_return DiskPartError::SelectPartitionFailed;
  }
}

auto AssignLetter(std::u8string &buffer, asio::writable_pipe &diskpartIn, char assignLetter)
  -> asio::awaitable<DiskPartError>
{
  fmt::format_to(std::back_inserter(buffer), "assign letter={}\n", assignLetter);
  auto [ec, size] = co_await asio::async_write(diskpartIn, asio::buffer(buffer), asio::as_tuple(asio::use_awaitable));
  if (ec == boost::system::errc::success) {
    fmt::println("assigning partition to letter {:?}", assignLetter);
    co_return DiskPartError::Success;
  } else {
    fmt::println("{}", ec.what());
    co_return DiskPartError::IO;
  }
}

auto ReadAssignLetter(std::u8string &buffer, asio::readable_pipe &diskpartOut) -> asio::awaitable<DiskPartError>
{
  auto [ec, size] = co_await asio::async_read_until(
    diskpartOut, asio::dynamic_buffer(buffer, 1024 * 5), "DISKPART>", asio::as_tuple(asio::use_awaitable));
  if (ec == boost::system::errc::success) {
    if (ctre::multiline_search<"DiskPart successfully assigned the drive letter or mount point.">(buffer)) {
      fmt::println("successfully assign drive letter");
      co_return DiskPartError::Success;
    } else {
      fmt::println("diskpart: {}", std::string_view(reinterpret_cast<const char *>(buffer.data()), buffer.size()));
      // assert(false && "unexpected unsuccessfully assign drive letter");
      co_return DiskPartError::AssignLetterFailed;
    }
  } else {
    fmt::println("{}", ec.what());
    co_return DiskPartError::IO;
  }
}

auto RemoveLetter(std::u8string &buffer, asio::writable_pipe &diskpartIn, char removeLetter)
  -> asio::awaitable<DiskPartError>
{
  fmt::format_to(std::back_inserter(buffer), "remove letter={}\n", removeLetter);
  auto [ec, size] = co_await asio::async_write(diskpartIn, asio::buffer(buffer), asio::as_tuple(asio::use_awaitable));
  if (ec == boost::system::errc::success) {
    fmt::println("removing partition to letter {:?}", removeLetter);
    co_return DiskPartError::Success;
  } else {
    fmt::println("{}", ec.what());
    co_return DiskPartError::IO;
  }
}

auto ReadRemoveLetter(std::u8string &buffer, asio::readable_pipe &diskpartOut) -> asio::awaitable<DiskPartError>
{
  auto [ec, size] = co_await asio::async_read_until(
    diskpartOut, asio::dynamic_buffer(buffer, 1024 * 5), "DISKPART>", asio::as_tuple(asio::use_awaitable));
  if (ec == boost::system::errc::success) {
    if (ctre::multiline_search<"DiskPart successfully removed the drive letter or mount point.">(buffer)) {
      fmt::println("successfully remove drive letter");
      co_return DiskPartError::Success;
    } else {
      fmt::println("diskpart: {}", std::string_view(reinterpret_cast<const char *>(buffer.data()), buffer.size()));
      // assert(false && "unexpected unsuccessfully remove drive letter");
      co_return DiskPartError::RemoveLetterFailed;
    }
  } else {
    fmt::println("{}", ec.what());
    co_return DiskPartError::IO;
  }
}

auto Exit(asio::writable_pipe &diskpartIn) -> asio::awaitable<DiskPartError>
{
  auto [ec, size] = co_await asio::async_write(diskpartIn, asio::buffer("exit"), asio::as_tuple(asio::use_awaitable));
  if (ec == boost::system::errc::success) {
    fmt::println("exiting diskpart");
    co_return DiskPartError::Success;
  } else {
    fmt::println("{}", ec.what());
    co_return DiskPartError::IO;
  }
}

}// namespace Blt