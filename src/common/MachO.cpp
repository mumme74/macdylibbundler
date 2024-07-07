/*
The MIT License (MIT)

Copyright (c) 2024 Fredrik Johansson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <fstream>
#include <iostream>
#include <cstring>
#include <cassert>
#include <cmath>
#include "MachO.h"
#include "Types.h"


static uint32_t readMagic(std::ifstream& file)
{
  uint32_t magic = 0;
  auto n = file.readsome((char*)&magic, sizeof(magic));
  if (n != sizeof(magic))
    return 0;
  return magic;
}



// -----------------------------------------------------------

using namespace MachO;

const char*
MachO::FiletypeStr(FileType type)
{
  switch (type) {
  case MH_OBJECT:       return "MH_OBJECT";
  case MH_EXECUTE:      return "MH_EXECUTE";
  case MH_FVMLIB:       return "MH_FVMLIB";
  case MH_CORE:         return "MH_CORE";
  case MH_PRELOAD:      return "MH_PRELOAD";
  case MH_DYLIB:        return "MH_DYLIB";
  case MH_DYLINKER:     return "MH_DYLINKER";
  case MH_BUNDLE:       return "MH_BUNDLE";
  case MH_DYLIB_STUB:   return "MH_DYLIB_STUB";
  case MH_DSYM:         return "MH_DSYM";
  case MH_KEXT_BUNDLE:  return "MH_KEXT_BUNDLE";
  }
  static char buf[40] = {0};
  sprintf(buf, "MH_FILETYPE_UNKNOWN (%2x)", (uint32_t)type);
  return buf;
}

const char*
MachO::FlagsStr(Flags flag)
{
  switch (flag) {
  /* Constants for the flags field of the mach_header */
  case MH_NOUNDEFS:                 return "MH_NOUNDEFS";
  case MH_INCRLINK:                 return "MH_INCRLINK";
  case MH_DYLDLINK:                 return "MH_DYLDLINK";
  case MH_BINDATLOAD:               return "MH_BINDATLOAD";
  case MH_PREBOUND:                 return "MH_PREBOUND";
  case MH_SPLIT_SEGS:               return "MH_SPLIT_SEGS";
  case MH_LAZY_INIT:                return "MH_LAZY_INIT";
  case MH_TWOLEVEL:                 return "MH_TWOLEVEL";
  case MH_FORCE_FLAT:               return "MH_FORCE_FLAT";
  case MH_NOMULTIDEFS:              return "MH_NOMULTIDEFS";
  case MH_NOFIXPREBINDING:          return "MH_NOFIXPREBINDING";
  case MH_PREBINDABLE:              return "MH_PREBINDABLE";
  case MH_ALLMODSBOUND:             return "MH_ALLMODSBOUND";
  case MH_SUBSECTIONS_VIA_SYMBOLS:  return "MH_SUBSECTIONS_VIA_SYMBOLS";
  case MH_CANONICAL:                return "MH_CANONICAL";
  case MH_WEAK_DEFINES:             return "MH_WEAK_DEFINES";
  case MH_BINDS_TO_WEAK:            return "MH_BINDS_TO_WEAK";
  case MH_ALLOW_STACK_EXECUTION:    return "MH_ALLOW_STACK_EXECUTION";
  case MH_ROOT_SAFE:                return "MH_ROOT_SAFE";
  case MH_SETUID_SAFE:              return "MH_SETUID_SAFE";
  case MH_NO_REEXPORTED_DYLIBS:     return "MH_NO_REEXPORTED_DYLIBS";
  case MH_PIE:                      return "MH_PIE";
  case MH_DEAD_STRIPPABLE_DYLIB:    return "MH_DEAD_STRIPPABLE_DYLIB";
  case MH_HAS_TLV_DESCRIPTORS:      return "MH_HAS_TLV_DESCRIPTORS";
  case MH_NO_HEAP_EXECUTION:        return "MH_NO_HEAP_EXECUTION";
  case MH_APP_EXTENSION_SAFE:       return "MH_APP_EXTENSION_SAFE";
  }
  static char buf[40] = {0};
  sprintf(buf, "MH_FLAG_UNKOWN (%2x)", (uint32_t)flag);
  return buf;
}

const char*
MachO::CpuTypeStr(CpuType type)
{
  switch (type) {
  case MH_ANY:       return "MH_ANY";
  case MH_VAX:       return "MH_VAX";
  case MH_MC680x0:   return "MH_MC680x0";
  case MH_X86:       return "MH_X86";
  case MH_X86_64:    return "MH_X86_64";
  case MH_MC98000:   return "MH_MC98000";
  case MH_HPPA:      return "MH_HPPA";
  case MH_ARM:       return "MH_ARM";
  case MH_ARM64:     return "MH_ARM64";
  case MH_MC88000:   return "MH_MC88000";
  case MH_SPARC:     return "MH_SPARC";
  case MH_I860:      return "MH_I860";
  case MH_POWERPC:   return "MH_POWERPC";
  case MH_POWERPC64: return "MH_POWERPC64";
  }
  static char buf[40] = {0};
  sprintf(buf, "MH_CPU_UNKOWN (%2x)", (uint32_t)type);
  return buf;
}


