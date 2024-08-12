#
# Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.
#   * Neither the name of Intel Corporation nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# property right is granted to or conferred upon you by disclosure or delivery
# of the Materials, either expressly, by implication, inducement, estoppel or
# otherwise. Any license under such intellectual property rights must be
# express and approved by Intel(R) in writing.
#

######## SGX SDK Settings ########
SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= HW
SGX_ARCH ?= x64
UNTRUSTED_DIR=app

ifeq ($(shell getconf LONG_BIT), 32)
	SGX_ARCH := x86
else ifeq ($(findstring -m32, $(CXXFLAGS)), -m32)
	SGX_ARCH := x86
endif

ifeq ($(SGX_ARCH), x86)
	$(error x86 build is not supported, only x64!!)
else
	SGX_COMMON_CFLAGS := -m64
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib64
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x64/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x64/sgx_edger8r
endif

ifeq ($(SGX_DEBUG), 1)
ifeq ($(SGX_PRERELEASE), 1)
$(error Cannot set SGX_DEBUG and SGX_PRERELEASE at the same time!!)
endif
endif

PROTECTED_FS_INC_DIR := $(SGX_SDK)/include
PROTECTED_FS_LIB_DIR := $(SGX_SDK)/lib64
PROTECTED_FS_LIB := sgx_uprotected_fs

ifeq ($(SGX_DEBUG), 1)
        SGX_COMMON_CFLAGS += -O0 -g -DDEBUG
else
        SGX_COMMON_CFLAGS += -O2 -D_FORTIFY_SOURCE=2
endif


######## App Settings ########

App_Cpp_Files := $(wildcard $(UNTRUSTED_DIR)/*.cpp)
App_Cpp_Objects := $(App_Cpp_Files:.cpp=.o)

App_Include_Paths := -I$(UNTRUSTED_DIR) -I$(SGX_SDK)/include

App_C_Flags := $(SGX_COMMON_CFLAGS) -fpic -fpie -fstack-protector -Wformat -Wformat-security -Wno-attributes $(App_Include_Paths)
App_Cpp_Flags := $(App_C_Flags) -std=c++11

ifneq ($(SGX_MODE), HW)
	Urts_Library_Name := sgx_urts_sim
	UaeService_Library_Name := sgx_uae_service_sim
else
	Urts_Library_Name := sgx_urts
	UaeService_Library_Name := sgx_uae_service
endif

Security_Link_Flags := -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -pie

App_Link_Flags := $(SGX_COMMON_CFLAGS) $(Security_Link_Flags) -l$(Urts_Library_Name) -l$(UaeService_Library_Name) -L$(PROTECTED_FS_LIB_DIR) -l$(PROTECTED_FS_LIB) -lpthread 


ifeq ($(SGX_MODE), HW)
ifneq ($(SGX_DEBUG), 1)
ifneq ($(SGX_PRERELEASE), 1)
Build_Mode = HW_RELEASE
endif
endif
endif


.PHONY: all run

all: TestApp

run: all
ifneq ($(Build_Mode), HW_RELEASE)
	@$(CURDIR)/TestApp
	@echo "RUN  =>  TestApp [$(SGX_MODE)|$(SGX_ARCH), OK]"
endif

######## App Objects ########

$(UNTRUSTED_DIR)/TestEnclave_u.c: $(SGX_EDGER8R) enclave/TestEnclave.edl
	@cd $(UNTRUSTED_DIR) && $(SGX_EDGER8R) --untrusted ../enclave/TestEnclave.edl --search-path ../$(PROTECTED_FS_INC_DIR) --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

$(UNTRUSTED_DIR)/TestEnclave_u.o: $(UNTRUSTED_DIR)/TestEnclave_u.c
	@$(CC) $(App_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

$(UNTRUSTED_DIR)/%.o: $(UNTRUSTED_DIR)/%.cpp
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

TestApp: $(UNTRUSTED_DIR)/TestEnclave_u.o $(App_Cpp_Objects)
	@$(CXX) $^ -o $@ $(App_Link_Flags)
	@echo "LINK =>  $@"


.PHONY: clean

clean:
	@rm -f TestApp  $(App_Cpp_Objects) $(UNTRUSTED_DIR)/TestEnclave_u.* 
	
