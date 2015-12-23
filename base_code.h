/*
 * =====================================================================================
 *
 *       Filename:  base_code.hpp
 *
 *    Description:  Common function
 *
 *        Version:  1.0
 *        Created:  2015年11月25日 16时11分13秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (chenhao), 
 *        Company:  
 *
 * =====================================================================================
 */

#ifndef __X__H__20151026__
#define __X__H__20151026__

#include <iostream>
#include <string>
#include <cstdarg>
#include <cstdio>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

// MutexLock
class MutexLock
{
public:
	MutexLock(): holder_(0)
	{
		pthread_mutex_init(&mutex_, NULL);
	}
	~MutexLock()
	{
		assert(holder_ == 0);
		pthread_mutex_destroy(&mutex_);
	}
	bool isLockedBySelf() {return holder_ == pthread_self();}
	void lock()
	{ 
		pthread_mutex_lock(&mutex_);
		holder_ = pthread_self();
	}
	void unlock()
	{ 
		holder_ = 0;
		pthread_mutex_unlock(&mutex_);
	}
	pthread_mutex_t* getPthreadMutex(){return &mutex_;}

private:
	MutexLock(const MutexLock& rx);
	MutexLock& operator=(const MutexLock&);

private:
	pthread_mutex_t mutex_;
	pthread_t holder_;
};

// MutexLockGuard
class MutexLockGuard
{
public:
	explicit MutexLockGuard(MutexLock& lock):mutex_lock_(lock)
	{
		mutex_lock_.lock();
	}
	~MutexLockGuard()
	{
		mutex_lock_.unlock();
	}

private:
	MutexLockGuard();
	MutexLockGuard(const MutexLockGuard& rx);
	MutexLockGuard& operator=(const MutexLockGuard& rx);

private:
	MutexLock& mutex_lock_;
};

//Condtion for threading
class Condition
{
public:
	explicit Condition(MutexLock& mutex):mutex_(mutex)
	{ 
		pthread_cond_init(&cond_, NULL);
	}
	~Condition(){pthread_cond_destroy(&cond_);}

	void wait()
	{
		pthread_cond_wait(&cond_, mutex_.getPthreadMutex());
	}

	void notify()
	{
		pthread_cond_signal(&cond_);
	}
	void notifyAll()
	{
		pthread_cond_broadcast(&cond_);
	}

private:
	MutexLock& mutex_;
	pthread_cond_t cond_;
};

//CountDownLatch
class CountDownLatch
{
public:
	explicit CountDownLatch(int count):count_(count), cond_(mutex_){}
	void wait()
	{
		MutexLockGuard lock(mutex_);
		while (count_ > 0)
		{
			cond_.wait();
		}
	}
	void countDown()
	{
		MutexLockGuard lock(mutex_);
		count_--;
		if (count_ == 0)
		{
			cond_.notifyAll();
		}
	}
private:
	int count_;
	mutable MutexLock mutex_;
	Condition cond_;

};

// FileHandle
class FileHandle
{
public:
    FileHandle(const char* s, const char* sMode):file_name_(s)
    {
        file_ = fopen(s, sMode);
		if (file_ == NULL)
		{
			std::cerr<<s<<" open failed"<<std::endl;	
		}
    }
    FileHandle(const std::string& s, const char* sMode):file_name_(s)
    {
        std::cout<<file_name_<<" opened"<<std::endl;
        file_ = fopen(s.c_str(), sMode);
		if (file_ == NULL)
		{
			std::cerr<<s<<" open failed"<<std::endl;	
		}
    }
    ~FileHandle()
    {
		if (file_)
        	fclose(file_);
    }
    FILE* get(){return file_;}
    std::string&  get_name(){return file_name_;};

private:
    FileHandle();
    FileHandle(const FileHandle& rx);
    FileHandle& operator = (const FileHandle& rx);

protected:
    FILE* file_;
    std::string  file_name_;
};


// has lock for threading
class Loger:public FileHandle
{
public:
    Loger(const char* s):FileHandle(s, "a+"){}
    Loger(const std::string& s):FileHandle(s, "a+"){}
	void _(const char* sFile, const int iLine, const char* sFormat, ...)
    {
		va_list sList;
		va_start(sList, sFormat);
		gettimeofday(&tt, NULL);
		ttt = localtime(&tt.tv_sec);
		//time pid file line
		fprintf(file_, "%04d%02d%02d-%02d:%02d:%02d.%03ld|%6d|%15s|% 5d|-> ", 
				ttt->tm_year+1900,ttt->tm_mon,ttt->tm_mday, ttt->tm_hour, ttt->tm_min, ttt->tm_sec, 
				tt.tv_usec/1000, getpid() ,sFile, iLine);
		vfprintf(file_, sFormat, sList);
		fprintf(file_, "\n");
		va_end(sList);
	}
	void _(int lock_flag, const char* sFile, const int iLine, const char* sFormat, ...)
    {
		MutexLockGuard lock(lock_);
		va_list sList;
		va_start(sList, sFormat);
		gettimeofday(&tt, NULL);
		ttt = localtime(&tt.tv_sec);
		fprintf(file_, "%04d%02d%02d-%02d:%02d:%02d.%03ld|%6d|%15s|% 5d|-> ", 
				ttt->tm_year+1900,ttt->tm_mon,ttt->tm_mday, ttt->tm_hour, ttt->tm_min, ttt->tm_sec, 
				tt.tv_usec/1000, getpid() ,sFile, iLine);
		vfprintf(file_, sFormat, sList);
		fprintf(file_, "\n");
		va_end(sList);
	}

private:
    Loger();
    Loger(const Loger& rx);
    Loger& operator = (const Loger& rx);
private:
	struct 	tm* ttt;
	struct timeval tt;
	mutable MutexLock lock_;
};

#define NO_LOCK  __FILE__,__LINE__
#define LOCK     1,__FILE__,__LINE__

//Loger log("webtest.log");
//#define _ log._

// To String
#include <algorithm>
template <typename T>
inline std::string toString(const T& value)
{
	T i = value;
	static const char* sDigits = "0123456789";

	char buf[64] = "";
	char* p = buf;
	do{
		int lsd = static_cast<int>(i % 10);
		i /= 10;
		*p++ = sDigits[lsd];
	}while(i != 0);

	if (value < 0)
	{
		*p++ = '-';
	}
	*p = '\0';
	std::reverse(buf, p);

	return buf;
}

#endif