const char*
MachO::LoadCmdStr(LoadCmds cmd)
{
  switch (cmd) {
  case LC_SEGMENT:                   return "LC_SEGMENT";
  case LC_SYMTAB:                    return "LC_SYMTAB";
  case LC_SYMSEG:                    return "LC_SYMSEG";
  case LC_THREAD:                    return "LC_THREAD";
  case LC_UNIXTHREAD:                return "LC_UNIXTHREAD";
  case LC_LOADFVMLIB:                return "LC_LOADFVMLIB";
  case LC_IDFVMLIB:                  return "LC_IDFVMLIB";
  case LC_IDENT:                     return "LC_IDENT";
  case LC_FVMFILE:                   return "LC_FVMFILE";
  case LC_PREPAGE:                   return "LC_PREPAGE";
  case LC_DYSYMTAB:                  return "LC_DYSYMTAB";
  case LC_LOAD_DYLIB:                return "LC_LOAD_DYLIB";
  case LC_ID_DYLIB:                  return "LC_ID_DYLIB";
  case LC_LOAD_DYLINKER:             return "LC_LOAD_DYLINKER";
  case LC_ID_DYLINKER:               return "LC_ID_DYLINKER";
  case LC_PREBOUND_DYLIB:            return "LC_PREBOUND_DYLIB";
  case LC_ROUTINES:                  return "LC_ROUTINES";
  case LC_SUB_FRAMEWORK:             return "LC_SUB_FRAMEWORK";
  case LC_SUB_UMBRELLA:              return "LC_SUB_UMBRELLA";
  case LC_SUB_CLIENT:                return "LC_SUB_CLIENT";
  case LC_SUB_LIBRARY:               return "LC_SUB_LIBRARY";
  case LC_TWOLEVEL_HINTS:            return "LC_TWOLEVEL_HINTS";
  case LC_PREBIND_CKSUM:             return "LC_PREBIND_CKSUM";
  case LC_LOAD_WEAK_DYLIB:           return "LC_LOAD_WEAK_DYLIB";
  case LC_SEGMENT_64:                return "LC_SEGMENT_64";
  case LC_ROUTINES_64:               return "LC_ROUTINES_64";
  case LC_UUID:                      return "LC_UUID";
  case LC_RPATH:                     return "LC_RPATH";
  case LC_CODE_SIGNATURE:            return "LC_CODE_SIGNATURE";
  case LC_SEGMENT_SPLIT_INFO:        return "LC_SEGMENT_SPLIT_INFO";
  case LC_REEXPORT_DYLIB:            return "LC_REEXPORT_DYLIB";
  case LC_LAZY_LOAD_DYLIB:           return "LC_LAZY_LOAD_DYLIB";
  case LC_ENCRYPTION_INFO:           return "LC_ENCRYPTION_INFO";
  case LC_DYLD_INFO:                 return "LC_DYLD_INFO";
  case LC_DYLD_INFO_ONLY:            return "LC_DYLD_INFO_ONLY";
  case LC_LOAD_UPWARD_DYLIB:         return "LC_LOAD_UPWARD_DYLIB";
  case LC_VERSION_MIN_MACOSX:        return "LC_VERSION_MIN_MACOSX";
  case LC_VERSION_MIN_IPHONEOS:      return "LC_VERSION_MIN_IPHONEOS";
  case LC_FUNCTION_STARTS:           return "LC_FUNCTION_STARTS";
  case LC_DYLD_ENVIRONMENT:          return "LC_DYLD_ENVIRONMENT";
  case LC_MAIN:                      return "LC_MAIN";
  case LC_DATA_IN_CODE:              return "LC_DATA_IN_CODE";
  case LC_SOURCE_VERSION:            return "LC_SOURCE_VERSION";
  case LC_DYLIB_CODE_SIGN_DRS:       return "LC_DYLIB_CODE_SIGN_DRS";
  case LC_ENCRYPTION_INFO_64:        return "LC_ENCRYPTION_INFO_64";
  case LC_LINKER_OPTION:             return "LC_LINKER_OPTION";
  case LC_LINKER_OPTIMIZATION_HINT:  return "LC_LINKER_OPTIMIZATION_HINT";
  case LC_VERSION_MIN_TVOS:          return "LC_VERSION_MIN_TVOS";
  case LC_VERSION_MIN_WATCHOS:       return "LC_VERSION_MIN_WATCHOS";
  case LC_NOTE:                      return "LC_NOTE";
  case LC_BUILD_VERSION:             return "LC_BUILD_VERSION";
  }
  static char buf[40] = {0};
  sprintf(buf, "LC_UNKNOWN (0x%x)", cmd);
  return buf;
}

const char*
MachO::ToolsStr(Tools tool)
{
  switch (tool) {
  case TOOL_CLANG: return "TOOL_CLANG";
  case TOOL_SWIFT: return "TOOL_SWIFT";
  case TOOL_LD:    return "TOOL_LD";
  }
  static char buf[40] = {0};
  sprintf(buf, "UNKNOWN TOOL (%2x)", (uint32_t)tool);
  return buf;
}

const char*
MachO::PlatformsStr(Platforms platform)
{
  switch (platform) {
  case PLATFORM_MACOS: return "PLATFORM_MACOS";
  case PLATFORM_IOS: return "PLATFORM_IOS";
  case PLATFORM_TVOS: return "PLATFORM_TVOS";
  case PLATFORM_WATCH: return "PLATFORM_WATCH";
  }
  static char buf[40] = {0};
  sprintf(buf, "UNKNOWN PLATFORM (%2x)", (uint32_t)platform);
  return buf;
}

// ------------------------------------------------------------

fat_header::fat_header()
  : m_magic{0}
  , m_nfat_arch{0}
{}

fat_header::fat_header(std::ifstream& file)
{
  readInto<fat_header>(file, this);
}

Magic
fat_header::magicRaw() const
{
  return m_magic;
}

Magic
fat_header::magic() const
{
  if constexpr(hostIsBigEndian)
    return isBigEndian() ? m_magic
           : (Magic)reverseEndian<uint32_t>(m_magic);
  else
    return isBigEndian() ? (Magic)reverseEndian<uint32_t>(m_magic)
           : m_magic;
}

bool
fat_header::isBigEndian() const
{
  if constexpr(hostIsBigEndian)
    return m_magic == FatMagic;
  return m_magic == FatCigam;
}

uint32_t
fat_header::nfat_arch() const
{
  if constexpr(hostIsBigEndian)
    return isBigEndian() ? m_nfat_arch : reverseEndian(m_nfat_arch);
  else
    return isBigEndian() ? reverseEndian(m_nfat_arch) : m_nfat_arch;
}

// -----------------------------------------------------------

fat_arch::fat_arch()
  : m_cputype{0}
  , m_cpusubtype{0}
  , m_offset{0}
  , m_size{0}
  , m_align{0}
{}

fat_arch::fat_arch(std::ifstream& file, const mach_fat_object& fat)
{
  readInto<fat_arch>(file, this);
  // five 32bit values
  uint32_t *buf = (uint32_t*)&m_cputype;
  for (size_t i = 0; i < 5; ++i)
    buf[i] = fat.endian(buf[i]);
}

