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

#ifndef MACHO_
#define MACHO_

#include <vector>
#include <fstream>
#include <stdint.h>
#include "Common.h"
#include "Types.h"

namespace MachO {

/**
 * A mach-o file is a executable format used by apple OS macos, iOs etc.
 *
 * May be a multi architecture file with several different object files
 * for different architectures. Called a fat binary.
 *
 * If it is not a fat binary it only contains one object file. (ideally)
 *
 *  Simple one arch object structure
 * +-----------------------+
 * |    mach_header        |       sizeofcmds LCCMDS
 * +-----------------------+
 * |    LC_CMDS no in hdr  |       Each LC_CMD gives i size in bytes padded to %4 for 32bit and %8 for 64
 * +-----------------------+
 * |    __TEXT other init  |       fileoff points to were code starts
 * +-----------------------+
 * |   ___DATA             |
 * +-----------------------+
 * |   __DATA_CONST        |
 * +-----------------------+
 *
 *
 * a fat binary:
 * +-----------------------+
 * |  fat_header           |
 * +-----------------------+
 * |  fat_arch 0           | cpu for this arch and offset to object file
 * +-----------------------+
 * |  fat arch 1           | as above but for another cpu
 * +-----------------------+
 * |  Mach-o object arch 0 | -> is on of simple one arch object above
 * +-----------------------+
 * |  Mach-o object arch 1 | -> is another one
 * +-----------------------+
 *
 * A load command can have many shapes but in it always has:
 * +-----------------------+
 * | CMD_TYPE              |    Each command has its complete size given
 * | CMD_SIZE              |    stuff after here must padded by 0 to align with 32 or 64 bits
 * | ** possible more      |
 * +-----------------------+
 */

using cpu_type_t = uint32_t;
using cpu_subtype_t = uint32_t;
using vm_prot_t = int;
const cpu_type_t CPU_ARCH_MASK = 0xff000000;		/* mask for architecture bits */
const cpu_type_t CPU_ARCH_ABI64	= 0x01000000;		/* 64 bit ABI */


enum Magic: uint32_t {
  Magic32 = 0xfeedface,
  Cigam32 = 0xcefaedfe, // 0xfeedface reversed (aka little endian)
  Magic64 = 0xfeedfacf,
  Cigam64 = 0xcffaedfe,  // 0xfeedfacf reversed (aka little endian)
  FatMagic = 0xcafebabe,
  FatCigam = 0xbebafeca
};

enum FileType: uint32_t {
  MH_OBJECT = 0x1,		  /* relocatable object file */
  MH_EXECUTE = 0x2,		  /* demand paged executable file */
  MH_FVMLIB = 0x3,		  /* fixed VM shared library file */
  MH_CORE = 0x4,		    /* core file */
  MH_PRELOAD = 0x5,		  /* preloaded executable file */
  MH_DYLIB = 0x6,		    /* dynamically bound shared library */
  MH_DYLINKER = 0x7,		/* dynamic link editor */
  MH_BUNDLE = 0x8,		  /* dynamically bound bundle file */
  MH_DYLIB_STUB = 0x9,	/* shared library stub for static */
                        /*  linking only, no section contents */
  MH_DSYM = 0xa,		    /* companion file with only debug */
                        /*  sections */
  MH_KEXT_BUNDLE = 0xb,	/* x86_64 kexts */
};

const char* FiletypeStr(FileType type);

enum Flags: uint32_t {
  /* Constants for the flags field of the mach_header */
  MH_NOUNDEFS = 0x1,    /* the object file has no undefined
                            references */
  MH_INCRLINK = 0x2,    /* the object file is the output of an
                            incremental link against a base file
                            and can't be link edited again */
  MH_DYLDLINK = 0x4,    /* the object file is input for the
                            dynamic linker and can't be staticly
                            link edited again */
  MH_BINDATLOAD = 0x8,  /* the object file's undefined
                            references are bound by the dynamic
                            linker when loaded. */
  MH_PREBOUND = 0x10,   /* the file has its dynamic undefined
                            references prebound. */
  MH_SPLIT_SEGS = 0x20, /* the file has its read-only and
                            read-write segments split */
  MH_LAZY_INIT = 0x40,  /* the shared library init routine is
                            to be run lazily via catching memory
                            faults to its writeable segments
                            (obsolete) */
  MH_TWOLEVEL = 0x80,   /* the image is using two-level name
                            space bindings */
  MH_FORCE_FLAT = 0x100, /* the executable is forcing all images
                            to use flat name space bindings */
  MH_NOMULTIDEFS = 0x200,	/* this umbrella guarantees no multiple
                              defintions of symbols in its
                              sub-images so the two-level namespace
                              hints can always be used. */
  MH_NOFIXPREBINDING = 0x400,	/* do not have dyld notify the
                                  prebinding agent about this
                                executable */
  MH_PREBINDABLE = 0x800, /* the binary is not prebound but can
                              have its prebinding redone. only used
                              when MH_PREBOUND is not set. */
  MH_ALLMODSBOUND = 0x1000, /* indicates that this binary binds to
                                all two-level namespace modules of
                                its dependent libraries. only used
                                when MH_PREBINDABLE and MH_TWOLEVEL
                                are both set. */
  MH_SUBSECTIONS_VIA_SYMBOLS = 0x2000,/* safe to divide up the sections into
                                          sub-sections via symbols for dead
                                          code stripping */
  MH_CANONICAL = 0x4000, /* the binary has been canonicalized
                            via the unprebind operation */
  MH_WEAK_DEFINES = 0x8000, /* the final linked image contains
                                external weak symbols */
  MH_BINDS_TO_WEAK = 0x10000,	/* the final linked image uses
                                  weak symbols */

