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
//#include <linux/fadvise.h>

#include "full-write.h"
#include "safe-read.h"
#include "stat-size.h"
#include "xfreopen.h"

#define PROGRAM_NAME "seqdump"

#define GETOPT_HELP_CHAR  (CHAR_MIN - 2)
#define GETOPT_VERSION_CHAR (CHAR_MIN - 3)

#ifndef MAX
# define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

enum { IO_BUFSIZE = 64*1024 };
static inline size_t
io_blksize (struct stat sb)
{
  return MAX (IO_BUFSIZE, ST_BLKSIZE (sb));
}

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

size_t page_size;

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

static inline void *
ptr_align (void const *ptr, size_t alignment)
{
  char const *p0 = ptr;
  char const *p1 = p0 + alignment - 1;
  return (void *) (p1 - (size_t) p1 % alignment);
}

bool
parse_fasta(
  char *in_buffer, 
  char *out_buffer,
  int *in_pos,
  int *out_pos,
  int *state,
  size_t buffer_size,
  size_t n_read)
{
  char *temp_ptr = out_buffer;
  char ch;
  while(*in_pos < n_read && *out_pos < buffer_size)
    {
      ch = in_buffer[*in_pos];
      if(ch == '\n')
        {
          switch(*state)
          {
            case 0:
            case 1:
              *state = 1;
              break;
            case 2:
              *state = 3;
              break;
            case 4:
              *state = 5;
              break;
            case 5:
              *state = 0;
              break;
            default:
              return false;
          }
        }
      else if(*state != 2 && (ch == ' ' || ch == '\t'))
        {
          switch(*state)
          {
            case 0:
            case 1:
              *state = 0;
              break;
            default:
              return false;
          }
        }
      else if(ch == '>')
        {
          switch(*state)
          {
            case 0:
            case 1:
              *state = 2;
              break;
            default:
              return false;
          }
        }
      else
        {
          /*TODO additional checks and transformations (casing)*/
          switch(*state)
          {
            case 2:
              break;
            case 3:
            case 4:
            case 5:
              *state = 4;
              *temp_ptr = in_buffer[*in_pos];
              temp_ptr++;
              //out_buffer[*out_pos]==in_buffer[*in_pos];
              (*out_pos)++;
              break;
            default:
              return false;
          }
        }
      (*in_pos)++;
    }

  if(*state == 0 || *state == 4 || *state == 5)
    return true;
  else
    return false;
}

bool
dump(char *in_buffer, char *temp_buffer, size_t bufsize)
{
  size_t n_read;
  int state = 0;
  int in_pos = bufsize;
  int temp_pos = 0;
  size_t n;
  char *temp = (char *)xmalloc (bufsize + page_size - 1);
  bool result;
  do
    {
      /* Read a block of input.  */
      if(in_pos == bufsize)
        {
          n_read = safe_read (input_descriptor, in_buffer, bufsize);
          in_pos = 0;
        }

      if (n_read == SAFE_READ_ERROR)
        {
          error (0, errno, "%s", infile);
          return false;
        }

      /*No more input data*/
      if(n_read == 0)
        {
          if(temp_pos > 0)
            {
              size_t t = temp_pos;
              if (full_write(STDOUT_FILENO, temp_buffer, t) != t)
                error(EXIT_FAILURE, errno, "write error");
            }
        }

      /*Fill temp_buffer*/
      while(in_pos < n_read && in_pos < bufsize)
        {
          result = parse_fasta(in_buffer, temp_buffer, &in_pos, &temp_pos, &state, bufsize, n_read);
           if(!result)
             error(EXIT_FAILURE, errno, "parsing error");
        }

      /*Empty temp_buffer and write data*/
      if(temp_pos == bufsize || (n_read < bufsize && in_pos == n_read))
        {
          if (full_write(STDOUT_FILENO, temp_buffer, n_read) != n_read)
            error(EXIT_FAILURE, errno, "write error");
          temp_pos = 0;
        }
    }
  while(n_read == bufsize);
  return true;
}

int
main(int argc, char *argv[])
{
  /* Optimal size of i/o operations of output.  */
  size_t outsize;
  
  /* Optimal size of i/o operations of input.  */
  size_t insize;
  
  page_size = getpagesize();
  
  struct stat stat_buf;
  int c;
  int option_index = 0;
  char *format;
  char *inbuf;
  char *tempbuf;
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
        }
    }
  
  int argind = optind;
  bool have_read_stdin = false;
  
  int file_open_mode = O_RDONLY;
  
  if (fstat (STDOUT_FILENO, &stat_buf) < 0)
    error (EXIT_FAILURE, errno, ("standard output"));
  
  outsize = io_blksize (stat_buf);
  
  infile = "-";
  
  do
    {
      if (argind < argc)
        infile = argv[argind];

      if (STREQ (infile, "-"))
      {
        have_read_stdin = true;
        input_descriptor = STDIN_FILENO;
        if ((file_open_mode & O_BINARY) && !isatty (STDIN_FILENO))
          xfreopen(NULL, "rb", stdin);
      }
      else
      {
        input_descriptor = open(infile, file_open_mode);
        if (input_descriptor < 0)
        {
          error (0, errno, "%s", infile);
          ok = false;
          continue;
        }
      }
      
      if (fstat(input_descriptor, &stat_buf) < 0)
      {
        error (0, errno, "%s", infile);
        ok = false;
        goto there;
      }
      insize = io_blksize(stat_buf);
      //fdadvise (input_descriptor, 0, 0, FADVISE_SEQUENTIAL);
      
      insize = MAX(insize, outsize);
      inbuf = (char *)xmalloc (insize + page_size - 1);
      tempbuf = (char *)xmalloc (insize + page_size - 1);
      
      ok &= dump(ptr_align(inbuf, page_size), ptr_align(tempbuf, page_size), insize);
      
      free(inbuf);
      free(tempbuf);

      /*here*/
      there:
        if (!STREQ (infile, "-") && close (input_descriptor) < 0)
        {
          error (0, errno, "%s", infile);
          ok = false;
        }
    }
  while (++argind < argc);

  exit (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
