#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "safe-read.h"
#include "full-write.h"

#define PROGRAM_NAME "seqdump"

#define GETOPT_HELP_CHAR  (CHAR_MIN - 2)
#define GETOPT_VERSION_CHAR (CHAR_MIN - 3)


#define GETOPT_HELP_OPTION_DECL \
  "help", no_argument, NULL, GETOPT_HELP_CHAR
#define GETOPT_VERSION_OPTION_DECL \
  "version", no_argument, NULL, GETOPT_VERSION_CHAR

#define HELP_OPTION_DESCRIPTION \
  _("      --help     display this help and exit\n")
#define VERSION_OPTION_DESCRIPTION \
  _("      --version  output version information and exit\n")

#define case_GETOPT_HELP_CHAR                   \
  case GETOPT_HELP_CHAR:                        \
  usage(EXIT_SUCCESS);                       \
  break;

  
#define STREQ(s1, s2) ((strcmp (s1, s2) == 0))

char* program_name = "seqdump";

static char const *infile;

int header_flag=0;
static int input_descriptor;

static struct option long_options[] =
  {
    /* These options set a flag. */
    { GETOPT_HELP_OPTION_DECL },
    { GETOPT_VERSION_OPTION_DECL },
    { "format", required_argument, 0, 'f' },
    { "header", no_argument, &header_flag, 1 },
    { 0, 0, 0, 0 }
  };

void
usage(int status)
{
  puts("Usage: biotranslator");
  exit(status);
}

void print_version()
{
  puts("seqdump 1.0");
}

void parse_fasta(char[] in_buffer, 
  char[] out_buffer,
  int *length,
  int *in_pos,
  int *out_pos,
  int *state)
{
  while(*in_pos<*legth && *out_pos<*legth)
    {
      switch(in_buffer[(*in_pos)++])
        {
          case '\n':
            if(*state==10)
              *state=20;
            else
              *state=0;
            break;
          case ' ':
          case '\t':
            *state=1;
            break;
          case '>':
            if(*state!=0)
              {
                state=-1;
                return;
              }
            break;
          case else:
            /*TODO additional checks and optional change case*/
            out_buffer[(*out_pos)++]==in_buffer[*in_pos];
        }
    }
}

bool dump(char *buf, size_t bufsize)
{
  size_t n_read;
  while (true)
    {
      n_read = safe_read (input_descriptor, buf, bufsize);
      
      if (n_read == SAFE_READ_ERROR)
        {
          error (0, errno, "%s", infile);
          return false;
        }
      
      /*write the last block*/
      if (n_read == 0)
        if (full_write (STDOUT_FILENO, buf, n) != n)
          error(EXIT_FAILURE, errno, "write error");
        return true;

      {
        size_t n = n_read;
        if(n_read == bufsize)
          {
          if (full_write (STDOUT_FILENO, buf, n) != n)
            error(EXIT_FAILURE, errno, "write error");
          }
        else
          {
            ;
          }        
      }
    }
}

static inline void *
ptr_align (void const *ptr, size_t alignment)
{
  char const *p0 = ptr;
  char const *p1 = p0 + alignment - 1;
  return (void *) (p1 - (size_t) p1 % alignment);
}

int
main(int argc, char *argv[])
{
  int c;
  int option_index = 0;
  char *format;
  bool ok;

  while ((c = getopt_long(argc, argv, "f:h", long_options, NULL))
         != -1)
    {

      switch (c)
        {
        case 'f':
    format=optarg;
          printf("Format: `%s'\n", format);
          break;

    case_GETOPT_HELP_CHAR;
    
        case GETOPT_VERSION_CHAR:
          print_version();
          return EXIT_SUCCESS;

        default:
          usage(EXIT_FAILURE);
          ;
        }
    }
  
  int argind = optind;
  bool have_read_stdin = false;
  
  int file_open_mode = O_RDONLY;
  
  do
    {
      if (argind < argc)
        infile = argv[argind];
      
      if (STREQ (infile, "-"))
      {
        have_read_stdin = true;
        input_descriptor = STDIN_FILENO;
        if ((file_open_mode) && !isatty (STDIN_FILENO))
          //open (stdin, "rb");
          ;
      }
      else
      {
        input_descriptor = open (infile, file_open_mode);
        if (input_descriptor < 0)
        {
          error (0, errno, "%s", infile);
          ok = false;
          continue;
        }
      }
      
    }
  while (++argind < argc);
/*
  if (optind < argc)
    {
      printf("non-option ARGV-elements: ");
      while (argind < argc)
        printf("%s ", argv[argind++]);
      putchar('\n');
    }
*/
  exit(0);
  return EXIT_SUCCESS;
  
}