  MH_ALLOW_STACK_EXECUTION = 0x20000,/* When this bit is set, all stacks
                                        in the task will be given stack
                                        execution privilege.  Only used in
                                        MH_EXECUTE filetypes. */
  MH_ROOT_SAFE = 0x40000, /* When this bit is set, the binary
                              declares it is safe for use in
                              processes with uid zero */

  MH_SETUID_SAFE = 0x80000,/* When this bit is set, the binary
                              declares it is safe for use in
                              processes when issetugid() is true */

  MH_NO_REEXPORTED_DYLIBS = 0x100000, /* When this bit is set on a dylib,
                                          the static linker does not need to
                                          examine dependent dylibs to see
                                        if any are re-exported */
  MH_PIE = 0x200000,       /* When this bit is set, the OS will
                              load the main executable at a
                              random address.  Only used in
                              MH_EXECUTE filetypes. */
  MH_DEAD_STRIPPABLE_DYLIB = 0x400000, /* Only for use on dylibs.  When
                                          linking against a dylib that
                                          has this bit set, the static linker
                                          will automatically not create a
                                          LC_LOAD_DYLIB load command to the
                                          dylib if no symbols are being
                                          referenced from the dylib. */
  MH_HAS_TLV_DESCRIPTORS = 0x800000, /* Contains a section of type
                                        S_THREAD_LOCAL_VARIABLES */

  MH_NO_HEAP_EXECUTION = 0x1000000,	/* When this bit is set, the OS will
                                        run the main executable with
                                        a non-executable heap even on
                                        platforms (e.g. i386) that don't
                                        require it. Only used in MH_EXECUTE
                                        filetypes. */

  MH_APP_EXTENSION_SAFE = 0x02000000, /* The code was linked for use in an
                                          application extension. */
};

const char* FlagsStr(Flags flag);



enum CpuType: uint32_t {

  MH_ANY =      ((cpu_type_t) -1),

