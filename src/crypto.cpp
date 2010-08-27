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
#include "crypto.h"

std::string sha256(std::string text) {
	std::string returnString;
	CryptoPP::SHA256 sha;
	CryptoPP::StringSink *sink = new CryptoPP::StringSink(returnString);
	CryptoPP::HashFilter *hasher = new CryptoPP::HashFilter(sha, sink);
	CryptoPP::StringSource source(text, true, hasher);

	return returnString;
}

/**
* This function return a random base64 encoded string of lenth len.
* See http://www.cryptopp.com/wiki/AutoSeededRandomPool
* for more information on OS support.
* \param len The required length
* \return A random base 64 encoded string of length len
*/
std::string AesClass::randomstring(int len) {
	CryptoPP::AutoSeededRandomPool rng;
	unsigned char randomBytes[len];
	rng.GenerateBlock(randomBytes,len);
	std::stringstream str;
	str << randomBytes;
	return base64_encodestring(str.str()).substr(0, len);
}

/**
* Encrypts the data. An iv is automatically
* created and prepended to the cipherstring. The output
* is raw binary and should be base64 encoded to be 
* human readable. Encryption mode is CBC-AES256.
* \param data The data to encrypt
* \return The encrypted binary data
*/
int AesClass::encryptRaw(std::string data) {
	// Generate a random initialization vector iv
	unsigned char iv[CryptoPP::AES::BLOCKSIZE];
	CryptoPP::AutoSeededRandomPool rng;
	rng.GenerateBlock( iv, sizeof(iv) );

	// Convert
	const unsigned char *saltedKey = reinterpret_cast<const unsigned char*>(saltedKey_.c_str());
	const unsigned char *bytePlain = reinterpret_cast<const unsigned char*>(data.c_str());

	// Set the encryption mode: CBC-AES256.

	CryptoPP::AES::Encryption aesEncryption(saltedKey, AES_KEYSIZE);
	CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv);
	
	// Encrypt
	std::string cipherString;
	CryptoPP::StreamTransformationFilter cbcEncryptor(cbcEncryption, new CryptoPP::StringSink(cipherString));
	cbcEncryptor.Put(bytePlain, data.size());
	cbcEncryptor.MessageEnd();

	// Add the iv to the beginning of the ciphertext, as in cbc mode the first block is always the iv
	ciphertext_ = std::string((char*)iv, CryptoPP::AES::BLOCKSIZE) + cipherString;

	return 0;
}

/**
* Decrypt the data. The iv is read from the first block
* The input must be unencoded raw binary. Encryption
* mode is CBC-AES256. In case of an error the errorcode
* is return and the error written to the plaintext string.
* \param data The data to encrypt
* \return errorcode. 0 for success. > 0 for error.
* \exception Throws an exception if the input length is not a
* multiple of the AES block length.
* In case of an error. The error is written to the plaintext
* string.
*/
int AesClass::decryptRaw(std::string data) {
	// Check cipher for valid length. A valid cipher must be a multiple of blocksize and
	// must contain at least two blocks (iv + data).
	// TODO: throw an exception. Error handling through plaintext string is horrible.
	if ((data.length() % CryptoPP::AES::BLOCKSIZE) != 0 || !(data.length() > CryptoPP::AES::BLOCKSIZE))
	{
		throw "Invalid cipher length. Please check database for consistency.";
	}

	// Get the initialization vector from first block. Then chop it off.
	unsigned char iv[CryptoPP::AES::BLOCKSIZE];
	strncpy((char*)iv, data.c_str(), CryptoPP::AES::BLOCKSIZE);
	data = data.substr(CryptoPP::AES::BLOCKSIZE);
	
	const unsigned char *saltedKey = reinterpret_cast<const unsigned char*>(saltedKey_.c_str());
	const unsigned char *byteIn = reinterpret_cast<const unsigned char*>(data.c_str());

	// Set the decryption mode: CBC-AES256
	CryptoPP::AES::Decryption aesDecryption(saltedKey, AES_KEYSIZE);
	CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv);

	// Decrypt
	CryptoPP::StreamTransformationFilter cbcDecryptor(cbcDecryption, new CryptoPP::StringSink(plaintext_));
	cbcDecryptor.Put(byteIn, data.size());
	cbcDecryptor.MessageEnd();

	return 0;
}

/**
* Returns the base 64 encoded string of the input
* \param data The data to encode
* \return The encoded string
*/
std::string AesClass::base64_encodestring(std::string data) {
	std::string returnString;
	CryptoPP::StringSink *sink = new CryptoPP::StringSink(returnString);
	CryptoPP::Base64Encoder *base64_enc = new CryptoPP::Base64Encoder(sink, false);
	CryptoPP::StringSource source(data, true, base64_enc);

	return returnString;
}

/**
* Returns the base 64 decoded string of the input
* \param data The data to decode
* \return The decoded string
*/
std::string AesClass::base64_decodestring(std::string data) {
	std::string returnString;
	CryptoPP::StringSink *sink = new CryptoPP::StringSink(returnString);
	CryptoPP::Base64Decoder *base64_dec = new CryptoPP::Base64Decoder(sink);
	CryptoPP::StringSource source(data, true, base64_dec);

	return returnString;
}

/**
* This method needs to be called every time the
* salt changes to update the encryption key or
* else the old key will still be used.
*/
void AesClass::updateSaltedKey() {
	saltedKey_ = base64_encodestring(sha256(key_ + salt_));
}

std::string AesClass::getSalt() {
	return salt_;
}

void AesClass::setSalt(std::string salt) {
	salt_ = salt;
	updateSaltedKey();
}

/**
* Constructor. If the object is reused to encrypt
* several passwords using the same encryption key,
* a call to gernateSalt() should be made for each
* password if using salts.
* \param key the encryption key to be used.
* \useSalt By default a salt is generated to salt
* weak keys and prevent rainbow table attacks.
*/
AesClass::AesClass(std::string key, bool useSalt)
{
	key_ = key;
	if (useSalt)
		generateSalt();
}
/**
* Encrypts a string using CBC-AES 256 and then
* returns a human readable base 64 encoded string.
* \param in The data to encrypt
* \param key The key to encrypt with
* \return The base 64 encoded encrypted data
*/
void AesClass::encrypt(std::string in) {
	encryptRaw(in);
	ciphertext_ = base64_encodestring(ciphertext_);
}

/*
* Decrypts a string using CBC-AES 256.
* \param in The base 64 encoded data to decrypt
* \param key The key to decrypt with
* \return The decrypted data
* \exception Passes an exception thrown in
* decryptRaw unhandled.
*/
int AesClass::decrypt(std::string in) {
	try{
		return decryptRaw(base64_decodestring(in));
	}
	catch (const char* mess) {
		throw;
	};
}

std::string AesClass::getPlaintext() {
	return plaintext_;
}

std::string AesClass::getCiphertext() {
	return ciphertext_;
}

/*
* Generates a new random salt. This method
* should be called when reusing the object for
* several passwords.
*/
void AesClass::generateSalt() {
	setSalt(randomstring(6));
}
