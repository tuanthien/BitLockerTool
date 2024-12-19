#include "Command.hpp"

#include <array>
#include <cassert>
#include <charconv>
#include <ctre-unicode.hpp>
#include <system_error>
#include <expected>

#include "Unit.hpp"


namespace Blt {

[[nodiscard]] auto ParseCommandLine() -> std::expected<MountInfo, ParseCommandLineError>
{
  MountInfo actionInfo;
  actionInfo.Action = CommandAction::Unknown;

  LPWSTR *szArglist;
  int nArgs;

  szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
  if (NULL == szArglist) {
    return std::expected<MountInfo, ParseCommandLineError>(std::unexpect, ParseCommandLineError::GetCommandLineFailed);
  }

  blt_defer {
    LocalFree(szArglist);
  };

  /*
  0: program
  1: action
  2: drive_number:capacity:unit
  3: partition_number:capacity:unit
  4: letter
  */
  std::array<char, 1024> buffer;
  if (nArgs >= 2) {
    auto written = WideCharToMultiByte(
      CP_UTF8, 0, szArglist[1], static_cast<int>(wcslen(szArglist[1])), buffer.data(), static_cast<int>(buffer.size()), nullptr, nullptr);
    assert(written);
    auto actionView = std::string_view(buffer.data(), written);

    if (actionView == "mount") {
      actionInfo.Action = CommandAction::Mount;
    } else if (actionView == "unmount") {
      actionInfo.Action = CommandAction::Unmount;
    }
  }
  constexpr auto getCapacity =
    [](std::string_view capacityStr, uint64_t capacityValue) -> std::expected<CapacityBytes, ParseCommandLineError> {
    using ReturnType = std::expected<CapacityBytes, ParseCommandLineError>;
    if (capacityStr == "KiB") {
      return ReturnType(std::in_place, capacityCast<CapacityBytes>(Kibibytes(capacityValue)));
    } else if (capacityStr == "MiB") {
      return ReturnType(std::in_place, capacityCast<CapacityBytes>(Mebibytes(capacityValue)));
    } else if (capacityStr == "GiB") {
      return ReturnType(std::in_place, capacityCast<CapacityBytes>(Gibibytes(capacityValue)));
    } else {
      return ReturnType(std::unexpect, ParseCommandLineError::ParseFailed);
    }
  };

  uint64_t capacityValue;
  switch (actionInfo.Action) {
  case CommandAction::Mount:
    [[fallthrough]];
  case CommandAction::Unmount: {
    assert(nArgs == 5);
    {
      {
        auto written = WideCharToMultiByte(
          CP_UTF8, 0, szArglist[2], static_cast<int>(wcslen(szArglist[2])), buffer.data(), static_cast<int>(buffer.size()), nullptr, nullptr);
        assert(written);
        auto driveView                                        = std::string_view(buffer.data(), written);
        auto [_, numberCapture, capacityCapture, unitCapture] = ctre::search<"^(\\d+):(\\d+):(.+)">(driveView);
        {
          auto view    = numberCapture.to_view();
          auto [_, ec] = std::from_chars(view.data(), view.data() + view.size(), actionInfo.Disk.Number, 10);
          if (ec != std::error_code())
            return std::expected<MountInfo, ParseCommandLineError>(std::unexpect, ParseCommandLineError::ParseFailed);
        }
        {
          auto view    = capacityCapture.to_view();
          auto [_, ec] = std::from_chars(view.data(), view.data() + view.size(), capacityValue, 10);
          if (ec != std::error_code())
            return std::expected<MountInfo, ParseCommandLineError>(std::unexpect, ParseCommandLineError::ParseFailed);
        }

        {
          if (auto capacityResult = getCapacity(unitCapture.to_view(), capacityValue); capacityResult) {
            actionInfo.Disk.Capacity = *capacityResult;
          } else {
            return std::expected<MountInfo, ParseCommandLineError>(std::unexpect, capacityResult.error());
          }
        }
      }
      {
        auto written = WideCharToMultiByte(
          CP_UTF8,
          0,
          szArglist[3],
          static_cast<int>(wcslen(szArglist[3])),
          buffer.data(),
          static_cast<int>(buffer.size()),
          nullptr,
          nullptr);
        assert(written);
        auto partitionView                                    = std::string_view(buffer.data(), written);
        auto [_, numberCapture, capacityCapture, unitCapture] = ctre::search<"^(\\d+):(\\d+):(.+)">(partitionView);
        {
          auto view    = numberCapture.to_view();
          auto [_, ec] = std::from_chars(view.data(), view.data() + view.size(), actionInfo.Partition.Number, 10);
          if (ec != std::error_code())
            return std::expected<MountInfo, ParseCommandLineError>(std::unexpect, ParseCommandLineError::ParseFailed);
        }
        {
          auto view    = capacityCapture.to_view();
          auto [_, ec] = std::from_chars(view.data(), view.data() + view.size(), capacityValue, 10);
          if (ec != std::error_code())
            return std::expected<MountInfo, ParseCommandLineError>(std::unexpect, ParseCommandLineError::ParseFailed);
        }
        {
          if (auto capacityResult = getCapacity(unitCapture.to_view(), capacityValue); capacityResult) {
            actionInfo.Partition.Capacity = *capacityResult;
          } else {
            return std::expected<MountInfo, ParseCommandLineError>(std::unexpect, capacityResult.error());
          }
        }
      }
      {
        auto written = WideCharToMultiByte(
          CP_UTF8,
          0,
          szArglist[4],
          static_cast<int>(wcslen(szArglist[4])),
          buffer.data(),
          static_cast<int>(buffer.size()),
          nullptr,
          nullptr);
        assert(written);
        auto letterView               = std::string_view(buffer.data(), written);
        constexpr auto validateLetter = [](char letter) {
          return (letter >= 'a' and letter < 'z') or (letter >= 'A' and letter <= 'Z');
        };
        if (letterView.size() == 1 and validateLetter(letterView[0])) {
          actionInfo.Letter = letterView[0];
        } else {
          return std::expected<MountInfo, ParseCommandLineError>(std::unexpect, ParseCommandLineError::ParseFailed);
        }
      }
    }
    break;
  }
  case CommandAction::Unknown: {
    return std::expected<MountInfo, ParseCommandLineError>(std::unexpect, ParseCommandLineError::UnknownAction);
  }
  }
  return std::expected<MountInfo, ParseCommandLineError>(std::in_place, actionInfo);
}
}// namespace Blt