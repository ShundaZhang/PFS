/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "TestEnclave_t.h"
#include "sgx_tprotected_fs.h"
#include <sgx_trts.h>
//#include <sgx_tae_service.h>
#include <string.h>
#include <stdio.h>

#define TEST_BUFFER_SIZE (256*1024*1024) // 256MB
#define BUFFER_SIZE (6*1024*1024) // 6MB

#define MAIN_LOOP_COUNT 2
#define SUB_LOOP_COUNT  10
#define RW_LOOP_COUNT   50

uint32_t random_number(uint32_t start, uint32_t end)
{
    uint32_t x;
    uint32_t ret;
	sgx_status_t status;

	status = sgx_read_rand((unsigned char*)&x, sizeof(uint32_t));
	if (status != SGX_SUCCESS)
	{
		uexit("sgx_read_rand failed", status);
	}

    ret = start + (uint32_t)((((double)(end - start) / (double)(0x100000000)) * (double)x)+0.5);

    return ret;
}


int runTest(char* file_buffer)
{
	sgx_status_t status = SGX_ERROR_BUSY;
	size_t count = 0;
	int result;
	SGX_FILE* file;
	uint32_t file_size = 0;
	uint32_t offset = 0;
	uint32_t temp;
	const char* filename = "test_file";
	sgx_key_128bit_t key;
	char progress[100];

	char* buffer = new char[BUFFER_SIZE];
	
	status = sgx_read_rand((unsigned char*)buffer, BUFFER_SIZE);
	if (status != SGX_SUCCESS)
	{
		uexit("sgx_read_rand failed", status);
	}

	//while ((status = sgx_create_pse_session()) == SGX_ERROR_BUSY && count++ < 10);
	//if (count >= 10 || status != SGX_SUCCESS)
	//{
	//	uexit("sgx_create_pse_session failed", status);
	//}

	for (int k = 0 ; k < MAIN_LOOP_COUNT ; k++) // loop to use fopen and fopen_ex
	{
		file_size = 0;

		snprintf(progress, 100, "main loop %d/%d\n", k+1, MAIN_LOOP_COUNT);
		uprint(progress);

		if (k % 2 == 0)
		{
			file = sgx_fopen_auto_key(filename, "w+b");
			if (file == NULL)
			{
				uexit("sgx_fopen_auto_key failed", 1);
			}
		}
		else
		{
			status = sgx_read_rand((unsigned char*)&key, sizeof(sgx_key_128bit_t));
			if (status != SGX_SUCCESS)
			{
				uexit("sgx_read_rand failed", status);
			}
			file = sgx_fopen(filename, "wb+", &key);
			if (file == NULL)
			{
				uexit("sgx_fopen_auto_key failed", 1);
			}
		}

		for (int j = 0 ; j < SUB_LOOP_COUNT ; j++) // loop to close and re-open the file
		{
			offset = 0;

			snprintf(progress, 100, "\tsub loop %d/%d\n", j+1, SUB_LOOP_COUNT);
			uprint(progress);

			for (int i = 0 ; i < RW_LOOP_COUNT ; i++) // write loop
			{
				uint32_t op_size;

				if (i < 10)
					op_size = random_number(1, 128); // first 10 operations - small ones...
				else
					op_size = random_number(1, BUFFER_SIZE);

				count = sgx_fwrite(buffer, 1, op_size, file);
				if (count != op_size)
				{
					int error = sgx_ferror(file);
					sgx_fclose(file);
					uexit("sgx_fwrite failed", error);
				}

				memcpy(&file_buffer[offset], buffer, op_size); // create an exact copy of the file inside the buffer

				file_size = offset + op_size > file_size ? offset + op_size : file_size;

				// randomize offset for next loop
				if (i % 5 != 0)
				{
					offset = random_number(0, file_size); // might increase the file size
				}
				else
				{
					offset = file_size; // increase the file size
				}

				if (file_size > TEST_BUFFER_SIZE - BUFFER_SIZE) // file size is bigger than 244MB...reaching end of test buffer
				{
					offset = random_number(0, TEST_BUFFER_SIZE - BUFFER_SIZE); // will not increase file size beyond 250MB
				}

				if (i % 10 != 0)
				{
					result = sgx_ferror(file);
					if (result != 0)
					{
						sgx_fclose(file);
						uexit("sgx_ferror returned an error!", result);
					}

					/*
					if (j == 5)
					{
						result = sgx_fflush_and_increment_mc(file);
						if ((k % 2) == 0) // shared file
						if (result == 0)
						{
							sgx_fclose(file);
							uexit("sgx_fflush_and_increment_mc succeeded for a shared file", 1);
						}
						else
						{
							sgx_clearerr(file);
						}

						if (((k % 2) == 1) && (result != 0)) // not shared
						{
							int error = sgx_ferror(file);
							sgx_fclose(file);
							uexit("sgx_fflush_and_increment_mc failed", error);
						}
					}
					else
					{*/
						result = sgx_fflush(file);
						if (result != 0)
						{
							int error = sgx_ferror(file);
							sgx_fclose(file);
							uexit("sgx_fflush failed", error);
						}
					//}				
				}

				if (i % 5 == 0)
					result = sgx_fseek(file, (int32_t)offset-(int32_t)file_size, SEEK_END);
				else
					result = sgx_fseek(file, offset, SEEK_SET);
				if (result != 0)
				{
					int error = sgx_ferror(file);
					sgx_fclose(file);
					uexit("sgx_fseek failed", error);
				}

				temp = (uint32_t)sgx_ftell(file);
				if (temp != offset)
				{
					sgx_fclose(file);
					uexit("sgx_ftell gave a weird number", 1);
				}

				if (i == 30)
				{
					result = sgx_fclear_cache(file);
					if (result != 0)
					{
						sgx_fclose(file);
						uexit("sgx_fclear_cache returned error", result);
					}
				}

			}

			for (int i = 0 ; i < RW_LOOP_COUNT ; i++) // read loop
			{
				uint32_t op_size;

				if (i < 10)
					op_size = random_number(1, 128); // first 10 operations - small ones...
				else
					op_size = random_number(1, BUFFER_SIZE > file_size ? file_size : BUFFER_SIZE);

				if (i % 20 != 0)
				{
					offset = random_number(0, file_size-op_size);
				}
				else
				{// read beyond the end of file
					offset = random_number(file_size-op_size, file_size);
				}
			
				if (i % 5 == 0)
					result = sgx_fseek(file, (int32_t)offset-(int32_t)file_size, SEEK_END);
				else
					result = sgx_fseek(file, offset, SEEK_SET);
				if (result != 0)
				{
					int error = sgx_ferror(file);
					sgx_fclose(file);
					uexit("sgx_fseek failed", error);
				}

				count = sgx_fread(buffer, 1, op_size, file);
				if (count != op_size)
				{
					if (offset + op_size <= file_size)
					{
						int error = sgx_ferror(file);
						sgx_fclose(file);
						uexit("sgx_fread failed", error);
					}
				}

				if (i % 10 != 0)
				{
					result = sgx_ferror(file);
					if (result != 0)
					{
						sgx_fclose(file);
						uexit("sgx_ferror returned an error!", result);
					}

					result = sgx_feof(file);
					if ((offset + op_size < file_size) && (result != 0))
					{
						sgx_fclose(file);
						uexit("sgx_feof returned true!", result);
					}
				}

				if (memcmp(&file_buffer[offset], buffer, count) != 0)
				{
					sgx_fclose(file);
					uexit("data read is different than expected", 1);
				}
			}

			result = sgx_fseek(file, -100, SEEK_END);
			if (result != 0)
			{
				int error = sgx_ferror(file);
				sgx_fclose(file);
				uexit("sgx_fseek failed", error);
			}

			count = sgx_fread(buffer, 1, 150, file);
			if (count != 100)
			{
				sgx_fclose(file);
				uexit("sgx_fread didn't return 100", (uint32_t)count);
			}

			result = sgx_feof(file);
			if (result == 0)
			{
				sgx_fclose(file);
				uexit("sgx_feof didn't return true", 0);
			}

			count = sgx_fread(buffer, 1, 150, file);
			if (count != 0)
			{
				sgx_fclose(file);
				uexit("sgx_fread didn't return 0", (uint32_t)count);
			}

			result = sgx_ferror(file);
			if (result != 0)
			{
				sgx_fclose(file);
				uexit("sgx_ferror returned error", result);
			}

			sgx_clearerr(file);
			result = sgx_ferror(file);
			if (result != 0)
			{
				sgx_fclose(file);
				uexit("sgx_ferror returned an error after clearerr", result);
			}

			result = sgx_feof(file);
			if (result == 1)
			{
				sgx_fclose(file);
				uexit("sgx_feof returned true after clearerr", 0);
			}
			
			result = sgx_fclose(file);
			if (result != 0)
			{
				int error = sgx_ferror(file);
				uexit("sgx_fclose failed", error);
			}

			if (k % 2 == 0 && 
				j % 4 == 1)
			{
				result = sgx_fexport_auto_key(filename, &key);
				if (result != 0)
				{
					int error = sgx_ferror(file);
					uexit("sgx_fexport_auto_key failed", error);
				}

				result = sgx_fimport_auto_key(filename, &key);
				if (result != 0)
				{
					int error = sgx_ferror(file);
					uexit("sgx_fimport_auto_key failed", error);
				}

			}


			if (k % 2 == 0)
			{
				file = sgx_fopen(filename, "r+b", &key);
				if (file != NULL)
				{
					sgx_fclose(file);
					uexit("sgx_fopen succeeded", 1);
				}

				file = sgx_fopen_auto_key(filename, "rb+");
				if (file == NULL)
				{
					uexit("sgx_fopen_auto_key failed", 1);
				}
			}
			else
			{
				file = sgx_fopen_auto_key(filename, "rb+");
				if (file != NULL)
				{
					sgx_fclose(file);
					uexit("sgx_fopen_auto_key succeeded!", 1);
				}

				file = sgx_fopen(filename, "r+b", &key);
				if (file == NULL)
				{
					uexit("sgx_fopen failed", 1);
				}
			}
		}

		result = sgx_fclose(file);
		if (result != 0)
		{
			int error = sgx_ferror(file);
			uexit("sgx_fclose failed", error);
		}

		result = sgx_remove(filename);
		if (result != 0)
		{
			uexit("sgx_remove failed", result);
		}

		snprintf(progress, 100, "final file size %d (%d MB)\n", file_size, (file_size/1024)/1024);
		uprint(progress);

	}

	//sgx_close_pse_session();

	delete[] buffer;

	return 0;
}

