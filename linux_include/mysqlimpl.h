/*
	mysql database access class.
    gcc -lmysqlclient
    
how to query:
	CMysqlImpl mysql;
	mysql.Connect("host","user","pass","db",port=0);
	mysql.OpenTable("table_test");
	wihle (!mysql.IsEOF()) {
		const chra* ptr = mysql.GetString("field");
        mysql.MoveNext();
	}

how to update:
	mysql.AddNew("table","id=1" or NULL);
	mysql["id"]=1;
	mysql["name"]="lsm";
	mysql.Update();
*/

#ifndef __MYSQLIMPL_HEADER__
#define __MYSQLIMPL_HEADER__

#include <string>
#include <sstream>
#include <stdlib.h>
#include <memory.h>
#include <lsm/types.h>
#include <lsm/smartmem.h>
#include <mysql/mysql.h>
#include <lsm/mysqlbind.h>

#ifdef WIN32
#pragma comment(lib,"mysqlclient.lib")
#pragma comment(lib,"ws2_32.lib")
#endif//WIN32

class CMysqlImpl
{
public://Constract.
	CMysqlImpl()
	{
		m_nLastError=0;
		m_strLastErr[0]='\0';
		m_bEOF = TRUE;
		m_pRS  = NULL;
		m_pResult = NULL;
		m_nFields = 0;
		m_pFields = NULL;
		mysql=newMysqlPtr();
	}

	virtual ~CMysqlImpl()
	{
		if (mysql) {
		    Close();
		}
	}

public:// Query.
	long Execute(const char* sql)
	{
		long nStatus=-1;
		do
		{
			// check if the mysql.
			if (!mysql) break;
			m_nLastError = mysql_query(mysql,"Set Names 'GBK'");
			if (S_OK != m_nLastError) {
				SetErrInfo("Set charset to gbk failed.");
			}

			// try to execute the special sql.
			nStatus = mysql_query(mysql,sql);

		} while(0);
	//Cleanup:
		return(nStatus);
	}

	// try to open the special table.
	long OpenTable(const char* sql,long* plCount=NULL)
	{
		long nStatus=-1;
		do
		{
			// try to execute the special sql now.
			m_nLastError = mysql_query(mysql,sql);
			if (S_OK != m_nLastError) {
				nStatus=m_nLastError;
				break;
			}

			// try to update the special status now.
			nStatus = S_OK;
			m_bEOF  = TRUE;
			m_pResult = mysql_store_result(mysql);
			if (!m_pResult) {
				SetErrInfo("mysql_store_result failed.");
			}

			//get count now.
			if (plCount) {
			   *plCount = (long)mysql_num_rows(m_pResult);
			}

			// try to get all the fields now.
			nStatus = FetchFields();
			if (S_OK != nStatus) {
				break;
			}

			// try to fetch all the result value now.
			m_pRS = mysql_fetch_row(m_pResult);
			if (!m_pRS) {
				mysql_free_result(m_pResult);
				m_pResult = NULL;
			}

			/// update eof.
			m_bEOF = FALSE;

		} while(0);
	//Cleanup:
		return (nStatus);
	}

	// is connected now.
	BOOL IsConnected()
	{
		BOOL bSucc=FALSE;
		do
		{
			if (!mysql) break;
			const char* sql = "select 1=1";
			long nError = mysql_query(mysql,sql);
			if (S_OK == nError) {
				bSucc=TRUE;
			}

		} while(0);
	//Cleanup:
		return (bSucc);
	}

	// check eof.
	BOOL IsEOF()
	{
		return (m_bEOF);
	}

	// try to move next.
	BOOL MoveNext()
	{
		BOOL bSucc=FALSE;
		do
		{
			m_bEOF=TRUE;
			m_pRS=mysql_fetch_row(m_pResult);
			if (m_pRS) {
				m_bEOF=FALSE;
				bSucc=TRUE;
			}

		} while(0);
	//Cleanup:
		return (bSucc);
	}

	// try to get the string value now.
	const char* GetString(const char* fieldname,const char* defval=NULL)
	{
		const char* ptr=defval;
		do
		{
			// try to get the special item value.
			long index = GetFieldIndex(fieldname);
			if (index<0) break;
			// the value item.
			ptr = m_pRS[index];

		} while(0);
	//Cleanup:
		return (ptr);
	}

	// try to get the special string item value now.
	long GetLong(const char* name,long defval=-1)
	{
		long val=defval;
		do
		{
			// try to get the special item index.
			long index = GetFieldIndex(name);
			if (index<0) break;
			// try to get the value item.
			const char* tmp=m_pRS[index];
			val=atol(tmp);
		} while(0);
	//Cleanup:
		return (val);
	}

