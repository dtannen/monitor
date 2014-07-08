// Copyright David Tannenbaum 2014 (Commands.com).
// Distributed under the http://creativecommons.org/licenses/by-nc-sa/4.0/
// (See accompanying file LICENSE.txt or copy at http://creativecommons.org/licenses/by-nc-sa/4.0/)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "util.h"

/* A static variable for holding the line. */
static char *line_read = (char *)NULL;
char *readline(const char *prompt);
void add_history(const char *line);

/***********************************************************
 *
 * Get the authorization key and <key> from the server
 *
 ***********************************************************/

int getAuthKey(char *username, char *password, char *authkey, char *key)
{
  CURL *curl;
  CURLcode res;
  char string[256];

  struct curl_slist *headers=NULL; // init to NULL is important
  headers = curl_slist_append( headers, "Accept: application/json");
  headers = curl_slist_append( headers, "Content-Type: application/json");
  headers = curl_slist_append( headers, "charsets: utf-8");

  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();

  if (curl) {
    struct return_string s;
    init_string(&s);

    sprintf(string,"{\"username\":\"%s\",\"password\":\"%s\"}",username,password);

    curl_easy_setopt(curl, CURLOPT_URL,           authUrl);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    string);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,    headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE,(curl_off_t)-1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, accumulate);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &s);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS,    1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL,      1L);
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    res = curl_easy_perform(curl);

    // set the status and message text
    //printf("result: %s\n",s.ptr);
    strcpy(authkey,s.ptr);

    free(s.ptr);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    // get a <key> value
    res = getKeyVal(key);

    return(res);

  } else {

    strcpy(authkey,"");
    strcpy(key,"");
    return (-99);

  }
}

/***********************************************************
 *
 * send the post.json processed file
 *
 ***********************************************************/

int postJsonFile(char *postfile)
{
  CURL *curl;
  CURLcode res;
  int json_fp = 0;
  char data[MAX_JSON_FILE_SIZE];
  struct stat stbuf;
  int n,filelen;
  char status[32],message[64];

  struct curl_slist *headers=NULL; // init to NULL is important
  headers = curl_slist_append( headers, "Accept: application/json");
  headers = curl_slist_append( headers, "Content-Type: application/json");
  headers = curl_slist_append( headers, "charsets: utf-8");

  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();

  if (curl) {
    struct return_string s;
    init_string(&s);


    stat(postfile,&stbuf);
    filelen = stbuf.st_size;
    if (filelen > MAX_JSON_FILE_SIZE) {
      close(json_fp);
      curl_easy_cleanup(curl);
      return(-3);
    }

    json_fp = open(postfile,O_RDONLY);
    if (json_fp <= 0) {
      printf("Unable to open file %s\n",postfile);
      curl_easy_cleanup(curl);
      return(-2);
    }

    n = read(json_fp,data,filelen);
    if (n != filelen) {
      close(json_fp);
      curl_easy_cleanup(curl);
      return (-4);
    }

    curl_easy_setopt(curl, CURLOPT_URL,           keyUrl);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,    headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    data);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE,(curl_off_t)-1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, accumulate);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &s);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS,    1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL,      1L);
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    res = curl_easy_perform(curl);

    // set the status and message text
    // {"status":"Success","userId":"6"}        // user
    // {"status":"Success"}                     // anonymous

    sscanf (s.ptr,"{\"%[^\"]\":\"%[^\"]",status,message);
    if (!strcmp(message,"Success")) {
      printf("\nSuccessfully posted data...\n");
    } else {
      printf("\nError: %s\n\n",message);
      return(-2);
    }


    free(s.ptr);
    close(json_fp);

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

  } else {

    return (-99);

  }

  return(res);
}

/***********************************************************
 *
 * get the <key>
 *
 ***********************************************************/

int getKeyVal(char *key)
{
  CURL *curl;
  CURLcode res;

  // now do a get on /command to get the key

  curl = curl_easy_init();

  if (curl) {
    struct return_string s;
    init_string(&s);

    curl_easy_setopt(curl, CURLOPT_URL,           keyUrl);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, accumulate);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &s);
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    res = curl_easy_perform(curl);

    //printf("result: %s\n",s.ptr);
    /* Check for errors */
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    strcpy(key,s.ptr);

    free(s.ptr);

    curl_easy_cleanup(curl);

  } else {

    res = -99;

  }

  return(res);
}


/***********************************************************
 *
 * Initialize the string handler so that it is thread safe
 *
 ***********************************************************/

void init_string(struct return_string *s)
{
  s->len = 0;
  s->ptr = malloc(s->len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(-1);
  }
  s->ptr[0] = '\0';
}

/***********************************************************
 *
 * Use the "writer" to accumulate text until done
 *
 ***********************************************************/

