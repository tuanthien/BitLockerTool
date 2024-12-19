#include <fmt/format.h>
#include <boost/asio.hpp>
#include "boost/asio/experimental/awaitable_operators.hpp"
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/process/v2.hpp>
#include <ctre-unicode.hpp>

#include <chrono>
#include <string>
#include <variant>

#include "DiskPart.hpp"
#include "Common.hpp"
#include "Command.hpp"
#include "Unit.hpp"

namespace proc = boost::process::v2;
namespace asio = boost::asio;

using namespace boost::asio::experimental::awaitable_operators;
using namespace std::chrono_literals;
using result_type = std::variant<
  std::tuple<boost::system::error_code, int>,
  std::tuple<boost::system::error_code, Blt::DiskPartError>,
  std::tuple<boost::system::error_code>>;


namespace Blt {

auto DiskPartMount(
  asio::io_context &ioc,
  asio::cancellation_signal &cancel,
  asio::readable_pipe &diskpartOut,
  asio::writable_pipe &diskpartIn,
  int desireDiskNumber,
  CapacityBytes desireDiskCapacity,
  int desirePartitionNumber,
  CapacityBytes desirePartitionCapacity,
  char assignLetter) -> asio::awaitable<DiskPartError>
{
  auto state     = DiskPartState::StartUp;
  auto nextState = DiskPartState::StartUp;

  std::u8string buffer;
  constexpr auto toCompatView = [](auto capture) -> std::string_view {
    auto view          = capture.to_view();
    using ViewCharType = typename std::remove_cvref_t<decltype(view)>::value_type;
    return std::string_view(reinterpret_cast<const char *>(view.data()), view.size() * sizeof(ViewCharType));
  };
  auto closeStreamsWithError = [&diskpartOut, &diskpartIn](DiskPartError error) {
    diskpartIn.close();
    diskpartOut.close();
    return error;
  };
  while (true) {
    switch (state) {
    case DiskPartState::StartUp: {
      if (auto error = co_await ReadComputerName(buffer, diskpartOut); error != DiskPartError::Success)
        co_return closeStreamsWithError(error);
      nextState = DiskPartState::ListDisk;
      break;
    }
    case DiskPartState::ListDisk: {
      if (auto error = co_await ListDisk(buffer, diskpartIn); error != DiskPartError::Success)
        co_return closeStreamsWithError(error);
      nextState = DiskPartState::ReadListDisk;
      break;
    }
    case DiskPartState::ReadListDisk: {
      if (auto error = co_await ReadListDisk(buffer, diskpartOut, desireDiskNumber, desireDiskCapacity);
          error != DiskPartError::Success)
        co_return closeStreamsWithError(error);

      nextState = DiskPartState::SelectDisk;
      break;
    }
    case DiskPartState::SelectDisk: {
      if (auto error = co_await SelectDisk(buffer, diskpartIn, desireDiskNumber); error != DiskPartError::Success)
        co_return closeStreamsWithError(error);

      nextState = DiskPartState::ReadSelectDisk;
      break;
    }
    case DiskPartState::ReadSelectDisk: {
      if (auto error = co_await ReadSelectDisk(buffer, diskpartOut, desireDiskNumber); error != DiskPartError::Success)
        co_return closeStreamsWithError(error);

      nextState = DiskPartState::ListPartition;
      break;
    }
    case DiskPartState::ListPartition: {
      if (auto error = co_await ListPartition(buffer, diskpartIn); error != DiskPartError::Success)
        co_return closeStreamsWithError(error);

      nextState = DiskPartState::ReadListPartition;
      break;
    }
    case DiskPartState::ReadListPartition: {
      if (auto error = co_await ReadListPartition(buffer, diskpartOut, desirePartitionNumber, desirePartitionCapacity);
          error != DiskPartError::Success)
        co_return closeStreamsWithError(error);

      nextState = DiskPartState::SelectPartition;
      break;
    }
    case DiskPartState::SelectPartition: {
      if (auto error = co_await SelectPartition(buffer, diskpartIn, desirePartitionNumber);
          error != DiskPartError::Success)
        co_return closeStreamsWithError(error);

      nextState = DiskPartState::ReadSelectPartition;
      break;
    }
    case DiskPartState::ReadSelectPartition: {
      if (auto error = co_await ReadSelectPartition(buffer, diskpartOut, desirePartitionNumber);
          error != DiskPartError::Success)
        co_return closeStreamsWithError(error);

      nextState = DiskPartState::AssignLetter;
      break;
    }
    case DiskPartState::AssignLetter: {
      if (auto error = co_await AssignLetter(buffer, diskpartIn, assignLetter); error != DiskPartError::Success) {
        co_return closeStreamsWithError(error);
      }

      nextState = DiskPartState::ReadAssignLetter;
      break;
    }
    case DiskPartState::ReadAssignLetter: {
      if (auto error = co_await ReadAssignLetter(buffer, diskpartOut); error != DiskPartError::Success) {
        co_return closeStreamsWithError(error);
      }

      nextState = DiskPartState::Exit;
      break;
    }
    case DiskPartState::Exit: {
      if (auto error = co_await Exit(diskpartIn); error != DiskPartError::Success)
        co_return closeStreamsWithError(error);

      co_return DiskPartError::Success;
    }
    }
    buffer.clear();
    state = nextState;
  }
  std::unreachable();
}


auto DiskPartUnmount(
  asio::io_context &ioc,
  asio::cancellation_signal &cancel,
  asio::readable_pipe &diskpartOut,
  asio::writable_pipe &diskpartIn,
  int desireDiskNumber,
  CapacityBytes desireDiskCapacity,
  int desirePartitionNumber,
  CapacityBytes desirePartitionCapacity,
  char assignLetter) -> asio::awaitable<DiskPartError>
{
  auto state     = DiskPartState::StartUp;
  auto nextState = DiskPartState::StartUp;

  std::u8string buffer;
  constexpr auto toCompatView = [](auto capture) -> std::string_view {
    auto view          = capture.to_view();
    using ViewCharType = typename std::remove_cvref_t<decltype(view)>::value_type;
    return std::string_view(reinterpret_cast<const char *>(view.data()), view.size() * sizeof(ViewCharType));
  };

  auto closeStreamsWithError = [&diskpartOut, &diskpartIn](DiskPartError error) {
    diskpartIn.close();
    diskpartOut.close();
    return error;
  };

  while (true) {
    switch (state) {
    case DiskPartState::StartUp: {
      if (auto error = co_await ReadComputerName(buffer, diskpartOut); error != DiskPartError::Success)
        co_return closeStreamsWithError(error);
      nextState = DiskPartState::ListDisk;
      break;
    }
    case DiskPartState::ListDisk: {
      if (auto error = co_await ListDisk(buffer, diskpartIn); error != DiskPartError::Success)
        co_return closeStreamsWithError(error);
      nextState = DiskPartState::ReadListDisk;
      break;
    }
    case DiskPartState::ReadListDisk: {
      if (auto error = co_await ReadListDisk(buffer, diskpartOut, desireDiskNumber, desireDiskCapacity);
          error != DiskPartError::Success)
        co_return closeStreamsWithError(error);

      nextState = DiskPartState::SelectDisk;
      break;
    }
    case DiskPartState::SelectDisk: {
      if (auto error = co_await SelectDisk(buffer, diskpartIn, desireDiskNumber); error != DiskPartError::Success)
        co_return closeStreamsWithError(error);

      nextState = DiskPartState::ReadSelectDisk;
      break;
    }
    case DiskPartState::ReadSelectDisk: {
      if (auto error = co_await ReadSelectDisk(buffer, diskpartOut, desireDiskNumber); error != DiskPartError::Success)
        co_return closeStreamsWithError(error);

      nextState = DiskPartState::ListPartition;
      break;
    }
    case DiskPartState::ListPartition: {
      if (auto error = co_await ListPartition(buffer, diskpartIn); error != DiskPartError::Success)
        co_return closeStreamsWithError(error);

      nextState = DiskPartState::ReadListPartition;
      break;
    }
    case DiskPartState::ReadListPartition: {
      if (auto error = co_await ReadListPartition(buffer, diskpartOut, desirePartitionNumber, desirePartitionCapacity);
          error != DiskPartError::Success)
        co_return closeStreamsWithError(error);

      nextState = DiskPartState::SelectPartition;
      break;
    }
    case DiskPartState::SelectPartition: {
      if (auto error = co_await SelectPartition(buffer, diskpartIn, desirePartitionNumber);
          error != DiskPartError::Success)
        co_return closeStreamsWithError(error);

      nextState = DiskPartState::ReadSelectPartition;
      break;
    }
    case DiskPartState::ReadSelectPartition: {
      if (auto error = co_await ReadSelectPartition(buffer, diskpartOut, desirePartitionNumber);
          error != DiskPartError::Success)
        co_return closeStreamsWithError(error);

      nextState = DiskPartState::RemoveLetter;
      break;
    }
    case DiskPartState::RemoveLetter: {
      if (auto error = co_await RemoveLetter(buffer, diskpartIn, assignLetter); error != DiskPartError::Success) {
        co_return closeStreamsWithError(error);
      }

      nextState = DiskPartState::ReadRemoveLetter;
      break;
    }
    case DiskPartState::ReadRemoveLetter: {
      if (auto error = co_await ReadRemoveLetter(buffer, diskpartOut); error != DiskPartError::Success) {
        co_return closeStreamsWithError(error);
      }

      nextState = DiskPartState::Exit;
      break;
    }
    case DiskPartState::Exit: {
      if (auto error = co_await Exit(diskpartIn); error != DiskPartError::Success)
        co_return closeStreamsWithError(error);

      co_return DiskPartError::Success;
    }
    }
    buffer.clear();
    state = nextState;
  }
  std::unreachable();
}

}// namespace Blt

