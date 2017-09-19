#define CERTFILELOCATION "./curl-ca-bundle.crt"
#define GAMECHOICESLOCATION "./GameChoices.txt"
#define INSTALLERFORMATLOCATION "./InstallerFormatString.txt"
// Please use in format SEVENZIPLOCATION""EXTRACTZIPCOMMAND
// x is for extract with paths -o is for output path -aoa is for overwrite without prompt
#define EXTRACTZIPCOMMAND " x %s -o%s -aoa" 

#define PLAT_UNKNOWN 0
#define PLAT_WINDOWS 1
#define PLAT_LINUX 2
#if _WIN32
	#define PLATFORM PLAT_WINDOWS
#elif __unix__
	#define PLATFORM PLAT_LINUX
#else
	#define PLATFORM PLAT_UNKNOWN
#endif

#define ISDEBUG 1

#ifndef SEVENZIPLOCATION
	#if PLATFORM == PLAT_WINDOWS
		#define SEVENZIPLOCATION "7z.exe"
	#elif PLATFORM == PLAT_LINUX
		#define SEVENZIPLOCATION "./7za"
	#else
		#error PLATFORM constant not found. Either set it, or define SEVENZIPLOCATION
	#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
// main.h
	signed char checkFileExist(const char* location);
	typedef struct NathanLinkedList_t{
		char* memory;
		struct NathanLinkedList_t* nextEntry;
	}NathanLinkedList;
// My headers
#include "Download.h"

