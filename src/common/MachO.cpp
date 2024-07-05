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
FiletypeStr(FileType type)
{
  switch (type) {
  case MH_OBJECT: return "MH_OBJECT";
  case MH_EXECUTE: return "MH_EXECUTE";
  case MH_FVMLIB: return "MH_FVMLIB";
  case MH_CORE: return "MH_CORE";
  case MH_PRELOAD: return "MH_PRELOAD";
  case MH_DYLIB: return "MH_DYLIB";
  case MH_DYLINKER: return "MH_DYLINKER";
  case MH_BUNDLE: return "MH_BUNDLE";
  case MH_DYLIB_STUB: return "MH_DYLIB_STUB";
  case MH_DSYM: return "MH_DSYM";
  case MH_KEXT_BUNDLE: return "MH_KEXT_BUNDLE";
  }
  return "MH_UNKNOWN";
}


const char*
MachO::LoadCmdStr(LoadCmds cmd)
{
  switch (cmd) {
  case SEGMENT: return "LC_SEGMENT";
  case SYMTAB: return "LC_SYMTAB";
  case SYMSEG: return "LC_SYMSEG";
  case THREAD: return "LC_THREAD";
  case UNIXTHREAD: return "LC_UNIXTHREAD";
  case LOADFVMLIB: return "LC_LOADFVMLIB";
  case IDFVMLIB: return "LC_IDFVMLIB";
  case IDENT: return "LC_IDENT";
  case FVMFILE: return "LC_FVMFILE";
  case PREPAGE: return "LC_PREPAGE";
  case DYSYMTAB: return "LC_DYSYMTAB";
  case LOAD_DYLIB: return "LC_LOAD_DYLIB";
  case ID_DYLIB: return "LC_ID_DYLIB";
  case LOAD_DYLINKER: return "LC_LOAD_DYLINKER";
  case ID_DYLINKER: return "LC_ID_DYLINKER";
  case PREBOUND_DYLIB: return "LC_PREBOUND_DYLIB";
  case ROUTINES: return "LC_ROUTINES";
  case SUB_FRAMEWORK: return "LC_SUB_FRAMEWORK";
  case SUB_UMBRELLA: return "LC_SUB_UMBRELLA";
  case SUB_CLIENT: return "LC_SUB_CLIENT";
  case SUB_LIBRARY: return "LC_SUB_LIBRARY";
  case TWOLEVEL_HINTS: return "LC_TWOLEVEL_HINTS";
  case PREBIND_CKSUM: return "LC_PREBIND_CKSUM";
  case LOAD_WEAK_DYLIB: return "LC_LOAD_WEAK_DYLIB";
  case SEGMENT_64: return "LC_SEGMENT_64";
  case ROUTINES_64: return "LC_ROUTINES_64";
  case UUID: return "LC_UUID";
  case RPATH: return "LC_RPATH";
  case CODE_SIGNATURE: return "LC_CODE_SIGNATURE";
  case SEGMENT_SPLIT_INFO: return "LC_SEGMENT_SPLIT_INFO";
  case REEXPORT_DYLIB: return "LC_REEXPORT_DYLIB";
  case LAZY_LOAD_DYLIB: return "LC_LAZY_LOAD_DYLIB";
  case ENCRYPTION_INFO: return "LC_ENCRYPTION_INFO";
  case DYLD_INFO: return "LC_DYLD_INFO";
  case DYLD_INFO_ONLY: return "LC_DYLD_INFO_ONLY";
  case LOAD_UPWARD_DYLIB: return "LC_LOAD_UPWARD_DYLIB";
  case VERSION_MIN_MACOSX: return "LC_VERSION_MIN_MACOSX";
  case VERSION_MIN_IPHONEOS: return "LC_VERSION_MIN_IPHONEOS";
  case FUNCTION_STARTS: return "LC_FUNCTION_STARTS";
  case DYLD_ENVIRONMENT: return "LC_DYLD_ENVIRONMENT";
  case MAIN: return "LC_MAIN";
  case DATA_IN_CODE: return "LC_DATA_IN_CODE";
  case SOURCE_VERSION: return "LC_SOURCE_VERSION";
  case DYLIB_CODE_SIGN_DRS: return "LC_DYLIB_CODE_SIGN_DRS";
  case ENCRYPTION_INFO_64: return "LC_ENCRYPTION_INFO_64";
  case LINKER_OPTION: return "LC_LINKER_OPTION";
  case LINKER_OPTIMIZATION_HINT: return "LC_LINKER_OPTIMIZATION_HINT";
  case VERSION_MIN_TVOS: return "LC_VERSION_MIN_TVOS";
  case VERSION_MIN_WATCHOS: return "LC_VERSION_MIN_WATCHOS";
  case NOTE: return "LC_NOTE";
  }
  return "LC_UNKNOWN";
}