auto Mount(
  asio::io_context &ioc, const Blt::MountInfo &info, std::string_view diskpartPath, std::string_view bdeunlockPath)
  -> asio::awaitable<void>
{
  asio::steady_timer timeout{co_await asio::this_coro::executor, 100s};
  asio::cancellation_signal sig;
  auto diskpartOut = asio::readable_pipe(co_await asio::this_coro::executor);
  auto diskpartIn  = asio::writable_pipe(co_await asio::this_coro::executor);
  auto result      = co_await (
    proc::async_execute(
      proc::process(
        co_await asio::this_coro::executor, diskpartPath, {}, proc::process_stdio{diskpartIn, diskpartOut, {}}),
      asio::bind_cancellation_slot(sig.slot(), asio::as_tuple(asio::use_awaitable)))
    || Blt::DiskPartMount(
      ioc,
      sig,
      diskpartOut,
      diskpartIn,
      info.Disk.Number,
      info.Disk.Capacity,
      info.Partition.Number,
      info.Partition.Capacity,
      info.Letter)
    || timeout.async_wait(asio::as_tuple(asio::use_awaitable)));

  if (const auto processResult = std::get_if<0>(&result)) {
    timeout.cancel();
    auto [ec, exitCode] = *processResult;
    if (ec == boost::system::errc::success && exitCode == 0) {
      fmt::println("start process success: {}", exitCode);
    }
  } else if (const auto readResult = std::get_if<1>(&result)) {
    timeout.cancel();
    sig.emit(asio::cancellation_type::terminal);
    Blt::DiskPartError opError = *readResult;
    switch (opError) {
    case Blt::DiskPartError::Success: {
      fmt::println("prompt bitlocker password");
      std::array<char, 3> buffer = {info.Letter, ':', '\0'};
      SHELLEXECUTEINFOA execInfo{
        .cbSize       = sizeof(SHELLEXECUTEINFOA),
        .fMask        = SEE_MASK_DEFAULT | SEE_MASK_UNICODE | SEE_MASK_NOCLOSEPROCESS,
        .hwnd         = nullptr,
        .lpVerb       = "runas",
        .lpFile       = bdeunlockPath.data(),
        .lpParameters = buffer.data(),
        .lpDirectory  = nullptr,
        .nShow        = SW_SHOWDEFAULT,
        .hInstApp     = nullptr,
        .hProcess     = nullptr,
      };

      ShellExecuteEx(&execInfo);

      if (execInfo.hProcess) {
        if (WaitForSingleObject(execInfo.hProcess, INFINITE) == WAIT_OBJECT_0) {
          unsigned long exitcode;
          if (GetExitCodeProcess(execInfo.hProcess, &exitcode) != 0)
            fmt::println("bdeunlock exit with code {}", exitcode);
          CloseHandle(execInfo.hProcess);
        } else {
          fmt::println("something went wrong when waiting for bdeunlock");
        }
      }

      fmt::println("mount complete");
      break;
    }
    default: {
      fmt::println("it went to shit");
      break;
    }
    }
    co_return;
  } else if (const auto timeoutResult = std::get_if<2>(&result)) {
    auto [timeoutError] = *timeoutResult;
    if (timeoutError == boost::system::errc::success) {
      fmt::println("something went wrong, timed out");
      sig.emit(asio::cancellation_type::terminal);
    } else {
      fmt::println("unexpected error relates to timeout");
      sig.emit(asio::cancellation_type::terminal);
    }
  }

  co_return;
}

