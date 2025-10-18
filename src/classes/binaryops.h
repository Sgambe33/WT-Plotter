#ifndef BINARYOPS_H
#define BINARYOPS_H

#include <cstdint>
#include <stdexcept>

#include <QByteArray>
#include <QIODevice>
#include <QString>

/**
 * @brief Parse a variable-length size from a QIODevice.
 *
 * This reads the same encoding used by the Go implementation you referenced.
 * The encoding is a small-prefix variable-length format:
 *  - 10xxxxxx -> 1 byte total (value in lower 7 bits)
 *  - 01xxxxxx + 1 byte -> 2 bytes total
 *  - 001xxxxx + 2 bytes -> 3 bytes total
 *  - 0001xxxx + 3 bytes -> 4 bytes total
 *  - 0000xxxx + 4 bytes -> 5 bytes total (full little-endian u32)
 *
 * Notes:
 *  - On success returns true, sets `out` to the parsed size and `eof` == false.
 *  - If the device had no bytes available when trying to read the first size byte,
 *    the function sets `eof = true` and returns false (no `err` required).
 *  - On other failures the function returns false, sets `err` describing the error,
 *    and sets `eof = false`.
 *  - If the decoded payload is negative or doesn't fit into uint32_t an error is returned.
 *
 * @param dev  Pointer to an open QIODevice to read from (must be readable).
 * @param out  Output parameter set to the parsed size on success.
 * @param err  If the function fails (and not EOF) this is set to a human-readable message.
 * @param eof  Set to true when the read failed due to EOF when trying to read the first byte.
 * @return true on success (out valid). false on failure or EOF.
 */
bool readVariableLengthSize(QIODevice *dev, uint32_t &out, QString &err, bool &eof);

/**
 * @brief Read exactly `n` bytes from the device and return them as QByteArray.
 *
 * This function will attempt to read until `n` bytes are acquired. On unexpected EOF
 * it throws std::runtime_error. Use this when you need an exact-length buffer and want
 * exceptions for short reads.
 *
 * @param dev Pointer to an open QIODevice to read from.
 * @param n   Number of bytes to read (non-negative).
 * @throws std::runtime_error on unexpected EOF or read failure.
 * @return QByteArray containing exactly `n` bytes.
 */
QByteArray readExact(QIODevice *dev, qint64 n);

/**
 * @brief Interpret 8 bytes (little-endian) as an IEEE-754 double.
 *
 * This helper reads a little-endian 64-bit integer from `p` and reinterprets the
 * bits as a double (IEEE-754). The pointer `p` must point to at least 8 bytes.
 *
 * @param p Pointer to buffer containing at least 8 bytes in little-endian order.
 * @return double value decoded from the 8 byte little-endian representation.
 */
double readLEDouble(const char *p);

/**
 * @brief Read `length` bytes from `dev` and return a spaced hex string.
 *
 * Reads exactly `length` bytes from the device using QIODevice::read and converts them
 * to a hex string separated by single spaces (e.g. "0a ff 01").
 *
 * If the read returns fewer bytes, behaviour follows QIODevice::read (it may return fewer
 * bytes) — callers should ensure sufficient bytes are available or call readExact().
 *
 * @param dev    Pointer to an open QIODevice.
 * @param length Number of bytes to read and convert to hex.
 * @return QString hex representation, bytes separated by spaces.
 */
QString readToHexStr(QIODevice *dev, int length);

/**
 * @brief Read a one-byte length followed by that many bytes, return as UTF-8 QString.
 *
 * This helper reads a single length byte (uint8), then reads that many bytes and decodes
 * them as UTF-8 into a QString. If the first length byte is not available, returns an
 * empty QString.
 *
 * This matches common small-length-prefixed string encodings used in packet formats.
 *
 * @param dev Pointer to an open QIODevice.
 * @return Decoded QString (may be empty if length was zero or read failed).
 */
QString readLenString(QIODevice *dev);

/**
 * @brief Read all remaining bytes from the device and return a spaced hex string.
 *
 * Convenience helper that calls QIODevice::readAll() and converts the bytes to a hex
 * string with single-space separators.
 *
 * @param dev Pointer to an open QIODevice.
 * @return QString hex representation of all remaining bytes.
 */
QString readToHexStrFull(QIODevice *dev);

#endif // BINARYOPS_H
