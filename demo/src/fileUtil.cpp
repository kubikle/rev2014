
#include <windows.h>
#include <map>
#include <string>

using namespace std;

std::map<string, FILETIME> files;
bool IsFileModified(string shader) {
#ifndef SYNC_PLAYER
	HANDLE hFile;
	FILETIME ftWrite;	

	hFile = CreateFile(shader.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, 0, NULL);

	GetFileTime(hFile, NULL, NULL, &ftWrite);

	CloseHandle(hFile);


	if(ftWrite.dwLowDateTime != files[shader].dwLowDateTime) {
		if(files[shader].dwLowDateTime != NULL) {
			Sleep(1000);
		}
		files[shader] = ftWrite;
		return true;
	}
	return false;
#else
	return true;
#endif
}