/* RedLock implement DLM(Distributed Lock Manager) with cpp.
 *
 * Copyright (c) 2014, jacketzhong <jacketzhong at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __redlock__
#define __redlock__

#include <iostream>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

#include "hiredis-vip/hiredis.h"
#include "hiredis-vip/sds.h"

#ifdef __cplusplus
}
#endif

//Redis实现分布式锁算法
namespace RedisLock {
	
	//CLock
	class CLock {
	public:
		CLock();
		~CLock();
	public:
		int m_validityTime; // 当前锁可以存活的时间, 毫秒
		sds m_resource;     // 要锁住的资源名称
		sds m_val;          // 锁住资源的进程随机名字
	};
	
	//CRedLock
	class CRedLock {
	public:
		CRedLock();
		virtual ~CRedLock();
	public:
		bool AddServerUrl(const char *ip, const int port);
		void SetRetry(const int count, const int delay);
		bool Lock(const char *resource, const int ttl, CLock &lock);
		bool ContinueLock(const char *resource, const int ttl, CLock &lock);
		bool Unlock(const CLock &lock);
	private:
		bool Initialize();
		bool LockInstance(redisContext *c, const char *resource, const char *val, const int ttl);
		bool ContinueLockInstance(redisContext *c, const char *resource, const char *val, const int ttl);
		void UnlockInstance(redisContext *c, const char *resource, const char *val);
		redisReply * RedisCommandArgv(redisContext *c, int argc, char **inargv);
		sds GetUniqueLockId();
	private:
		static int                 m_defaultRetryCount;    // 默认尝试次数3
		static int                 m_defaultRetryDelay;    // 默认尝试延时200毫秒
		static float               m_clockDriftFactor;     // 电脑时钟误差0.01
	private:
		sds                        m_unlockScript;         // 解锁脚本
		int                        m_retryCount;           // try count
		int                        m_retryDelay;           // try delay
		int                        m_quoRum;               // majority nums
		int                        m_fd;                   // rand file fd
		std::vector<redisContext*> m_redisServer;          // redis master servers
		CLock                      m_continueLock;         // 续锁
		sds                        m_continueLockScript;   // 续锁脚本
	};
	// 分布式锁的使用案例
	static void Test001() {
		CRedLock * dlm = new CRedLock();
		dlm->AddServerUrl("192.168.2.208", 6379);
		
		while (1) {
			CLock my_lock;
			bool flag = dlm->Lock("foo", 10000, my_lock);
			if (flag) {
				printf("获取成功, Acquired by client name:%s, res:%s, vttl:%d\n",
					my_lock.m_val, my_lock.m_resource, my_lock.m_validityTime);
				// do resource job
				sleep(20);
				dlm->Unlock(my_lock);
				// do other job
				//sleep(2);
			}
			else {
				printf("获取失败, lock not acquired, name:%s\n", my_lock.m_val);
				sleep(rand() % 3);
			}
		}
	}
	// 连续分布式锁的使用案例
	static void Test002() {
		CRedLock * dlm = new CRedLock();
		dlm->AddServerUrl("192.168.2.208", 6379);
		while (1) {
			CLock my_lock;
			bool flag = dlm->ContinueLock("foo", 14000, my_lock);
			if (flag) {
				printf("获取成功, Acquired by client name:%s, res:%s, vttl:%d\n",
					my_lock.m_val, my_lock.m_resource, my_lock.m_validityTime);
				// do resource job
				sleep(20);
			}
			else {
				printf("获取失败, lock not acquired, name:%s\n", my_lock.m_val);
				sleep(rand() % 3);
			}
		}
	}
}

#endif /* defined(__redlock__) */
