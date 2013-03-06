#include "quaziodevice.h"

#define QUAZIO_INBUFSIZE 4096
#define QUAZIO_OUTBUFSIZE 4096

/// \cond internal
class QuaZIODevicePrivate {
    friend class QuaZIODevice;
    QuaZIODevicePrivate(QIODevice *io);
    ~QuaZIODevicePrivate();
    QIODevice *io;
    z_stream zins;
    z_stream zouts;
    char *inBuf;
    int inBufPos;
    int inBufSize;
    char *outBuf;
    int outBufPos;
    int outBufSize;
    bool zBufError;
    int doFlush(QString &error);
};

QuaZIODevicePrivate::QuaZIODevicePrivate(QIODevice *io):
  io(io),
  inBuf(NULL),
  inBufPos(0),
  inBufSize(0),
  outBuf(NULL),
  outBufPos(0),
  outBufSize(0),
  zBufError(false)
{
  zins.zalloc = (alloc_func) NULL;
  zins.zfree = (free_func) NULL;
  zins.opaque = NULL;
  zouts.zalloc = (alloc_func) NULL;
  zouts.zfree = (free_func) NULL;
  zouts.opaque = NULL;
  inBuf = new char[QUAZIO_INBUFSIZE];
  outBuf = new char[QUAZIO_OUTBUFSIZE];
#ifdef QUAZIP_ZIODEVICE_DEBUG_OUTPUT
  debug.setFileName("debug.out");
  debug.open(QIODevice::WriteOnly);
#endif
#ifdef QUAZIP_ZIODEVICE_DEBUG_INPUT
  indebug.setFileName("debug.in");
  indebug.open(QIODevice::WriteOnly);
#endif
}

QuaZIODevicePrivate::~QuaZIODevicePrivate()
{
#ifdef QUAZIP_ZIODEVICE_DEBUG_OUTPUT
  debug.close();
#endif
#ifdef QUAZIP_ZIODEVICE_DEBUG_INPUT
  indebug.close();
#endif
  if (inBuf != NULL)
    delete[] inBuf;
  if (outBuf != NULL)
    delete[] outBuf;
}

int QuaZIODevicePrivate::doFlush(QString &error)
{
  int flushed = 0;
  while (outBufPos < outBufSize) {
    int more = io->write(outBuf + outBufPos, outBufSize - outBufPos);
    if (more == -1) {
      error = io->errorString();
      return -1;
    }
    if (more == 0)
      break;
    outBufPos += more;
    flushed += more;
  }
  if (outBufPos == outBufSize) {
    outBufPos = outBufSize = 0;
  }
  return flushed;
}

/// \endcond

// #define QUAZIP_ZIODEVICE_DEBUG_OUTPUT
// #define QUAZIP_ZIODEVICE_DEBUG_INPUT
#ifdef QUAZIP_ZIODEVICE_DEBUG_OUTPUT
#include <QFile>
static QFile debug;
#endif
#ifdef QUAZIP_ZIODEVICE_DEBUG_INPUT
#include <QFile>
static QFile indebug;
#endif

QuaZIODevice::QuaZIODevice(QIODevice *io, QObject *parent):
    QIODevice(parent),
    d(new QuaZIODevicePrivate(io))
{
  connect(io, SIGNAL(readyRead()), SIGNAL(readyRead()));
}

QuaZIODevice::~QuaZIODevice()
{
    if (isOpen())
        close();
    delete d;
}

QIODevice *QuaZIODevice::getIoDevice() const
{
    return d->io;
}

bool QuaZIODevice::open(QIODevice::OpenMode mode)
{
    if ((mode & QIODevice::Append) != 0) {
        setErrorString(trUtf8("QIODevice::Append is not supported for"
                    " QuaZIODevice"));
        return false;
    }
    if ((mode & QIODevice::ReadWrite) == QIODevice::ReadWrite) {
        setErrorString(trUtf8("QIODevice::ReadWrite is not supported for"
                    " QuaZIODevice"));
        return false;
    }
    if ((mode & QIODevice::ReadOnly) != 0) {
        if (inflateInit(&d->zins) != Z_OK) {
            setErrorString(d->zins.msg);
            return false;
        }
    }
    if ((mode & QIODevice::WriteOnly) != 0) {
        if (deflateInit(&d->zouts, Z_DEFAULT_COMPRESSION) != Z_OK) {
            setErrorString(d->zouts.msg);
            return false;
        }
    }
    return QIODevice::open(mode);
}

void QuaZIODevice::close()
{
    if ((openMode() & QIODevice::ReadOnly) != 0) {
        if (inflateEnd(&d->zins) != Z_OK) {
            setErrorString(d->zins.msg);
        }
    }
    if ((openMode() & QIODevice::WriteOnly) != 0) {
        flush();
        if (deflateEnd(&d->zouts) != Z_OK) {
            setErrorString(d->zouts.msg);
        }
    }
    QIODevice::close();
}

