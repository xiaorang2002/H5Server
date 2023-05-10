
#include <string>

extern "C"{
#include "sqlite3.h"
};

using namespace std;
int main(int argc, char* argv[])
{
	char* msg = NULL;
	sqlite3* db = NULL;
	int rc = sqlite3_open("test.db",&db);

	// rc = sqlite3_key(db,"123",3);

	// create the special statement of table.
	string sql = "CREATE TABLE IF NOT EXISTS test_table(id integer PRIMARY KEY,uid text,pwd text);";
	rc = sqlite3_exec(db,sql.c_str(),NULL,NULL,&msg);

	sql = "insert into test_table(uid,pwd) values('test1','test2');";
	rc = sqlite3_exec(db,sql.c_str(),NULL,NULL,&msg);

	return 0;
}