size_t accumulate(void *ptr, size_t size, size_t nmemb, struct return_string *s)
{
  size_t new_len = s->len + size*nmemb;
  s->ptr = realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(-1);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}


/***********************************************************
 *
 * This function converts the post.txt to post.txt.json
 * whicl will get posted to the server using the authkey
 * and key.
 *
 ***********************************************************/


int buildJsonResult(char *authkey, char *key)
{

  char buf[4096];
  char buftmp[4096];
  char command[256];
  char commandtmp[256];
  char temp[256];
  char out[MAX_JSON_FILE_SIZE];
  FILE *post_fp = (FILE *)0;
  FILE *json_fp = (FILE *)0;
  int  gotcommand,i,j,n,numCommand;
  fpos_t fpos;

  // open the post.txt file
  post_fp = fopen(fName(POST_NAME),"r");
  if (post_fp == NULL) {
    printf("unable to open post file %s\n",fName(POST_NAME));
    return(-1);
  }

  // create the post.json file
  json_fp = fopen(fName(POST_JSON),"w");
  if (json_fp == NULL) {
    printf("unable to create json file %s\n",fName(POST_JSON));
    return(-1);
  }

  // OK, lets build the new json file from the post.txt file

  gotcommand = 0;
  numCommand = 0;
  memset(buftmp,0,sizeof(buftmp));
  // are we logged in or are we anonymous?
  if (strlen(authkey))
    sprintf(buftmp,"{\"authkey\": \"%s\", \"key\":\"%s\", \"commands\":{\n",authkey,key);
  else
    sprintf(buftmp,"{\"key\":\"%s\", \"commands\":{\n",key);

  fputs(buftmp,json_fp);

  while(1) {

    memset(buf,0,sizeof(buf));
    if (fgets(buf,4096,post_fp) == NULL) break;

    // remove trailing \n
    n = strlen(buf);
    if (buf[n-1] == '\n') buf[n-1] = '\0';


    if (!strcmp(buf,"monitor$ ")) continue;		// just a blank line, skip it

    // get the command
    memset(command,0,sizeof(command));
    memset(commandtmp,0,sizeof(commandtmp));
    strcpy(command,&(buf[9]));

    // we got at least one command
    gotcommand = 1;

    n = strlen(command);
    j = 0;
    for(i=0;i<n;i++) {
      if (command[i] == '"') {
        commandtmp[j] = '\\';
        j++;
        commandtmp[j] = '"';
      } else if (command[i] == '\\') {
        commandtmp[j] = '\\';
        j++;
        commandtmp[j] = '\\';
      } else {
        commandtmp[j] = command[i];
      }
      j++;
    }

    strcpy(command,commandtmp);
    // write command section
    numCommand++;
    if (numCommand > 1) fputs("         },\n",json_fp);
    memset(temp,0,sizeof(temp));
    sprintf(temp,"    \"%d\": {\n",numCommand);
    fputs(temp,json_fp);
    memset(temp,0,sizeof(temp));
    sprintf(temp,"          \"command\": \"%s\",\n",command);
    // strip non printing characters
    n = strlen(temp);
    for(i=0;i<n;i++) {
      if (temp[i] <= 31) {
        temp[i] = ' ';
      } else if (temp[i] >= 127) {
        temp[i] = ' ';
      }
    }
    fputs(temp,json_fp);

    // initialize the output line
    memset(out,0,sizeof(out));
    strcpy(out,"          \"output\": \"");

    // now read all output until the next prompt
    while(1) {
      memset(buf,0,sizeof(buf));
      memset(buftmp,0,sizeof(buftmp));

      // save the current file position so that if the next fgets is the
      // monitor prompt (with a possible command), we will need to rewind
      // the file so that the fgets at the top of the while loop will
      // re-read the prompt and possible command

      fgetpos(post_fp,&fpos);

      if (fgets(buf,4096,post_fp) == NULL) break;
      n = strlen(buf);
      if (buf[n-1] == '\n') buf[n-1] = '\0';
      if (strstr(buf,"monitor$ ")) {			// another prompt. we are done here
        // rewind the file pointer
        fsetpos(post_fp,&fpos);
        break;
      }

      // convert <tab> to \t
      // convert <nl>  to \n
      // convert "     to \"
      // convert \     to backslash backslash

      n = strlen(buf);
      j = 0;
      for(i=0;i<n;i++) {
        if (buf[i] == '\t') {
          buftmp[j] = '\\';
          j++;
          buftmp[j] = 't';
        } else if (buf[i] == '\n') {
          buftmp[j] = '\\';
          j++;
          buftmp[j] = 'n';
        } else if (buf[i] == '"') {
          buftmp[j] = '\\';
          j++;
          buftmp[j] = '"';
        } else if (buf[i] == '\\') {
          buftmp[j] = '\\';
          j++;
          buftmp[j] = '\\';
        } else if (buf[i] <= 31) {
          buftmp[j] = ' ';
        } else if (buf[i] >= 127) {
          buftmp[j] = ' ';
        } else {
          buftmp[j] = buf[i];
        }
        j++;
      }

      strcat(out,buftmp);
      strcat(out,"\\n");

    }
    // append closing quote
    strcat(out,"\"\n");
    fputs(out,json_fp);


  }

  fputs("         }\n",json_fp);
  fputs("}}\n",json_fp);

  fclose(post_fp);
  fclose(json_fp);

  if (gotcommand)
    return(0);
  else
    return(-1);
}