  MH_VAX =      (cpu_type_t) 1,
/* skip				((cpu_type_t) 2)	*/
/* skip				((cpu_type_t) 3)	*/
/* skip				((cpu_type_t) 4)	*/
/* skip				((cpu_type_t) 5)	*/
  MH_MC680x0 =  (cpu_type_t) 6,
  MH_X86     =  (cpu_type_t) 7,
  MH_X86_64  =  (cpu_type_t)(7 | CPU_ARCH_ABI64),

/* skip CPU_TYPE_MIPS		((cpu_type_t) 8)	*/
/* skip 			((cpu_type_t) 9)	*/
  MH_MC98000  = ((cpu_type_t) 10),
  MH_HPPA     = ((cpu_type_t) 11),
  MH_ARM      = ((cpu_type_t) 12),
  MH_ARM64    = (cpu_type_t) (12 | CPU_ARCH_ABI64),
  MH_MC88000  = ((cpu_type_t) 13),
  MH_SPARC    = ((cpu_type_t) 14),
  MH_I860     = ((cpu_type_t) 15),
/* skip	CPU_TYPE_ALPHA		((cpu_type_t) 16)	*/
/* skip				((cpu_type_t) 17)	*/
  MH_POWERPC  = ((cpu_type_t) 18),
  MH_POWERPC64 =(cpu_type_t) (18 | CPU_ARCH_ABI64),
};

const char* CpuTypeStr(CpuType type);


/* Constants for the cmd field of all load commands, the type */
const uint32_t LC_REQ_DYLD = 0x80000000;
enum LoadCmds: uint32_t {
    LC_SEGMENT = 0x1, 	/* segment of this file to be mapped */
    LC_SYMTAB = 0x2, 	/* link-edit stab symbol table info */
    LC_SYMSEG = 0x3, 	/* link-edit gdb symbol table info (obsolete) */
    LC_THREAD = 0x4, 	/* thread */
    LC_UNIXTHREAD = 0x5, 	/* unix thread (includes a stack) */
    LC_LOADFVMLIB = 0x6, 	/* load a specified fixed VM shared library */
    LC_IDFVMLIB = 0x7, 	/* fixed VM shared library identification */
    LC_IDENT = 0x8, 	/* object identification info (obsolete) */
    LC_FVMFILE = 0x9, 	/* fixed VM file inclusion (internal use) */
    LC_PREPAGE = 0xa,      /* prepage command (internal use) */
    LC_DYSYMTAB = 0xb, 	/* dynamic link-edit symbol table info */
    LC_LOAD_DYLIB = 0xc, 	/* load a dynamically linked shared library */
    LC_ID_DYLIB = 0xd, 	/* dynamically linked shared lib ident */
    LC_LOAD_DYLINKER = 0xe, 	/* load a dynamic linker */
    LC_ID_DYLINKER = 0xf, 	/* dynamic linker identification */
    LC_PREBOUND_DYLIB = 0x10, 	/* modules prebound for a dynamically */
				/*  linked shared library */
    LC_ROUTINES = 0x11, 	/* image routines */
    LC_SUB_FRAMEWORK = 0x12, 	/* sub framework */
    LC_SUB_UMBRELLA = 0x13, 	/* sub umbrella */
    LC_SUB_CLIENT = 0x14, 	/* sub client */
    LC_SUB_LIBRARY = 0x15, 	/* sub library */
    LC_TWOLEVEL_HINTS = 0x16, 	/* two-level namespace lookup hints */
    LC_PREBIND_CKSUM = 0x17, 	/* prebind checksum */

/*
 * load a dynamically linked shared library that is allowed to be missing
 * (all symbols are weak imported).
 */
    LC_LOAD_WEAK_DYLIB = (0x18 | LC_REQ_DYLD),