// -----------------------------------------------------------

mach_header_32::mach_header_32()
  : m_magic{0}
  , m_cputype{0}
  , m_cpusubtype{0}
  , m_filetype{0}
  , m_ncmds{0}
  , m_sizeofcmds{0}
  , m_flags{0}
{}

mach_header_32::mach_header_32(std::ifstream& file)
  : m_magic{0}
  , m_cputype{0}
  , m_cpusubtype{0}
  , m_filetype{0}
  , m_ncmds{0}
  , m_sizeofcmds{0}
  , m_flags{0}
{
  readInto<mach_header_32>(file, this);
  endian();
}

void
mach_header_32::endian()
{
  if (m_magic != Magic32 && m_magic != Magic64) {
    // all items i header these are 32 bits
    // dont touch the magic though, used to check endianness
    uint32_t* buf = (uint32_t*)this;
    constexpr size_t end = sizeof(mach_header_32)/sizeof(uint32_t);
    for (size_t i = 1; i < end ; ++i)
      buf[i] = reverseEndian(buf[i]);
  }
}

bool
mach_header_32::isBigEndian() const {
  if (hostIsBigEndian)
    return m_magic == Magic32 || m_magic == Magic64;
  return m_magic == Cigam32 || m_magic == Cigam64;
}

bool
mach_header_32::is64bits() const
{
  return m_magic == Magic64 || m_magic == Cigam64;
}

std::ofstream&
mach_header_32::operator>>(std::ofstream& file) const
{
  mach_header_32 out{};
  memcpy((void*)&out, (void*)this, sizeof(mach_header_32));
  out.endian();
  file << out.m_magic << out.m_cputype << out.m_cpusubtype
       << out.m_filetype << out.m_ncmds << out.m_sizeofcmds
       << out.m_flags;
  return file;
}

// ----------------------------------------------------------

mach_header_64::mach_header_64()
  : mach_header_32{}
  , m_reserved{0}
{}

mach_header_64::mach_header_64(std::ifstream& file)
  : mach_header_32{}
  , m_reserved{0}
{
  readInto<mach_header_64>(file, this);
  endian();
  if (m_magic != Magic64)
    m_reserved = reverseEndian(m_magic);
}

std::ofstream&
mach_header_64::operator>>(std::ofstream& file) const
{
  mach_header_64 out{};
  memcpy((void*)&out, (void*)this, sizeof(mach_header_32));
  out.endian();
  file << out.m_magic << out.m_cputype << out.m_cpusubtype
       << out.m_filetype << out.m_ncmds << out.m_sizeofcmds
       << out.m_flags << out.m_reserved;
  return file;
}

// -----------------------------------------------------------

_lcStr::_lcStr()
  : ptr64bit{nullptr}
{}

_lcStr::_lcStr(const char* bytes, const mach_object& obj)
{
  if (obj.is64bits()) {
    uint64_t ofs = (uint64_t)bytes[0];
    ptr64bit = (char*)obj.endian(ofs);
  } else {
    offset = (uint32_t)obj.endian((uint32_t)bytes[0]);
  }
}

// -----------------------------------------------------------

load_command::load_command()
  : m_cmd{0}
  , m_cmdsize{0}
{}

load_command::load_command(
  const load_command_bytes& cmd, const mach_object& obj
) : m_cmd{obj.endian(cmd.cmd())}
  , m_cmdsize{obj.endian(cmd.cmdsize())}
{}

// -----------------------------------------------------------

load_command_bytes::load_command_bytes()
  : load_command{}
  , bytes{nullptr}
  //, owner{nullptr}
{}

/*
load_command_bytes::load_command_bytes(mach_object* owner)
  : load_command{}
  , bytes{nullptr}
  //, owner{owner}
{}*/

load_command_bytes::load_command_bytes(
  std::ifstream& file, mach_object& obj
) {
  if (!file)
    return;

  std::streamsize sz = sizeof(load_command);
  if (file.readsome((char*)this, sz) != sz) {
    std::cerr << "Failed to read header of LC_CMD\n";
    file.setstate(std::ios::badbit);
    return;
  }

  m_cmd = obj.endian(m_cmd);
  m_cmdsize = obj.endian(m_cmdsize);

  sz = m_cmdsize - sz;
  bytes = std::make_unique<char[]>(sz);
  if (file.readsome(bytes.get(), sz) != sz) {
    std::cerr << "Failed to read LC_CMD\n";
    file.setstate(std::ios::badbit);
    return;
  }

  bool is64Bit = obj.is64bits();
  if ((m_cmdsize % 4) != 0 || (is64Bit && (m_cmdsize % 8) != 0)) {
    std::cerr << "Uneven loadCmd size, bad file, bailing out\n";
    file.setstate(std::ios::badbit);
    return;
  }
}

// -----------------------------------------------------------

dylib_command::dylib_command()
  : m_name{}
  , m_timestamp{0}
  , m_current_version{0}
  , m_compatibility_version{0}
{}

dylib_command::dylib_command(
  const load_command_bytes& from, const mach_object& obj
) : load_command{from, obj}
  , m_name{from.bytes.get(), obj}
{
  const size_t nameOffset = lc_str::lc_STR_OFFSET; // always offset by 4 regardless of 32 or 64 bits
  auto buf = &from.bytes.get()[nameOffset];
  memcpy((void*)&m_timestamp,  buf,
          sizeof(dylib_command) - sizeof(load_command) - nameOffset);
  m_timestamp = obj.endian(m_timestamp);
  m_current_version = obj.endian(m_current_version);
  m_compatibility_version = obj.endian(m_compatibility_version);
}

// -----------------------------------------------------------

section_64::section_64(const char* bytes, const mach_object& obj)
  : _section<uint64_t>(bytes, obj)
{
  m_reserved3 = obj.endian(m_reserved3);
}

// -----------------------------------------------------------

linkedit_data_command::linkedit_data_command()
  : load_command{}
  , m_dataoff{0}
  , m_datasize{0}
{}