	// try to get the blob data item content value item now.
	Smartmem GetBlobData(const char* name,uint32* pnSize)
	{
		Smartmem mem;
		do
		{
			long index=GetFieldIndex(name);
			if (index<0) {
				break;
			}
			if (MYSQL_TYPE_BLOB == m_pFields[index].type)
			{
				// try to get the size of item.
				uint32 size = GetBlobSize(name);
				if (pnSize) *pnSize=size;
				/// size.
				if (size) 
				{
					/// try to get content value.
					mem.Alloc(m_pRS[index],size);
				}

			}

		} while(0);
	//Cleanup:
		return (mem);
	}

	// close table now.
	void CloseTable()
	{
		if (m_pResult)
		{
			mysql_free_result(m_pResult);
			m_pResult=NULL;
		}
	}

public://Transaction.
	long Transaction()
	{
		long nStatus=-1;
		do
		{
			if (!mysql) break;
			nStatus=mysql_query(mysql,"START TRANSACTION");
			if (S_OK==nStatus) {
				m_bTrans=TRUE;
			}

		} while(0);
	//Cleanup:
		return (nStatus);
	}

	long Commit()
	{
		long nStatus=-1;
		do
		{
			if (!mysql) break;
			nStatus=mysql_query(mysql,"COMMIT");
			m_bTrans=FALSE;
		} while(0);
	//Cleanup:
		return (nStatus);
	}

	long Rollback()
	{
		long nStatus=-1;
		do
		{
			if (!m_pRS) break;
			nStatus=mysql_query(mysql,"ROLLBACK");
			m_bTrans=FALSE;
		} while(0);
	//Cleanup:
		return (nStatus);
	}

public://Update,find like "id=1".
	long Addnew(const char* szTable,const char* szFind=NULL)
	{
		long nStatus=-1;
		do
		{
			m_szFind[0]='\0';
			// try to update the parameter item value now.
			strncpy(m_szTable,szTable,sizeof(m_szTable));
			if (szFind) {
			    strncpy(m_szFind,szFind,sizeof(m_szFind));
			}

			// Succeeded.
			nStatus=S_OK;
		} while(0);
	//Cleanup:
		return (nStatus);
	}

	// try to add new field value item now.
	MysqlBind& operator[](const char* name)
	{
		return (blist[name]);
	}

	// update now.
	long Update()
	{
		long nStatus=-1;
		do
		{
			// try to update the data now.
			nStatus = BuildStmt();
			if (S_OK != nStatus){
				break;
			}

			// try to bind all the parameter now.
			nStatus=BindStmtData();
			if (S_OK!=nStatus) {
				break;
			}

			// try to execute the special sql item now.
			nStatus=mysql_stmt_execute(m_pUpdateStmt);
			if (!m_bTrans) {
				mysql_stmt_close(m_pUpdateStmt);
				m_pUpdateStmt=NULL;
			}

			// free data now.
			if (m_pBinds) {
				free(m_pBinds);
				m_pBinds=NULL;
			}

		} while(0);
	//Cleanup:
		return (nStatus);
	}	


public://connection.
	// try to connect to the special remote host now.
	long Connect(const char* szhost,
		     const char* szuser,
		     const char* szpass,
		     const char* szdb,
		     long port=0)
	{
		long nStatus=-1;
		do
		{
			// check if the pointer.
			if (!mysql) {
				 mysql=newMysqlPtr();
				// again now.
				if (!mysql) {
					nStatus=-2;
				}
			}

			// try to connect to the special database.
			if (mysql_real_connect(mysql,szhost,szuser,szpass,szdb,port,NULL,0)) {
				nStatus=S_OK;
			}	else
			{
				nStatus=mysql_errno(mysql);
			}

		} while(0);
	//Cleanup:
		return (nStatus);
	}

	//// Close()
	void Close()
	{
		if (mysql) {
			mysql_close(mysql);
			free(mysql);
			mysql=NULL;
		};
		
	}


protected://protect function.
	MYSQL* newMysqlPtr()
	{
		mysql = (MYSQL*)malloc(sizeof(MYSQL));
		if (mysql) {
			memset(mysql,0,sizeof(MYSQL));
			mysql_init(mysql);
			mysql->reconnect = 1;	
		}
	//Cleanup:
		return (mysql);
	}

	// set the last error information.
	void SetErrInfo(const char* info)
	{
		strncpy(m_strLastErr,info,255);
	}

	// fetch
	long FetchFields()
	{
		long nStatus=-1;
		do
		{
			// check the result now.
			if (!m_pResult) break;
			m_nFields = mysql_num_fields(m_pResult);
			if (!m_nFields) break;
		
			// try to get all the fields parameter.
			m_pFields = mysql_fetch_fields(m_pResult);
			if (m_pFields) {
				nStatus=S_OK;
			} else
			{
				SetErrInfo("fetch fields failed.");
				nStatus=-5;
			}

		} while(0);	
	//Cleanup:
		return (nStatus);
	}