auto Unmount(
  asio::io_context &ioc, const Blt::MountInfo &info, std::string_view diskpartPath, std::string_view managebdePath)
  -> asio::awaitable<void>
{
  // bdeunlockPath
  // "manage-bde -lock -ForceDismount x:"
  fmt::println("locking partition");
  std::array<char, 24> buffer = {'-', 'l', 'o', 'c', 'k', ' ', '-', 'F', 'o', 'r', 'c', 'e',
                                 'D', 'i', 's', 'm', 'o', 'u', 'n', 't', ' ', 'x', ':'};
  buffer[21]                  = info.Letter;

  SHELLEXECUTEINFOA execInfo{
    .cbSize       = sizeof(SHELLEXECUTEINFOA),
    .fMask        = SEE_MASK_DEFAULT | SEE_MASK_UNICODE | SEE_MASK_NOCLOSEPROCESS,
    .hwnd         = nullptr,
    .lpVerb       = "runas",
    .lpFile       = managebdePath.data(),
    .lpParameters = buffer.data(),
    .lpDirectory  = nullptr,
    .nShow        = SW_HIDE,
    .hInstApp     = nullptr,
    .hProcess     = nullptr,
  };

  if (not ShellExecuteEx(&execInfo)) {
    co_return;
  }
  if (execInfo.hProcess) {
    if (WaitForSingleObject(execInfo.hProcess, INFINITE) == WAIT_OBJECT_0) {
      unsigned long exitcode;
      if (GetExitCodeProcess(execInfo.hProcess, &exitcode) != 0) fmt::println("manage-bde exit with code {}", exitcode);
      CloseHandle(execInfo.hProcess);
    } else {
      fmt::println("something went wrong when waiting for manage-bde");
    }
  }
  asio::steady_timer timeout{co_await asio::this_coro::executor, 100s};
  asio::cancellation_signal sig;
  auto diskpartOut = asio::readable_pipe(co_await asio::this_coro::executor);
  auto diskpartIn  = asio::writable_pipe(co_await asio::this_coro::executor);
  auto result      = co_await (
    proc::async_execute(
      proc::process(
        co_await asio::this_coro::executor, diskpartPath, {}, proc::process_stdio{diskpartIn, diskpartOut, {}}),
      asio::bind_cancellation_slot(sig.slot(), asio::as_tuple(asio::use_awaitable)))
    || Blt::DiskPartUnmount(
      ioc,
      sig,
      diskpartOut,
      diskpartIn,
      info.Disk.Number,
      info.Disk.Capacity,
      info.Partition.Number,
      info.Partition.Capacity,
      info.Letter)
    || timeout.async_wait(asio::as_tuple(asio::use_awaitable)));

  if (const auto processResult = std::get_if<0>(&result)) {
    timeout.cancel();
    auto [ec, exitCode] = *processResult;
    if (ec == boost::system::errc::success && exitCode == 0) {
      fmt::println("start process success: {}", exitCode);
    }
  } else if (const auto readResult = std::get_if<1>(&result)) {
    timeout.cancel();
    sig.emit(asio::cancellation_type::terminal);
    Blt::DiskPartError opError = *readResult;
    switch (opError) {
    case Blt::DiskPartError::Success: {
      fmt::println("unmount complete");
      break;
    }
    default: {
      fmt::println("it went to shit");
      break;
    }
    }
    co_return;
  } else if (const auto timeoutResult = std::get_if<2>(&result)) {
    auto [timeoutError] = *timeoutResult;
    if (timeoutError == boost::system::errc::success) {
      fmt::println("something went wrong, timed out");
      sig.emit(asio::cancellation_type::terminal);
    } else {
      fmt::println("unexpected error relates to timeout");
      sig.emit(asio::cancellation_type::terminal);
    }
  }

  co_return;
}

