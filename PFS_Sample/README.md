# SGX Protected Fs Demo

this is a simple demo shows how to use intel Sgx protected file system

the same functions to the [Windows Example](https://software.intel.com/en-us/articles/overview-of-intel-protected-file-system-library-using-software-guard-extensions)

# Dependency

Please install libc++-dev via ```apt install libc++-dev```

# Generate you own Enclave private key

openssl genpkey -algorithm RSA -out Enclave_private.pem -pkeyopt rsa_keygen_bits:3072 -pkeyopt rsa_keygen_pubexp:3
