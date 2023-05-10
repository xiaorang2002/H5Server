#ifndef __MemAlloc_h__
#define __MemAlloc_h__

#include <queue>
#include <map>
#include <vector>
#include <kernel/CriticalSection.h>

using namespace std;

template <typename T>
class MemAlloc {
	typedef std::map<T*, unsigned int> ObjectAllocMap;
	typedef std::vector<T*> ObjectVector;
public:
	MemAlloc() {
#ifdef __MULTI_THREAD__
        CCriticalSectionEx::Create(&m_cs);
#endif//__MULTI_THREAD__
	}

	~MemAlloc()
	{
#ifdef __MULTI_THREAD__
        CCriticalSectionEx autoLock;
        autoLock.Lock(&m_cs);
#endif//__MULTI_THREAD__

		typename ObjectVector::iterator iter = m_objArr.begin();
		while(iter != m_objArr.end()) {
			delete *iter;
			++iter;
		}

        // try to destroy critical section.
        CCriticalSectionEx::Destroy(&m_cs);
	}

    void reserve(unsigned int size)
    {
		for (unsigned int i = 0; i < size; ++i) {
			alloc();
		}
		freeAll();
	}

    T* alloc()
    {
#ifdef __MULTI_THREAD__
        CCriticalSectionEx autoLock;
        autoLock.Lock(&m_cs);
#endif//__MULTI_THREAD__

		T* p = 0;
        if (m_queue.size() > 0)
        {
			unsigned int index = m_queue.front();
			m_queue.pop();
			p = m_objArr.at(index);
			m_objMap.insert(typename ObjectAllocMap::value_type(p, index));
// 			m_objMap[p] = index;
        }
        else
        {
			p = new T;
			m_objMap.insert(typename ObjectAllocMap::value_type(p, m_objArr.size()));
// 			m_objMap[p] = m_objArr.size();
			m_objArr.push_back(p);
		}

		return p;
	}

    void free(T* t)
    {
#ifdef __MULTI_THREAD__
        CCriticalSectionEx autoLock;
        autoLock.Lock(&m_cs);
#endif//__MULTI_THREAD__

		typename ObjectAllocMap::iterator iter = m_objMap.find(t);
		if (iter != m_objMap.end()) {
			unsigned int value = iter->second;
			m_queue.push(value);
			m_objMap.erase(iter);
		}
	}

    void freeAll()
    {
#ifdef __MULTI_THREAD__
        CCriticalSectionEx autoLock;
        autoLock.Lock(&m_cs);
#endif//__MULTI_THREAD__

		typename ObjectAllocMap::iterator iter = m_objMap.begin();
		while(iter != m_objMap.end()) {
			m_queue.push(iter->second);
			++iter;
		}
		m_objMap.clear();
	}

    unsigned int size()
    {
#ifdef __MULTI_THREAD__
        CCriticalSectionEx autoLock;
        autoLock.Lock(&m_cs);
#endif//__MULTI_THREAD__

        return m_objMap.size();
    }

protected:
	ObjectVector m_objArr;
	queue<unsigned int> m_queue;
//	vector<unsigned int> m_objMap;
	ObjectAllocMap m_objMap;
    CRITICAL_SECTION m_cs;
};

#endif//__MemAlloc_h__
