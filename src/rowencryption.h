#pragma once

#include <QByteArray>
#include <QString>

/**
 * AES-256-GCM symmetric encryption for JSONROWDATA payloads.
 *
 * Wire format stored on the server (JSONROWDATA becomes a JSON string):
 *   "enc:" + Base64Url( 12-byte IV || 16-byte GCM tag || ciphertext )
 *
 * When no encryption key is set, JSONROWDATA is stored as a plain JSON object
 * (the existing behaviour).  The "enc:" prefix distinguishes encrypted rows
 * from plain-text rows so both formats can coexist during key migration.
 */
namespace RowEncryption {

/**
 * Encrypt plaintext bytes with AES-256-GCM.
 * @param plaintext  Raw bytes to encrypt (typically compact JSON).
 * @param key32      Exactly 32 bytes of key material.
 * @return Wire-format string "enc:<base64url>" or an empty string on error.
 */
QString encrypt(const QByteArray &plaintext, const QByteArray &key32);

/**
 * Decrypt a wire-format string produced by encrypt().
 * @param encoded    The full "enc:<base64url>" value.
 * @param key32      The same 32-byte key used for encryption.
 * @return Decrypted plaintext or an empty QByteArray on auth failure / bad input.
 */
QByteArray decrypt(const QString &encoded, const QByteArray &key32);

} // namespace RowEncryption
