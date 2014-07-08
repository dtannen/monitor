// Copyright David Tannenbaum 2014 (Commands.com).
// Distributed under the http://creativecommons.org/licenses/by-nc-sa/4.0/
// (See accompanying file LICENSE.txt or copy at http://creativecommons.org/licenses/by-nc-sa/4.0/)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>

#include "util.h"

#define MAX_LINE 4096
#define MAX_WORDS MAX_LINE / 2  // because (one letter + one space) is two bytes
                                // so only can have half the number of characters
char *rl_gets();

// local global vars for parent/child
static FILE *post_fp = (FILE *)0;
static FILE *tmp_fp = (FILE *)0;
static int fclosed = 0;
static int gotdebug = 0;
extern char *optarg;
static char displayUrl[128];
static char authKeyVal[128];
static char keyVal[128];

// global filenames for all functions. genGlobalFiles() holds these. for the
// rest of us, we decalre them external.

extern char *readline (const char *);

void tokenize(char *line, char **words, int *nwords);
void terminate();
void terminate2();

int main(int nargs, char *args[])
{
  char *line, *words[MAX_WORDS];
  char string[1024],combined[1024],line_s[MAX_LINE];
  char c, *dir=NULL,*cwd, pwd[PATH_MAX];
  char username[128],password[128],viFile[256],viTemp[256];
  char comname[128],authkey[80],authkeyword[64],key[64];
  int  gotauthkey=0,gotusername=0,gotvi=0,gotnew=0;
  int  res, i, n, status, nwords=0;
  FILE *auth_fp = (FILE *)0;
  FILE *key_fp = (FILE *)0;
  pid_t wpid, pid;
  char mode[] = "0755";
  struct stat stbuf;

  // get server login user and password
  while ((c = getopt(nargs, args, "dhu:l:")) != -1)
    switch (c) {

    case 'h':             // help
      printf("Usage: %s {-d} {-h} {-u <username>}\n\n",args[0]);
      printf("-d : don't delete /tmp files\n");
      printf("-h : help\n");
      printf("-u : commands.com username\n");
      exit(0);

    case 'u':             // username
      strcpy(username,optarg);
      gotusername = 1;
      strcpy(password,getpass("Password: "));
      break;

    case 'd':             // debug, don't delete /tmp files
      gotdebug = 1;
      break;

    default:
      printf("Usage: %s {-d} {-h} {-u <username>}\n\n",args[0]);
      printf("-d : don't delete /tmp files\n");
      printf("-h : help\n");
      printf("-u : commands.com username\n");
      exit(0);
      break;
    }

  // if we have both the username and password, login and
  // get the authkey

  // get the authorization key for the username
  if (gotusername) {
    if ((res = getAuthKey(username, password, authkey, key)) != 0) {
      printf("Unable to get authkey from server (%d)\n",res);
      exit(1);
    }
  } else {
    getKeyVal(key);
  }

  // create the authkey.json file only if we have -u
  sprintf (comname,"/tmp/.%s.commands.com",getenv("USER"));
  if (gotusername) {
    sscanf (authkey,"{\"%[^\"]\":\"%[^\"]",authkeyword,authKeyVal);
    if (!strcmp(authkeyword,"error")) {
      printf("Invalid Login.. Exiting.\n");
      exit(1);
      // do not save error as authkey
    } else {
      auth_fp = fopen(comname,"w");
      if (auth_fp == NULL) {
        printf("unable to create %s\n",comname);
        exit(1);
      }
      fputs(authkey,auth_fp);
      fputs("\n",auth_fp);
      fclose(auth_fp);
    }
  } else {
    auth_fp = fopen(comname, "r");
    if (auth_fp != NULL) {
      if(fgets(authkey, 64, auth_fp) == NULL) {
        printf("Unable to read authkey from %s\n",comname);
        exit(1);
      }
      // remove trailing \n
      n = strlen(authkey);
      if (authkey[n-1] == '\n') authkey[n-1] = '\0';
      gotauthkey = 1;
    }
  }

  // create the key.json file
  key_fp = fopen(fName(KEY_NAME),"w");
  if (key_fp == NULL) {
    printf("unable to create %s\n",fName(KEY_NAME));
    exit(1);
  }
  fputs(key,key_fp);
  fputs("\n",key_fp);
  fclose(key_fp);

  // save off the key url to display to the user when we are done
  sscanf (key,"{\"key\":\"%[^\"]",keyVal);
  sprintf (displayUrl,"%s/%s",gotoKeyUrl,keyVal);

  // save off the authkey value
  if (gotusername || gotauthkey) {
    sscanf (authkey,"{\"%[^\"]\":\"%[^\"]",authkeyword,authKeyVal);
    if (!strcmp(authkeyword,"error")) {
      printf("\nError: %s\n\n",authKeyVal);
      exit(1);
    } else {
      printf("\nSuccessfully logged in...");
      if (gotusername) {
        printf("\nAuthKey saved to %s.  Delete file to return to Anonymous posting.\n",comname);
      } else {
        printf("\nAuthKey retrieved from %s.  Delete file to return to Anonymous posting.\n",comname);
      }
    }
  } else {
    strcpy(authKeyVal,"");
  }


  // set up termination function
  atexit((void *)terminate);

  // catch abort signals
  signal(SIGINT,(void *)terminate2);	// trap ctl-c
  signal(SIGTERM,(void *)terminate2);	// trap kill -15

  // create the post.txt file
  post_fp = fopen(fName(POST_NAME),"w");
  if (post_fp == NULL) {
    printf("unable to create post file %s\n",fName(POST_NAME));
    exit(1);
  }

  // record all input from the user
  while(1)
    {
      if (!gotdebug)
        unlink(fName(SHELL_NAME));

      line = rl_gets();
      strcpy(line_s, line);
      if (line == NULL) {		// trap ctl-d
        exit(1);
      }

      // break the line up into words
      tokenize(line, words, &nwords);

      // just a blank line?
      if (words[0] == NULL) {
        continue;
      }

      // are we done ?
      if (!strcasecmp(words[0], "exit")) {
        exit(1);
      }

      fputs("monitor$ ",post_fp);
      fflush(post_fp);
      fputs(line_s,post_fp);
      fflush(post_fp);
      fputs("\n",post_fp);
      fflush(post_fp);



      // toss out any commands that cannot be handled such
      // as those that use libcurses.so

      if (!strcasecmp(words[0], "top")) {
        printf("Unable to capture output from %s\n",words[0]);
        sprintf(string,"Unable to capture output from %s\n",words[0]);
        fputs(string,post_fp);
        fflush(post_fp);
        continue;
      }


      // builtin command
      if (!strcasecmp(words[0], "cd")) {
        if (nwords == 1)                     dir = getenv("HOME");
        if (nwords == 2 && *words[1] == '~') dir = getenv("HOME");
        if (nwords == 2 && *words[1] != '~') dir = words[1];
        if (chdir(dir) == -1) {
          perror("chdir");
          fputs(strerror(errno),post_fp);
          fflush(post_fp);
          fputs("\n",post_fp);
          fflush(post_fp);
          continue;
        }
        continue;
      }

      // builtin command
      if (!strcasecmp(words[0], "pwd")) {
        if(NULL == (cwd = getcwd(pwd, PATH_MAX))) {
          strcpy(pwd,"Unable to get current working directory\n");
        }
        printf("%s\n",pwd);
        fputs(pwd,post_fp);
        fflush(post_fp);
        fputs("\n",post_fp);
        fflush(post_fp);
        continue;
      }

      // builtin command
      if (!strcasecmp(words[0], "export")) {
        if (nwords > 1) {
          putenv(words[1]);
          continue;
        }
      }

      // look for "ls" by itself and add -C to make it tabbed format
      // because when piped through tee, it thinks it is not connected
      // to a terminal.
      if (!strcasecmp(words[0], "ls")) {
        if (nwords == 1) {
          words[1] = "-C";
          nwords = 2;
        }
      }

      // look for "man" and add | col -b
      if (!strcasecmp(words[0], "man")) {
        if (nwords == 2) {
          words[2] = "|";
          words[3] = "col";
          words[4] = "-b";
          nwords = 5;
        }
        // for when there is a "man 3 foo"
        if (nwords == 3) {
          words[3] = "|";
          words[4] = "col";
          words[5] = "-b";
          nwords = 6;
        }
      }

      // process special "vi" command
      if (!strcasecmp(words[0], "vi")) {
        if (nwords == 1) {
          printf("Please specify the new file you wish to create.\n");
          printf("It is required to correctly log the new file.\n");
          continue;
        }
        gotvi = 1;
        if (stat(words[1],&stbuf) != 0) {
          // new file
          gotnew=1;
          strcpy(viFile,words[1]);
        } else {
          // make copy of existing file
          strcpy(viFile,words[1]);
          strcpy(viTemp,fName(TEMP_NAME));
          my_cp(viFile,viTemp);
          gotnew=0;
        }
      } else {
        gotvi = 0;
      }

      // close the post.txt before forking
      fclose(post_fp);

      // OK, lets process the external command using fork/execvp
      if ((pid = fork ()) < 0) {
        perror ("fork");
        exit(0);
      }

      // this will split output between the terminal and post.txt
      //  <command> 2>&1 | tee -ai <file>
      if (pid == 0) {		// if child then exec the command
        if (!gotvi) {
          // create the bash script
          tmp_fp = fopen(fName(SHELL_NAME),"w");
          if (tmp_fp == NULL) {
            printf("unable to create %s file\n",fName(SHELL_NAME));
            exit(1);
          }
          // set file permission to 755
          i = strtol(mode, 0, 8);
          chmod (fName(SHELL_NAME),i);
          memset(combined,0,sizeof(combined));
          for(i=0;i<nwords;i++) {
            strcat(combined,words[i]);
            strcat(combined," ");
          }

          sprintf(string,"#!/bin/bash -l\n%s 2>&1 | tee -ai %s\n",combined,fName(POST_NAME));
          fputs(string,tmp_fp);
          fclose(tmp_fp);

          execlp ("/bin/bash","bash","-c",fName(SHELL_NAME),(char *)0);
          perror ("execlp");
          exit(0);
        } else {

          execlp ("vi","vi",viFile,(char *)0);
          perror ("execlp");
          exit(0);
        }
      }

      if (pid > 0)            // parent waits for child process to terminate
        {
          do {
            wpid = waitpid(pid, &status, WUNTRACED);
            if (wpid == -1) {
              perror("waitpid");
              return(0);
            }


            if (WIFEXITED(status)) {
              //printf("child exited, status=%d\n", WEXITSTATUS(status));


            } else if (WIFSIGNALED(status)) {
              printf("process killed (signal %d)\n", WTERMSIG(status));


            } else if (WIFSTOPPED(status)) {
              printf("process stopped (signal %d)\n", WSTOPSIG(status));


            } else {    // Non-standard case -- may never happen
              printf("Unexpected status (0x%x)\n", status);
            }
          } while (!WIFEXITED(status) && !WIFSIGNALED(status));

          // existing file
          if (gotvi && !gotnew) {
            my_diff(viTemp,viFile,fName(POST_NAME));
            unlink(viTemp);
          }

          // new file
          if (gotvi && gotnew) {
            my_append(viFile,fName(POST_NAME));
          }

          // re-open the post.txt file
          post_fp = fopen(fName(POST_NAME),"a");
          if (post_fp == NULL) {
            printf("unable to re-open post file %s\n",fName(POST_NAME));
            exit(2);
          }

        }



    }

  exit(0);
}