    LC_SEGMENT_64 = 0x19, 	/* 64-bit segment of this file to be
				   mapped */
    LC_ROUTINES_64 = 0x1a, 	/* 64-bit image routines */
    LC_UUID = 0x1b, 	/* the uuid */
    LC_RPATH = (0x1c | LC_REQ_DYLD),    /* runpath additions */
    LC_CODE_SIGNATURE = 0x1d, 	/* local of code signature */
    LC_SEGMENT_SPLIT_INFO = 0x1e,  /* local of info to split segments */
    LC_REEXPORT_DYLIB = (0x1f | LC_REQ_DYLD), /* load and re-export dylib */
    LC_LAZY_LOAD_DYLIB = 0x20, 	/* delay load of dylib until first use */
    LC_ENCRYPTION_INFO = 0x21, 	/* encrypted segment information */
    LC_DYLD_INFO = 0x22, 	/* compressed dyld information */
    LC_DYLD_INFO_ONLY = (0x22 | LC_REQ_DYLD),	/* compressed dyld information only */
    LC_LOAD_UPWARD_DYLIB = (0x23 | LC_REQ_DYLD), /* load upward dylib */
    LC_VERSION_MIN_MACOSX = 0x24,    /* build for MacOSX min OS version */
    LC_VERSION_MIN_IPHONEOS = 0x25,  /* build for iPhoneOS min OS version */
    LC_FUNCTION_STARTS = 0x26,  /* compressed table of function start addresses */
    LC_DYLD_ENVIRONMENT = 0x27,  /* string for dyld to treat
				    like environment variable */
    LC_MAIN = (0x28|LC_REQ_DYLD), /* replacement for LC_UNIXTHREAD */
    LC_DATA_IN_CODE = 0x29,  /* table of non-instructions in __text */
    LC_SOURCE_VERSION = 0x2A,  /* source version used to build binary */
    LC_DYLIB_CODE_SIGN_DRS = 0x2B,  /* Code signing DRs copied from linked dylibs */
    LC_ENCRYPTION_INFO_64 = 0x2C,  /* 64-bit encrypted segment information */
    LC_LINKER_OPTION = 0x2D,  /* linker options in MH_OBJECT files */
    LC_LINKER_OPTIMIZATION_HINT = 0x2E,  /* optimization hints in MH_OBJECT files */
    LC_VERSION_MIN_TVOS = 0x2F,  /* build for AppleTV min OS version */
    LC_VERSION_MIN_WATCHOS = 0x30,  /* build for Watch min OS version */
    LC_NOTE = 0x31,  /* arbitrary data included within a Mach-O file */
};

const char* LoadCmdStr(LoadCmds cmd);

class MachOLoader;
class mach_object;
class load_command_bytes;
class data_segment;

template<typename T>
std::ifstream& readInto(std::ifstream& file, T* dest)
{
  if (file.fail()) return file;
  std::streamsize nBytes = sizeof(T);
  if (file.readsome((char*)dest, nBytes) != nBytes) {
    file.setstate(std::ios::failbit);
  }
  return file;
}



#pragma pack(push, 1)

// if we have a fat binary this is what
// in the very beginning of the file
class fat_header {
public:
  fat_header();
  fat_header(std::ifstream& file);

  Magic magicRaw() const;
  Magic magic() const;
  bool isBigEndian() const;
  uint32_t nfat_arch() const;
private:
	Magic	    m_magic;      /* FAT_MAGIC */
	uint32_t	m_nfat_arch;  /* number of structs that follow */
};

struct fat_arch {
  fat_arch();
  fat_arch(std::ifstream& file);
	CpuType	cputype;	/* cpu specifier (int) */
	cpu_subtype_t	cpusubtype;	/* machine specifier (int) */
	uint32_t	offset;		/* file offset to this object file */
	uint32_t	size;		/* size of this object file */
	uint32_t	align;		/* alignment as a power of 2 */
};

// architecture header
class mach_header_32
{
public:
  mach_header_32();
  mach_header_32(std::ifstream& file);
  Magic magic() const { return m_magic; };        // the magic marker, determines
  CpuType cputype() const { return m_cputype; }    // the cputype
  cpu_subtype_t cpusubtype() const { return m_cpusubtype; } // subtype, machine type
  FileType filetype() const { return m_filetype; }  // type of file
  uint32_t ncmds() const { return m_ncmds; }     // num of lod commands
  uint32_t sizeofcmds() const { return m_sizeofcmds; } // size of all load commands, may vary
  bool isBigEndian() const;
  bool is64bits() const;
  std::ofstream& operator>>(std::ofstream& file) const;

protected:
  uint32_t convertEndian(uint32_t) const;
  void endian();