qint64 QuaZIODevice::readData(char *data, qint64 maxSize)
{
  int read = 0;
  while (read < maxSize) {
    if (d->inBufPos == d->inBufSize) {
      d->inBufPos = 0;
      d->inBufSize = d->io->read(d->inBuf, QUAZIO_INBUFSIZE);
      if (d->inBufSize == -1) {
        d->inBufSize = 0;
        setErrorString(d->io->errorString());
        return -1;
      }
      if (d->inBufSize == 0)
        break;
    }
    while (read < maxSize && d->inBufPos < d->inBufSize) {
      d->zins.next_in = (Bytef *) (d->inBuf + d->inBufPos);
      d->zins.avail_in = d->inBufSize - d->inBufPos;
      d->zins.next_out = (Bytef *) (data + read);
      d->zins.avail_out = (uInt) (maxSize - read); // hope it's less than 2GB
      int more = 0;
      switch (inflate(&d->zins, Z_SYNC_FLUSH)) {
      case Z_OK:
        read = (char *) d->zins.next_out - data;
        d->inBufPos = (char *) d->zins.next_in - d->inBuf;
        break;
      case Z_STREAM_END:
        read = (char *) d->zins.next_out - data;
        d->inBufPos = (char *) d->zins.next_in - d->inBuf;
        return read;
      case Z_BUF_ERROR: // this should never happen, but just in case
        if (!d->zBufError) {
          qWarning("Z_BUF_ERROR detected with %d/%d in/out, weird",
              d->zins.avail_in, d->zins.avail_out);
          d->zBufError = true;
        }
        memmove(d->inBuf, d->inBuf + d->inBufPos, d->inBufSize - d->inBufPos);
        d->inBufSize -= d->inBufPos;
        d->inBufPos = 0;
        more = d->io->read(d->inBuf + d->inBufSize, QUAZIO_INBUFSIZE - d->inBufSize);
        if (more == -1) {
          setErrorString(d->io->errorString());
          return -1;
        }
        if (more == 0)
          return read;
        d->inBufSize += more;
        break;
      default:
        setErrorString(QString::fromLocal8Bit(d->zins.msg));
        return -1;
      }
    }
  }
#ifdef QUAZIP_ZIODEVICE_DEBUG_INPUT
  indebug.write(data, read);
#endif
  return read;
}

qint64 QuaZIODevice::writeData(const char *data, qint64 maxSize)
{
  int written = 0;
  QString error;
  if (d->doFlush(error) == -1) {
    setErrorString(error);
    return -1;
  }
  while (written < maxSize) {
      // there is some data waiting in the output buffer
    if (d->outBufPos < d->outBufSize)
      return written;
    d->zouts.next_in = (Bytef *) (data + written);
    d->zouts.avail_in = (uInt) (maxSize - written); // hope it's less than 2GB
    d->zouts.next_out = (Bytef *) d->outBuf;
    d->zouts.avail_out = QUAZIO_OUTBUFSIZE;
    switch (deflate(&d->zouts, Z_NO_FLUSH)) {
    case Z_OK:
      written = (char *) d->zouts.next_in - data;
      d->outBufSize = (char *) d->zouts.next_out - d->outBuf;
      break;
    default:
      setErrorString(QString::fromLocal8Bit(d->zouts.msg));
      return -1;
    }
    if (d->doFlush(error) == -1) {
      setErrorString(error);
      return -1;
    }
  }
#ifdef QUAZIP_ZIODEVICE_DEBUG_OUTPUT
  debug.write(data, written);
#endif
  return written;
}

bool QuaZIODevice::flush()
{
    QString error;
    if (d->doFlush(error) < 0) {
        setErrorString(error);
        return false;
    }
    // can't flush buffer, some data is still waiting
    if (d->outBufPos < d->outBufSize)
        return true;
    Bytef c = 0;
    d->zouts.next_in = &c; // fake input buffer
    d->zouts.avail_in = 0; // of zero size
    do {
        d->zouts.next_out = (Bytef *) d->outBuf;
        d->zouts.avail_out = QUAZIO_OUTBUFSIZE;
        switch (deflate(&d->zouts, Z_SYNC_FLUSH)) {
        case Z_OK:
          d->outBufSize = (char *) d->zouts.next_out - d->outBuf;
          if (d->doFlush(error) < 0) {
              setErrorString(error);
              return false;
          }
          if (d->outBufPos < d->outBufSize)
              return true;
          break;
        case Z_BUF_ERROR: // nothing to write?
          return true;
        default:
          setErrorString(QString::fromLocal8Bit(d->zouts.msg));
          return false;
        }
    } while (d->zouts.avail_out == 0);
    return true;
}

bool QuaZIODevice::isSequential() const
{
  return true;
}
