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
#ifndef FIFO_ASYNC_H
#define FIFO_ASYNC_H

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <error.h>
#include <string.h>
#include <unistd.h>
#include <filesystem>
#include <functional>
#include <vector>
#include <iostream>
#include <exception>

namespace afifo {

using Bytes = std::vector<char>;

time_t millis(time_t forward_ms = 0);

/// not meant to be used directly
class FileBase {
public:
  FileBase(FILE* fp);
  FileBase(const FileBase& other) = delete;
  FileBase(FileBase&& rhs);
  virtual ~FileBase();
  virtual void init(time_t timeout_ms);

  FileBase& operator=(FileBase&& rhs);
  FileBase& operator=(const FileBase& other) = delete;
  bool operator==(const FileBase& other);

  FILE* file();
  int fileno() const;

  virtual bool isWritable() const;
  bool good() const;

  void setBlocking() const;
  void setNonBlocking() const;
  bool isBlocking() const;

  virtual void closeFile() = 0;

protected:
  FILE* _fp;
};

// ----------------------------------

class Readable : public FileBase {
public:
  Readable(FILE* fp);
  Readable(Readable&& rhs);
  Readable(const Readable& other) = delete;
  virtual ~Readable();
  Readable& operator=(Readable&& rhs);

  std::vector<char> read();
};

// -------------------------------------

class Writable :public FileBase {
  Bytes _wrBytes;
public:
  Writable(FILE* fp);
  Writable(Writable&& rhs);
  Writable(const Writable& other) = delete;
  ~Writable();

  Writable& operator=(Writable&& rhs);
  Writable& operator=(const Writable& other) = delete;

  void write(const Bytes& bytes);
  void write(const std::string& str);

  int flush();

  bool isWritable() const;
};

// -------------------------------------------------------


template<typename T>
class Fifo : public T
{
  const char* _path;
  bool _createdFifo;
public:
  Fifo(const char *path):
    T{nullptr},
    _path(path), _createdFifo{false}
  {
    namespace fs = std::filesystem;
    if (!fs::exists(_path)) {
      if (mkfifo(_path, 0660) != 0)
        throw std::runtime_error(
          std::string("Could not mkfifo(") +_path + ", 0660) "
            + strerror(errno));
      _createdFifo = true;
      usleep(10000);
    }
  }

  Fifo(const Fifo<T>& other) = delete;

  Fifo(Fifo<T>&& rhs):
    T{std::move(rhs)},
    _path{std::move(_path)},
    _createdFifo{std::move(_createdFifo)}
  {
    std::cout << "Fifo<";
    if constexpr(std::is_same_v<T, Writable>)
      std::cout << "Writable";
    else
      std::cout << "Readable";
    std::cout << "> move constructor" << std::endl;

  }

  ~Fifo()
  {
    std::cout << "Closing fifo " << _path << "\n";
    if constexpr (std::is_same_v<T, Writable>)
      this->flush();

    this->closeFile();
    if (_createdFifo)
      std::filesystem::remove(_path);
  }

  void init(time_t timeout_ms)
  {
    if (this->_fp)
      return;

    int mode = O_RDONLY;
    if constexpr (std::is_same_v<T, Writable>)
      mode =  O_WRONLY;

    auto now = []()->time_t {
      time_t ms = millis();
      usleep(1000);
      return ms;
    };

    int fd;
    time_t end = millis(timeout_ms);
    do {
      fd = open(_path, O_NONBLOCK | mode);
    } while(fd < 0 && now() < end);

    if (fd <= 0)
      throw std::runtime_error(
        std::string("Could not create fifo ") + _path +
              " " + strerror(errno));

    this->_fp = fdopen(fd, mode & O_WRONLY ? "w" : "r");
    if (this->_fp == nullptr)
      throw std::runtime_error(
        std::string("Could not create fifo fdopen ") + _path +
              " " +  strerror(errno));

    this->setBlocking();
  }

  Fifo<T>& operator=(Fifo<T>&& rhs)
  {

    std::cout << "Fifo<";
    if constexpr(std::is_same_v<T, Writable>)
      std::cout << "Writable";
    else
      std::cout << "Readable";
    std::cout << "> move assignment" << std::endl;

    _path = std::move(rhs._path);
    _createdFifo = std::move(rhs._createdFifo);
    T::operator=(std::move(rhs));
    return *this;
  }

  Fifo<T>& operator=(const Fifo<T>& other) = delete;

  const char* path() const { return _path; }

  void closeFile() override {
    if (this->_fp) {
      this->setNonBlocking();
      fclose(this->_fp);
      this->_fp = nullptr;
    }
  }
};

// -----------------------------------------------

class Process : public Readable
{
  std::string _cmd;
  std::vector<std::string> _args;
public:
  Process(const char* cmd, const std::vector<std::string>& args);
  Process(Process&& rhs);
  Process(const Process& other) = delete;
  ~Process();


  void init(time_t timeout_ms);

  Process& operator=(Process&& rhs);
  Process& operator=(const Process& other) = delete;

  std::string command() const;

  const std::vector<std::string>& args();

  void closeFile() override;

private:
  int popen2();
  int pclose2();
};

class AsyncIO {
public:
  using ReadCB = std::function<void(const Bytes& bytes, Readable* file)>;
  using WriteCB = std::function<void(Writable* file)>;
  AsyncIO();
  AsyncIO(AsyncIO&& rhs);
  AsyncIO(const AsyncIO& other) = delete;
  virtual ~AsyncIO();

  AsyncIO& operator=(AsyncIO&& rhs);
  AsyncIO& operator=(const AsyncIO& other) = delete;

