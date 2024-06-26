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

#include "asyncfifo.hpp"
#include <exception>

using namespace afifo;

time_t afifo::millis(time_t forward_ms) {
  struct timespec t ;
  clock_gettime ( CLOCK_REALTIME , & t ) ;
  time_t now = t.tv_sec * 1000 + ( t.tv_nsec + 500000 ) / 1000000;
  return now + forward_ms;
}

FileBase::FileBase(FILE* fp):
  _fp{fp}
{}

FileBase::FileBase(FileBase&& rhs):
  _fp{std::move(rhs._fp)}
{
  std::cout<< "FileBase move constructor" << std::endl;
  rhs._fp = nullptr; // so file does not get closed
}

FileBase::~FileBase()
{}

void
FileBase::init(time_t timeout_ms)
{
  (void)timeout_ms;
}

FileBase&
FileBase::operator=(FileBase&& rhs)
{
  std::cout << "FileBase::operator=(&&Filebase)" << std::endl;
  _fp = std::move(rhs._fp);
  return *this;
};

bool
FileBase::operator==(const FileBase& other)
{
  return fileno() == other.fileno();
}

FILE*
FileBase::file()
{
  return _fp;
}

int
FileBase::fileno() const
{
  return ::fileno(_fp);
}

bool
FileBase::isWritable() const
{
  struct stat st;
  if (fstat(fileno(), &st) == 0)
    return st.st_mode & O_WRONLY;
  return false;
}

bool
FileBase::good() const
{
  if (isWritable() && _fp) {
    fd_set wrSet{0};
    int num = fileno();
    FD_SET(num, &wrSet);
    struct timeval tv{0,0};
    // write fifo becomes readable when other side disconnects
    auto ret = select(num+1, &wrSet, nullptr, nullptr, &tv);
    if (ret < 0)
      return false;
    if (FD_ISSET(num, &wrSet))
      return false;
    return true;
  }
  return _fp != nullptr; // readable is always good to read from, no broken pipe
}

void
FileBase::setBlocking() const
{
  int flags = fcntl(fileno(), F_GETFL, 0);
  flags &= ~(O_NONBLOCK);
  fcntl(fileno(), F_SETFL, flags);
}

void
FileBase::setNonBlocking() const
{
  int flags = fcntl(fileno(), F_GETFL, 0);
  fcntl(fileno(), F_SETFL, O_NONBLOCK | flags);
}

bool
FileBase::isBlocking() const
{
  int flags = fcntl(fileno(), F_GETFL, 0);
  return (O_NONBLOCK | flags) != O_NONBLOCK;
}

// -----------------------------------------------

Readable::Readable(FILE* fp):
  FileBase{fp}
{}

Readable::Readable(Readable&& rhs):
  FileBase(std::move(rhs))
{
  std::cout << "Readable move constructor" << std::endl;
}

Readable::~Readable() {}

Readable& Readable::operator=(Readable&& rhs)
{
  std::cout << "Readable move assignment" << std::endl;
  FileBase::operator=(std::move(rhs));
  return *this;
}

std::vector<char>
Readable::read()
{
  int c = 0;
  std::vector<char> bytes{};

  setNonBlocking();

  for (;;) {
    c = fgetc(_fp);
    if (c == -1) {
      if (errno != EAGAIN)
        std::cout << "error while reading: " << strerror(errno) << std::endl;
      break;
    }
    bytes.emplace_back(static_cast<char>(c));
  }

  setBlocking();

  return bytes;
}

// ------------------------------------------------

Writable::Writable(FILE* fp):
  FileBase{fp},
  _wrBytes{}
{}

Writable::Writable(Writable&& rhs):
  FileBase(std::move(rhs)),
  _wrBytes{std::move(rhs._wrBytes)}
{
  std::cout<<"Writable move constructor" << std::endl;
}

Writable::~Writable()
{
  flush();
}

Writable& Writable::operator=(Writable&& rhs)
{
  std::cout<<"Writable move assignment" << std::endl;
  _wrBytes = std::move(rhs._wrBytes);
  FileBase::operator=(std::move(rhs));
  return *this;
}

void
Writable::write(const Bytes& bytes)
{
  _wrBytes = bytes;
}

void
Writable::write(const std::string& str)
{
  _wrBytes.reserve(str.size()+1);
  for (const auto& c : str)
    _wrBytes.emplace_back(c);
  _wrBytes.emplace_back('\0');
}