linkedit_data_command::linkedit_data_command(
  const load_command_bytes& cmd, const mach_object& obj
) : load_command{cmd, obj}
{
  memcpy((void*)&m_dataoff, (void*)cmd.bytes.get(),
    sizeof(linkedit_data_command) - sizeof(load_command));
  m_dataoff = obj.endian(m_dataoff);
  m_datasize = obj.endian(m_datasize);
}

// -----------------------------------------------------------

mach_object::mach_object()
  : m_start_pos{0}
  , m_hdr{}
  , m_load_cmds{}
  , m_data_segments{}
{}

mach_object::mach_object(std::ifstream& file)
  : m_start_pos{static_cast<size_t>(file.tellg())}
  , m_hdr{}
  , m_load_cmds{}
  , m_data_segments{}
{
  readHdr(file);
  if (!file)
    fail();
  else
    readCmds(file);

  if (!file)
    fail();
  else
    readData(file);
}

bool
mach_object::is64bits() const
{
  return m_hdr && m_hdr->is64bits();
}


const mach_header_32*
mach_object::header32() const
{
  return m_hdr ? m_hdr.get() : nullptr;
  if (m_hdr) {
    return m_hdr->is64bits()
            ? static_cast<mach_header_32*>(m_hdr.get())
            : m_hdr.get();
  }
  return nullptr;
}

const mach_header_64*
mach_object::header64() const
{
  return m_hdr
          ? static_cast<mach_header_64*>(m_hdr.get())
          : nullptr;
}

bool
mach_object::failure() const
{
  return !m_hdr || !m_load_cmds.size() || !m_data_segments.size();
}

void
mach_object::fail()
{
  m_hdr.release();
  m_load_cmds.clear();
  m_data_segments.clear();
}

void
mach_object::readHdr(std::ifstream& file)
{
  // read header
  uint32_t magic = readMagic(file);
  if (magic == 0) {
    fail(); return;
  }

  file.seekg(m_start_pos);

  if (magic == Magic64 || magic == Cigam64) {
    mach_header_64 hdr{file};
    m_hdr = std::make_unique<mach_header_64>(hdr);
  } else if (magic == Magic32 || magic == Cigam32) {
    mach_header_32 hdr{file};
    m_hdr = std::make_unique<mach_header_32>(hdr);
  } else {
    file.setstate(std::ios::failbit); // not a mach binary
    std::cerr << "Not a macho object file";
  }
}

void
mach_object::readCmds(std::ifstream& file)
{
  // read load commands
  for (size_t i = 0, end = m_hdr->ncmds(); i < end; ++i) {
    load_command_bytes cmd{file, *this};

    if (!file)
      return;

    m_load_cmds.emplace_back(std::move(cmd));
  }
}

void
mach_object::readData(std::ifstream& file)
{
  for (const auto& cmd : m_load_cmds) {
    switch (cmd.cmd()) {
    case LC_CODE_SIGNATURE:
    case LC_SEGMENT_SPLIT_INFO:
    case LC_FUNCTION_STARTS:
    case LC_DATA_IN_CODE:
    case LC_DYLIB_CODE_SIGN_DRS:
    case LC_LINKER_OPTIMIZATION_HINT: /*{
      auto link = std::make_unique<data_segment>();
      link->asLinkEdit(file, cmd, *this);
      m_data_segments.emplace_back(std::move(link));
    }*/ break;
    case LC_SEGMENT: {
      auto seg = std::make_unique<data_segment>();
      seg->asSegment<segment_command>(file, cmd, *this);
      m_data_segments.emplace_back(std::move(seg));
    } break;
    case LC_SEGMENT_64:{
      auto seg = std::make_unique<data_segment>();
      seg->asSegment<segment_command_64>(file, cmd, *this);
      m_data_segments.emplace_back(std::move(seg));
    } break;
    default:; // nothing in data sections
    }
  }
}

bool
mach_object::isBigEndian() const
{
  return m_hdr && m_hdr->isBigEndian();
}

std::vector<Path>
mach_object::rpaths() const
{
  std::vector<Path> rpaths;
  for (const auto& loadCmd : m_load_cmds) {
    if (loadCmd.cmd() == LC_RPATH) {
      lc_str lc(loadCmd.bytes.get(), *this);
      auto str = &loadCmd.bytes.get()
        [lc.offset - sizeof(load_command)];
      rpaths.emplace_back((char*)str);
    }
  }
  return rpaths;
}

std::vector<Path>
mach_object::loadDylibPaths() const
{
  return searchForDylibs(LC_LOAD_DYLIB);
}

std::vector<Path>
mach_object::reexportDylibPaths() const
{
  return searchForDylibs(LC_REEXPORT_DYLIB);
}

std::vector<Path>
mach_object::weakLoadDylib() const
{
  return searchForDylibs(LC_LOAD_WEAK_DYLIB);
}

std::vector<const data_segment*>
mach_object::dataSegments() const
{
  std::vector<const data_segment*> vec;
  if (!m_data_segments.size()) return vec;
  for (const auto& sec : m_data_segments)
    vec.emplace_back(static_cast<data_segment*>(sec.get()));
  return vec;
}

 const std::vector<load_command_bytes>&
 mach_object::loadCommands() const
 {
  return m_load_cmds;
 }

bool
mach_object::hasBeenSigned() const
{
  for (const auto& cmd : m_load_cmds) {
    if (cmd.cmd() == LC_CODE_SIGNATURE)
      return true;
  }
  return false;
}

std::vector<Path>
mach_object::searchForDylibs(LoadCmds type) const
{
  std::vector<Path> loadDylibs;
  for (const auto& loadCmd : m_load_cmds) {
    if (loadCmd.cmd() == type) {
      dylib_command dy{loadCmd, *this};
      auto b = loadCmd.bytes.get();
      auto str = &b[dy.name().offset - sizeof(load_command)];
      loadDylibs.emplace_back((char*)str);
    }
  }
  return loadDylibs;
}

