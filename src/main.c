#define _GNU_SOURCE

#define CERTFILELOCATION "./curl-ca-bundle.crt"
#define GAMECHOICESLOCATION "./GameChoices.txt"
#define INSTALLERFORMATLOCATION "./InstallerFormatString.txt"

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
	return 1;
}
void init(){
	initDownload();
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
int main(int argc, char *argv[]){
	if (!checkRequiredFiles()){
		return 1;
	}
	init();
	char* userChosenGame = selectGame();
	char* batchFileURL = getOfficialInstallerUrl(userChosenGame);
	printf("You chose %s\n",userChosenGame);
	printf("official url is %s\n",batchFileURL);
	
	free(userChosenGame);
	//downloadToFile("http://example.com","./google2.html");
	//downloadWebpageData(&noobStruct,"https://www.google.com");
	quitApplication();
	return 0;
}