// ------------------------------------------------------------

fat_header::fat_header()
  : m_magic{0}
  , m_nfat_arch{0}
{}


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

// -----------------------------------------------------------



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
  *this << file;
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

Magic
mach_header_32::magicRaw() const     // the magic marker, not accounted for endianness
{
  return m_magic;
}

Magic
mach_header_32::magic() const        // the magic marker, determines
{
  return (Magic)convertEndian(m_magic);
}

CpuType
mach_header_32::cputype() const    // the cputype
{
  return (CpuType)convertEndian(m_cputype);
}

cpu_subtype_t
mach_header_32::cpusubtype() const // subtype, machine type
{
  return (cpu_subtype_t)convertEndian(m_cpusubtype);
}

FileType
mach_header_32::filetype() const  // type of file
{
  return (FileType)convertEndian(m_filetype);
}

uint32_t
mach_header_32::ncmds() const     // num of lod commands
{
  return (uint32_t)convertEndian(m_ncmds);
}

uint32_t
mach_header_32::sizeofcmds() const // size of all load commands, may vary
{
  return (uint32_t)convertEndian(m_sizeofcmds);
}


uint32_t
mach_header_32::convertEndian(uint32_t in) const
{
  if constexpr(hostIsBigEndian)
    return isBigEndian() ? in : reverseEndian<uint32_t>(in);
  else
    return isBigEndian() ? reverseEndian<uint32_t>(in) : in;
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
  *this << file;
}

uint32_t
mach_header_64::reserved() const
{
  return convertEndian(m_reserved);
}

// -----------------------------------------------------------

load_command::load_command()
  : cmd{0}
  , cmdsize{0}
{}

load_command::load_command(const load_command_bytes& cmd)
  : cmd{cmd.cmd}
  , cmdsize{cmd.cmdsize}
{}

// -----------------------------------------------------------

load_command_bytes::load_command_bytes()
  : load_command{}
  , bytes{nullptr}
  , owner{nullptr}
{}

load_command_bytes::load_command_bytes(mach_object* owner)
  : load_command{}
  , bytes{nullptr}
  , owner{owner}
{}

std::ifstream&
load_command_bytes::operator<<(std::ifstream& file)
{
  if (!file) return file;
  std::streamsize sz = sizeof(load_command);
  if (file.readsome((char*)this, sz) != sz) {
    std::cerr << "Failed to read header of LC_CMD\n";
    file.setstate(std::ios::badbit);
    return file;
  }

  sz = owner->endian(cmdsize) - sz;
  bytes = std::make_unique<char[]>(sz);
  if (file.readsome(bytes.get(), sz) != sz) {
    std::cerr << "Failed to read LC_CMD\n";
    file.setstate(std::ios::badbit);
    return file;
  }

  auto cmdsz = owner->endian<uint32_t>(cmdsize);
  bool is64Bit = owner->is64bits();
  if ((cmdsz % 4) != 0 || (is64Bit && (cmdsz % 8) != 0)) {
    std::cerr << "Uneven loadCmd size, bad file, bailing out\n";
    file.setstate(std::ios::badbit);
    return file;
  }

  return file;
}

uint32_t
load_command_bytes::bytessize() const
{
  return owner->endian(cmdsize) - sizeof(load_command);
}

// -----------------------------------------------------------

segment_command::segment_command(const load_command_bytes& cmd)
  : load_command{cmd}
{
  memcpy((void*)segname,  cmd.bytes.get(),
    sizeof(segment_command_64) - sizeof(load_command));
}

segment_command_64::segment_command_64(const load_command_bytes& cmd)
  : load_command{cmd}
{
  memcpy((void*)segname, cmd.bytes.get(),
    sizeof(segment_command_64) - sizeof(load_command));
}

// -----------------------------------------------------------

section::section()
  : sectname{0}
  , segname{0}
  , addr{0}
  , size{0}
  , offset{0}
  , align{0}
  , reloff{0}
  , flags{0}
  , reserved1{0}
  , reserved2{0}
{}