size_t
mach_object::fileoff() const
{
  if (m_hdr) {
    if (m_hdr->is64bits())
      return m_start_pos + sizeof(mach_header_64) + m_hdr->sizeofcmds();
    return m_start_pos + sizeof(mach_header_32) + m_hdr->sizeofcmds();
  }
  return -1;
}


void
mach_object::changeRPath(PathRef oldPath, PathRef newPath)
{
  (void)oldPath; (void)newPath;
  std::cerr << "Unimplemented\n";
}

void
mach_object::changeLoadDylibPaths()
{
  std::cerr << "Unimplemented\n";
}

void
mach_object::changeReexportDylibPaths()
{
  std::cerr << "Unimplemented\n";
}

// -----------------------------------------------------------

fwlib_command::fwlib_command(
  const load_command_bytes& cmd, const mach_object& obj
) : load_command{cmd, obj}
  , m_name{cmd.bytes.get(), obj}
{
  memcpy((void*)&m_minor_version, &cmd.bytes.get()[lc_str::lc_STR_OFFSET],
         sizeof(fwlib_command) - sizeof(load_command) - lc_str::lc_STR_OFFSET);
}


// -----------------------------------------------------------

prebound_dylib_command::prebound_dylib_command(
  const load_command_bytes& cmd, const mach_object& obj
) : load_command{cmd, obj}
  , m_name{cmd.bytes.get(), obj}
  , m_linked_modules{
      &cmd.bytes.get()
        [lc_str::lc_STR_OFFSET + sizeof(m_nmodules)],
      obj
    }
{
  memcpy((void*)&m_nmodules, (void*)&cmd.bytes.get()
          [lc_str::lc_STR_OFFSET], sizeof(m_nmodules));
  m_nmodules = obj.endian(m_nmodules);
}

// -----------------------------------------------------------

symtab_command::symtab_command(
  const load_command_bytes& cmd, const mach_object& obj
) : load_command{cmd, obj}
{
  memcpy((void*)&m_symoff, cmd.bytes.get(),
        sizeof(symtab_command) - sizeof(load_command));
  uint32_t* buf = &m_symoff;
  for (size_t i = 0; i < 4; ++i)
    buf[i] = obj.endian(buf[i]);
}

// -----------------------------------------------------------

dysymtab_command::dysymtab_command(
  const load_command_bytes& cmd, const mach_object& obj
) : load_command{cmd, obj}
{
  constexpr size_t mysize = sizeof(dysymtab_command)
                          - sizeof(load_command);
  memcpy((void*)&m_ilocalsym, cmd.bytes.get(), mysize);
  uint32_t *buf = &m_ilocalsym;
  for (size_t i = 0; i < mysize; ++i)
    buf[i] = obj.endian(buf[i]);
}

// ----------------------------------------------------------

prebind_checksum_command::prebind_checksum_command(
  const load_command_bytes& cmd, const mach_object& obj
) : load_command{cmd, obj}
  , m_chksum(obj.endian((uint32_t)cmd.bytes.get()[0]))
{}

// ----------------------------------------------------------

twolevel_hints_command::twolevel_hints_command(
  const load_command_bytes& cmd, const mach_object& obj
) : load_command{cmd, obj}
{
  uint32_t *buf = (uint32_t*)cmd.bytes.get();
  uint32_t *me = &m_offset;
  for(size_t i = 0; i < 2; ++i)
    me[i] = buf[i];
}

twolevel_hint::twolevel_hint(
  const char* buf, const mach_object& obj
) {
  uint32_t *me = (uint32_t*)this;
  *me = obj.endian((uint32_t)buf[0]);
}

// ----------------------------------------------------------

encryption_info_command::encryption_info_command(
  const load_command_bytes& cmd, const mach_object& obj
) : load_command{cmd, obj}
{
  constexpr size_t mysize = sizeof(encryption_info_command)
                          - sizeof(load_command);
  memcpy((void*)&m_cryptoff, cmd.bytes.get(), mysize);
  uint32_t *buf = &m_cryptoff;
  for (size_t i = 0; i < mysize; ++i)
    buf[i] = obj.endian(buf[i]);
}

// -----------------------------------------------------------

version_min_command::version_min_command(
  const load_command_bytes& cmd, const mach_object& obj
) : load_command{cmd, obj}
{
  uint32_t *buf = (uint32_t*)cmd.bytes.get();
  m_version = obj.endian(buf[0]);
  m_sdk = obj.endian(buf[1]);
}

// -----------------------------------------------------------

build_version_command::build_version_command(
  const load_command_bytes& cmd, const mach_object& obj
) : load_command{cmd, obj}
{
  uint32_t *buf = (uint32_t*)cmd.bytes.get();
  uint32_t *me = (uint32_t*)&m_platform;
  for(size_t i = 0; i < 4; ++i)
    me[i] = obj.endian(buf[i]);
}

build_tool_version::build_tool_version(
  const char* bytes, const mach_object& obj
) {
  uint32_t *buf = (uint32_t*)bytes;
  uint32_t *me = (uint32_t*)&m_tool;
  for (size_t i = 0; i < 2; ++i)
    me[i] = obj.endian(buf[i]);
}

// -----------------------------------------------------------

dyld_info_command::dyld_info_command(
  const load_command_bytes& cmd, const mach_object& obj
) : load_command{cmd, obj}
{
  uint32_t *buf = (uint32_t*)cmd.bytes.get();
  uint32_t *me = &m_rebase_off;
  for (size_t i = 0; i < 10; ++i)
    me[i] = buf[i];
}

// -----------------------------------------------------------

linker_option_command::linker_option_command(
  const load_command_bytes& cmd, const mach_object& obj
) : load_command{cmd, obj}
  , m_count(obj.endian((uint32_t)cmd.bytes.get()[0]))
{}

// -----------------------------------------------------------

fvmfile_command::fvmfile_command(
  const load_command_bytes& cmd, const mach_object& obj
) : load_command{cmd, obj}
  , m_name{cmd.bytes.get(), obj}
{
  m_header_addr = obj.endian((uint32_t)cmd.bytes.get()[lc_str::lc_STR_OFFSET]);
}

// -----------------------------------------------------------