int
Writable::flush()
{
  if (_wrBytes.size()) {
    auto buf = _wrBytes.data();
    auto ret = fwrite(buf, sizeof(buf[0]), _wrBytes.size(), _fp);
    _wrBytes.resize(_wrBytes.size()-ret);
    if (ret == 0)
      throw std::runtime_error(std::string("Error writing to file ")
        + strerror(errno));
    fflush(_fp);
    return ret;
  }
  return 0;
}

bool
Writable::isWritable() const
{
  return true;
}

// ----------------------------------------------------

 Process::Process(const char* cmd, const std::vector<std::string>& args) :
  Readable{nullptr},
  _cmd(cmd), _args{args}
{}

Process::Process(Process&& rhs):
  Readable{std::move(rhs)},
  _cmd{std::move(rhs._cmd)},
  _args{std::move(rhs._args)}
{
  std::cout << "Process move constructor"<< std::endl;
}

Process::~Process()
{
  std::cout << "closing progress\n";
  closeFile();
}

void
Process::init(time_t timeout_ms)
{
  (void)timeout_ms;
  _fp = popen(command().c_str(), "r");
  if (_fp == nullptr)
    throw std::runtime_error(
      std::string("Could not open process: ") + command());
}

Process&
Process::operator=(Process&& rhs)
{
  std::cout << "Process assigment operator" << std::endl;
  _cmd = std::move(rhs._cmd);
  _args = std::move(rhs._args);
  Readable::operator=(std::move(rhs));
  return *this;
}

std::string
Process::command() const
{
  std::string cmd{_cmd};
  for (const auto& arg : _args)
    cmd += " " + arg;
  return cmd;
}

const std::vector<std::string>&
Process::args()
{
  return _args;
}

void
Process::closeFile()
{
  if (_fp) {
      pclose(_fp);
      _fp = nullptr;
  }
}

// ---------------------------------------------------
AsyncIO::AsyncIO():
  _files{}, _setRd{}, _setWr{},
  _readSubscribers{},
  _writeSubscribers{}
{
  FD_ZERO(&_setRd);
  FD_ZERO(&_setWr);
}

AsyncIO::AsyncIO(AsyncIO&& rhs):
  _files{std::move(rhs._files)},
  _setRd{std::move(rhs._setRd)},
  _setWr{std::move(rhs._setWr)},
  _readSubscribers{std::move(rhs._readSubscribers)},
  _writeSubscribers{std::move(rhs._writeSubscribers)}
{}

AsyncIO::~AsyncIO()
{
  for (const auto& file : _files) {
    if (file->file() != nullptr) {
      unregisterFile(file.get());
      file->closeFile();
    }
  }
}

AsyncIO&
AsyncIO::operator=(AsyncIO&& rhs)
{
  _files = std::move(rhs._files);
  _setRd = std::move(rhs._setRd);
  _setWr = std::move(rhs._setWr);
  _readSubscribers = std::move(rhs._readSubscribers);
  _writeSubscribers = std::move(rhs._writeSubscribers);
  return *this;
}

FileBase&
AsyncIO::operator[](std::size_t idx)
{
  return *_files[idx];
}

std::vector<std::unique_ptr<FileBase>>::iterator
AsyncIO::begin()
{
  return _files.begin();
}

std::vector<std::unique_ptr<FileBase>>::iterator
AsyncIO::end()
{
  return _files.end();
}

void
AsyncIO::initFiles(time_t timeout_ms)
{
  for (const auto& file : _files) {
    file->init(timeout_ms);

    int flags = fcntl(file->fileno(), F_GETFL, 0);

    // register to select
    // O_RDONLY is 0x00 so we must do in this order
    if (flags & O_WRONLY)
      FD_SET(file->fileno(), &_setWr);
    else
      FD_SET(file->fileno(), &_setRd);
    }
}

/**
 * @brief Checks if all files are good
 *
 * @return true If all files are in a good state
 * @return false If not
 */
bool
AsyncIO::good() const
{
  for(const auto& f : _files){
    if (!f->good())
      return false;
  }
  return true;
}

/**
 * @brief Pool all files for new info, timeout
 *
 * @param timeout_ms How many milli seconds to wait
 */
