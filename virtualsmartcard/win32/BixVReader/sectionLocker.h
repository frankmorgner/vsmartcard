#pragma once
#include "internal.h"

class SectionLocker {
	char *Function;
	int Line;
	void *Object;
public:
	CRITICAL_SECTION *section;
	
	SectionLocker(CRITICAL_SECTION &critsection,char *function,int line,void* object);
	//SectionLocker(CRITICAL_SECTION &section);
	~SectionLocker();
};

class SectionLogger {
	DWORD start;
	char *SectionName;
	public:
	SectionLogger(char *section);
	~SectionLogger();
};

#define lock(x) lockObj(x,__FUNCTION__,__LINE__,NULL)