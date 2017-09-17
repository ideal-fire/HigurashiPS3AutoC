#define CERTFILELOCATION "./curl-ca-bundle.crt"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <curl/curl.h>

/*============================================================================*/
typedef struct grhuigruei{
	char *memory;
	size_t size;
}MemoryStruct;
CURL* curl_handle;
/*============================================================================*/
signed char checkFileExist(const char* location){
	if( access( location, F_OK ) != -1 ) {
		return 1;
	} else {
		return 0;
	}
}
size_t curlWriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp){
	size_t realsize = size * nmemb;
	MemoryStruct* mem = (MemoryStruct*) userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		/* out of memory! */
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}
size_t curlWriteDataFile(void *ptr, size_t size, size_t nmemb, void *stream){
	size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
	return written;
}
void downloadFile(char* passedUrl, char* passedFilename){
	FILE* fp;
	curl_easy_setopt(curl_handle, CURLOPT_URL, passedUrl);
	fp = fopen(passedFilename, "wb");
	if(fp) {
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curlWriteDataFile);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, fp);
		curl_easy_perform(curl_handle);
		fclose(fp);
	}
}
void downloadWebpageData(MemoryStruct* chunkToDownloadTo, char* url){
	CURLcode res;

	chunkToDownloadTo->memory = malloc(1);  /* will be grown as needed by the realloc above */
	chunkToDownloadTo->size = 0;    /* no data at this point */
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curlWriteMemoryCallback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)chunkToDownloadTo);
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	res = curl_easy_perform(curl_handle);
	if(res != CURLE_OK) {
		printf("Failed, the world is over.\n");
	}
}
void initCurl(){
	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
	// Loads the certificate for https stuff. If the certificate file does not exist, allow insecure connections
	if (!checkFileExist(CERTFILELOCATION)){
		printf(CERTFILELOCATION" not found! Will allow insecure connections!");
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);
	}else{
		curl_easy_setopt(curl_handle, CURLOPT_CAINFO, CERTFILELOCATION);
	}
}
void quitApplication(){
	curl_easy_cleanup(curl_handle);
}
/*============================================================================*/
int main(int argc, char *argv[]){
	initCurl();
	//downloadFile("https://www.google.com","./google1.html");
	downloadFile("https://www.google.com","./google2.html");
	MemoryStruct noobStruct;
	//downloadWebpageData(&noobStruct,"https://www.google.com");
	//printf("%s\n",noobStruct.memory);
	quitApplication();
	return 0;
}