// =====================================================
char* urlGraphicsPatch;
char* urlVoicesPatch;
char* urlMangaGamerGraphics;
char* urlScriptPatch;
// =====================================================
signed char checkFileExist(const char* location){
	if( access( location, F_OK ) != -1 ) {
		return 1;
	} else {
		return 0;
	}
}
void quitApplication(){
	quitDownload();
}
// Removes all 0x0D and 0x0A from last two characters of string by moving null character.
void removeNewline(char** _toRemove){
	int _cachedStrlen = strlen(*_toRemove);
	int i;
	for (i=0;i!=2;i++){
		if (!(((*_toRemove)[_cachedStrlen-(i+1)]==0x0A) || ((*_toRemove)[_cachedStrlen-(i+1)]==0x0D))){
			break;
		}
	}
	(*_toRemove)[_cachedStrlen-i] = '\0';
}
// 1 based
NathanLinkedList* getLinkedList(NathanLinkedList* _startingList,int num){
	NathanLinkedList* listOn = _startingList;
	int i;
	for (i=0;i<num-1;i++){
		listOn=(listOn->nextEntry);
	}
	return listOn;
}
// 1 based
int getLinkedListLength(NathanLinkedList* _startingList){
	int i;
	for (i=1;;i++){
		if (_startingList->nextEntry!=NULL){
			_startingList = (_startingList->nextEntry);
		}else{
			break;
		}
	}
	return i;
}
// Adds to the end. Will return what you pass to it if what you passed points to NULL for memory
NathanLinkedList* addToLinkedList(NathanLinkedList* _startingList){
	if (_startingList->memory==NULL){
		return _startingList;
	}else{
		NathanLinkedList* tempList = calloc(1,sizeof(NathanLinkedList));
		NathanLinkedList* listOn = (getLinkedList(_startingList,getLinkedListLength(_startingList)));
		if (listOn->nextEntry){
			// ??? There's already another entry???
			printf("Problem add to list. Maybe we'll loose some entries.");
		}
		listOn->nextEntry=tempList;
		return tempList;
	}
}
void freeLinkedList(NathanLinkedList* _startingList){
	int i;
	NathanLinkedList* _currentListToFree = _startingList;
	NathanLinkedList* _nextListToFree = _startingList;
	int _cachedListLength = getLinkedListLength(_startingList);
	for (i=0;i<_cachedListLength;i++){
		free(_currentListToFree->memory);
		_nextListToFree = _currentListToFree->nextEntry;
		free(_currentListToFree);
		_currentListToFree = _nextListToFree;
	}
}
// Like getchar, but doesn't keep the buffer after.
char Goodgetchar(){
	char _userCharInput;
	do{
		_userCharInput = getchar();
	}while(_userCharInput=='\n' || _userCharInput==EOF);
	fflush(stdin);
	return _userCharInput;
}
// Returns malloc'd string of the game name chosen.
// Loads file from GAMECHOICESLOCATION
char* selectGame(){
	NathanLinkedList* _tempGamenameList = calloc(1,sizeof(NathanLinkedList));

	char* lastReadLine = malloc(256);
	FILE* fp = fopen(GAMECHOICESLOCATION, "r");
	while (fgets(lastReadLine, 256, fp)) {
		removeNewline(&lastReadLine);
		NathanLinkedList* _tempAddList = addToLinkedList(_tempGamenameList);
		_tempAddList->memory = malloc(strlen(lastReadLine)+1);
		strcpy(_tempAddList->memory,lastReadLine);
	}
	fclose(fp);
	
	char _userChosenGame;
	printf("\n==========");
	do{
		printf("\nPlease enter the number for the game you want to patch.\n==========\n");
		int i;
		for (i=0;i<getLinkedListLength(_tempGamenameList);i++){
			printf("(%d) %s\n",(i+1),getLinkedList(_tempGamenameList,i+1)->memory);
		}
		_userChosenGame = Goodgetchar();
		if (_userChosenGame>=58 || _userChosenGame<=48 || _userChosenGame-48>getLinkedListLength(_tempGamenameList)){
			printf("==========\n(Previous input invalid. Enter a number 1 through %d)",getLinkedListLength(_tempGamenameList));
			_userChosenGame=0;
		}else{
			_userChosenGame-=48;
		}
	}while(_userChosenGame==0);
	char* _userChosenGameString = malloc(strlen(getLinkedList(_tempGamenameList,_userChosenGame)->memory)+1);
	strcpy(_userChosenGameString,getLinkedList(_tempGamenameList,_userChosenGame)->memory);
	freeLinkedList(_tempGamenameList);
	free(lastReadLine);
	return _userChosenGameString;
}
// Uses format string from INSTALLERFORMATLOCATION
// Returns malloc'd complete url
char* getOfficialInstallerUrl(char* _userChosenGame){
	// Read the format string
	char* lastReadLine = malloc(256);
	FILE* fp = fopen(INSTALLERFORMATLOCATION, "r");
	fgets(lastReadLine, 256, fp);
	fclose(fp);
	removeNewline(&lastReadLine);
	// Holds the format string after the big buffer has been freed
	char* _officialUrlFormatString = malloc(strlen(lastReadLine)+1);
	strcpy(_officialUrlFormatString,lastReadLine);
	free(lastReadLine);
	// Holds the completed url
	char* _officialUrl = malloc(strlen(_officialUrlFormatString)+1+strlen(_userChosenGame));
	sprintf(_officialUrl,_officialUrlFormatString,_userChosenGame);
	free(_officialUrlFormatString);
	return _officialUrl;
}
NathanLinkedList* GetUrls(char* batchFileURL){
	NathanLinkedList* _tempUrlList = calloc(1,sizeof(NathanLinkedList));
	size_t sizeDownloadedBatchFile;
	char* downloadedBatchFile;
	downloadWebpageData(batchFileURL,&downloadedBatchFile,&sizeDownloadedBatchFile);
	char str[] ="- This, a sample string.";
	char* lastFoundLine;
	lastFoundLine = strtok (downloadedBatchFile,"\n");
	while (lastFoundLine != NULL){
		char* _urlSearchResult = strstr(lastFoundLine,"http");
		if (_urlSearchResult!=NULL){
			int i;
			for (i=0;i<strlen(_urlSearchResult);i++){
				if (_urlSearchResult[i]==' '){
					break;
				}
			}
			NathanLinkedList* _tempAddList = addToLinkedList(_tempUrlList);
			_tempAddList->memory = malloc(i+1);
			strncpy(_tempAddList->memory,_urlSearchResult,i);
			(_tempAddList->memory)[i]='\0';
		}
		lastFoundLine = strtok (NULL, "\n");
	}
	return _tempUrlList;
}
// Downloads first 4 URLs of linked list
// First one is to ./CG.zip
// Second one is to ./Voices.zip
// Third one is to ./CGAlt.zip
// Fourth one is to ./Patch.zip
void downloadListURLs(NathanLinkedList* urlList){

	#if ISDEBUG == 1
		if (checkFileExist("./CG.zip") && checkFileExist("./Voices.zip") && checkFileExist("./CGAlt.zip") && checkFileExist("./Patch.zip")){
			printf("This is debug mode, and the zip files already exist. Would you like to redownload them? (y/n)");
			char _userCharInput = Goodgetchar();
			if (_userCharInput!='y'){
				return;
			}
		}
	#endif


	char* CGURL;
	char* VoicesURL;
	char* CGAltURL;
	char* PatchURL;

	CGURL = malloc(strlen(getLinkedList(urlList,1)->memory)+1);
	strcpy(CGURL,getLinkedList(urlList,1)->memory);
	removeNewline(&CGURL);
	VoicesURL = malloc(strlen(getLinkedList(urlList,2)->memory)+1);
	strcpy(VoicesURL,getLinkedList(urlList,2)->memory);
	removeNewline(&VoicesURL);
	CGAltURL = malloc(strlen(getLinkedList(urlList,3)->memory)+1);
	strcpy(CGAltURL,getLinkedList(urlList,3)->memory);
	removeNewline(&CGAltURL);
	PatchURL = malloc(strlen(getLinkedList(urlList,4)->memory)+1);
	strcpy(PatchURL,getLinkedList(urlList,4)->memory);
	removeNewline(&PatchURL);

	printf("Downloading PS3 graphics\n%s\n",CGURL);
	downloadToFile(CGURL,"./CG.zip");
	printf("Downloading PS3 voices\n%s\n",VoicesURL);
	downloadToFile(VoicesURL,"./Voices.zip");
	printf("Downloading MangaGamer graphics\n%s\n",CGURL);
	downloadToFile(CGAltURL,"./CGAlt.zip");
	printf("Downloading patch\n%s\n",PatchURL);
	downloadToFile(PatchURL,"./Patch.zip");

	free(CGURL);
	free(VoicesURL);
	free(CGAltURL);
	free(PatchURL);
}
// Easy ZIP extraction with 7ZIP or p7ZIP
void extractZIP(char* sourceFile, char* destDirectory){
	char* _extractionCommand = malloc(strlen(SEVENZIPLOCATION)+strlen(EXTRACTZIPCOMMAND)+1+strlen(sourceFile)+strlen(destDirectory));
	sprintf(_extractionCommand,SEVENZIPLOCATION""EXTRACTZIPCOMMAND,sourceFile,destDirectory);
	system(_extractionCommand);
	free(_extractionCommand);
}
/*============================================================================*/
// Returns 0 if required file is missing, 1 otherwise.
char checkRequiredFiles(){
	if (checkFileExist(GAMECHOICESLOCATION)==0){
		printf(GAMECHOICESLOCATION" is missing! Please redownload.");
		return 0;
	}
	if (checkFileExist(INSTALLERFORMATLOCATION)==0){
		printf(INSTALLERFORMATLOCATION" is missing! Please redownload.");
		return 0;
	}
	if (checkFileExist(SEVENZIPLOCATION)==0){
		printf(SEVENZIPLOCATION" is missing! Please redownload.");
		return 0;
	}
	return 1;
}
void init(){
	initDownload();
}
/*============================================================================*/
int main(int argc, char *argv[]){
	// TODO - CHeck for leftover folders from previous extraction?
		// Even though I move them if the instillation succeeds, the user could cancel halfway through
	if (!checkRequiredFiles()){
		return 1;
	}
	init();
	char* userChosenGame = selectGame();
	char* batchFileURL = getOfficialInstallerUrl(userChosenGame);
	printf("Downloading script...\n");
	NathanLinkedList* urlList = GetUrls(batchFileURL);
	free(batchFileURL);
	free(userChosenGame);
	// Make sure we got exactly 4 URLs
	if (getLinkedListLength(urlList)!=4){
		printf("Incorrect number of URLs found. %d URLs were found, when exactly 4 were expected.\nURLs found:\n",getLinkedListLength(urlList));
		int i;
		for (i=0;i<getLinkedListLength(urlList);i++){
			printf("(%d) %s\n",(i+1),getLinkedList(urlList,i+1)->memory);
		}
		return 1;
	}
	downloadListURLs(urlList);
	freeLinkedList(urlList);

	extractZIP("./CG.zip","./ExtractedCG");
	extractZIP("./Voices.zip","./ExtractedVoices");
	extractZIP("./CGAlt.zip","./ExtractedCGAlt");
	extractZIP("./Patch.zip","./ExtractedPatch");

	quitApplication();
	return 0;
}
