#include <cstdio>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;
int main() {
    FILE *pub, *pri;
    RSA *publicRSA = nullptr, *privateRSA = nullptr;  // 因為回傳的是指標
    if((pub = fopen("pub.pem","r")) == NULL) {
        cout << "pub Error" << endl;
        exit(-1);
    }
    if((pri = fopen("pri.pem","r")) == NULL) {
        cout << "pri Error" << endl;
        exit(-1);
    }
    // 初始化算法庫
    OpenSSL_add_all_algorithms();
    // 從 .pem 格式讀取公私鑰
    if((privateRSA = PEM_read_RSAPrivateKey(pri, NULL,NULL,NULL)) == NULL) { 
        cout << "Read pri error" << endl;
    }
    if((publicRSA = PEM_read_RSA_PUBKEY(pub,NULL,NULL,NULL)) == NULL) {
        cout << "REad pub erro " << endl;
    }
    fclose(pri), fclose(pub);
    cout << "pub: " << publicRSA << endl;
    cout << "pri: " << privateRSA << endl;
    int rsa_len = RSA_size(publicRSA); // 幫你算可以加密 block 大小，字數超過要分開加密
    
    const unsigned char * src = (const unsigned char *)"67122"; //  測試的明文
    // 要開空間來存放加解密結果，型態要改成 unsigned char *
    unsigned char * enc = (unsigned char *)malloc(0x300); 
    unsigned char * enc2 = (unsigned char *)malloc(0x300); 
    unsigned char * dec = (unsigned char *)malloc(0x300); 
    unsigned char * dec2 = (unsigned char *)malloc(0x300); 
    // 加密時因為 RSA_PKCS1_PADDING 的關係，加密空間要減 11，小於零出錯
    int enc2l,enc1l;
    
    if((enc1l=RSA_public_encrypt(rsa_len-11, src, enc, publicRSA, RSA_PKCS1_PADDING)) < 0) {
        cout << "enc1 error" << endl;
    }
    if((enc2l = RSA_public_encrypt(rsa_len+11, enc, enc2, publicRSA, RSA_PKCS1_PADDING)) < 0) {
        cout << "enc2 error" << endl;
    }
    // 加密後變成奇怪的一堆字元
    cout << "enc1: " << enc1l  << endl;
    cout << "enc2: " << enc2l << endl; 
    // 解密時不用減 11，小於零出錯
    if(RSA_private_decrypt(enc2l, enc2,dec,privateRSA, RSA_PKCS1_PADDING) < 0){
        cout << "dec1 error" << endl;
    }
    if(RSA_private_decrypt(enc1l, dec,dec2,privateRSA, RSA_PKCS1_PADDING) < 0){
        cout << "dec2 error" << endl;
    }
    cout << "enc1: " << enc << endl;
    cout << "dec: " << dec << endl;
    // 因為是它的函式 new 出來的東東，需要用他的函式釋放記憶體
    RSA_free(publicRSA);
    RSA_free(privateRSA);
}