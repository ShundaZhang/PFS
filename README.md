# SGX Protected File System Sample

This repository provides sample code and test suite for implementing a protected file system using Intel SGX enclaves. The protected file system ensures confidentiality and integrity of files using SGX's security features.

## Repository Structure

```
.
├── PFS_Sample/                # Protected File System Implementation
│   ├── Enclave_private.pem   # Enclave signing key
│   ├── Makefile              # Build configuration
│   ├── app.c                 # Untrusted application (file operations)
│   ├── app.h                 # Untrusted header
│   ├── data.enc              # Sample encrypted file
│   ├── enclave.c             # Trusted enclave code (encryption/decryption)
│   ├── enclave.config.xml    # Enclave configuration
│   ├── enclave.edl           # Interface between app and enclave
│   ├── enclave.h             # Trusted header
│   ├── enclave.lds           # Enclave linker script
│   └── enclave_hash.hex      # Enclave measurement
│
├── PFS_Test/                 # Test Suite
    ├── Makefile              
    ├── app/                  
    │   └── test_app.cpp      # File system test cases
    ├── enclave/              
    │   ├── TestEnclave.config.xml
    │   ├── TestEnclave.edl   
    │   ├── TestEnclave.lds   
    │   ├── TestEnclave_private.pem
    │   └── test_enclave.cpp  # Test enclave implementation
    ├── sgx_t.mk              # Trusted build rules
    └── sgx_u.mk              # Untrusted build rules
```

## Features

- Transparent file encryption/decryption inside SGX enclave
- Secure key management using SGX sealing
- File integrity protection
- Support for basic file operations (read/write/seek)

## Prerequisites

- Intel SGX SDK
- Intel SGX PSW (Platform Software)
- Build tools (GCC, Make)

## Building

### Sample Implementation
```bash
cd PFS_Sample
make
```

### Test Suite
```bash
cd PFS_Test
make
```

## Running Tests

```bash
cd PFS_Test
./test_app
```

The test suite verifies:
- Basic file operations
- Encryption/decryption functionality
- Data integrity protection
- Key management
- Error handling