	// try to query if the special field exist.
	long GetFieldIndex(const char* name)
	{
		long index=-1;
		do
		{
			if (!m_nFields) break;
			for (int i=0;i<m_nFields;i++)
			{
				if (strcasecmp(name,m_pFields[i].name)==0)
				{
					index=i;
					break;
				}
			}

		} while (0);
	//Cleanup:
		return (index);
	}

        // try to get the blob data size.
        long GetBlobSize(const char* name)
        {
                long size=0;
                do
                {
                        long index=GetFieldIndex(name);
                        if (index<0) break;
                        // try to fetch all the item length vlaue for user now.
                        unsigned long* plLens=mysql_fetch_lengths(m_pResult);
                        if (!plLens) break;
                        size = plLens[index];

                } while(0);
        //Cleanup:
                return (size);
        }

	// build sql now.
	long BuildStmt()
	{
		long nStatus=-1;
		BOOL bUpdate=FALSE;
		char szItem[1024]={0};
		char szFields[1024]={0};
		char szQuest[1024]={0};

		do
		{
			std::ostringstream oss;
			// try to check table now.
			if (!strlen(m_szTable)) {
				nStatus=-2;
				break;
			}

			/// check if insert sql.
			if (strlen(m_szFind)) {
				bUpdate=TRUE;
			}


			// try to build the quest list.
			for (int i=0;i<blist.size();i++)
			{
				MysqlBind* pb=blist[i];
				if (pb)
				{
					// check update.
					if (!bUpdate)
					{
						// build item insert sql item.
						strncpy(szItem,pb->name,1024);
						strcat(szItem,",");
						strcat(szFields,szItem);
						strcat(szQuest,"?,");
					}
					else
					{
						/// build item update sql item.
						strncpy(szItem,pb->name,1024);
						strcat(szItem,"=?,");
						strcat(szFields,szItem);
					}
				}			
			}

			// reset the last char to the value of '\0'.
			if (',' == szFields[strlen(szFields)-1]) {
				szFields[strlen(szFields)-1]='\0';
			}

			if (',' == szQuest[strlen(szQuest)-1]) {
				szQuest[strlen(szQuest)-1]='\0';
			}

			// update insert.
			if (!bUpdate) {
				oss << "insert into " << m_szTable << "(" << szFields << ") values(" << szQuest << ");";
			}  else
			{
				oss << "update " << m_szTable << " set " << szFields << " where " << m_szFind << ";";
			}

			// initialize the sql update item now.
			m_pUpdateStmt=mysql_stmt_init(mysql);
			if (!m_pUpdateStmt) {
				nStatus=-5;
				break;
			}

			std::string sql=oss.str();
			// try to prepare the special sql string for later user item value.
			nStatus=mysql_stmt_prepare(m_pUpdateStmt,sql.c_str(),sql.length());

		} while(0);
	//Cleanup:
		return (nStatus);
	}

	// try to bind data.
	long BindStmtData()
	{
		long nStatus=-1;
		do
		{
			long size=blist.size();
			if (!size) {
				nStatus=S_OK;
				break;
			}

			// try to allocate the special data item content value. 
			m_pBinds=(MYSQL_BIND*)malloc(size*sizeof(MYSQL_BIND));
			if (!m_pBinds) {
				nStatus=-5;
				break;
			}

			// try to get data content.
			for (int i=0;i<size;i++) {
				MysqlBind* pb=blist[i];
				memcpy(&m_pBinds[i],pb->bind(),sizeof(MYSQL_BIND));
			}

			// try to bind the parameter content item value data.
			nStatus=mysql_stmt_bind_param(m_pUpdateStmt,m_pBinds);
		} while(0);
	//Cleanup:
		return (nStatus);
	}

protected:
	long m_nLastError;
	char m_strLastErr[255];
	char m_szTable[255];		// the update table name.
	char m_szFind[255];		// the find sql string.
	MYSQL_RES* m_pResult;
	MYSQL_FIELD* m_pFields;
	MYSQL_BIND*  m_pBinds;		// binds data for temp user.
	MYSQL_STMT*  m_pUpdateStmt;	// the update stmt sql string.
	MysqlBindList blist;		// mysql bind parameter list.
	BOOL    m_bTrans;		// is current transaction.
	BOOL      m_bEOF;
	LONG   m_nFields;
	MYSQL_ROW  m_pRS;
	MYSQL*     mysql;
};

#endif//__MYSQLIMPL_HEADER__
