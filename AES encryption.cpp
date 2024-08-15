#include <iostream>
#include <iomanip>

enum keySize
{
    SIZE_16 = 16,
    SIZE_24 = 24,
    SIZE_32 = 32
};

enum errorCode
{
    SUCCESS = 0,
    ERROR_AES_UNKNOWN_KEYSIZE,
    ERROR_MEMORY_ALLOCATION_FAILED,
};


const uint8_t sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

const uint8_t Rcon[11] = {
    0x00, 0x01, 0x02,
    0x04, 0x08, 0x10,
    0x20, 0x40, 0x80,
    0x1b, 0x36
};

void xorBytes(const uint8_t* input1, const uint8_t* input2, uint8_t* output, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        output[i] = input1[i] ^ input2[i];
    }
}

void rotWord(const uint8_t* word, uint8_t* result) {
    result[0] = word[1];
    result[1] = word[2];
    result[2] = word[3];
    result[3] = word[0];
}

void subWord(const uint8_t* word, uint8_t* result) {
    for (size_t i = 0; i < 4; ++i) {
        result[i] = sbox[word[i]];
    }
}

void keyexpansion(const uint8_t key[], uint8_t w[][4], size_t rounds, size_t column) {
    uint8_t temp[4];
    size_t n = 4;

    for (size_t i = 0; i < column; ++i) {
        w[i][0] = key[4 * i];
        w[i][1] = key[4 * i + 1];
        w[i][2] = key[4 * i + 2];
        w[i][3] = key[4 * i + 3];
    }

    for (size_t i = column; i < n * (rounds + 1); ++i) {
        temp[0] = w[i - 1][0];
        temp[1] = w[i - 1][1];
        temp[2] = w[i - 1][2];
        temp[3] = w[i - 1][3];

        if (i % column == 0) {
            uint8_t rcon[4] = { static_cast<uint8_t>(Rcon[i / column]), 0x00, 0x00, 0x00 };
            uint8_t temp2[4];
            rotWord(temp, temp2);
            subWord(temp2, temp2);
            xorBytes(temp2, rcon, temp2, 4);
            temp[0] = temp2[0];
            temp[1] = temp2[1];
            temp[2] = temp2[2];
            temp[3] = temp2[3];
        }

        w[i][0] = w[i - column][0] ^ temp[0];
        w[i][1] = w[i - column][1] ^ temp[1];
        w[i][2] = w[i - column][2] ^ temp[2];
        w[i][3] = w[i - column][3] ^ temp[3];
    }
}

void addRoundKey(uint8_t* state, const uint8_t* roundKey) {
    for (int i = 0; i < 16; ++i) {
        state[i] ^= roundKey[i];
    }
}

void subBytes(uint8_t* state) {
    // using AES s-Box (16x16 byte)
    for (int i = 0; i < 16; ++i) {
        state[i] = sbox[state[i]];
    }
}

void shiftRows(uint8_t* state) {
    uint8_t temp[16];
    // 1st row no change
    // 2nd row lcs by 1 byte
    // 3rd row lcs by 2 byte
    // 4th row lcs by 3 byte 
    
    temp[0] = state[0];
    temp[1] = state[5];
    temp[2] = state[10];
    temp[3] = state[15];

    temp[4] = state[4];
    temp[5] = state[9];
    temp[6] = state[14];
    temp[7] = state[3];

    temp[8] = state[8];
    temp[9] = state[13];
    temp[10] = state[2];
    temp[11] = state[7];

    temp[12] = state[12];
    temp[13] = state[1];
    temp[14] = state[6];
    temp[15] = state[11];

    for (int i = 0; i < 16; ++i) {
        state[i] = temp[i];
    }
}

void mixColumns(uint8_t* state) {
    uint8_t temp[16];

    for (int i = 0; i < 4; ++i) {
        // martix to be multiplied
        /* {(2,3,1,1),
            (1,2,3,1),
            (1,1,2,3),
            (3,1,1,2)*/
            
        temp[4 * i] = (uint8_t)(0x02 * state[4 * i] ^ 0x03 * state[4 * i + 1] ^ state[4 * i + 2] ^ state[4 * i + 3]); //{2,3,1,1}
        temp[4 * i + 1] = (uint8_t)(state[4 * i] ^ 0x02 * state[4 * i + 1] ^ 0x03 * state[4 * i + 2] ^ state[4 * i + 3]); //{1,2,3,1}
        temp[4 * i + 2] = (uint8_t)(state[4 * i] ^ state[4 * i + 1] ^ 0x02 * state[4 * i + 2] ^ 0x03 * state[4 * i + 3]); //{1,1,2,3}
        temp[4 * i + 3] = (uint8_t)(0x03 * state[4 * i] ^ state[4 * i + 1] ^ state[4 * i + 2] ^ 0x02 * state[4 * i + 3]); //{3,1,1,2}
    }

    for (int i = 0; i < 16; ++i) {
        state[i] = temp[i];
    }
}

void aes_main(uint8_t* block, uint8_t w[][4], int nbrRounds) {
    uint8_t roundKey[16];

    // Initial round key addition
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            roundKey[i * 4 + j] = w[i][j];
        }
    }
    addRoundKey(block, roundKey);

    // Main rounds
    for (int round = 1; round < nbrRounds; ++round) {
        subBytes(block);
        shiftRows(block);
        mixColumns(block);
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                roundKey[i * 4 + j] = w[round * 4 + i][j];
            }
        }
        addRoundKey(block, roundKey);
    }

    // Final round
    subBytes(block);
    shiftRows(block);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            roundKey[i * 4 + j] = w[nbrRounds * 4 + i][j];
        }
    }
    addRoundKey(block, roundKey);
}

char aes_encrypt(unsigned char *input, unsigned char *output, unsigned char *key, enum keySize size) {
    int expandedKeySize;
    int nbrRounds;
    uint8_t w[60][4] = {0};  // Max size for 14 rounds (AES-256)

    unsigned char block[16];
    int i, j;

    switch (size) {
    case SIZE_16:
        nbrRounds = 10;
        break;
    case SIZE_24:
        nbrRounds = 12;
        break;
    case SIZE_32:
        nbrRounds = 14;
        break;
    default:
        return ERROR_AES_UNKNOWN_KEYSIZE;
    }

    expandedKeySize = (16 * (nbrRounds + 1));

    // Set the block values
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++)
            block[(i + (j * 4))] = input[(i * 4) + j];
    }

    // Expand the key
    keyexpansion(key, w, nbrRounds, 4);

    // Encrypt the block using the expanded key
    aes_main(block, w, nbrRounds);

    // Unmap the block again into the output
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++)
            output[(i * 4) + j] = block[(i + (j * 4))];
    }

    return SUCCESS;
}

int main() {
    unsigned char plain_text[16] = {0x32, 0x43, 0xf6, 0xa8, 0x88, 0x5a, 0x30, 0x8d, 0x31, 0x31, 0x98, 0xa2, 0xe0, 0x37, 0x07, 0x34};
    unsigned char key[16] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0xcf, 0x29, 0x8a, 0xbf, 0xab, 0x33};
    unsigned char cipher_text[16];

    if (aes_encrypt(plain_text, cipher_text, key, SIZE_16) == SUCCESS) {
        printf("cipher text:\n");
        for (int i = 0; i < 16; ++i) {
            printf("%02x ", cipher_text[i]);
        }
        printf("\n");
    } else {
        printf("Encryption failed!\n");
    }

    return 0;
}
