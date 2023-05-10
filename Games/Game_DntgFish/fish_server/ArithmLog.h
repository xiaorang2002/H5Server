#pragma once

#include <object/ObjectAllocator.h>
#include "TraceInfo.h"
// #include "PlayerLogMDB.h"
#define ARITHM_DB _T("ArithmLog.mdb")
#define ARITHM_DIR _T("ArithmLog")

#import ".\msado15.dll" no_namespace rename("EOF","adoEOF") rename("BOF","adoBOF")

struct tagArithmData
{
	WORD roomID;
	WORD tableID;
	WORD chairID;
	double realProfit;
	double exchangeProfit;
};

class ArithmLog
{
	typedef DWORD KEY_OF_ARITHM_DATA;
	typedef std::map<KEY_OF_ARITHM_DATA, tagArithmData*> ArithmDataMap;
	typedef std::map<wstring, TraceInfo*> ArithmDataFileMap;
public:
	ArithmLog() : m_exitThreadSQLCommand(false)
	{
		CreateDirectory(ARITHM_DIR, NULL);
		CCriticalSectionEx::Create(&m_csSqlCmd);
		CCriticalSectionEx::Create(&m_csAlloc);
		m_connectString = _T("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=");
		m_connectString.append(ARITHM_DB);
		loadArithmInfo();
		m_threadSQLCommand = CreateThread(NULL, 0, AgentThreadSQLCommand, this, 0, NULL);
	}
	~ArithmLog()
	{
		m_exitThreadSQLCommand = true;
		WaitForSingleObject(m_threadSQLCommand, INFINITE);
		CloseHandle(m_threadSQLCommand);
		CCriticalSectionEx::Destroy(&m_csSqlCmd);
		CCriticalSectionEx::Destroy(&m_csAlloc);
	}

public:
	void updateData(WORD roomID, WORD tableID, WORD chairID, long long realProfit, long long exchangeProfit, unsigned char logType)
	{
		tagArithmData* p = getArithmData(roomID, tableID, chairID);
		TraceInfo* trace = NULL;
		if (!p)
		{
			p = allocArithmData();
			if (p)
			{
				p->roomID = roomID;
				p->tableID = tableID;
				p->chairID = chairID;
				p->realProfit = realProfit;
				p->exchangeProfit = exchangeProfit;
				if (addArithmData(p))
				{
					CString str;
					str.Format(_T("INSERT INTO Arithm(RoomID, [TableID], ChairID, RealProfit, ExchangeProfit) VALUES(%d, %d, %d, %I64d, %I64d)"),
						roomID, tableID, chairID, realProfit, exchangeProfit);
					addSqlCommand((LPCTSTR)str);
				}
				else
				{
					freeArithmData(p);
					p = NULL;
				}
			}
		}
		else
		{
			p->realProfit += realProfit;
			p->exchangeProfit += exchangeProfit;
			CString str;
			str.Format(_T("UPDATE Arithm SET RealProfit=RealProfit+%I64d, ExchangeProfit=ExchangeProfit+%I64d WHERE RoomID=%d AND [TableID]=%d AND ChairID=%d"),
				realProfit, exchangeProfit, roomID, tableID, chairID);
			addSqlCommand((LPCTSTR)str);
		}
		if (p)
		{
			wstring keyFile = makeArithmDataFileName(roomID, tableID);
			trace = getArithmDataFile(keyFile);
			if (!trace)
			{
				trace = new TraceInfo(keyFile.c_str());
				if (!addArithmDataFile(keyFile, trace))
				{
					delete trace;
					trace = NULL;
				}
			}
			if (trace)
			{
				if (logType & 1)
				{
					trace->Trace(_T("BEGIN1\n=======================================================\n实际盈利情况: (触发台位: %d)"), chairID);
					long long sum = 0;
					for (int i = 0; i < 4; ++i)
					{
						p = getArithmData(roomID, tableID, i);
						long long tmp = 0;
						if (p)
						{
							tmp = p->realProfit;
						}
						sum += tmp;
						trace->TraceWithNoTimeStamp(_T("\t(%d)\t%I64d\n"), i, tmp);
					}
					trace->TraceWithNoTimeStamp(_T("\t总:\t%I64d\n"), sum);
					trace->TraceWithNoTimeStamp(_T("=======================================================\n"));
					trace->Trace(_T("END1"));
				}
				if (logType & 2)
				{
					trace->Trace(_T("BEGIN2\n=======================================================\n兑换盈利情况: (触发台位: %d)"), chairID);
					long long sum = 0;
					for (int i = 0; i < 4; ++i)
					{
						p = getArithmData(roomID, tableID, i);
						long long tmp = 0;
						if (p)
						{
							tmp = p->exchangeProfit;
						}
						sum += tmp;
						trace->TraceWithNoTimeStamp(_T("\t(%d)\t%I64d\n"), i, tmp);
					}
					trace->TraceWithNoTimeStamp(_T("\t总:\t%I64d\n"), sum);
					trace->TraceWithNoTimeStamp(_T("=======================================================\n"));
					trace->Trace(_T("END2"));
				}
			}
		}
	}

protected:
	void loadArithmInfo()
	{
		_ConnectionPtr pConnection;
		_RecordsetPtr recordset;
		HRESULT hr;

		try {
			hr = pConnection.CreateInstance(__uuidof(Connection));///创建Connection对象
			if(SUCCEEDED(hr) && SUCCEEDED(recordset.CreateInstance(__uuidof(Recordset))))
			{
				pConnection->CursorLocation = adUseClient;
				hr = pConnection->Open(m_connectString.c_str(), _T(""),_T(""),adModeUnknown);///连接数据库
				if (SUCCEEDED(hr))
				{
					wstring strCmdText = _T("SELECT * FROM Arithm");
					getAllRows(strCmdText.c_str(), pConnection, recordset);
					while(!recordset->EndOfFile)
					{
						tagArithmData* ArithmData = allocArithmData();
						if (ArithmData)
						{
							ArithmData->roomID = recordset->GetCollect("RoomID");
							ArithmData->tableID = recordset->GetCollect("TableID");
							ArithmData->chairID = recordset->GetCollect("ChairID");
							ArithmData->realProfit = recordset->GetCollect("RealProfit");
							ArithmData->exchangeProfit = recordset->GetCollect("ExchangeProfit");
							if (!addArithmData(ArithmData))
							{
								freeArithmData(ArithmData);
							}
						}
						recordset->MoveNext();
					}
					recordset->Close();
				}
			}
			pConnection->Close();
			//_TraceLog.Trace(_T("加载[User.mdb]成功！"));
		}
		catch(_com_error e) { ///捕捉异常
			if (recordset->State)
			{
				recordset->Close();
			}
			if (pConnection->State)
			{
				pConnection->Close();
			}
			// 			_TraceLog.Trace(_T("[loadUserInfo]连接数据库[ARITHM_DB]失败!错误信息:[%d]%s"),
			// 				e.Error(), e.ErrorMessage());///显示错误信息
		}
	}

