#include "ecdsa.h"
#include "ecc.h"

// #include <stdio.h>
#include <assert.h>
#include <string.h>

#include <secp256k1.h>

extern secp256k1_context* secp256k1_context_static;
extern secp256k1_context* secp256k1_context_sign;

uint32_t htobe32(uint32_t x) /* aka bswap_32 */
{
    return (((x & 0xff000000U) >> 24) | ((x & 0x00ff0000U) >>  8) |
            ((x & 0x0000ff00U) <<  8) | ((x & 0x000000ffU) << 24));
}

void WriteLE32(u_char* ptr, uint32_t x)
{
    uint32_t v = htobe32(x);
    memcpy(ptr, (char*)&v, 4);
}

// Check that the sig has a low R value and will be less than 71 bytes
char SigHasLowR(const secp256k1_ecdsa_signature* sig)
{
    secp256k1_context *context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    u_char compact_sig[64];
    secp256k1_ecdsa_signature_serialize_compact(context, compact_sig, sig);

    // In DER serialization, all values are interpreted as big-endian, signed integers. The highest bit in the integer indicates
    // its signed-ness; 0 is positive, 1 is negative. When the value is interpreted as a negative integer, it must be converted
    // to a positive value by prepending a 0x00 byte so that the highest bit is 0. We can avoid this prepending by ensuring that
    // our highest bit is always 0, and thus we must check that the first byte is less than 0x80.
    return compact_sig[0] < 0x80;
}

int sign(u_char* signatureOut, size_t* signatureOutLength,const u_char message[32], const u_char secretKey[32], const u_char grind) {
    const size_t SIGNATURE_SIZE = 72;
    const u_char test_case = 0;
    u_char extra_entropy[32] = {0};
    WriteLE32(extra_entropy, test_case);
    secp256k1_ecdsa_signature sig;
    uint32_t counter = 0;
    int ret = secp256k1_ecdsa_sign(secp256k1_context_sign, &sig, message, secretKey, secp256k1_nonce_function_rfc6979, (!grind && test_case) ? extra_entropy : NULL);
    
    // Grind for low R
    while (ret && !SigHasLowR(&sig) && grind) {
        WriteLE32(extra_entropy, ++counter);
        ret = secp256k1_ecdsa_sign(secp256k1_context_sign,  &sig, message, secretKey, secp256k1_nonce_function_rfc6979, extra_entropy);
    }
    assert(ret);
    size_t sigLen = SIGNATURE_SIZE;
    u_char *signature = malloc(sigLen);
    ret = secp256k1_ecdsa_signature_serialize_der(secp256k1_context_sign, signature, &sigLen, &sig);
    assert(ret);
    // Additional verification step to prevent using a potentially corrupted signature
    secp256k1_pubkey pk;
    ret = secp256k1_ec_pubkey_create(secp256k1_context_sign, &pk, secretKey);
    assert(ret);
    // secp256k1_context_no_precomp should be secp256k1_context_static
    ret = secp256k1_ecdsa_verify(secp256k1_context_no_precomp, &sig, message, &pk);
    assert(ret);
    memcpy(signatureOut, signature, sigLen);
    *signatureOutLength = sigLen;
    return 1;
}

/// <#Description#>
/// - Parameters:
///   - signature: <#signature description#>
///   - signatureLen: <#signatureLen description#>
const int verifySignature(const u_char *signature, const size_t signatureLen, const u_char message[32], const u_char secretKey[32]) {
    secp256k1_context *secp256k1_context_verify = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);

    secp256k1_ecdsa_signature sig;
    int ret = secp256k1_ecdsa_signature_parse_der(secp256k1_context_static, &sig, signature, signatureLen);
    assert(ret);
    
    secp256k1_pubkey pk;
    ret = secp256k1_ec_pubkey_create(secp256k1_context_sign, &pk, secretKey);
    assert(ret);
    // secp256k1_context_no_precomp should be secp256k1_context_static
    ret = secp256k1_ecdsa_verify(secp256k1_context_verify, &sig, message, &pk);
    return ret;
}