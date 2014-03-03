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

#define PROGRAM_NAME "biotranslator"

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

char* program_name = "biotranslator";

static char const *infile;

int header_flag=0;

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

        default:
          usage(EXIT_FAILURE);
          ;
        }
    }
  
  int argind = optind;
  bool have_read_stdin = false;
  static int input_desc;
  int file_open_mode = O_RDONLY;
  
  do
    {
      if (argind < argc)
        infile = argv[argind];
      
      if (STREQ (infile, "-"))
      {
        have_read_stdin = true;
        input_desc = STDIN_FILENO;
        if ((file_open_mode) && !isatty (STDIN_FILENO))
          //open (stdin, "rb");
          ;
      }
      else
      {
        input_desc = open (infile, file_open_mode);
        if (input_desc < 0)
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