	static DWORD WINAPI AgentThreadSQLCommand(LPVOID lparam)
	{
		ArithmLog* sp = static_cast<ArithmLog*>(lparam);
		if (sp)
		{
			CoInitialize(NULL);
Loop:
			_ConnectionPtr conn;
			_RecordsetPtr rs;
			try
			{
				HRESULT hr1 = conn.CreateInstance(__uuidof(Connection));
				HRESULT hr2 = rs.CreateInstance(__uuidof(Recordset));
				if (SUCCEEDED(hr1) && SUCCEEDED(hr2))
				{
					conn->CursorLocation = adUseClient;
					HRESULT hr = conn->Open(sp->m_connectString.c_str(), _T(""),_T(""),adModeUnknown);///连接数据库
					if (SUCCEEDED(hr))
					{
						CCriticalSectionEx lock;
						while (!sp->m_exitThreadSQLCommand)
						{
							Sleep(1000);
							if (sp->m_queueSqlCmd.size())
							{
								if (lock.TryLock(&sp->m_csSqlCmd))
								{
									std::vector<wstring> vec = sp->m_queueSqlCmd;
									sp->m_queueSqlCmd.clear();
									lock.Unlock();
									for (unsigned i = 0; i < vec.size(); ++i)
									{
										sp->executeSQL(vec.at(i).c_str(), conn);
									}
								}
							}
						}
					}
				}
				if (rs != NULL && rs->State)
				{
					rs->Close();
				}
				if(conn != NULL && conn->State)
				{
					conn->Close(); ///如果已经打开了连接则关闭它
				}
			}
			catch(_com_error e)
			{ ///捕捉异常
				if (rs != NULL && rs->State)
				{
					rs->Close();
				}
				if(conn != NULL && conn->State)
				{
					conn->Close(); ///如果已经打开了连接则关闭它
				}
// 				_TraceLog.Trace(_T("[AgentThreadSQLCommand]查询数据库失败!错误信息:[%d]%s"),
// 					e.Error(), e.ErrorMessage());///显示错误信息
				if (!sp->m_exitThreadSQLCommand)
				{
					goto Loop;
				}
			}
			CoUninitialize();
		}
		return 0;
	}

protected:
	void addSqlCommand(LPCTSTR sqlText)
	{
		CCriticalSectionEx lock;
		lock.Lock(&m_csSqlCmd);
		wstring text = sqlText;
		m_queueSqlCmd.push_back(text);
	}
	void executeSQL(LPCTSTR cmdText, _ConnectionPtr connection)
	{
		try
		{
			connection->Execute(cmdText, NULL, adCmdText);
		}
		catch(_com_error e) // 捕捉异常
		{
// 			_TraceLog->Trace(_T("[executeSQL]查询数据库[ARITHM_DB]失败!错误信息:[%d]%s[%s]"),
// 				e.Error(), e.ErrorMessage(), cmdText);///显示错误信息
		}
	}
	void getAllRows(LPCTSTR cmdText, _ConnectionPtr connection, _RecordsetPtr rs)
	{
		if (rs->State)
		{
			rs->Close();
		}
		rs->Open(cmdText, _variant_t((IDispatch *)connection, true),
			adOpenForwardOnly, adLockReadOnly, adCmdText);
	}

protected:
	unsigned int makeKey(WORD roomID, WORD tableID, WORD chairID)
	{
		return (roomID << 12) | (tableID << 4) | (chairID);
	}
	void freeArithmData(tagArithmData* ArithmData)
	{
		CCriticalSectionEx lock;
		lock.Lock(&m_csAlloc);
		m_storageArithmLog.free(ArithmData);
	}
	tagArithmData* getArithmData(KEY_OF_ARITHM_DATA key)
	{
		ArithmDataMap::iterator iter = m_ArithmDataMap.find(key);
		if (iter != m_ArithmDataMap.end())
		{
			return iter->second;
		}
		return NULL;
	}
	tagArithmData* getArithmData(WORD roomID, WORD tableID, WORD chairID)
	{
		KEY_OF_ARITHM_DATA key = makeKey(roomID, tableID, chairID);
		return getArithmData(key);
	}
	bool addArithmData(tagArithmData* ArithmData)
	{
		if (!ArithmData) return false;
		DWORD key = makeKey(ArithmData->roomID, ArithmData->tableID, ArithmData->chairID);
		if (!getArithmData(key))
		{
			m_ArithmDataMap[key] = ArithmData;
			return true;
		}
		return false;
	}
	tagArithmData* allocArithmData()
	{
		CCriticalSectionEx lock;
		lock.Lock(&m_csAlloc);
		tagArithmData* ArithmData = m_storageArithmLog.alloc();
		if (ArithmData)
		{
			memset(ArithmData, 0, sizeof(tagArithmData));
		}
		return ArithmData;
	}

protected:
	wstring makeArithmDataFileName(WORD roomID, WORD tableID)
	{
		CString str;
		str.Format(_T("%s/Room%04d_Table%04d"), ARITHM_DIR, roomID, tableID);
		return str.GetString();
	}
	TraceInfo* getArithmDataFile(wstring key)
	{
		ArithmDataFileMap::iterator iter = m_ArithmDataFileMap.find(key);
		if (iter != m_ArithmDataFileMap.end())
		{
			return iter->second;
		}
		return NULL;
	}
	bool addArithmDataFile(wstring key, TraceInfo* traceInfo)
	{
		if (!traceInfo) return false;
		if (!getArithmDataFile(key))
		{
			m_ArithmDataFileMap[key] = traceInfo;
			return true;
		}
		return false;
	}

protected:
	ObjectAllocator<tagArithmData> m_storageArithmLog;
	ArithmDataMap m_ArithmDataMap;
	ArithmDataFileMap m_ArithmDataFileMap;
	std::vector<wstring> m_queueSqlCmd;
	wstring m_connectString;
	CRITICAL_SECTION m_csAlloc, m_csSqlCmd;
	HANDLE m_threadSQLCommand;
	bool m_exitThreadSQLCommand;
	TraceInfo* _TraceLog;
};