  FileBase& operator[](std::size_t idx);
  std::vector<std::unique_ptr<FileBase>>::iterator begin();
  std::vector<std::unique_ptr<FileBase>>::iterator end();

  void initFiles(time_t timeout_ms = 20);

  /**
   * @brief Checks if all files are good
   *
   * @return true If all files are in a good state
   * @return false If not
   */
  virtual bool good() const;

  /**
   * @brief Pool all files for new info, timeout
   *
   * @param timeout_ms How many milli seconds to wait
   */
  void poll(time_t timeout_ms = 20);

  /**
   * @brief Register a File to be pooled among others
   *
   * @param file The file to be checked for content
   */
  void registerFile(
    std::unique_ptr<FileBase> file
  );

  /**
   * @brief Remove File from the checked files
   *
   * @param file The file to remove
   */
  void unregisterFile(FileBase* file);

  /**
   * @brief Remove File with this file descriptor
   *
   * @param idx The fileno of the file to remove
   */
  void unregisterFile(std::size_t idx);

  /**
   * @brief Subscribe to read events for File has news
   *
   * @param file The file we want events from
   * @param cb The callback to to invoked
   * @return std::size_t The subscription index,
   *          used if you want to unsubscribe
   */
  std::size_t subscribeRead(const FileBase& file, ReadCB cb);
  std::size_t subscribeRead(std::size_t idx, ReadCB cb);

  /**
   * @brief Subscribe to write events for File has news
   *
   * @param file The file we want events from
   * @param cb The callback to to invoked
   * @return std::size_t The subscription index,
   *          used if you want to unsubscribe
   */
  std::size_t subscribeWrite(const FileBase& file, WriteCB cb);
  std::size_t subscribeWrite(std::size_t idx, WriteCB cb);

  /**
   * @brief Unsubscribe from read events
   *
   * @param subscribeIdx The subscription index returned from subscribeRead(...)
   * @return true If successful
   * @return false not successful
   */
  bool unsubscribeRead(std::size_t subscribeIdx);

  /**
   * @brief Unsubscribe from write events
   *
   * @param subscribeIdx The subscription index returned from subscribeWrite(...)
   * @return true If successful
   * @return false not successful
   */
  bool unsubscribeWrite(std::size_t subscribeIdx);

  /**
   * @brief Instance wrapper for static method bytesToStr
   *
   * @param bytes The bytes to escape and convert
   * @return std::string The resulting string
   */
  std::string bytesToString(const Bytes& bytes) const;

  /**
   * @brief Escapes \0 and other and converts to std::string
   *
   * Useful When we want to read a response as a string
   *
   * @param bytes The bytes to escape and convert
   * @return std::string The resulting string
   */
  static std::string bytesToStr(const Bytes& bytes);

protected:
  std::vector<std::unique_ptr<FileBase>> _files;
private:
  fd_set _setRd, _setWr;
  std::vector<std::pair<int, ReadCB>> _readSubscribers;
  std::vector<std::pair<int, WriteCB>> _writeSubscribers;

  void onReadEvent(FileBase* file);

  void onWriteEvent(FileBase* file);

  template<typename T>
  std::size_t _subscribe(
    const FileBase& file,
    std::vector<std::pair<int, T>>&vec,
    T cb
  ) {
    vec.emplace_back(
      std::pair<int, T>{file.fileno(), cb});
    return vec.size() - 1;
  }

  template<typename T>
  bool _unsubscribe(
    std::size_t subscribeIdx,
    std::vector<std::pair<int, T>>& vec
  ) {
    if (subscribeIdx >= vec.size())
      return false;

    vec.erase(vec.begin() + subscribeIdx);
    return true;
  }
};

// -------------------------------------------

class ScriptIO : public AsyncIO
{
public:
  ScriptIO(
    const std::string& command,
    const std::vector<std::string>& args,
    const char* toScriptFifo = "./.to.fifo",
    const char* fromScriptFifo = "./.from.fifo"
  );

  ScriptIO(ScriptIO&& rhs);

  ~ScriptIO();

  ScriptIO& operator=(ScriptIO&& rhs);
  ScriptIO& operator=(const ScriptIO& other) = delete;


  /**
   * @brief Run as a event loop continuously,
   *
   * see AsyncIO poll if you need your own eventloop
   * @param timeout_ms How many ms to wait in worse case
   */
  void run(time_t timeout_ms);

  /**
   * @brief Stout received from script
   *
   * @param msg The message received
   * @param file The file that it got received from
   */
  void stdoutFromScript(
    const Bytes& bytes, Readable *file);

  /**
   * @brief Event callback when received from script
   *
   * @param msg The string message received from script
   * @param file The file that sent the message, ie fromFifo
   */
  virtual void fromScript(
    const Bytes& bytes, Readable *file);

 /**
   * @brief Event callback when write buffer is ready to receive
   *
   * @param file The file that can be written to, ie toFifo
   */
  virtual void writeReady(Writable* file);

  /**
   * @brief Gets the fifo used to write to script
   *
   * @return Fifo<Writable>& The writable fifo, ie toFifo
   */
  Fifo<Writable>& toScriptFifo();

  /**
   * @brief Gets the fifo used to read from script
   *
   * @return Fifo<Readable>&  The readable fifo, ie fromFifo
   */
  Fifo<Readable>& fromScriptFifo();

  /**
   * @brief Gets the file used for reading script normal output
   *
   * @return Fifo<Readable>& The file used to read from stdout
   */
  Fifo<Readable>& scriptStdout();
};

} // namespace afifo

#endif // FIFO_ASYNC_H