	Magic         m_magic;		      // magic number identifier
	CpuType	      m_cputype;	      // cpu type
	cpu_subtype_t	m_cpusubtype;	    // machine type
	FileType      m_filetype;	      // type of file
	uint32_t	    m_ncmds;		      // number of load commands
	uint32_t	    m_sizeofcmds;	    // the size of all the load commands
	uint32_t	    m_flags;		      // flags
};

class mach_header_64 : public mach_header_32 {
public:
  mach_header_64();
  mach_header_64(std::ifstream& file);
  uint32_t reserved() const { return m_reserved; }
  std::ofstream& operator>>(std::ofstream& file) const;

private:
  uint32_t      m_reserved;       // not used?
};

/*
 * A variable length string in a load command is represented by an lc_str
 * union.  The strings are stored just after the load command structure and
 * the offset is from the start of the load command structure.  The size
 * of the string is reflected in the cmdsize field of the load command.
 * Once again any padded bytes to bring the cmdsize field to a multiple
 * of 4 bytes must be zero.
 */
typedef union _lcStr
{
  _lcStr();
  _lcStr(const char* bytes, const mach_object* obj);
	uint32_t	offset;	/* offset to the string */
	char		*ptr64bit;	/* pointer to the string, only on 64bit targets */
} lc_str ;

/**
 * Load commands follow directly after header. Total size of load
 * commands is given by sizeofcmds field in header
 * All commands begins with these two values.
 * The cmdsize field is the size in bytes
 * of the particular load command structure plus anything that follows it that
 * is a part of the load command (i.e. section structures, strings, etc.).  To
 * advance to the next load command the cmdsize can be added to the offset or
 * pointer of the current load command. The cmdsize for 32-bit architectures
 * MUST be a multiple of 4 bytes and for 64-bit architectures MUST be a multiple
 * of 8 bytes (these are forever the maximum alignment of any load commands).
 * The padded bytes must be zero.  All tables in the object file must also
 * follow these rules so the file can be memory mapped.  Otherwise the pointers
 * to these tables will not work well or at all on some machines.  With all
 * padding zeroed like objects will compare byte for byte.
 */
struct load_command
{
  load_command();
  load_command(const load_command_bytes& cmd);
  LoadCmds cmd() const { return m_cmd; }
  uint32_t cmdsize() const { return m_cmdsize; }
protected:
  LoadCmds m_cmd;        // the type of command
  uint32_t m_cmdsize;    // size of this command including
};

struct load_command_bytes : load_command
{
  load_command_bytes();
  //load_command_bytes(mach_object* owner);
  load_command_bytes(std::ifstream& file, mach_object& owner);
  std::unique_ptr<char[]> bytes;
  //mach_object* owner;
};

// used by LC_LOAD_DYLIB, LC_LOAD_WEK_DYLIB and LC_REEXPORT_DYLIB
struct dylib_command : load_command
{
  dylib_command();
  dylib_command(const load_command_bytes& from, const mach_object& obj);
  lc_str name() const  { return m_name; }
  uint32_t timestamp() const { return m_timestamp; }
  uint32_t current_version() const { return m_current_version; }
  uint32_t compatibility_version() const { return m_compatibility_version; }

private:
  lc_str  m_name;			/* library's path name */
  uint32_t m_timestamp;			/* library's build time stamp */
  uint32_t m_current_version;		/* library's current version number */
  uint32_t m_compatibility_version;	/* library's compatibility vers number*/
};

/*
 * The segment load command indicates that a part of this file is to be
 * mapped into the task's address space.  The size of this segment in memory,
 * vmsize, maybe equal to or larger than the amount to map from this file,
 * filesize.  The file is mapped starting at fileoff to the beginning of
 * the segment in memory, vmaddr.  The rest of the memory of the segment,
 * if any, is allocated zero fill on demand.  The segment's maximum virtual
 * memory protection and initial virtual memory protection are specified
 * by the maxprot and initprot fields.  If the segment has sections then the
 * section structures directly follow the segment command and their size is
 * reflected in cmdsize.
 */
struct segment_command : load_command { /* for 32-bit architectures */
  segment_command(const load_command_bytes& cmd, const mach_object& obj);
  const char* segname() const { return m_segname; }
  uint32_t vmaddr() const { return m_vmaddr; }
  uint32_t vmsize() const { return m_vmsize; }
  uint32_t fileoff() const { return m_fileoff; }
  uint32_t filesize() const { return m_filesize; }
  vm_prot_t maxprot() const { return m_maxprot; }
  vm_prot_t initprot() const { return m_initprot; }
  uint32_t nsects() const { return m_nsects; }
  uint32_t flags() const { return m_flags; }

