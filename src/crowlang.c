#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>

#include <crow/types.h>
#include <crow/math.h>
#include <crow/core.h>
#include <crow/io.h>
#include <crow/data.h>
#include <crow/funcond.h>
#include <crow/system.h>
#include <crow/string.h>

/*char *trim(char *str)
{
    size_t len = strlen(str);

    while(isspace(str[len - 1])) --len;
    while(*str && isspace(*str)) ++str, --len;

    return strndup(str, len);
}*/

/*CRO_Value CRO_error (const char *msg) {
  CRO_setColor(RED);
  printf("ERROR: %s\n", msg);
  CRO_setColor(RESET);
  return CRO_toNone();
*/

/* We have to put all of these so we can free them from the handler.  Please
 * tell me if there is a more elegant way to do this */
static CRO_State *s;
static char *input;
static FILE *srcfile;

static int execType = 0;

static void handler (int sig) {
  if (execType == 1) {
    fclose(srcfile);
  }
  else if (execType == 2) {
    free(input);
  }
  CRO_freeState(s);
  exit(0);
}

int main (int argc, char *argv[]) {
    CRO_initColor();
  signal(SIGINT, handler);

  s = CRO_createState();

  CRO_exposeStandardFunctions(s);

  if (argc >= 1) {
    /* TODO: Manually expose arguements here */
  }

  /* We probably shouldnt be executing this, but in the off chance that
   * we are, we return the error and exit */
  if (s->exitCode >= CRO_ExitCode) {
    CRO_printError();
    exit(1);
  }

  if (argc > 1) {
    CRO_Value v;

    execType = 1;
    srcfile = fopen(argv[1], "r");

    v = CRO_evalFile(s, srcfile);

    if (s->exitCode >= CRO_ExitCode) {
      CRO_printError();
      exit(1);
    }

    fclose(srcfile);
    CRO_freeState(s);

    if (v.type == CRO_Number) {
      return (int)v.value.number;
    }
    else {
      return 0;
    }
  }
  else {
    FILE *src;
    CRO_Value v;
    int c, paren, state, lsp, ptr, size, com, le, sc;


    execType = 2;

    /* We are reading from stdin, since this is taken from CRO_evalFile, its
     * easier to set this as our 'src' file */
    src = stdin;

    com = 0;
    paren = 0;
    state = CC_NONE;
    lsp = 1;
    le = 0;
    sc = 0;

    size = CRO_BUFFER_SIZE;
    ptr = 0;
    input = (char*)malloc(size * sizeof(char));

    /* Print the PS1 to tell the user we are at the terminal mode */
    printf("%% ");
    fflush(stdout);

    c = fgetc(src);

    /* Run for as long as we aren't hitting EOF, in this case a CTRL+D */
    while (running && c != EOF) {

      if (com != 2 && c == ';') {
        com++;
      }
      else if (com == 1) {
        com = 0;
      }

      /* We are, so deal with that accordingly */
      if (com == 2) {
        /* Read until the new line, thats how we figure out if we are out
         * of the comment or not */
        if (c == '\n') {
          com = 0;

          /* There should be one paren still in here */
          ptr--;

          /* If the ptr is 0, then we are back to the beginning of the line, so
           * show the PS1 */
          if (ptr == 0) {
            printf("%% ");
            fflush(stdout);
          }
        }

        c = fgetc(src);
        continue;
      }

      /* Make sure we normalize how many spaces we take in */

      /* If the state is CC_EXEC, we need to ignore this so that we execute
       * normally, in CC_EXEC the final character of the input is a \n, which
       * means it will be trapped here and not execute, we ignore the \n in
       * execution anyway */

       /* Also make sure we don't trim strings */
      if (state != CC_EXEC && state != CC_STRING && c <= 32) {
        if (lsp) {
          c = fgetc(src);
          continue;
        }
        else {
          c = ' ';
          lsp = 1;
        }
      }
      else {
        lsp = 0;
      }

      switch (state) {

        /* We currently aren't processing anything yet */
        case CC_NONE: {

          /* TODO: Eventually make a CC_STRING to make sure strings are properly
           * closed */
          if (c <= 32) {
            c = fgetc(src);
            continue;
          }
          else if (c == '(') {
            paren = 1;
            state = CC_STATEMENT;
          }
          else if (c == '\"' || c == '\'') {
            state = CC_STRING;
            sc = c;
          }
          else {
            state = CC_VARIABLE;
          }
        }
        break;

        /* We are processing a function call */
        case CC_STATEMENT: {
          /* Keep track of how many parenthesis deep we are */
          if (c == '(') {
            paren += 1;
          }
          else if (c == ')') {
            paren -= 1;

            /* If we reached zero, we know we can start executing */
            if (paren == 0) {
              state = CC_EXEC;
            }
          }
          /* We encountered a new line in the statement, which means the user is
           * probably inserting a new line into their statement, which means we
           * should indicate to them the program is not hung */
          else if (c == '\n') {
            printf("> ");
            fflush(stdout);
          }
        }
        break;

        /* We are processing a value call */
        case CC_VARIABLE: {
          /* If we are reading a value statement and we see a (, we know we
           * are now reading a function call.  Likewise if we see a space we
           * know we reached the end of the statement */
          if (c == '(' || c <= 32) {
            state = CC_EXEC;
            continue;
          }
        }
        break;

        case CC_STRING: {
          /* If we see a \, and our last character was not an escape, then this
           * one is. */
          if (le == 0 && c == '\\') {
            le = 1;
          }
          /* If we had an escape last character, it means the escape is now over
           * since we have no support for the longer escapes */
          else if (le == 1) {
            le = 0;
          }
          /* If we don't have an escape, but we do have either a ' or ", which
           * ever started the string, then we are at the end of the string and
           * are safe to start executing */
          else if (le == 0 && c == sc) {
            state = CC_EXEC;
          }
          /* If we have a new line, it will print that way in the console, so
           * we make sure to visually indicate that */
          else if (c == '\n') {
            printf("> ");
            fflush(stdout);
          }

        }
        break;

        /* We are executing the command */
        case CC_EXEC: {
          input[ptr] = 0;

          v = CRO_innerEval(s, input);



          /* Check our exit code */
          if (s->exitCode == CRO_ExitCode) {
            c = EOF;
            continue;
          }
          else if (s->exitCode == CRO_ErrorCode) {
            CRO_printError();
            s->exitCode = CRO_None;
          }

          CRO_printStd(v);

          ptr = 0;
          lsp = 1;

#ifndef CROWLANG_DISABLE_GC
          CRO_GC(s);
#endif

          state = CC_NONE;
          printf("%% ");
          fflush(stdout);
        }
        continue;

      }

      /* If we get here, was are intending on adding the character to input, if
       * we don't intend on doing this, call 'continue' rather than 'break' */
      input[ptr++] = (char)c;

      if (ptr >= size) {
        size *= 2;
        input = realloc(input, size * sizeof(char));
      }

      c = fgetc(src);
    }

    /* Good bye */
    free(input);
    CRO_freeState(s);

    if (v.type == CRO_Number) {
      return (int)v.value.number;
    }
    else {
      return 0;
    }
  }
  return 0;
}