entry_point_command::entry_point_command(
  const load_command_bytes& cmd, const mach_object& obj
) : load_command{cmd, obj}
{
  uint64_t *me = (uint64_t*)this;
  uint64_t *buf = (uint64_t*)cmd.bytes.get();
  for ( size_t i = 0; i < 2; ++i)
    me[i] = obj.endian(buf[i]);
}

// -----------------------------------------------------------

source_version_command::source_version_command(
  const load_command_bytes& cmd, const mach_object& obj
) : load_command{cmd, obj}
{
  m_version = obj.endian((uint64_t)cmd.bytes.get()[0]);
}

// -----------------------------------------------------------

data_segment::data_segment()
  : m_segname{0}
  , m_filesize{0}
  , m_fileoff{0}
{}

void
data_segment::asLinkEdit(
  std::ifstream& file,
  const load_command_bytes& cmd,
  const mach_object& obj
) {
  const char segname[segment_command::SZ_SEGNAME] = "__LINKEDIT";
  linkedit_data_command link{cmd, obj};
  memcpy((void*)this->m_segname, (void*)segname, sizeof(m_segname));
  m_fileoff = link.dataoff();
  m_filesize = link.datasize();

  read_into(file, obj);
}

void
data_segment::read_into(
  std::ifstream& file, const mach_object& obj
) {
  file.seekg(obj.startPos() + m_fileoff);

  std::streampos sz = m_filesize;
  bytes = std::make_unique<char[]>(sz);
  if (file.readsome(bytes.get(), sz) != sz) {
    file.setstate(std::ios::badbit);
    return;
  }
}


// -----------------------------------------------------------

mach_fat_object::mach_fat_object()
  : m_hdr{nullptr}
  , m_fat_arch{}
  , m_objects{}
{}

mach_fat_object::mach_fat_object(std::ifstream& file)
  : m_hdr{nullptr}
  , m_fat_arch{}
  , m_objects{}
{
  auto hdr = std::make_unique<fat_header>(file);
  if (!file || hdr->magic() != FatMagic)
    return;

  m_hdr = std::move(hdr);

  // load architecture
  for (size_t i = 0, end = m_hdr->nfat_arch(); i < end; ++i) {
    fat_arch arch{file, *this};
    if (!file || !arch.size()) {
      fail();
      return;
    }

    m_fat_arch.emplace_back(std::move(arch));
  }

  if (!file) {
    fail();
    return;
  }

  // load objects from within this fat object
  for (const auto& arch : m_fat_arch) {
    file.seekg(arch.offset());

    mach_object obj{file};
    if (!file) {
      fail();
      return;
    }

    assert(file.tellg() == arch.offset() + arch.size() &&
          "Object size mismatch in a fat binary, "
          "please report this");

    m_objects.emplace_back(std::move(obj));
  }
}

bool
mach_fat_object::isBigEndian() const
{
  return m_hdr && m_hdr->isBigEndian();
}

bool
mach_fat_object::failure() const
{
  return !m_hdr || m_fat_arch.empty() || m_objects.empty();
}

const std::vector<mach_object>&
mach_fat_object::objects() const
{
  return m_objects;
}

const std::vector<fat_arch>&
mach_fat_object::architectures() const
{
  return m_fat_arch;
}

void
mach_fat_object::fail()
{
  m_hdr.release();
  m_fat_arch.clear();
  m_objects.clear();
}

// -----------------------------------------------------------

introspect_object::introspect_object(const mach_object* obj)
  : m_obj{obj}
{}