/**
 * BitLockerTool.exe  unmount   0:1863:GiB                6:362:GiB                  X
 * BitLockerTool.exe  mount     0:1863:GiB                6:362:GiB                  X
 *                    <action>  <disk>:<capacity>:<unit>  <disk>:<capacity>:<unit>   <letter>
 */
int main()
{
  auto comInitialized        = false;
  auto comInitializedHResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (comInitialized = (comInitializedHResult == S_OK); not comInitialized) return -1;

  blt_defer{
    if (comInitialized) CoUninitialize();
  };

  auto parseResult = Blt::ParseCommandLine();

  PWSTR windowPath;
  SHGetKnownFolderPath(FOLDERID_Windows, 0, nullptr, &windowPath);
  auto windowPathView = std::wstring_view(static_cast<const wchar_t *>(windowPath), wcslen(windowPath));
  blt_defer{
    CoTaskMemFree(windowPath);
  };

  auto getSystem32Executable = [&windowPathView](std::array<char, MAX_PATH> &utf8buffer, std::string_view executable) {
    constexpr static std::wstring_view systemPath = L"\\System32\\";
    std::array<wchar_t, MAX_PATH> buffer;
    std::copy(windowPathView.cbegin(), windowPathView.cend(), buffer.data());
    std::copy(systemPath.cbegin(), systemPath.cend(), buffer.data() + windowPathView.size());
    std::copy(executable.cbegin(), executable.cend(), buffer.data() + windowPathView.size() + systemPath.size());
    const auto pathSize = windowPathView.size() + systemPath.size() + executable.size();
    buffer[pathSize]    = L'\0';

    auto written =
      WideCharToMultiByte(CP_UTF8, 0, buffer.data(), static_cast<int>(pathSize), utf8buffer.data(), static_cast<int>(utf8buffer.size()), nullptr, nullptr);
    assert(written);
    assert(utf8buffer.size() >= (written + 1));
    utf8buffer[written] = '\0';

    return written;
  };

  std::array<char, MAX_PATH> diskpartBuffer;
  auto diskpartPath = std::string_view(diskpartBuffer.data(), getSystem32Executable(diskpartBuffer, "diskpart.exe"));

  std::array<char, MAX_PATH> bdeunlockBuffer;
  auto bdeunlockPath =
    std::string_view(bdeunlockBuffer.data(), getSystem32Executable(bdeunlockBuffer, "bdeunlock.exe"));

  std::array<char, MAX_PATH> managebdeBuffer;
  auto managebdePath =
    std::string_view(managebdeBuffer.data(), getSystem32Executable(managebdeBuffer, "manage-bde.exe"));

  if (not parseResult) {
    switch (parseResult.error()) {
    case Blt::ParseCommandLineError::GetCommandLineFailed: {
      break;
    }
    case Blt::ParseCommandLineError::UnknownAction: {
      break;
    }
    case Blt::ParseCommandLineError::ParseFailed: {
      break;
    }
    }
    return static_cast<int>(parseResult.error());
  }

  asio::io_context ioc;
  boost::system::error_code ec;
  constexpr auto exceptionHandler = [](std::exception_ptr e) {
    if (e) try {
        std::rethrow_exception(e);
      } catch (std::exception &ex) {
        fmt::println("Error ===> {}", ex.what());
      }
  };

  switch (parseResult->Action) {
  case Blt::CommandAction::Mount: {
    asio::co_spawn(ioc, Mount(ioc, *parseResult, diskpartPath, bdeunlockPath), exceptionHandler);
    break;
  }
  case Blt::CommandAction::Unmount: {
    asio::co_spawn(ioc, Unmount(ioc, *parseResult, diskpartPath, managebdePath), exceptionHandler);
    break;
  }
  }

  ioc.run();

  return EXIT_SUCCESS;
}