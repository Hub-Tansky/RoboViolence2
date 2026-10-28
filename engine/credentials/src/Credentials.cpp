#include "Credentials.h"

#include <sodium.h>

#include <cstdlib>
#include <cstring>

namespace credentials
{
	namespace
	{
		// libsodium wants sodium_init() before any other call
		void ready()
		{
			static const bool ok = sodium_init() >= 0;
			if (!ok) abort(); // no random source
		}

		void start(crypto_auth_hmacsha256_state & state, const char * key)
		{
			ready();
			crypto_auth_hmacsha256_init(&state, reinterpret_cast<const unsigned char *>(key), strlen(key));
		}
	}

	void hmac(const char * key, const void * data, size_t size, uint8_t out[MAC_SIZE])
	{
		crypto_auth_hmacsha256_state state;
		start(state, key);
		crypto_auth_hmacsha256_update(&state, static_cast<const unsigned char *>(data), size);
		crypto_auth_hmacsha256_final(&state, out);
	}

	void nonce(uint8_t out[NONCE_SIZE])
	{
		ready();
		randombytes_buf(out, NONCE_SIZE);
	}

	void prove(const char * password, const char * label, const uint8_t nonce[NONCE_SIZE], uint8_t out[MAC_SIZE])
	{
		crypto_auth_hmacsha256_state state;
		start(state, password);
		// The label's '\0' separates it from the nonce
		crypto_auth_hmacsha256_update(&state, reinterpret_cast<const unsigned char *>(label), strlen(label) + 1);
		crypto_auth_hmacsha256_update(&state, nonce, NONCE_SIZE);
		crypto_auth_hmacsha256_final(&state, out);
	}

	bool check(const char * password, const char * label, const uint8_t nonce[NONCE_SIZE], const uint8_t mac[MAC_SIZE])
	{
		uint8_t expected[MAC_SIZE];
		prove(password, label, nonce, expected);
		return equal(expected, mac);
	}

	bool equal(const uint8_t a[MAC_SIZE], const uint8_t b[MAC_SIZE])
	{
		ready();
		return sodium_memcmp(a, b, MAC_SIZE) == 0;
	}
}
