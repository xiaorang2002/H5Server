/*
    带缓存、预分配内存管理类.
    ObjectAllocator<ObjType> m_alloc;
    m_alloc.reserve(count);
    m_alloc.alloc();
*/

#ifndef __ObjectAllocator_h__
#define __ObjectAllocator_h__

// #include <boost/tr1/unordered_map.hpp>
#include <queue>
#include <map>
#include <vector>
#include <kernel/CriticalSection.h>
using namespace std;

template <typename T>
class ObjectAllocator {
	typedef std::map<T*, unsigned int> ObjectAllocMap;
	typedef std::vector<T*> ObjectAllocVector;
	
public:
	ObjectAllocator() {
		CCriticalSectionEx::Create(&m_cs);
	}
	
	~ObjectAllocator()
	{
		CCriticalSectionEx autoLock;
		autoLock.Lock(&m_cs);

		typename ObjectAllocVector::iterator iter = m_objArr.begin();
		while(iter != m_objArr.end()) {
			delete *iter;
			++iter;
		}

		CCriticalSectionEx::Destroy(&m_cs);

	}
	
	void reserve(unsigned int size) {
		for (unsigned int i = 0; i < size; ++i) {
			alloc();
		}
		freeAll();
	}
	
	T* alloc() 
	{
                CCriticalSectionEx autoLock;
                autoLock.Lock(&m_cs);

		T* p = 0;
		if (m_queue.size() > 0) {
			unsigned int index = m_queue.front();
			m_queue.pop();
			p = m_objArr.at(index);
			m_objMap.insert(typename ObjectAllocMap::value_type(p, index));
// 			m_objMap[p] = index;
		} else {
			p = new T;
			m_objMap.insert(typename ObjectAllocMap::value_type(p, m_objArr.size()));
// 			m_objMap[p] = m_objArr.size();
			m_objArr.push_back(p);
		}

		return p;
	}
	
	void free(T* t) 
	{
                CCriticalSectionEx autoLock;
                autoLock.Lock(&m_cs);

		typename ObjectAllocMap::iterator iter = m_objMap.find(t);
		if (iter != m_objMap.end()) {
			unsigned int value = iter->second;
			m_queue.push(value);
			m_objMap.erase(iter);
		}
	}
	
	void freeAll() 
	{
                CCriticalSectionEx autoLock;
                autoLock.Lock(&m_cs);

		typename ObjectAllocMap::iterator iter = m_objMap.begin();
		while(iter != m_objMap.end()) {
			m_queue.push(iter->second);
			++iter;
		}
		m_objMap.clear();
	}
	
	unsigned int size() 
	{
                CCriticalSectionEx autoLock;
                autoLock.Lock(&m_cs);

		return m_objMap.size();
	}	
	
protected:
	CRITICAL_SECTION m_cs;
	ObjectAllocVector m_objArr;
	queue<unsigned int> m_queue;
//	vector<unsigned int> m_objMap;
	ObjectAllocMap m_objMap;
};

#endif//__ObjectAllocator_h__