  enum {SZ_SEGNAME = 16};

private:
	char		m_segname[SZ_SEGNAME];	/* segment name */
	uint32_t	m_vmaddr;		/* memory address of this segment */
	uint32_t	m_vmsize;		/* memory size of this segment */
	uint32_t	m_fileoff;	/* file offset of this segment */
	uint32_t	m_filesize;	/* amount to map from the file */
	vm_prot_t	m_maxprot;	/* maximum VM protection */
	vm_prot_t	m_initprot;	/* initial VM protection */
	uint32_t	m_nsects;		/* number of sections in segment */
	uint32_t	m_flags;		/* flags */
};

/*
 * The 64-bit segment load command indicates that a part of this file is to be
 * mapped into a 64-bit task's address space.  If the 64-bit segment has
 * sections then section_64 structures directly follow the 64-bit segment
 * command and their size is reflected in cmdsize.
 */
struct segment_command_64 : load_command { /* for 64-bit architectures */
  segment_command_64(const load_command_bytes& cmd, const mach_object& obj);
  const char* segname() const { return m_segname; }
  uint64_t vmaddr() const { return m_vmaddr; }
  uint64_t vmsize() const { return m_vmsize; }
  uint64_t fileoff() const { return m_fileoff; }
  uint64_t filesize() const { return m_filesize; }
  vm_prot_t maxprot() const { return m_maxprot; }
  vm_prot_t initprot() const { return m_initprot; }
  uint32_t nsects() const { return m_nsects; }
  uint32_t flags() const { return m_flags; }

private:
	char		m_segname[16];	/* segment name */
	uint64_t	m_vmaddr;		/* memory address of this segment */
	uint64_t	m_vmsize;		/* memory size of this segment */
	uint64_t	m_fileoff;	/* file offset of this segment */
	uint64_t	m_filesize;	/* amount to map from the file */
	vm_prot_t	m_maxprot;	/* maximum VM protection */
	vm_prot_t	m_initprot;	/* initial VM protection */
	uint32_t	m_nsects;		/* number of sections in segment */
	uint32_t	m_flags;		/* flags */
};

struct section { /* for 32-bit architectures */
  section();
  section(const char* bytes, const mach_object& obj);
  const char* sectname() const { return m_sectname; }
  const char* segname()  const { return m_segname; }
  uint32_t addr() const { return m_addr; }
  uint32_t size() const { return m_size; }
  uint32_t offset() const { return m_offset; }
  uint32_t align() const { return m_align; }
  uint32_t reloff() const { return m_reloff; }
  uint32_t nreloc() const { return m_nreloc; }
  uint32_t flags() const { return m_flags; }
  uint32_t reserved1() const { return m_reserved1; }
  uint32_t reserved2() const { return m_reserved2; }

private:
	char		m_sectname[16];	/* name of this section */
	char		m_segname[16];	/* segment this section goes in */
	uint32_t	m_addr;		/* memory address of this section */
	uint32_t	m_size;		/* size in bytes of this section */
	uint32_t	m_offset;		/* file offset of this section */
	uint32_t	m_align;		/* section alignment (power of 2) */
	uint32_t	m_reloff;		/* file offset of relocation entries */
	uint32_t	m_nreloc;		/* number of relocation entries */
	uint32_t	m_flags;		/* flags (section type and attributes)*/
	uint32_t	m_reserved1;	/* reserved (for offset or index) */
	uint32_t	m_reserved2;	/* reserved (for count or sizeof) */
};

struct section_64 : section { /* for 64-bit architectures */
  section_64();
  section_64(const char* bytes, const mach_object& obj);
  uint32_t reserved3() const { return m_reserved3; }

private:
	uint32_t	m_reserved3;	/* reserved */
};

// ---------------------------------------------------------------

class mach_object {
public:
  mach_object();
  mach_object(std::ifstream& file);
  bool isBigEndian() const;
  bool is64bits() const;
  const mach_header_32* header32() const;
  const mach_header_64* header64() const;
  std::vector<Path> rpaths() const;
  std::vector<Path> loadDylibPaths() const;
  std::vector<Path> reexportDylibPaths() const;
  std::vector<Path> weakLoadDylib() const;
  std::vector<const data_segment*> dataSegments() const;
  void changeRPath(PathRef oldPath, PathRef newPath);
  void changeLoadDylibPaths();
  void changeReexportDylibPaths();
  bool failure() const;

