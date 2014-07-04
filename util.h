// Copyright David Tannenbaum 2014 (Commands.com).
// Distributed under the http://creativecommons.org/licenses/by-nc-sa/4.0/
// (See accompanying file LICENSE.txt or copy at http://creativecommons.org/licenses/by-nc-sa/4.0/)

#ifndef UTIL_H
#define UTIL_H

#define MAX_JSON_FILE_SIZE 1000000      // 1 MB

// common URLs
#define authUrl "https://commands.com/user/login"
#define keyUrl "https://commands.com/command"
#define gotoKeyUrl "https://commands.com"

typedef enum {
  POST_NAME,
  POST_JSON,
  AUTHKEY_NAME,
  KEY_NAME,
  SHELL_NAME,
  SHELL_NAME2,
  TEMP_NAME
} NAMETYPE;

// common shared filenames
char postFile[256];
char jsonFile[256];
char authKeyJsonFile[256];
char keyJsonFile[256];
char shellFile[256];
char shellFile2[256];
char tempFile[256];

// exported functions
int getAuthKey(char *username, char *password, char *authkey, char *key);
int buildJsonResult(char *authkey, char *key);
int getKeyVal(char *key);
int postJsonFile(char *postfile);
int genGlobalFilenames();
char *fName(NAMETYPE name);
void scratchname (char *name,char *suffix);
void my_cp(char *from, char *to);
void my_diff(char *source, char *dest, char *appendfile);
void my_append(char *source, char *appendfile);

// return data from the server
struct return_string {
  char *ptr;
  size_t len;
};

// utility functions
size_t accumulate(void *ptr, size_t size, size_t nmemb, struct return_string *s);
void init_string(struct return_string *s);

#endif