/******************************************************************
 *
 * split up the command line into tokens
 *
 ******************************************************************/

void tokenize(char *line, char **words, int *nwords)
{

  *nwords = 1;

  for(words[0]=strtok(line," \t\n");
      (*nwords<MAX_WORDS)&&(words[*nwords]=strtok(NULL," \t\n"));
      *nwords=*nwords+1);
  return;
}


/******************************************************************
 *
 * catch ctl-C and kill -15
 *
 ******************************************************************/

void terminate2()
{
  // user hit ctl-c or kill -15, so put exit command in post file
  fclose(post_fp);
  fclosed = 1;
  exit(0);
}


/******************************************************************
 *
 * process the input file and post to the server
 *
 ******************************************************************/

void terminate()
{
  int res;

  if (!fclosed) fclose(post_fp);

  // process post file.
  res = buildJsonResult(authKeyVal,keyVal);
  if (res == 0) {
    // send the processed json file to the server
    res = postJsonFile(fName(POST_JSON));
    // display the displayUrl for the user so they know how to view their output
    printf("\nCommands Url: %s\n",displayUrl);
  } else {
    printf("\nNo commands entered. Nothing posted to the site.\n");
    res = 0;
  }

  // remove all files unless there is an error
  if (res == 0) {
    if (!gotdebug) {
      unlink(fName(POST_NAME));
      unlink(fName(POST_JSON));
      // unlink(fName(AUTHKEY_NAME));
      unlink(fName(KEY_NAME));
      unlink(fName(SHELL_NAME));
      unlink(fName(SHELL_NAME2));
      unlink(fName(TEMP_NAME));
    }
  } else {
    printf("Input file:   %s\n",fName(POST_NAME));
    printf("Json file:    %s\n",fName(POST_JSON));
    printf("Authkey file: %s\n",fName(AUTHKEY_NAME));
    printf("Key file:     %s\n",fName(KEY_NAME));
  }

  printf("\nExiting...\n");
  exit(0);
}
