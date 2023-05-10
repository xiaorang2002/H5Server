
#include <stdio.h>
#include <stdlib.h>
#include <xml/XmlParser.h>

int main(int argc,char* argv[])
{
	CXmlParser xml;

	xml.Load("test.xml");

	xml.Sel("/global");

	const char* ptr = xml.GetString("name");
	printf("name:[%s]\n",ptr);

	int age = xml.GetLong("age");
	printf("age:[%d]\n",age);

	printf("all done!\n");

	return 0;
}


