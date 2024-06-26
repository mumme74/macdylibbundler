import sys, os, time, errno, stat, select

class Msg:
  def __init__(self, msg = ""):
    self.msg = msg
  def getIncoming(self, fifoCls) -> str:
    hdr = os.read(fifoCls.fifo0, 4)
    length = 0
    for i in range(3, -1, -1): # big endian
      length |= hdr[3-i] << (8*i)
    self.msg = fifoCls._read(length).decode("utf-8")
    return self.msg

  def sendPacket(self, fifoCls) -> None:
    length = len(self.msg)
    bts = length.to_bytes(4, 'big')
    bts += self.msg.encode("utf-8")
    fifoCls._write(bts)


class Fifo:
  fifo_read = ""
  fifo_wr = ""
  def __init__(self, fifo_read = "", fifo_wr = ""):
    if not fifo_read:
      self.fifo_read = sys.argv[1] if len(sys.argv) > 1 else ".to.fifo"
    else:
      self.fifo_read = fifo_read

    if not fifo_wr:
      self.fifo_wr = sys.argv[2] if len(sys.argv) > 2 else ".from.fifo"
    else:
      self.fifo_wr = fifo_wr

    if not stat.S_ISFIFO(os.stat(self.fifo_read).st_mode) or \
       not stat.S_ISFIFO(os.stat(self.fifo_wr).st_mode):
      raise Exception(errno.ENOTRECOVERABLE,
        f"{self.fifo_read} not a fifo.")

    self.fifo0 = self.open(self.fifo_read, os.O_RDONLY | os.O_NONBLOCK)
    self.fifo1 = self.open(self.fifo_wr, os.O_WRONLY | os.O_NONBLOCK)

    self.poll = select.poll()
    self.poll.register(self.fifo0, select.POLLIN)

  def __del__(self):
    if hasattr(self, 'poll'):
      self.poll.unregister(self.fifo0)
    if hasattr(self, 'fifo0'):
      os.close(self.fifo0)
    if hasattr(self, 'fifo1'):
      os.close(self.fifo1)


  def open(self, file, mode):
    retry = 500
    while True:
      try:
        dev = os.open(file, mode)
        if retry < 500:
          print(F"is now connected to fifo {file}")
        return dev
      except OSError as e:
        if (e.errno == errno.ENXIO):
          print(F"connecting to fifo {file}, other end must connect")
          retry -= 1
          time.sleep(0.2)
        else:
          raise e

  def _read(self, numBytes):
    retry = 500
    while True:
      try:
        ret = os.read(self.fifo0, numBytes)
        if ret == b'':
          print("retry ", retry)
          raise OSError(errno.EWOULDBLOCK, "Try again")
        return ret

      except OSError as e:
        if e.errno == errno.EWOULDBLOCK and retry > 0:
          time.sleep(0.1)
          retry -= 1
        else:
          raise e
    return ""

  def _write(self, bts):
    sent = 0
    retry = 500
    while True:
      try:
        sent += os.write(self.fifo1, bts[sent:])
        if sent == len(bts):
          break

      except OSError as e:
        if e.errno == errno.EWOULDBLOCK and retry > 0:
          time.sleep(0.1)
          retry -= 1
        else:
          raise e

  def write(self, s):
    msg = Msg(s)
    msg.sendPacket(self)


  def read(self) -> str:
    while True:
      # Check if there's data to read. Timeout after 20 sec.
      pres = self.poll.poll(20000)
      print(pres)
      if len(pres)>0 and self.fifo0 == pres[0][0]:
        if select.POLLIN & pres[0][1]:
          msg = Msg()
          return msg.getIncoming(self)
        if select.POLLHUP:
          print(f"{self.fifo_read} has no writer")
          return ""
        # No data, do something else
        raise Exception(errno.EFAULT, "No response")
