#include "sectionLocker.h"
void *ShowMe=NULL;
SectionLocker::SectionLocker(CRITICAL_SECTION &critsection,char *function,int line,void* object) {
	critsection;
	this->section=&critsection;
	Function=function;
	Line=line;
	Object=object;
	DWORD start=GetTickCount();
	if (TryEnterCriticalSection(section)==0) {
		char logBuffer[500];
		sprintf(logBuffer,"[BixVReader]Locking:Function:%s,Line:%i,Object:%p,Lock:%p",Function,Line,Object,section);
		OutputDebugStringA(logBuffer);
		ShowMe=section;
		EnterCriticalSection(section);
		DWORD end=GetTickCount();	
		sprintf(logBuffer,"[BixVReader]Elapsed:%i ms",end-start);
		OutputDebugStringA(logBuffer);
	}
}

SectionLocker::~SectionLocker() {
	if (section==ShowMe) {
		char logBuffer[500];
		sprintf(logBuffer,"[BixVReader]Unlocking:Function:%s,Line:%i,Object:%p,Lock:%p",Function,Line,Object,section);
		OutputDebugStringA(logBuffer);
		ShowMe=NULL;
	}
	LeaveCriticalSection(section);
}

SectionLogger::SectionLogger(char *section) {
	section;
	start=GetTickCount();
	char logBuffer[500];
	SectionName=section;
	sprintf(logBuffer,"[BixVReader]Start section:%s",SectionName);
	OutputDebugStringA(logBuffer);
}
SectionLogger::~SectionLogger() {
	DWORD end=GetTickCount();	
	char logBuffer[500];
	sprintf(logBuffer,"[BixVReader]End section:%s elapsed:%i",SectionName,end-start);
	OutputDebugStringA(logBuffer);
}

