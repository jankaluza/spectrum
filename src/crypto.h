/**
 * C++ Crypto++ CBC-AES 256 Wrapper
 *
 * Copyright (C) 2010, Patrick Baus <patrick.baus@arcor.de> and Martin Soemer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */
#ifndef _HI_CRYPTO_H
#define _HI_CRYPTO_H

#include <sstream>

#include "cryptopp/osrng.h"
#include "cryptopp/modes.h"
#include "cryptopp/aes.h"
#include "cryptopp/filters.h"
#include "cryptopp/base64.h"
#include "cryptopp/sha.h"

std::string sha256(std::string text);

class AesClass {
public:
	//Constructor
	AesClass(std::string key, bool useSalt = true);
	
	void encrypt(std::string in);
	int decrypt(std::string in);

	std::string getPlaintext();
	std::string getCiphertext();
	std::string getSalt();
	void setSalt(std::string salt);
	void generateSalt();
private:
	//Define AES key size. For AES256 this would be 32 bytes.
	static const int AES_KEYSIZE = 32;
	std::string plaintext_, ciphertext_, key_, salt_, saltedKey_;

	std::string randomstring(int len);

	int encryptRaw(std::string data);
	int decryptRaw(std::string data);

	std::string base64_encodestring(std::string data);
	std::string base64_decodestring(std::string data);

	//This method needs to be run every time the salt is changed!
	void updateSaltedKey();
};
#endif // _HI_CRYPTO_H
