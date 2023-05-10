#pragma once

#ifdef  _WIN32
#include <Windows.h>
#else
#include <tick.h>
#endif//_WIN32

#include <vector>
using namespace std;
#include "../FishProduceUtils/Random.h"

class CSceneManager
{
public:
	CSceneManager()
	: m_dwCurTick(0)
	{
		Reset();
	}

public:
	void Start(int scene_id)
	{
		if (scene_id < m_vecScene.size())
		{
			m_bIsPlaying = true;
			m_nCurSceneId = scene_id;
			m_dwCurTick = GetTickCount();
			m_dwTotalTick = m_vecScene[scene_id];
			m_dwTickElapsed = 0;
			m_dwAddTick = 0;
		}
	}

	void Reset()
	{
		m_nCurSceneId = -1;
		m_dwTickElapsed = 0;
		m_dwTotalTick = 1;
		m_bIsPlaying = false;
		m_dwAddTick = 0;
	}

	void Push(DWORD duration)
	{
		m_vecScene.push_back(duration);
	}

	int GetNextSceneId()
	{
		if (-1 == m_nCurSceneId)
        {
            //LOG(WARNING)<<"GetNextSceneId()   --->randomint()";
			return Random::getInstance()->RandomInt(0, m_vecScene.size() - 1);
		}
		else
		{
			return (m_nCurSceneId + 1) % m_vecScene.size();
		}
	}

	inline bool IsDone() {return m_dwTickElapsed >= m_dwTotalTick + m_dwAddTick;}

	inline bool IsPlaying() {return m_bIsPlaying;}

	inline int GetCurSceneId() {return m_nCurSceneId;}

	inline DWORD GetRunTime() {return m_dwTickElapsed;}

	void Update()
	{
		if (m_bIsPlaying)
		{
			DWORD dwTick = GetTickCount();
			if (dwTick < m_dwCurTick) {
				m_dwTickElapsed += 0xFFFFFFFF - m_dwCurTick + dwTick;
			} else {
				m_dwTickElapsed += dwTick - m_dwCurTick;
			}
			if (IsDone())
			{
				m_bIsPlaying = false;
			}
			m_dwCurTick = dwTick;
		}
	}
	void SetAddTick(DWORD dwTick)
	{
		m_dwAddTick = dwTick;
	}
	//场景剩下的时间
	DWORD GetLessRunTime() 
	{ 
		return m_dwTotalTick - m_dwTickElapsed;
	}
	bool IsInSceneTime() { return m_dwTotalTick >= m_dwTickElapsed; }
protected:
	int m_nCurSceneId;
	DWORD m_dwCurTick, m_dwTotalTick, m_dwTickElapsed, m_dwAddTick;
	bool m_bIsPlaying;
	vector<DWORD> m_vecScene;
};