std::string
introspect_object::loadCmds() const
{
  auto getStr = [&](const lc_str lc, const load_command_bytes& cmd){
    return &cmd.bytes.get()[lc.offset - sizeof(load_command)];
  };

  std::stringstream ss;
  int cmdNr = 0;
  for (const auto& cmd : m_obj->loadCommands()) {
    ss << std::setw(9) << std::right
       << "Load command " << cmdNr++ << "\n"
       << " cmd " << LoadCmdStr(cmd.cmd()) << "\n"
       << "  cmdsize " << cmd.cmdsize() << "\n";

    switch (cmd.cmd()) {
    case LC_SUB_FRAMEWORK: {
      lc_str lc{cmd.bytes.get(), *m_obj};
      ss << "  umbrella " << getStr(lc, cmd) << "\n";
    } break;
    case LC_SUB_CLIENT:{
      lc_str lc{cmd.bytes.get(), *m_obj};
      ss << "  sub_umbrella " << getStr(lc, cmd) << "\n";
    } break;
    case LC_SUB_LIBRARY:{
      lc_str lc{cmd.bytes.get(), *m_obj};
      ss << "  sub_library " << getStr(lc, cmd) << "\n";
    } break;
    case LC_ID_DYLINKER:
    case LC_LOAD_DYLINKER:
		case LC_DYLD_ENVIRONMENT:{
      lc_str lc{cmd.bytes.get(), *m_obj};
      ss << "  name " << getStr(lc, cmd) << "\n";
    } break;
    case LC_PREBOUND_DYLIB: {
      prebound_dylib_command pre{cmd, *m_obj};
      ss << "  name " << getStr(pre.name(), cmd) << "\n"
         << "  nmodules " << pre.nmodules() << "\n"
         << "  linked_modules " << getStr(pre.linked_modules(), cmd)
         << "\n";
    } break;
    case LC_ROUTINES: {
      routines_command rout{cmd, *m_obj};
      ss << "  init_address " << rout.init_address() << "\n"
         << "  init_module " << rout.init_module() << "\n"
         << "  reserved1 " << rout.reserved1() << "\n"
         << "  reserved2 " << rout.reserved2() << "\n"
         << "  reserved3 " << rout.reserved3() << "\n"
         << "  reserved4 " << rout.reserved4() << "\n"
         << "  reserved5 " << rout.reserved5() << "\n"
         << "  reserved6 " << rout.reserved6() << "\n";

    } break;
    case LC_ROUTINES_64: {
      routines_command_64 rout{cmd, *m_obj};
      ss << "  init_address " << rout.init_address() << "\n"
         << "  init_module " << rout.init_module() << "\n"
         << "  reserved1 " << rout.reserved1() << "\n"
         << "  reserved2 " << rout.reserved2() << "\n"
         << "  reserved3 " << rout.reserved3() << "\n"
         << "  reserved4 " << rout.reserved4() << "\n"
         << "  reserved5 " << rout.reserved5() << "\n"
         << "  reserved6 " << rout.reserved6() << "\n";

    } break;
    case LC_SYMTAB: {
      symtab_command sym{cmd, *m_obj};
      ss << "  symoff " << sym.symoff() << "\n"
         << "  syms " << sym.syms() << "\n"
         << "  stroff " << sym.stroff() << "\n"
         << "  strsize " << sym.strsize() << "\n";
    } break;
    case LC_DYSYMTAB: {
      dysymtab_command dsym{cmd, *m_obj};
      ss << "  ilocalsym " << dsym.ilocalsym() << "\n"
         << "  nlocalsym " << dsym.nlocalsym() << "\n"
         << "  iextsym "  << dsym.iextsym() << "\n"
         << "  nextsym " << dsym.nextsym() << "\n"
         << "  iundefsym " << dsym.iundefsym() << "\n"
         << "  nundefsym " << dsym.nundefsym() << "\n"
         << "  tocoff " << dsym.tocoff() << "\n"
         << "  ntoc " << dsym.ntoc() << "\n"
         << "  modtaboff " << dsym.modtaboff() << "\n"
         << "  nmodtab " << dsym.nmodtab() << "\n"
         << "  extrefsymoff " << dsym.extrefsymoff() << "\n"
         << "  nextrefsyms " << dsym.nextrefsyms() << "\n"
         << "  indirectsymsoff " << dsym.indirectsymsoff() << "\n"
         << "  nindrectsyms " << dsym.nindrectsyms() << "\n"
         << "  extreloff " << dsym.extreloff() << "\n"
         << "  nextrel " << dsym.nextrel() << "\n"
         << "  locreloff " << dsym.locreloff() << "\n"
         << "  locrel " << dsym.locrel() << "\n";
    } break;
    case LC_DYLD_INFO:
    case LC_DYLD_INFO_ONLY: {
      dyld_info_command dinfo{cmd, *m_obj};
      ss << "  rebase_off " << dinfo.rebase_off() << "\n"
         << "  rebase_size " << dinfo.rebase_size() << "\n"
         << "  bind_off " << dinfo.bind_off() << "\n"
         << "  bind_size " << dinfo.bind_size() << "\n"
         << "  weak_bind_off " << dinfo.weak_bind_off() << "\n"
         << "  weak_bind_size " << dinfo.weak_bind_size() << "\n"
         << "  lazy_bind_off " << dinfo.lazy_bind_off() << "\n"
         << "  lazy_bind_size " << dinfo.lazy_bind_size() << "\n"
         << "  export_off " << dinfo.export_off() << "\n"
         << "  export_size " << dinfo.export_size() << "\n";

    } break;
    case LC_TWOLEVEL_HINTS: {
      twolevel_hints_command lvl{cmd, *m_obj};
      ss << "  offset " << lvl.offset() << "\n"
         << "  nhints " << lvl.nhints() << "\n";
      if (lvl.nhints())
        ss << "Hints ----------------------------------\n";
      for (size_t i = 0; i < lvl.nhints(); ++i) {
        size_t offset = sizeof(twolevel_hints_command) - sizeof(load_command);
        offset += sizeof(twolevel_hint) * i;
        twolevel_hint hint{&cmd.bytes.get()[offset], *m_obj};
        ss << "    isubimage " << hint.isubimage() << "\n"
           << "    itoc " << hint.itoc() << "\n";
      }
    } break;
    case LC_SOURCE_VERSION: {
      source_version_command src{cmd, *m_obj};
      ss << "  version " << sourceVersionStr(src.version()) << "\n";

    } break;
    case LC_VERSION_MIN_MACOSX:
		case LC_VERSION_MIN_IPHONEOS:
		case LC_VERSION_MIN_WATCHOS:
		case LC_VERSION_MIN_TVOS: {
      version_min_command ver{cmd, *m_obj};
      ss << "  version " << versionStr(ver.version()) << "\n"
         << "  sdk " << versionStr(ver.sdk()) << "\n";
    } break;
    case LC_BUILD_VERSION: {
      build_version_command bver{cmd, *m_obj};
      ss << "  platform " << PlatformsStr(bver.platform()) << "\n"
         << "  minos " << versionStr(bver.minos()) << "\n"
         << "  sdk " << versionStr(bver.sdk()) << "\n"
         << "  tools " << bver.tools() << "\n";
         if (bver.tools())
           ss << "Tools ------------------------------\n   tool:\n";
      for (size_t i = 0; i < bver.tools(); ++i) {
        size_t offset = sizeof(build_version_command) - sizeof(load_command);
        offset += sizeof(build_tool_version) * i;
        build_tool_version tver{&cmd.bytes.get()[offset], *m_obj};
        ss << "    tool " << ToolsStr(tver.tool()) << "\n"
           << "    version " << versionStr(tver.version()) << "\n";
      }
    } break;
    case LC_IDFVMLIB:
    case LC_LOADFVMLIB: {
      fwlib_command fwlib{cmd, *m_obj};
      ss << "  name " << getStr(fwlib.name(), cmd) << "\n"
         << "  minor_version " << fwlib.minor_version() << "\n";
    } break;
    case LC_PREBIND_CKSUM: {
      prebind_checksum_command pre{cmd, *m_obj};
      ss << "  chksum " << toChkSumStr(pre.chksum()) << "\n";
    } break;
    case LC_UUID:
      // should always be stored as bigendian
      ss << "  uuid " << toUUID(cmd.bytes.get()) << "\n";
      break;
    case LC_SEGMENT:
      ss << parseSegment<segment_command, section>(cmd);
      break;
    case LC_SEGMENT_64:
      ss << parseSegment<segment_command_64, section_64>(cmd);
      break;
    case LC_LOAD_DYLIB:
    case LC_LOAD_WEAK_DYLIB:
    case LC_REEXPORT_DYLIB: {
      dylib_command dylib{cmd, *m_obj};
      ss << "  name " << getStr(dylib.name(), cmd) << "\n"
         << "  timestamp " <<
          timestampStr(dylib.timestamp()) << "\n"
         << "  current_version " <<
          versionStr(dylib.current_version()) << "\n"
         << "  compatibility_version " <<
          versionStr(dylib.compatibility_version()) << "\n";
    } break;
    case LC_RPATH: {
      lc_str lc{cmd.bytes.get(), *m_obj};
      ss << "  path " << getStr(lc, cmd) << "\n";
    } break;
    case LC_CODE_SIGNATURE:
    case LC_SEGMENT_SPLIT_INFO:
    case LC_FUNCTION_STARTS:
    case LC_DATA_IN_CODE:
		case LC_DYLIB_CODE_SIGN_DRS:
    case LC_LINKER_OPTIMIZATION_HINT: {
      linkedit_data_command link{cmd, *m_obj};
      ss << "  dataoff " << link.dataoff() << "\n"
            "  datasize " << link.datasize() << "\n";
    } break;
    case LC_LINKER_OPTION: {
      linker_option_command opt{cmd, *m_obj};
      ss << "  count " << opt.count() << "\n";
      if (opt.count())
        ss << "Options ---------------------------\n";
      for (size_t i = 0, len = 0; i < opt.count(); ++i) {
        size_t offset = sizeof(linker_option_command);
        offset += len;
        len = strnlen(&cmd.bytes.get()[offset], cmd.cmdsize());
        ss << "    " << &cmd.bytes.get()[offset] << "\n";
      }
    } break;
    case LC_ENCRYPTION_INFO:
    case LC_ENCRYPTION_INFO_64: {
      encryption_info_command enc{cmd, *m_obj};
      ss << "  cryptoff " << enc.cryptoff() << "\n"
         << "  cryptsize " << enc.cryptsize() << "\n"
         << "  cryptid" << enc.cryptid() << "\n";
    } break;
    case LC_MAIN: {
      entry_point_command ent{cmd, *m_obj};
      ss << "  entryoff " << ent.entryoff() << "\n"
         << "  stacksize " << ent.stacksize() << "\n";
    } break;
    case LC_FVMFILE: {
      fvmfile_command fvm{cmd, *m_obj};
      ss << "  name " << getStr(fvm.name(), cmd) << "\n";
      ss << "  header_addr " << std::setfill('0')
         << std::setw(8)  << std::hex << fvm.header_addr();
    } break;
    default:{
      hexdump(ss, cmd.bytes.get(),
        cmd.cmdsize() - sizeof(load_command));
    } break;
    }

    ss << "-----------------------------------------------\n";
  }
  return ss.str();
}