void
AsyncIO::poll(time_t timeout_ms)
{
  if (_files.size() == 0)
    return;

  int nfds = 0;
  for (const auto& f : _files)
    nfds = f->fileno() > nfds ? f->fileno() : nfds;

  struct timeval tv;
  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec =  (timeout_ms % 1000) * 1000;

  fd_set rd{_setRd}, wr{_setWr};

  int bytes = select(nfds+1, &rd, &wr, nullptr, &tv);
  if (bytes == 0)
    return;
  else if (bytes < 0)
    throw std::runtime_error(
      std::string("select failed with: ") + strerror(errno));

  for (auto& f : _files) {
    auto num = f->fileno();
    if (FD_ISSET(num, &_setRd)) {
      onReadEvent(f.get());
    } else if (FD_ISSET(num, &_setWr)) {
      onWriteEvent(f.get());
    }
  }
}

/**
 * @brief Register a File to be pooled among others
 *
 * @param file The file to be checked for content
 */
void
AsyncIO::registerFile(
  std::unique_ptr<FileBase> file
){
  _files.emplace_back(std::move(file));
  std::cout<<"done registerFile" << std::endl;
}

/**
 * @brief Remove File from the checked files
 *
 * @param file The file to remove
 */
void
AsyncIO::unregisterFile(FileBase* file)
{
  if (FD_ISSET(file->fileno(), &_setRd))
    FD_CLR(file->fileno(), &_setRd);
  if (FD_ISSET(file->fileno(), &_setWr))
    FD_CLR(file->fileno(), &_setWr);
}

/**
 * @brief Remove File with this file descriptor
 *
 * @param idx The fileno of the file to remove
 */
void
AsyncIO::unregisterFile(std::size_t idx)
{
  if (idx < _files.size())
    unregisterFile(_files[idx].get());
}

/**
 * @brief Subscribe to read events for File has news
 *
 * @param file The file we want events from
 * @param cb The callback to to invoked
 * @return std::size_t The subscription index,
 *          used if you want to unsubscribe
 */
std::size_t
AsyncIO::subscribeRead(const FileBase& file, ReadCB cb)
{
  return _subscribe<ReadCB>(file, _readSubscribers, cb);
}

std::size_t
AsyncIO::subscribeRead(std::size_t idx, ReadCB cb)
{
  if (idx < _files.size())
    return subscribeRead(*_files[idx], cb);
  return -1;
}

/**
 * @brief Subscribe to write events for File has news
 *
 * @param file The file we want events from
 * @param cb The callback to to invoked
 * @return std::size_t The subscription index,
 *          used if you want to unsubscribe
 */
std::size_t
AsyncIO::subscribeWrite(const FileBase& file, WriteCB cb)
{
  return _subscribe<WriteCB>(file, _writeSubscribers, cb);
}

std::size_t
AsyncIO::subscribeWrite(std::size_t idx, WriteCB cb)
{
  if (idx < _files.size())
    return subscribeWrite(*_files[idx], cb);
  return -1;
}

/**
 * @brief Unsubscribe from read events
 *
 * @param subscribeIdx The subscription index returned from subscribeRead(...)
 * @return true If successful
 * @return false not successful
 */
bool
AsyncIO::unsubscribeRead(std::size_t subscribeIdx)
{
  return _unsubscribe<ReadCB>(subscribeIdx, _readSubscribers);
}

/**
 * @brief Unsubscribe from write events
 *
 * @param subscribeIdx The subscription index returned from subscribeWrite(...)
 * @return true If successful
 * @return false not successful
 */
bool
AsyncIO::unsubscribeWrite(std::size_t subscribeIdx)
{
  return _unsubscribe<WriteCB>(subscribeIdx, _writeSubscribers);
}

/**
 * @brief Instance wrapper for static method bytesToStr
 *
 * @param bytes The bytes to escape and convert
 * @return std::string The resulting string
 */
std::string
AsyncIO::bytesToString(const Bytes& bytes) const
{
  return AsyncIO::bytesToStr(bytes);
}

/**
 * @brief Escapes \0 and other and converts to std::string
 *
 * Useful When we want to read a response as a string
 *
 * @param bytes The bytes to escape and convert
 * @return std::string The resulting string
 */
