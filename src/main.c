#define VERSION 1
#define ISDEBUG 0

#define CERTFILELOCATION "./curl-ca-bundle.crt"
#define GAMECHOICESLOCATION "./GameChoices.txt"
#define INSTALLERFORMATLOCATION "./InstallerFormatString.txt"

#define INSTALLEFRORMATLOCATIONVITA "./InstallerFormatStringVita.txt"
// Please use in format SEVENZIPLOCATION""EXTRACTZIPCOMMAND
// x is for extract with paths -o is for output path -aoa is for overwrite without prompt
#define EXTRACTZIPCOMMAND " x \"%s\" -o\"%s\" -aoa" 

#define DOWNLOADLIST_ALL 0
#define DOWNLOADLIST_CG 1
#define DOWNLOADLIST_VOICES 2
#define DOWNLOADLIST_CGALT 3
#define DOWNLOADLIST_PATCH 4
#define DOWNLOADLIST_HIGURASHIVITA 5

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
#ifndef SEVENZIPLOCATION
	#if PLATFORM == PLAT_WINDOWS
		#define SEVENZIPLOCATION "7z.exe"
	#elif PLATFORM == PLAT_LINUX
		#define SEVENZIPLOCATION "./7za"
	#else
		#error PLATFORM constant not found. Either set it, or define SEVENZIPLOCATION
	#endif
#endif
#if PLATFORM == PLAT_LINUX
	// First string is home directory, including slash
	#define DEFAULTSTEAMDIR "%s/.steam/steam/steamapps/common/"
	#define FOLDERDELETIONCOMMAND "rm -r \"%s\""
	#define ALLFILEDELETECOMMAND "rm \"%s/\"*"
	#define SLASH "/"
#elif PLATFORM == PLAT_WINDOWS
	// No string arguments.
	#define DEFAULTSTEAMDIR "C:\\Program Files (x86)\\Steam\\steamapps\\common\\"
	#define FOLDERDELETIONCOMMAND "rmdir \"%s\" /S /Q"
	#define ALLFILEDELETECOMMAND "del \"%s\\*\" /S /Q > nul"
	#define SLASH "\\"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#if PLATFORM == PLAT_LINUX
	#include <sys/types.h>
	#include <pwd.h>
#endif
// main.h
	signed char checkFileExist(const char* location);
// My headers
#include "Download.h"
#include "LinkedList.h"
// =====================================================
// =====================================================
#if PLATFORM == PLAT_WINDOWS
	// getline replacement for Windows
	size_t getline(char **lineptr, size_t *n, FILE *stream) {
		/* This code (applies to this method only) is public domain -- Will Hartung 4/9/09 */
		char *bufptr = NULL;
		char *p = bufptr;
		size_t size;
		int c;
	
		if (lineptr == NULL) {
			return -1;
		}
		if (stream == NULL) {
			return -1;
		}
		if (n == NULL) {
			return -1;
		}
		bufptr = *lineptr;
		size = *n;
	
		c = fgetc(stream);
		if (c == EOF) {
			return -1;
		}
		if (bufptr == NULL) {
			bufptr = malloc(128);
			if (bufptr == NULL) {
				return -1;
			}
			size = 128;
		}
		p = bufptr;
		while(c != EOF) {
			if ((p - bufptr) > (size - 1)) {
				size = size + 128;
				bufptr = realloc(bufptr, size);
				if (bufptr == NULL) {
					return -1;
				}
			}
			*p++ = c;
			if (c == '\n') {
				break;
			}
			c = fgetc(stream);
		}
	
		*p++ = '\0';
		*lineptr = bufptr;
		*n = size;
	
		return p - bufptr - 1;
	}
#endif
#if PLATFORM == PLAT_LINUX
	char* getHomeDirectory(){
		struct passwd *pw = getpwuid(getuid());
		if (strlen(pw->pw_dir)==0){
			return NULL;
		}
		char* _toReturnString = malloc(strlen(pw->pw_dir));
		strcpy(_toReturnString,pw->pw_dir);
		return _toReturnString;
	}
	void itoa(int _num, char* _buffer, int _uselessBase){
		sprintf(_buffer, "%d", _num);
	}
