/*
* Copyright 2018 NXP.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
*
* Redistributions in binary form must reproduce the above copyright notice, this
* list of conditions and the following disclaimer in the documentation and/or
* other materials provided with the distribution.
*
* Neither the name of the NXP Semiconductor nor the names of its
* contributors may be used to endorse or promote products derived from this
* software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*/
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <string.h>

#pragma once

using namespace std;

class TransBase
{
public:
	string m_path;
	void * m_devhandle;
	TransBase() { m_devhandle = NULL; }
	virtual int open(void *) { return 0; };
	virtual int close() { return 0; };
	virtual int write(void *buff, size_t size) = 0;
	virtual int read(void *buff, size_t size, size_t *return_size) = 0;
	int write(vector<uint8_t> & buff) { return write(buff.data(), buff.size()); }
	int read(vector<uint8_t> &buff)
	{
		size_t size;
		int ret = read(buff.data(), buff.size(), &size);
		if (ret)
			return ret;
		buff.resize(size);
		return ret;
	}
};

class EPInfo
{
public:
	EPInfo() { addr = 0; package_size = 64; }
	EPInfo(int a, int size) { addr = a; package_size = size; };
	int addr;
	int package_size;
};

class USBTrans : public TransBase
{
public:
	vector<EPInfo> m_EPs;
	virtual int open(void *p);
	virtual int close();
};
class HIDTrans : public USBTrans
{
	int m_set_report;
public:
	int m_read_timeout;
	HIDTrans():the_thread(){ m_set_report = 9; m_read_timeout = 1000; }
	~HIDTrans() { if (m_devhandle) close();  m_devhandle = NULL;
		stop_thread = true;
		if(the_thread.joinable())
			the_thread.join();
		printf("%s %d\r\n", __func__, __LINE__);
	}
	int write(void *buff, size_t size);
	int read(void *buff, size_t size, size_t *return_size);

private:
    thread the_thread;
    bool stop_thread = false; // Super simple thread stopping.
	char m_buff[65];
	bool m_done;
    void ThreadMain(){
		size_t actual;
        while(!stop_thread){
            // Do something useful, e.g:
            //std::this_thread::sleep_for( std::chrono::seconds(1) );
			m_done = false;
			memset(m_buff, 0, 65);
			printf("Thread %p is running\r\n", this);
			while ( strncmp(m_buff, "OKAY", 4) && strncmp(m_buff, "FAIL", 4))
				continue;
			m_done = true;
        }
    }
};

class BulkTrans : public USBTrans
{
	void Init()
	{
		m_MaxTransPreRequest = 0x100000;
		m_b_send_zero = 0;
		m_timeout = 2000;
	}

public:
	EPInfo m_ep_in;
	EPInfo m_ep_out;
	size_t m_MaxTransPreRequest;
	int m_b_send_zero;
	uint64_t m_timeout;

	BulkTrans():the_thread() {
		Init();
	}

	virtual int open(void *p);

	~BulkTrans() {
		if (m_devhandle) close();
		m_devhandle = NULL;
		stop_thread = true;
		if(the_thread.joinable())
			the_thread.join();
		printf("%s %d\r\n", __func__, __LINE__);
	}

	int write(void *buff, size_t size);
	int read(void *buff, size_t size, size_t *return_size);
	int read_thread(void *buff, size_t size, size_t *rsize);

private:
    thread the_thread;
    bool stop_thread = false; // Super simple thread stopping.
	char m_buff[65];
	bool m_done;
    void ThreadMain(){
		size_t actual;
        while(!stop_thread){
            // Do something useful, e.g:
            //std::this_thread::sleep_for( std::chrono::seconds(1) );
			m_done = false;
			memset(m_buff, 0, 65);
			printf("Thread %p is running\r\n", this);
			read_thread(m_buff, 64, &actual);
			while ( strncmp(m_buff, "OKAY", 4) && strncmp(m_buff, "FAIL", 4))
				continue;
			m_done = true;
        }
    }
};

int polling_usb(std::atomic<int>& bexit);
