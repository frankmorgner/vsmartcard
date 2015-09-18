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
		OutputDebugStringA("[BixVReader]RACE!");
		char logBuffer[500];
		sprintf(logBuffer,"[BixVReader]Richiesto da:Function:%s,Line:%i,Object:%Ix,Lock:%Ix",Function,Line,Object,section);
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
		sprintf(logBuffer,"[BixVReader]Posseduto da:Function:%s,Line:%i,Object:%Ix,Lock:%Ix",Function,Line,Object,section);
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

