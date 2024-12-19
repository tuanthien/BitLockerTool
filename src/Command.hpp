#pragma once

#include "Common.hpp"
#include "Unit.hpp"

#include <expected>

namespace Blt {

enum struct CommandAction {
  Unknown,
  Mount,
  Unmount
};

struct DriveId
{
  int Number;
  CapacityBytes Capacity;
};

struct PatitionId
{
  int Number;
  CapacityBytes Capacity;
};

struct MountInfo
{
  CommandAction Action;
  DriveId Disk;
  PatitionId Partition;
  char Letter;
};

enum struct ParseCommandLineError {
  GetCommandLineFailed = 1,
  UnknownAction,
  UnsupportedCapacityUnit,
  ParseFailed,
};

auto ParseCommandLine() -> std::expected<MountInfo, ParseCommandLineError>;

}// namespace Blt