/***********************************************************
 *
 * generate/access temporary filenames
 *
 ***********************************************************/

char *fName(NAMETYPE name)
{
  static int gotpostname=0,gotpostjson=0,gotauthkey=0,gotkey=0;
  static int gotshell=0,gotshell2=0,gottemp=0;


  switch (name) {
  case POST_NAME:
    if (!gotpostname) {
      scratchname(postFile,"txt");
      gotpostname = 1;
    }
    return(postFile);
    break;
  case POST_JSON:
    if (!gotpostjson) {
      scratchname(jsonFile,"json");
      gotpostjson = 1;
    }
    return(jsonFile);
    break;
  case AUTHKEY_NAME:
    if (!gotauthkey) {
      scratchname(authKeyJsonFile,"json");
      gotauthkey = 1;
    }
    return(authKeyJsonFile);
    break;
  case KEY_NAME:
    if (!gotkey) {
      scratchname(keyJsonFile,"json");
      gotkey = 1;
    }
    return(keyJsonFile);
    break;
  case SHELL_NAME:
    if (!gotshell) {
      scratchname(shellFile,"sh");
      gotshell = 1;
    }
    return(shellFile);
    break;
  case SHELL_NAME2:
    if (!gotshell2) {
      scratchname(shellFile2,"sh2");
      gotshell2 = 1;
    }
    return(shellFile2);
    break;
  case TEMP_NAME:
    if (!gottemp) {
      scratchname(tempFile,"tmp");
      gottemp = 1;
    }
    return(tempFile);
    break;
  default:
    return(NULL);
    break;
  }

  return(NULL);
}

/***********************************************************
 *
 * generate scratchname
 *
 ***********************************************************/

void scratchname (char *name,char *suffix)
{
  char        proposed[257];
  int         version;
  char        shortpath[257];
  FILE        *fd = (FILE *)0;
  pid_t       pid;
  struct stat stbuf;

  strcpy(shortpath,"/tmp/");

  pid = abs(getpid()%1000000);

  /* Generate possible scratch file names, and test them for existance
   * until we find one that doesn't already exist
   */

  for (version = 0; ; version++)
    {
      sprintf (proposed, "%scli%04d%d.%s", shortpath, version, pid, suffix);

      if (stat (proposed, &stbuf) != 0)
        break;

      if (version == 9999)
        {
          strcpy(name,"");
          return;
        }
    }

  strcpy (name, proposed);

  /* Create the placeholder */
  fd = fopen(name,"w");
  if (fd > 0) {
    fclose(fd);
  } else {
    strcpy(name,"");
  }

  return;

}

/***********************************************************
 *
 * copy a file
 *
 ***********************************************************/

void my_cp(char *source, char *dest)
{
  int childExitStatus;
  char string[512];
  pid_t pid;
  int i,status;
  FILE *tmp_fp = (FILE *)0;
  char mode[] = "0755";
  status = 0;
  if (status == 0) {}
  // create the bash script
  tmp_fp = fopen(fName(SHELL_NAME2),"w");
  if (tmp_fp == NULL) {
    printf("unable to create %s file\n",fName(SHELL_NAME2));
    exit(1);
  }
  sprintf(string,"#!/bin/bash -l\ncp %s %s\n",source,dest);
  fputs(string,tmp_fp);
  fclose(tmp_fp);
  // set file permission to 755
  i = strtol(mode, 0, 8);
  chmod (fName(SHELL_NAME2),i);

  pid = fork();

  if (pid == 0) { /* child */
    execlp ("/bin/bash", "bash", "-c",fName(SHELL_NAME2),(char *)0);
  }
  else if (pid < 0) {
    /* error - couldn't start process - you decide how to handle */
  }
  else {
    /* parent - wait for child - this has all error handling, you
     * could just call wait() as long as you are only expecting to
     * have one child process at a time.
     */
    pid_t ws = waitpid( pid, &childExitStatus, WUNTRACED);
    if (ws == -1)
      {
        perror("waitpid");
        return;
      }

    if( WIFEXITED(childExitStatus)) /* exit code in childExitStatus */
      {
        status = WEXITSTATUS(childExitStatus); /* zero is normal exit */
        /* handle non-zero as you wish */
      }
    else if (WIFSIGNALED(childExitStatus)) /* killed */
      {
      }
    else if (WIFSTOPPED(childExitStatus)) /* stopped */
      {
      }
  }
}