std::string
AsyncIO::bytesToStr(const Bytes& bytes) {
  std::stringstream ss;
  for (const auto& b : bytes) {
    switch (b) {
    case '\0': ss << "\\0"; break;
    case '\n': ss << "\\n"; break;
    case '\r': ss << "\\r"; break;
    case '\t': ss << "\\t"; break;
    default:
      if (b < ' ')
        ss << "\\" << std::to_string(b);
      else
        ss << static_cast<char>(b);
    }
  }
  return ss.str();
}


void
AsyncIO::onReadEvent(FileBase* file)
{
  auto fptr = static_cast<Readable*>(file);
  auto bytes = fptr->read();
  if (bytes.size() == 0)
    return;
  for (const auto& s : _readSubscribers) {
    if (s.first == fptr->fileno())
      s.second(bytes, fptr);
  }
}

void
AsyncIO::onWriteEvent(FileBase* file)
{
  auto fptr = static_cast<Writable*>(file);
  fptr->flush();
  for (const auto& s : _writeSubscribers) {
    if (s.first == fptr->fileno())
      s.second(fptr);
  }
}

// -----------------------------------------------

ScriptIO::ScriptIO(
  const std::string& command,
  const std::vector<std::string>& args,
  const char* toScriptFifo,
  const char* fromScriptFifo
):
  AsyncIO{}
{
  {
  auto toFifo = std::make_unique<Fifo<Writable>>(toScriptFifo);
  registerFile(std::move(toFifo));
  }
  {
  auto fromFifo = std::make_unique<Fifo<Readable>>(fromScriptFifo);
  registerFile(std::move(fromFifo));
  }
  {
  auto process = std::make_unique<Process>(command.c_str(), args);
  registerFile(std::move(process));
  }
  _files[2]->init(20);
  _files[1]->init(20);
  _files[0]->init(20);
std::cout<<"Done register\n";
  subscribeWrite(0, [&](Writable* file){ writeReady(file); });
  subscribeRead(1, [&](const Bytes& bytes, Readable* file){
    fromScript(bytes, file); });
  subscribeRead(2, [&](const Bytes &bytes, Readable* file){
    stdoutFromScript(bytes, file); });


  std::cout << "leaving ScriptIO\n";
  std::cout.flush();
}

ScriptIO::ScriptIO(ScriptIO&& rhs):
  AsyncIO(std::move(rhs))
{}

ScriptIO::~ScriptIO()
{
  std::cout << "~ScriptIO\n";
}

ScriptIO&
ScriptIO::operator=(ScriptIO&& rhs)
{
  AsyncIO::operator=(std::move(rhs));
  return *this;
}

/**
 * @brief Run as a event loop continuously,
 *
 * see AsyncIO poll if you need your own eventloop
 */
void
ScriptIO::run(time_t timeout_ms)
{
  time_t end = millis(timeout_ms);
  while(good() && millis() < end) {
    this->poll();
    usleep(1000);
  }
  std::cout << "leaving run()" << '\n';
}

/**
 * @brief Stout received from script
 *
 * @param msg The message received
 * @param file The file that it got received from
 */
void
ScriptIO::stdoutFromScript(
  const Bytes& bytes, Readable *file)
{
  (void)file;
  std::cout << "script stdout:" << bytes.data();
}

/**
 * @brief Event callback when received from script
 *
 * @param msg The string message received from script
 * @param file The file that sent the message, ie fromFifo
 */
void
ScriptIO::fromScript(
  const Bytes& bytes, Readable *file)
{
  (void)bytes; (void)file;
}

/**
 * @brief Event callback when write buffer is ready to receive
 *
 * @param file The file that can be written to, ie toFifo
 */
void
ScriptIO::writeReady(Writable* file)
{
  (void)file;
}

/**
 * @brief Gets the fifo used to write to script
 *
 * @return Fifo<Writable>& The writable fifo, ie toFifo
 */
Fifo<Writable>&
ScriptIO::toScriptFifo()
{
  return *static_cast<Fifo<Writable>*>(_files[0].get());
}

/**
 * @brief Gets the fifo used to read from script
 *
 * @return Fifo<Readable>&  The readable fifo, ie fromFifo
 */
Fifo<Readable>&
ScriptIO::fromScriptFifo()
{
  return *static_cast<Fifo<Readable>*>(_files[1].get());
}

/**
 * @brief Gets the file used for reading script normal output
 *
 * @return Fifo<Readable>& The file used to read from stdout
 */
Fifo<Readable>&
ScriptIO::scriptStdout()
{
  return *static_cast<Fifo<Readable>*>(_files[2].get());
}