std::stringstream&
introspect_object::hexdump(
  std::stringstream& ss, const char* buf, size_t nBytes
) const {
  ss << "  bits:\n";
  for (size_t i = 0; i < nBytes; i += 16) {
    ss << "  " << std::setfill('0') << std::setw(8)
        << std::hex << i << std::setw(2);

    for (size_t j = 0; j < 16; ++j) {
      auto byte = buf[i+j];
      ss << " " << std::right << std::setfill('0')
          << std::setw(2) << std::hex
          << ((uint16_t)byte & 0x00FF);
    }

    ss << "  " << std::setw(0);
    for ( size_t j = 0; j < 16; ++j) {
      auto byte = buf[i+j];
      ss << (byte >= ' ' ? byte : '.') << ' ';
    }
    ss << '\n';
  }
  ss << std::dec;
  return ss;
}


std::string
introspect_object::versionStr(uint32_t version) const
{
  std::stringstream ss;
  uint major = ((version >> 16) & 0xFFFF),
       minor = ((version >> 8) & 0xFF),
       micro = (version & 0xFF);
  ss << major << "." << minor << "." << micro;
  return ss.str();
}

std::string
introspect_object::timestampStr(uint32_t timestamp) const
{
  time_t temp = timestamp;
  std::tm* t = std::gmtime(&temp);
  std::stringstream ss; // or if you're going to print, just input directly into the output stream
  ss << std::put_time(t, "%Y-%m-%d %I:%M:%S %p");
  return ss.str();
}

std::string
introspect_object::toUUID(const char* uuid) const
{
  char buf[40] = {0};
  char *ptr = buf;

  for (size_t i = 0; i < 16; ++i) {
    sprintf(ptr, "%02X", uuid[i] & 0xFF);
    ptr += 2;
    switch (i) {
    case 3: case 5: case 7: case 9:
      *ptr++ = '-';
    }
  }

  return buf;
}

std::string
introspect_object::toChkSumStr(uint32_t chksum) const
{
  char buf[10] = {0};
  const char *sum = (char*)&chksum;
  char *ptr = buf;
  for (size_t i = 0; i < sizeof(chksum); ++i) {
    sprintf(ptr, "%02X", sum[i]);
    ptr += 2;
  }
  return buf;
}

std::string
introspect_object::sourceVersionStr(uint64_t version) const
{
  //  A.B.C.D.E packed as a24.b10.c10.d10.e10
  uint32_t a = (version >> 40) & 0x00FFFFFF,
           b = (version >> 30) & 0x000003FF,
           c = (version >> 20) & 0x000003FF,
           d = (version >> 10) & 0x000003FF,
           e = (version >> 00) & 0x000003FF;
  std::stringstream ss;
  if (a) ss << a << ".";
  if (b) ss << b << ".";
  if (c) ss << c << ".";
  ss << d << "." << e;
  return ss.str();
}
// -----------------------------------------------------------

MachOLoader::MachOLoader(PathRef binPath)
  : m_binPath{binPath}
  , m_error{}
{
  std::ifstream file;
  file.open(binPath, std::ios::binary );
  auto magic = readMagic(file);
  file.seekg(file.beg);


  switch (magic) {
  case FatMagic: case FatCigam:
    // read as fat binary
    break;
  case Magic32: case Cigam32:
    // read all headers and arch segments
    break;
  case Magic64: case Cigam64:
    // read 64 bits
    break;
  default:
    m_error = "Not a valid mach-o object";
    return;
  }
}


