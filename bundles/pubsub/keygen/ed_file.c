/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/**
 * ed_file.c
 *
 *  \date       Dec 2, 2016
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <czmq.h>
#include <openssl/evp.h>

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

#define MAX_KEY_FILE_LENGTH 256
#define MAX_LINE_LENGTH 64
#define AES_KEY_LENGTH 32
#define AES_IV_LENGTH 16

#define KEY_TO_GET "aes_key"
#define IV_TO_GET "aes_iv"

int generate_sha256_hash(char* text, unsigned char* digest);
int encrypt_aes(unsigned char *plaintext, int plaintext_len, unsigned char *key, unsigned char *iv, unsigned char *ciphertext);
int decrypt_aes(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *iv, unsigned char *plaintext);

static char* read_keys_file_content(const char *filePath);
static void parse_key_lines(char *keysBuffer, char **key, char **iv);
static void parse_key_line(char *line, char **key, char **iv);

int main(int argc, const char* argv[])
{
    if (argc < 4) {
        printf("Usage: %s <key_file> <input_file> <output_file> [options]\n", argv[0]);
        printf("Default behavior: encrypting a file\n");
        printf("Options:\n");
        printf("\t-d\tSpecify to decrypt a file\n");
        printf("\n");
        return EXIT_FAILURE;
    }

    int rc = 0;

    const char* keys_file_path = argv[1];
    const char* input_file_path = argv[2];
    const char* output_file_path = argv[3];

    bool decryptParam = false;
    if (argc > 4 && strcmp(argv[4], "-d") == 0) {
        decryptParam = true;
    }

    if (!zsys_file_exists(keys_file_path)) {
        printf("Keys file '%s' doesn't exist!\n", keys_file_path);
        return EXIT_FAILURE;
    }

    if (!zsys_file_exists(input_file_path)) {
        printf("Input file does not exist!\n");
        return EXIT_FAILURE;
    }

    char* keys_data = read_keys_file_content(keys_file_path);
    if (keys_data == NULL) {
        return EXIT_FAILURE;
    }

    char* key = NULL;
    char* iv = NULL;
    parse_key_lines(keys_data, &key, &iv);
    free(keys_data);

    if (key == NULL || iv == NULL) {
        printf("Loading AES key and/or AES iv failed!\n");
        free(key);
        free(iv);
        return EXIT_FAILURE;
    }

    printf("Using AES Key \t\t'%s'\n", key);
    printf("Using AES IV \t\t'%s'\n", iv);
    printf("Input file path \t'%s'\n", input_file_path);
    printf("Output file path \t'%s'\n", output_file_path);
    printf("Decrypting \t\t'%s'\n\n", (decryptParam) ? "true" : "false");

    unsigned char key_digest[EVP_MAX_MD_SIZE];
    unsigned char iv_digest[EVP_MAX_MD_SIZE];
    generate_sha256_hash((char*) key, key_digest);
    generate_sha256_hash((char*) iv, iv_digest);

    zchunk_t* input_chunk = zchunk_slurp (input_file_path, 0);
    if (input_chunk == NULL) {
        printf("Input file not correct!\n");
        free(key);
        free(iv);
        return EXIT_FAILURE;
    }

    //Load input data from file
    int input_file_size = (int) zchunk_size (input_chunk);
    char* input_file_data = zchunk_strdup(input_chunk);
    zchunk_destroy (&input_chunk);

    int output_len;
    unsigned char output[input_file_size];
    if (decryptParam) {
        output_len = decrypt_aes((unsigned char*) input_file_data, input_file_size, key_digest, iv_digest, output);
        output[output_len] = '\0';
    }else{
        output_len = encrypt_aes((unsigned char*) input_file_data, input_file_size, key_digest, iv_digest, output);
    }

    //Write output data to file
    zfile_t* output_file = zfile_new (".", output_file_path);
    zchunk_t* output_chunk = zchunk_new(output, output_len);
    rc = zfile_output (output_file);
    if (rc != 0) {
        printf("Problem with opening file for writing!\n");
        zchunk_destroy (&output_chunk);
        zfile_close (output_file);
        zfile_destroy (&output_file);
        free(input_file_data);
        free(key);
        free(iv);

        return EXIT_FAILURE;
    }

    rc = zfile_write (output_file, output_chunk, 0);
    if (rc != 0) {
        printf("Problem with writing output to file!\n");
    }
    printf("Output written to file:\n");
    if (decryptParam) {
        printf("%s\n", output);
    }else{
        BIO_dump_fp (stdout, (const char *) output, output_len);
    }

    zchunk_destroy (&output_chunk);
    zfile_close (output_file);
    zfile_destroy (&output_file);
    free(input_file_data);
    free(key);
    free(iv);

    return EXIT_SUCCESS;
}

int generate_sha256_hash(char* text, unsigned char* digest)
{
    unsigned int digest_len;

    EVP_MD_CTX * mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(mdctx, text, strlen(text));
    EVP_DigestFinal_ex(mdctx, digest, &digest_len);
    EVP_MD_CTX_free(mdctx);

    return digest_len;
}

int encrypt_aes(unsigned char *plaintext, int plaintext_len, unsigned char *key, unsigned char *iv, unsigned char *ciphertext)
{
    int len;
    int ciphertext_len;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len);
    ciphertext_len = len;
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int decrypt_aes(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *iv, unsigned char *plaintext)
{
    int len;
    int plaintext_len;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len);
    plaintext_len = len;
    EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}

static char* read_keys_file_content(const char *keys_file_path) {
    char* keys_file_full_path = strndup(keys_file_path, MAX_KEY_FILE_LENGTH);
    char* keys_file_name = NULL;

    char* sep_kf_at = strrchr(keys_file_path, '/');
    if (sep_kf_at != NULL) {
        *sep_kf_at = '\0';
        keys_file_name = sep_kf_at + 1;
    }else{
        keys_file_name = (char*) keys_file_path;
        keys_file_path = (const char*) ".";
    }

    printf("Keys file path: %s\n", keys_file_full_path);

    int rc = 0;

    zfile_t* keys_file = zfile_new (keys_file_path, keys_file_name);
    rc = zfile_input (keys_file);
    if (rc != 0) {
        printf("Keys file '%s' not readable!\n", keys_file_full_path);
        zfile_destroy(&keys_file);
        free(keys_file_full_path);
        return NULL;
    }

    ssize_t keys_file_size = zsys_file_size (keys_file_full_path);
    zchunk_t* keys_chunk = zfile_read (keys_file, keys_file_size, 0);
    if (keys_chunk == NULL) {
        printf("Can't read file '%s'!\n", keys_file_full_path);
        zfile_close(keys_file);
        zfile_destroy(&keys_file);
        free(keys_file_full_path);
        return NULL;
    }

    char* keys_data = zchunk_strdup(keys_chunk);
    zchunk_destroy(&keys_chunk);
    zfile_close(keys_file);
    zfile_destroy (&keys_file);

    return keys_data;
}

static void parse_key_lines(char *keysBuffer, char **key, char **iv) {
    char *line = NULL, *saveLinePointer = NULL;

    bool firstTime = true;
    do {
        if (firstTime) {
            line = strtok_r(keysBuffer, "\n", &saveLinePointer);
            firstTime = false;
        }else {
            line = strtok_r(NULL, "\n", &saveLinePointer);
        }

        if (line == NULL) {
            break;
        }

        parse_key_line(line, key, iv);

    } while ((*key == NULL || *iv == NULL) && line != NULL);

}

static void parse_key_line(char *line, char **key, char **iv) {
    char *detectedKey = NULL, *detectedValue= NULL;

    char* sep_at = strchr(line, ':');
    if (sep_at == NULL) {
        return;
    }

    *sep_at = '\0'; // overwrite first separator, creating two strings.
    detectedKey = line;
    detectedValue = sep_at + 1;

    if (detectedKey == NULL || detectedValue == NULL) {
        return;
    }
    if (detectedKey[0] == '\0' || detectedValue[0] == '\0') {
        return;
    }

    if (*key == NULL && strcmp(detectedKey, KEY_TO_GET) == 0) {
        *key = strndup(detectedValue, AES_KEY_LENGTH);
    } else if (*iv == NULL && strcmp(detectedKey, IV_TO_GET) == 0) {
        *iv = strndup(detectedValue, AES_IV_LENGTH);
    }
}

