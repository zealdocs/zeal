#ifndef QUAZIP_QUAZIODEVICE_H
#define QUAZIP_QUAZIODEVICE_H

#include <QIODevice>
#include "quazip_global.h"

#include <zlib.h>

class QuaZIODevicePrivate;

/// A class to compress/decompress QIODevice.
/**
  This class can be used to compress any data written to QIODevice or
  decompress it back. Compressing data sent over a QTcpSocket is a good
  example.
  */
class QUAZIP_EXPORT QuaZIODevice: public QIODevice {
  Q_OBJECT
public:
  /// Constructor.
  /**
    \param io The QIODevice to read/write.
    \param parent The parent object, as per QObject logic.
    */
  QuaZIODevice(QIODevice *io, QObject *parent = NULL);
  /// Destructor.
  ~QuaZIODevice();
  /// Flushes data waiting to be written.
  /**
    Unfortunately, as QIODevice doesn't support flush() by itself, the
    only thing this method does is write the compressed data into the
    device using Z_SYNC_FLUSH mode. If you need the compressed data to
    actually be flushed from the buffer of the underlying QIODevice, you
    need to call its flush() method as well, providing it supports it
    (like QTcpSocket does). Example:
    \code
    QuaZIODevice dev(&sock);
    dev.open(QIODevice::Write);
    dev.write(yourDataGoesHere);
    dev.flush();
    sock->flush(); // this actually sends data to network
    \endcode

    This may change in the future versions of QuaZIP by implementing an
    ugly hack: trying to cast the QIODevice using qobject_cast to known
    flush()-supporting subclasses, and calling flush if the resulting
    pointer is not zero.
    */
  virtual bool flush();
  /// Opens the device.
  /**
    \param mode Neither QIODevice::ReadWrite nor QIODevice::Append are
    not supported.
    */
  virtual bool open(QIODevice::OpenMode mode);
  /// Closes this device, but not the underlying one.
  /**
    The underlying QIODevice is not closed in case you want to write
    something else to it.
    */
  virtual void close();
  /// Returns the underlying device.
  QIODevice *getIoDevice() const;
  /// Returns true.
  virtual bool isSequential() const;
protected:
  /// Implementation of QIODevice::readData().
  virtual qint64 readData(char *data, qint64 maxSize);
  /// Implementation of QIODevice::writeData().
  virtual qint64 writeData(const char *data, qint64 maxSize);
private:
  QuaZIODevicePrivate *d;
};
#endif // QUAZIP_QUAZIODEVICE_H