  template<typename T>
  T endian(T in) const
  {
    if constexpr(hostIsBigEndian)
      return isBigEndian() ? in : reverseEndian<T>(in);
    else
      return isBigEndian() ? reverseEndian<T>(in) : in;
  }

private:
  void fail();
  void readHdr(std::ifstream& file);
  void readCmds(std::ifstream& file);
  void readData(std::ifstream& file);
  std::vector<Path> searchForDylibs(LoadCmds type) const;
  size_t fileoff() const;

  const std::streampos m_startpos;
  std::unique_ptr<mach_header_32> m_hdr;
  std::vector<load_command_bytes> m_load_cmds;
  std::vector<std::unique_ptr<data_segment>> m_data_segments;
};

// ------------------------------------------------------------------

/// raw data copied from data segment
class data_segment {
public:
  data_segment();
  data_segment(
    std::ifstream& file,
    const load_command_bytes& cmd,
    const mach_object& obj);

  const char* segname() const { return m_segname; }
  uint64_t filesize() const { return m_filesize; }
  uint64_t fileoff() const { return m_fileoff; }


private:
  char m_segname[segment_command::SZ_SEGNAME];
  uint64_t m_filesize;
  uint64_t m_fileoff;
  std::unique_ptr<char[]> bytes;

  template<typename T, typename U>
    void read_into(
      std::ifstream& file,
      const load_command_bytes& cmd,
      const mach_object& obj)
  {
    T seg{cmd, obj};
    memcpy((void*)this->m_segname, (void*)seg.segname(), sizeof(m_segname));
    m_fileoff = seg.fileoff();
    m_filesize = seg.filesize();

    file.seekg(m_fileoff);

    std::streampos sz = m_filesize;
    bytes = std::make_unique<char[]>(sz);
    if (file.readsome(bytes.get(), sz) != sz) {
      file.setstate(std::ios::badbit);
      return;
    }
  }
};

// -------------------------------------------------

class mach_fat_object
{
public:
  mach_fat_object();
  mach_fat_object(std::ifstream& file);

  const std::vector<mach_object>& objects() const;
  const std::vector<fat_arch>& architectures() const;
  bool isBigEndian() const;

  bool failure() const;

  template<typename T>
    T endian(T in) const
  {
    if constexpr(hostIsBigEndian)
      return isBigEndian() ? in : reverseEndian<T>(in);
    else
      return isBigEndian() ? reverseEndian<T>(in) : in;
  }

private:
  void fail();

  std::unique_ptr<fat_header> m_hdr;
  std::vector<fat_arch> m_fat_arch;
  std::vector<mach_object> m_objects;
};



#pragma pack(pop)

// --------------------------------------------------

class MachOLoader
{
public:
  MachOLoader(PathRef binPath);

  bool isFat() const;
  bool hasArm() const;
  bool hasX86() const;
  uint32_t nArchitectures() const;
  CpuType cpuFor(int architecture) const;


private:
  void readHeader(std::ifstream& file);

  Path m_binPath;
  fat_header m_fat_header;
  fat_arch m_fat_arch;
  std::string m_error;
};

} // namespace Macho

#endif // MACHO_