#endif
void DeleteFolder(char* folderPath){
	perror("This function should never be used. Deleting CompiledUpdateScripts folder breaks the game.");
	return;
	char* _deletionCommand = malloc(strlen(FOLDERDELETIONCOMMAND)+strlen(folderPath)+1);
	sprintf(_deletionCommand,FOLDERDELETIONCOMMAND,folderPath);
	system(_deletionCommand);
	free(_deletionCommand);
}
// Folder should not end with slash
void DeleteAllInFolder(char* folderPath){
	char* _deletionCommand = malloc(strlen(ALLFILEDELETECOMMAND)+strlen(folderPath)+1);
	sprintf(_deletionCommand,ALLFILEDELETECOMMAND,folderPath);
	system(_deletionCommand);
	free(_deletionCommand);
}
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
// Like getchar, but uses getline and returns the first char the user enters
char Goodgetchar(){
	char* userDataFolderPathInput=NULL;
	size_t userDataFolderPathInputBufferSize;
	getline(&userDataFolderPathInput,&userDataFolderPathInputBufferSize,stdin);
	char _cachedChar = userDataFolderPathInput[strlen(userDataFolderPathInput)-2]; // Subtract two as there is null char
	free(userDataFolderPathInput);
	return _cachedChar;
}
void printDivider(){
	printf("============\n");
}
// Returns malloc'd string of the game name chosen.
// Loads file from GAMECHOICESLOCATION
char selectGame(NathanLinkedList* _tempGamenameList){
	char _userChosenGame;
	do{
		printDivider();
		printf("Please enter the number for the game you want to patch.\n");
		printDivider();
		int i;
		for (i=0;i<getLinkedListLength(_tempGamenameList);i++){
			printf("%d) %s\n",(i+1),getLinkedList(_tempGamenameList,i+1)->memory);
		}
		_userChosenGame = Goodgetchar();
		if (_userChosenGame>=58 || _userChosenGame<=48 || _userChosenGame-48>getLinkedListLength(_tempGamenameList)){
			printDivider();
			printf("(Previous input invalid. Enter a number 1 through %d)",getLinkedListLength(_tempGamenameList));
			_userChosenGame=0;
		}else{
			_userChosenGame-=48;
		}
	}while(_userChosenGame==0);
	return _userChosenGame;
}
// Uses format string from _passedFormat
// Returns malloc'd complete url
char* getOfficialInstallerUrl(char* _userChosenGame, char* _passedFormat){
	// Read the format string
	char* lastReadLine = malloc(256);
	FILE* fp = fopen(_passedFormat, "r");
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
			removeNewline(&(_tempAddList->memory));
		}
		lastFoundLine = strtok (NULL, "\n");
	}
	return _tempUrlList;
}
// Easy ZIP extraction with 7ZIP or p7ZIP
void extractZIP(char* sourceFile, char* destDirectory){
	char* _extractionCommand = malloc(strlen(SEVENZIPLOCATION)+strlen(EXTRACTZIPCOMMAND)+1+strlen(sourceFile)+strlen(destDirectory));
	sprintf(_extractionCommand,SEVENZIPLOCATION""EXTRACTZIPCOMMAND,sourceFile,destDirectory);
	system(_extractionCommand);
	free(_extractionCommand);
	if (remove(sourceFile)!=0){
		printf("Failed to delete %s\n",sourceFile);
	}
}
char checkDirectoryExists(char* filepath){
	DIR* dir = opendir(filepath);
	if (dir){
		closedir(dir);
		return 1;
	}else if (ENOENT == errno){
		return 0;
	}else{
		printf("Could not open directory %s\nThe world is over.\n",filepath);
		return 0;
	}
}
// Assumes lists are uninitialized
char ReadGameList(NathanLinkedList** _gameNameList, NathanLinkedList** _gameFolderList){
	*_gameNameList = calloc(1,sizeof(NathanLinkedList));
	*_gameFolderList = calloc(1,sizeof(NathanLinkedList));

	char* lastReadLine = malloc(256);
	FILE* fp = fopen(GAMECHOICESLOCATION, "r");

	char isReadingGameFolders=0;

	while (fgets(lastReadLine, 256, fp)) {
		removeNewline(&lastReadLine);
		if (strcmp(lastReadLine,"---")==0){
			isReadingGameFolders=1;
			continue;
		}
		NathanLinkedList* _tempAddList;
		if (isReadingGameFolders==1){
			_tempAddList = addToLinkedList(*_gameFolderList);
		}else{
			_tempAddList = addToLinkedList(*_gameNameList);
		}
		_tempAddList->memory = malloc(strlen(lastReadLine)+1);
		strcpy(_tempAddList->memory,lastReadLine);
	}
	fclose(fp);
	free(lastReadLine);
	if (isReadingGameFolders==0){
		printf("!!!!!!!!!!!!!!!!!!!!\nError, \"---\" not found.\nThis a line that seperates the game name list from the game folder list. Please add the three dashes and the game folder names.\n!!!!!!!!!!!!!!!!!!!!\n");
		return 0;
	}
	return 1;
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
	printf("========================\nHigurashi: When They Cry PS3 Voices & Graphics auto installer\nv%d\n",VERSION);
	if (!checkRequiredFiles()){
		return 1;
	}
	init();
	// Holds the names of the games. Ex: onikakushi
	NathanLinkedList* gameList;
	// Holds the names of the game's folders relative to steamapps/common/ . Ex: Higurashi 02 - Watanagashi\HigurashiEp02_Data
	NathanLinkedList* gameFolderList;
	if (!ReadGameList(&gameList,&gameFolderList)){
		return 1;
	}

	int userChosenGame = selectGame(gameList);
	printf("User chose %d\n",userChosenGame);

	// Updater options
	short userUpdateChoice;
	do{
		printDivider();
		printf("What would you like to do?\n");
		printf("1) Install the patch. (Do this if it's your first time.)\n");
		printf("2) Update the PS3 graphics patch\n");
		printf("3) Update the Voice patch\n");
		printf("4) Update the MangaGamer graphics patch\n");
		printf("5) Update the patch scripts (Update folder)\n");
		printf("6) Install the older Higurashi-Vita compatible version.\n");

		userUpdateChoice = (short)Goodgetchar();
		if (userUpdateChoice>=58 || userUpdateChoice<=48 || userUpdateChoice-48>6){
			printDivider();
			printf("(Previous input invalid. Enter a number 1 through %d)\n",5);
			userUpdateChoice=-1;
		}else{
			userUpdateChoice-=49;
			if (userUpdateChoice!=DOWNLOADLIST_ALL && userUpdateChoice!=DOWNLOADLIST_HIGURASHIVITA){
				printDivider();
				printf("Just for clarification, you have chosen to update a SINGLE COMPONENT of the entire patch.\nIf you haven't installed the entire patch to this specific game before, this will not do anything for you.\nIs this okay? (y/n)\n");
				char _userYesOrNo = Goodgetchar();
				if (tolower(_userYesOrNo)!='y'){
					userUpdateChoice=-1;
				}
			} 
		}
	}while(userUpdateChoice==-1);

	
	char* batchFileURL;
	if (userUpdateChoice==DOWNLOADLIST_HIGURASHIVITA){
		batchFileURL = getOfficialInstallerUrl(getLinkedList(gameList,userChosenGame)->memory,INSTALLEFRORMATLOCATIONVITA);
	}else{
		batchFileURL = getOfficialInstallerUrl(getLinkedList(gameList,userChosenGame)->memory,INSTALLERFORMATLOCATION);
	}
	printf("Downloading script...\n");
	NathanLinkedList* urlList = GetUrls(batchFileURL);
	// Make sure we got exactly 4 URLs
	if (getLinkedListLength(urlList)!=4){
		printf("Incorrect number of URLs found. %d URLs were found, when exactly 4 were expected.\nURLs found:\n",getLinkedListLength(urlList));
		int i;
		for (i=0;i<getLinkedListLength(urlList);i++){
			printf("%d) %s\n",(i+1),getLinkedList(urlList,i+1)->memory);
		}
		return 1;
	}

	// Ask user for data directory
	char* exampleFolderPath;
	#if PLATFORM == PLAT_LINUX
		char* userHomeDirectory = getHomeDirectory();
		exampleFolderPath = malloc(strlen(DEFAULTSTEAMDIR)+strlen(userHomeDirectory)+strlen(getLinkedList(gameFolderList,userChosenGame)->memory)+1);
		sprintf(exampleFolderPath,DEFAULTSTEAMDIR,userHomeDirectory);
		strcat(exampleFolderPath,getLinkedList(gameFolderList,userChosenGame)->memory);
		free(userHomeDirectory);
	#elif PLATFORM == PLAT_WINDOWS
		exampleFolderPath = malloc(strlen(DEFAULTSTEAMDIR)+strlen(getLinkedList(gameFolderList,userChosenGame)->memory)+1);
		strcpy(exampleFolderPath,DEFAULTSTEAMDIR);
		strcat(exampleFolderPath,getLinkedList(gameFolderList,userChosenGame)->memory);
	#endif
	char exampleDirectoryIsInvalid = !(checkDirectoryExists(exampleFolderPath));
	char* userDataFolderPathInput;
	char isSecondTimeLooped=0;
	do{
		if (isSecondTimeLooped==1){
			printf("!!!!!!!!!!!!\n%s does not exist!\n!!!!!!!!!!!!\n",userDataFolderPathInput);
			free(userDataFolderPathInput);
		}
		userDataFolderPathInput = NULL;
		printDivider();
		printf("Input the path of your Higurashi: When They Cry game's data folder.\n");
		printf("Example:\n%s\n",exampleFolderPath);
		if (exampleDirectoryIsInvalid==1){
			printf("--- (Note that the example directory does not exist, so it's invalid.) ---\n");
		}else{
			printf("--- (Enter nothing to use the example.) ---\n");
		}
		printDivider();
		size_t userDataFolderPathInputBufferSize;
		getline(&userDataFolderPathInput,&userDataFolderPathInputBufferSize,stdin);
		removeNewline(&userDataFolderPathInput);
		// Trim end slash
		if (userDataFolderPathInput[strlen(userDataFolderPathInput)-1]=='/' || userDataFolderPathInput[strlen(userDataFolderPathInput)-1]=='\\'){
			userDataFolderPathInput[strlen(userDataFolderPathInput)-1]='\0';
		}
		// Copy example if use entered nothing
		if (strlen(userDataFolderPathInput)==0){
			free(userDataFolderPathInput);
			userDataFolderPathInput = malloc(strlen(exampleFolderPath)+1);
			strcpy(userDataFolderPathInput,exampleFolderPath);
		}
		isSecondTimeLooped=1;
	}while(checkDirectoryExists(userDataFolderPathInput)==0);
	free(exampleFolderPath);
	// Make the path to the StreamingAssets folder. This is where the ZIP files are extracted to.
	char* streamingAssetsPath=malloc(strlen(userDataFolderPathInput)+strlen("/StreamingAssets")+1);
	strcpy(streamingAssetsPath,userDataFolderPathInput);
	strcat(streamingAssetsPath,SLASH"StreamingAssets");
	free(userDataFolderPathInput);

	if (userUpdateChoice==DOWNLOADLIST_ALL || userUpdateChoice==DOWNLOADLIST_PATCH){
		char* _tempFolderCheck=malloc(strlen(streamingAssetsPath)+strlen("/CompiledUpdateScripts")+1);
		strcpy(_tempFolderCheck,streamingAssetsPath);
		strcat(_tempFolderCheck,SLASH "CompiledUpdateScripts");
		if (checkDirectoryExists(_tempFolderCheck)){
			printf("Deleting precompiled update scripts...\n");
			DeleteAllInFolder(_tempFolderCheck);
		}
		free(_tempFolderCheck);
	}

	// Actually download the files
	if (userUpdateChoice==DOWNLOADLIST_ALL || userUpdateChoice == DOWNLOADLIST_HIGURASHIVITA){
		int i;
		for (i=1;i<=4;i++){
			printf("Downloading file %d/4...\n",i);
			char _completedFilename[6]; // Null .zip one digit number
			itoa(i,_completedFilename,10);
			strcat(_completedFilename,".zip");
			downloadToFile(getLinkedList(urlList,i)->memory,_completedFilename);
		}
	}else{
		printf("Downloading file 1/1...\n");
		char _completedFilename[6];
		itoa(userUpdateChoice,_completedFilename,10);
		strcat(_completedFilename,".zip");
		downloadToFile(getLinkedList(urlList,userUpdateChoice)->memory,_completedFilename);
	}
	freeLinkedList(urlList);

	if (userUpdateChoice==DOWNLOADLIST_ALL || userUpdateChoice==DOWNLOADLIST_HIGURASHIVITA){
		extractZIP("./1.zip",streamingAssetsPath);
		extractZIP("./2.zip",streamingAssetsPath);
		extractZIP("./3.zip",streamingAssetsPath);
		extractZIP("./4.zip",streamingAssetsPath);
	}else{
		char _completedFilename[6]; // Null .zip one digit number
		itoa(userUpdateChoice,_completedFilename,10);
		strcat(_completedFilename,".zip");
		extractZIP(_completedFilename,streamingAssetsPath);
	}	
	printDivider();
	printf("Done.\nI have no idea if it worked.\n");
	printDivider();
	quitApplication();
	return 0;
}