/***********************************************************
 *
 * my_diff
 *
 ***********************************************************/

void my_diff(char *source, char *dest, char *appendfile)
{
  int childExitStatus;
  char string[512];
  pid_t pid;
  int i,status;
  FILE *tmp_fp = (FILE *)0;
  char mode[] = "0755";
  status = 0;
  if (status == 0) {}
  // create the bash script
  tmp_fp = fopen(fName(SHELL_NAME2),"w");
  if (tmp_fp == NULL) {
    printf("unable to create %s file\n",fName(SHELL_NAME2));
    exit(1);
  }
  sprintf(string,"#!/bin/bash -l\ndiff %s %s >> %s\n",source,dest,appendfile);
  fputs(string,tmp_fp);
  fclose(tmp_fp);
  // set file permission to 755
  i = strtol(mode, 0, 8);
  chmod (fName(SHELL_NAME2),i);

  pid = fork();

  if (pid == 0) { /* child */
    execlp ("/bin/bash","bash","-c",fName(SHELL_NAME2),(char *)0);
  }
  else if (pid < 0) {
    /* error - couldn't start process - you decide how to handle */
  }
  else {
    /* parent - wait for child - this has all error handling, you
     * could just call wait() as long as you are only expecting to
     * have one child process at a time.
     */
    pid_t ws = waitpid( pid, &childExitStatus, WUNTRACED);
    if (ws == -1)
      {
        perror("waitpid");
        return;
      }

    if( WIFEXITED(childExitStatus)) /* exit code in childExitStatus */
      {
        status = WEXITSTATUS(childExitStatus); /* zero is normal exit */
        /* handle non-zero as you wish */
      }
    else if (WIFSIGNALED(childExitStatus)) /* killed */
      {
      }
    else if (WIFSTOPPED(childExitStatus)) /* stopped */
      {
      }
  }
}

/***********************************************************
 *
 * my_append
 *
 ***********************************************************/

void my_append(char *source, char *appendfile)
{
  int childExitStatus;
  char string[512];
  pid_t pid;
  int i,status;
  FILE *tmp_fp = (FILE *)0;
  char mode[] = "0755";
  status = 0;
  if (status==0) {}
  // create the bash script
  tmp_fp = fopen(fName(SHELL_NAME2),"w");
  if (tmp_fp == NULL) {
    printf("unable to create %s file\n",fName(SHELL_NAME2));
    exit(1);
  }
  sprintf(string,"#!/bin/bash -l\ncat %s >> %s\n",source,appendfile);
  fputs(string,tmp_fp);
  fclose(tmp_fp);
  // set file permission to 755
  i = strtol(mode, 0, 8);
  chmod (fName(SHELL_NAME2),i);

  pid = fork();

  if (pid == 0) { /* child */
    execlp ("/bin/bash","bash","-c",fName(SHELL_NAME2),(char *)0);
  }
  else if (pid < 0) {
    /* error - couldn't start process - you decide how to handle */
  }
  else {
    /* parent - wait for child - this has all error handling, you
     * could just call wait() as long as you are only expecting to
     * have one child process at a time.
     */
    pid_t ws = waitpid( pid, &childExitStatus, WUNTRACED);
    if (ws == -1)
      {
        perror("waitpid");
        return;
      }

    if( WIFEXITED(childExitStatus)) /* exit code in childExitStatus */
      {
        status = WEXITSTATUS(childExitStatus); /* zero is normal exit */
        /* handle non-zero as you wish */
      }
    else if (WIFSIGNALED(childExitStatus)) /* killed */
      {
      }
    else if (WIFSTOPPED(childExitStatus)) /* stopped */
      {
      }
  }
}


/***********************************************************
 *
 * Implement the GNU readline
 *
 ***********************************************************/


/* Read a string, and return a pointer to it.  Returns NULL on EOF. */
char *rl_gets ()
{
  /* If the buffer has already been allocated, return the memory
     to the free pool. */
  if (line_read)
    {
      free (line_read);
      line_read = (char *)NULL;
    }

  /* Get a line from the user. */
  line_read = readline ("monitor$ ");

  /* If the line has any text in it, save it on the history. */
  if (line_read && *line_read)
    add_history (line_read);

  return (line_read);
}
