// Copyright (C) 2026 Paul McKinney
#include "rowencryption.h"

#include <QRandomGenerator>

#include <openssl/evp.h>

#include <cstring>
#include <memory>

namespace RowEncryption {

static constexpr int kIvLen  = 12;  // GCM recommended nonce length (96-bit)
static constexpr int kTagLen = 16;  // AES-GCM authentication tag (128-bit)

// ── Encrypt ─────────────────────────────────────────────────────────────────

QString encrypt(const QByteArray &plaintext, const QByteArray &key32)
{
    if (key32.size() != 32)
        return {};

    // Generate a cryptographically random 12-byte IV (3 x quint32 words).
    static_assert(kIvLen == 12, "IV must be 12 bytes");
    quint32 ivWords[3];
    QRandomGenerator::securelySeeded().fillRange(ivWords);

    QByteArray iv(kIvLen, Qt::Uninitialized);
    std::memcpy(iv.data(), ivWords, kIvLen);

    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>
        ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!ctx)
        return {};

    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(),
                           nullptr, nullptr, nullptr) != 1)
        return {};

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN,
                            kIvLen, nullptr) != 1)
        return {};

    if (EVP_EncryptInit_ex(
            ctx.get(), nullptr, nullptr,
            reinterpret_cast<const unsigned char *>(key32.constData()),
            reinterpret_cast<const unsigned char *>(iv.constData())) != 1)
        return {};

    QByteArray ciphertext(plaintext.size() + EVP_MAX_BLOCK_LENGTH, Qt::Uninitialized);
    int outLen = 0;

    if (EVP_EncryptUpdate(
            ctx.get(),
            reinterpret_cast<unsigned char *>(ciphertext.data()), &outLen,
            reinterpret_cast<const unsigned char *>(plaintext.constData()),
            plaintext.size()) != 1)
        return {};

    int totalLen = outLen;

    if (EVP_EncryptFinal_ex(
            ctx.get(),
            reinterpret_cast<unsigned char *>(ciphertext.data()) + outLen,
            &outLen) != 1)
        return {};

    totalLen += outLen;
    ciphertext.resize(totalLen);

    // Retrieve GCM authentication tag
    QByteArray tag(kTagLen, Qt::Uninitialized);
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG,
                            kTagLen, tag.data()) != 1)
        return {};

    // Wire format: IV || TAG || CIPHERTEXT → base64url
    QByteArray wire;
    wire.reserve(kIvLen + kTagLen + ciphertext.size());
    wire.append(iv);
    wire.append(tag);
    wire.append(ciphertext);

    return QStringLiteral("enc:") +
           QString::fromLatin1(wire.toBase64(
               QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

// ── Decrypt ─────────────────────────────────────────────────────────────────

QByteArray decrypt(const QString &encoded, const QByteArray &key32)
{
    if (key32.size() != 32)
        return {};
    if (!encoded.startsWith(QStringLiteral("enc:")))
        return {};

    const QByteArray wire = QByteArray::fromBase64(
        encoded.mid(4).toLatin1(),
        QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);

    if (wire.size() < kIvLen + kTagLen)
        return {};

    const QByteArray iv         = wire.left(kIvLen);
    QByteArray       tag        = wire.mid(kIvLen, kTagLen);   // non-const: EVP_CTRL needs void*
    const QByteArray ciphertext = wire.mid(kIvLen + kTagLen);

    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>
        ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!ctx)
        return {};

    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(),
                           nullptr, nullptr, nullptr) != 1)
        return {};

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN,
                            kIvLen, nullptr) != 1)
        return {};

    if (EVP_DecryptInit_ex(
            ctx.get(), nullptr, nullptr,
            reinterpret_cast<const unsigned char *>(key32.constData()),
            reinterpret_cast<const unsigned char *>(iv.constData())) != 1)
        return {};

    QByteArray plaintext(ciphertext.size() + EVP_MAX_BLOCK_LENGTH, Qt::Uninitialized);
    int outLen = 0;

    if (EVP_DecryptUpdate(
            ctx.get(),
            reinterpret_cast<unsigned char *>(plaintext.data()), &outLen,
            reinterpret_cast<const unsigned char *>(ciphertext.constData()),
            ciphertext.size()) != 1)
        return {};

    int totalLen = outLen;

    // Set the expected tag before Final (verifies authenticity)
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG,
                            kTagLen, tag.data()) != 1)
        return {};

    // Returns 1 only if the tag verifies — authentication failure returns 0
    if (EVP_DecryptFinal_ex(
            ctx.get(),
            reinterpret_cast<unsigned char *>(plaintext.data()) + outLen,
            &outLen) != 1)
        return {};

    totalLen += outLen;
    plaintext.resize(totalLen);
    return plaintext;
}

} // namespace RowEncryption
