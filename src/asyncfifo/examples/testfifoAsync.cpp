#include "asyncfifo.hpp"
#include <iostream>

using namespace afifo;

class Msg {
  Bytes _bytes;
  std::size_t _size;
public:
  Msg(const Bytes& bytes):
    _bytes{bytes}, _size{0}
  {
    // big endian read header
    for (size_t i = 0; i < 4; ++i)
      _size |= bytes[i] << (8* (3-i));
  }

  Msg(const std::string& str):
    _bytes{}, _size{str.size() + 4}
  {
      std::cout << "sending " << str << '\n';
    _bytes.reserve(str.size()+5);
    // store size header big endian
    for (size_t i = 0; i < 4; i++)
      _bytes.emplace_back(_size & (0xff << (8*(3-i))));

    for (const auto& c : str)
      _bytes.emplace_back(c);
  }

  std::string str() const
  {
    std::string str;
    str.reserve(_size);
    for (const auto& b : _bytes)
      str += b;
    return str;
  }

  const Bytes& bytes() const
  {
    return _bytes;
  }

  bool isComplete() const
  {
    return _bytes.size() == _size +4;
  }

  void putBytes(const Bytes& bytes)
  {
    for (const auto& b : bytes)
      _bytes.emplace_back(b);
  }
};

class Script : public ScriptIO {
    std::unique_ptr<Msg> _rcvMsg;
public:
  Script(
    const std::string& scriptPath,
    const std::vector<std::string> args,
    const char* toFifo = "./.to.fifo",
    const char* fromFifo = "./.from.fifo"
  ):
    ScriptIO{scriptPath, args, toFifo, fromFifo},
    _rcvMsg{}
  {}

  void fromScript(const Bytes& bytes, Readable* file)
  {
    (void)file;
    std::cout << "got " << bytesToString(bytes)  << " from script\n";
    if (!_rcvMsg || _rcvMsg->isComplete())
      _rcvMsg.reset(new Msg(bytes));

    if (!_rcvMsg->isComplete())
      return;

    const auto str = _rcvMsg->str();
    if (str.back() == '1') {
      toScriptFifo().write(Msg("Here you go1").bytes());
    } else if (str.back() == '2') {
      toScriptFifo().write(Msg("Here you go2").bytes());
    } else if (str.back() == '3') {
      toScriptFifo().write(Msg("Here you go3").bytes());
    } else {
      toScriptFifo().write(Msg("Don't know what you want?").bytes());
    }
  }

  /*
  void stdOutFromScript(const Bytes& output, Readable* file)
  {
    (void)file;
    std::cout << ">>" << bytesToString(output);
  }*/

  /*
  void writeReady(Writable* file)
  {
    (void)file;
    //std::cout << "can write to script\n";
  }
  */
};


int main()
{
  std::vector<std::string> args {
    "./.to.fifo", "./.from.fifo"};
  Script app{"./test.py", args};
  app.run(20);
}