section::section(std::ifstream& file, mach_object& owner)
{
  readInto<section>(file, this);
  if (file) {
    uint32_t ali = owner.endian<uint32_t>(align);
    file.seekg(ali*2 + file.tellg());
  }
}


// -----------------------------------------------------------

mach_object::mach_object(std::ifstream& file)
  : m_hdr{}
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
  if (m_hdr && !m_hdr->is64bits())
    return m_hdr.get();
  return nullptr;
}

const mach_header_64*
mach_object::header64() const
{
  if (m_hdr && m_hdr->is64bits()) {
    mach_header_32* hdr = m_hdr.get();
    return static_cast<mach_header_64*>(hdr);
  }
  return nullptr;
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

  file.seekg(file.beg);

  if (magic == Magic64 || magic == Cigam64) {
    mach_header_64 hdr{file};
    m_hdr = std::make_unique<mach_header_64>(hdr);
  } else {
    mach_header_32 hdr{file};
    m_hdr = std::make_unique<mach_header_32>(hdr);
  }
}

void
mach_object::readCmds(std::ifstream& file)
{
  // read load commands
  for (size_t i = 0, end = m_hdr->ncmds(); i < end; ++i) {
    load_command_bytes cmd{this};

    cmd << file;
    if (!file)
      return;

    m_load_cmds.emplace_back(std::move(cmd));
  }
}

void
mach_object::readData(std::ifstream& file)
{
  for (const auto& cmd : m_load_cmds) {
    auto loadCmd = (LoadCmds)endian<uint32_t>(cmd.cmd);
    switch (loadCmd) {
    case LoadCmds::SEGMENT: [[fallthrough]];
    case LoadCmds::SEGMENT_64:{
      auto seg = std::make_unique<data_segment>(file, cmd);
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
    if ((LoadCmds)endian<uint32_t>(loadCmd.cmd) == RPATH) {
      lc_str lcStr;
      auto buf = loadCmd.bytes.get();
      readLcStr(lcStr, buf);
      auto str = &loadCmd.bytes.get()[lcStr.offset - sizeof(load_command)];
      rpaths.emplace_back((char*)str);
    }
  }
  return rpaths;
}

std::vector<Path>
mach_object::loadDylibPaths() const
{
  return searchForDylibs(LOAD_DYLIB);
}

std::vector<Path>
mach_object::reexportDylibPaths() const
{
  return searchForDylibs(REEXPORT_DYLIB);
}

std::vector<Path>
mach_object::weakLoadDylib() const
{
  return searchForDylibs(LOAD_WEAK_DYLIB);
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

void
mach_object::readLcStr(
  lc_str& lcStr, const char* buf
) const {
  lcStr.ptr64bit = nullptr;

  if (m_hdr->is64bits()) {
    lcStr.ptr64bit = (char*)endian<uint64_t>((uint64_t)(*buf));
  } else {
    lcStr.offset = endian<uint32_t>((uint32_t)(*buf));
  }
}

std::vector<Path>
mach_object::searchForDylibs(LoadCmds type) const
{
  std::vector<Path> loadDylibs;
  for (const auto& loadCmd : m_load_cmds) {
    if ((LoadCmds)endian<uint32_t>(loadCmd.cmd) == type) {
      auto buf = loadCmd.bytes.get();
      dylib dy;
      readLcStr(dy.name, buf);
      auto b = loadCmd.bytes.get();
      auto str = &b[dy.name.offset - sizeof(load_command)];
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
      return sizeof(mach_header_64) + m_hdr->sizeofcmds();
    return sizeof(mach_header_32) + m_hdr->sizeofcmds();
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

section_64::section_64()
  : section{}
  , reserved3{0}
{}

section_64::section_64(std::ifstream& file, mach_object& owner)
{
  readInto<section>(file, this);
  if (file) {
    uint32_t ali = owner.endian<uint32_t>(align);
    file.seekg(ali*2 + file.tellg());
  }
}

// -----------------------------------------------------------

data_segment::data_segment()
  : segname{0}
  , filesize{0}
  , fileoff{0}
{}

data_segment::data_segment(std::ifstream& file, const load_command_bytes& cmd)
  : segname{0}
  , filesize{0}
  , fileoff{0}
{
  if (cmd.cmd == SEGMENT_64) {
    read_into<segment_command_64, uint64_t>(file, cmd);
  } else {
    read_into<segment_command_64, uint32_t>(file, cmd);
  }
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


