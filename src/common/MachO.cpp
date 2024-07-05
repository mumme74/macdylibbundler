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

char*
fat_header::c_ptr()
{
  return (char*)m_magic;
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

char*
mach_header_32::c_ptr()
{
  return static_cast<char*>(static_cast<void*>(&m_magic));
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
  : m_reserved{0}
{}

uint32_t
mach_header_64::reserved() const
{
  return convertEndian(m_reserved);
}


// -----------------------------------------------------------

mach_object::mach_object(std::ifstream& file)
  : m_hdr{}
  , m_load_cmds{}
  , m_sections{}
{
  readHdr(file);
  if (m_hdr) {
    readCmds(file);
    readData(file);
  }
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
  return !m_hdr || !m_load_cmds.size() || !m_sections.size();
}

void
mach_object::fail()
{
  m_hdr.release();
  m_load_cmds.clear();
  m_sections.clear();
}

void
mach_object::readHdr(std::ifstream& file)
{
  // read header
  uint32_t magic = readMagic(file);
  if (magic == 0) {
    fail(); return;
  }

  std::streamsize nBytes;
  if (magic == Magic64 || magic == Cigam64) {
    m_hdr = std::make_unique<mach_header_64>();
    nBytes = sizeof(mach_header_64);
  } else {
    m_hdr = std::make_unique<mach_header_32>();
    nBytes = sizeof(mach_header_32);
  }

  file.seekg(file.beg);
  if (file.readsome(m_hdr->c_ptr(), nBytes) != nBytes) {
    fail(); return;
  }
}

void
mach_object::readCmds(std::ifstream& file)
{
  // read load commands
  std::streamsize nBytes = 0;
  for (size_t i = 0, end = m_hdr->ncmds(); i < end; ++i) {
    load_command_bytes cmd{};
    nBytes = sizeof(load_command);
    if (file.readsome((char*)&cmd.cmd, nBytes) != nBytes) {
      fail();
      return;
    }

    size_t cmdsz = endian<uint32_t>(cmd.cmdsize);
    if ((cmdsz % 4) != 0 || (m_hdr->is64bits() && (cmdsz % 8) != 0)) {
      std::cerr << "Uneven loadCmd size, bad file, bailing out\n";
      fail(); return;
    }

    nBytes = cmdsz - nBytes;
    auto bytes = std::make_unique<uint8_t[]>(nBytes);
    if (file.readsome((char*)bytes.get(), nBytes) != nBytes) {
      fail();
      return;
    }
    cmd.bytes = std::move(bytes);

    m_load_cmds.emplace_back(std::move(cmd));
  }
}


void
mach_object::readData(std::ifstream& file)
{
  return; // TODO not working yet
  for (const auto& cmd : m_load_cmds) {
    switch ((LoadCmds)endian<uint32_t>(cmd.cmd)) {
    case LoadCmds::SEGMENT:
      readSegment<segment_command, section, uint32_t>(file, cmd);
     break;
    case LoadCmds::SEGMENT_64:
      readSegment<segment_command_64, section_64, uint64_t>(
        file, cmd);
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
      const uint8_t* buf = loadCmd.bytes.get();
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

std::vector<const section*>
mach_object::sections32() const
{
  std::vector<const section*> vec;
  if (!m_sections.size()) return vec;
  for (const auto& sec : m_sections)
    vec.emplace_back(static_cast<section*>(sec.get()));
  return vec;
}

std::vector<const section_64*>
mach_object::sections64() const
{
  std::vector<const section_64*> vec;
  if (!m_sections.size()) return vec;
  for (const auto& sec : m_sections)
    vec.emplace_back(static_cast<section_64*>(sec.get()));
  return vec;
}

void
mach_object::readLcStr(
  lc_str& lcStr, const uint8_t* buf
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
      const uint8_t* buf = loadCmd.bytes.get();
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
      return sizeof(mach_header_32) + m_hdr->sizeofcmds();
    return sizeof(mach_header_64) + m_hdr->sizeofcmds();
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


