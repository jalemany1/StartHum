/*
 Copyright (C) 2013-2014 Jose Alemany Bordera <joalbor1@inf.upv.es>
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "utils.h"


char * humming_input1 = NULL;
char * humming_input2 = NULL;


/* Functions */

/**
 * @brief Shows how the program is used.
 *
 * Shows how the program is used and the allowed options.
 * After running, the program finishes execution.
 *
 * @param stream Pointer to a FILE object that identifies an output stream.
 * @param exit_code Status code.
 *                  If this is 0 or EXIT_SUCCESS, it indicates success.
 *                  If it is EXIT_FAILURE, it indicates failure.
 */
void usage (FILE * stream, int exit_code)
{
  fprintf (stream, "usage: %s humming_input1 humming_input2 \n", prog_name);
  fprintf (stream,
           "       -i      --input-database   xml file database\n"
           "       -o      --output-rank      output xml file with the rank list\n"
           "       -m      --matching         select matching melody algorithm\n"
           "       -t      --sim-threshold    set similarity detection threshold\n"
           "       -v      --verbose          be verbose\n"
           "       -h      --help             display this message\n"
           );
  exit (exit_code);
}

/**
 * @brief Parses command line arguments.
 *
 * Parses command line arguments and detects misuse.
 *
 * @param argc Number of arguments received by command line.
 * @param argv Arguments received by command line.
 */
int parse_args (int argc, char **argv)
{
  const char *options = "hvi:o:m:t";
  int next_option;
  struct option long_options[] = {
    {"help",                  0, NULL, 'h'},
    {"verbose",               0, NULL, 'v'},
    {"input-database",        1, NULL, 'i'},
    {"output-rank",           1, NULL, 'o'},
    {"matching",              1, NULL, 'm'},
    {"sim-threshold",         1, NULL, 't'},
    {NULL,                    0, NULL, 0}
  };
  
  prog_name = argv[0];
  
  // if required parameters are not received
  if (argc < 2) {
    usage (stderr, 1);
    return -1;
  }
  
  humming_input1 = argv[1];
  humming_input2 = argv[2];
  
  do {
    next_option = getopt_long (argc, argv, options, long_options, NULL);
    switch (next_option) {
      case 'h':                // help
        usage (stdout, 0);
        return -1;
      case 'v':                // verbose
        verbose = 1;
        break;
      case 'i':
        db_input = optarg;
        break;
      case 'o':
        rank_output = optarg;
        break;
      case 'm':
        matching_method = optarg;
        break;
      case 't':
        sim_threshold = atoi (optarg);
        break;
      case '?':                // unknown options
        usage (stderr, 1);
        break;
      case -1:                 // done with options
        break;
      default:                 // something else unexpected
        fprintf (stderr, "Error parsing option '%c'\n", next_option);
        abort ();
    }
  } while (next_option != -1);
  
  if (strcmp (matching_method, "default") != 0 &&
      strcmp (matching_method, "uds") != 0 &&
      strcmp (matching_method, "dtw") != 0) {
    errmsg ("Error: unknown matching method %s.\n", matching_method);
    exit (1);
  }
  
  return 0;
}


/* Main program */

int main(int argc, char **argv)
{
  // variables
  vector<int> seq;
  vector<int> reference_seq;
  vector<pair<int,double> > rank;
 
  
  // parse command line arguments
  parse_args (argc, argv);
  
  // read db.xml file
  XMLDocument doc;
  doc.LoadFile (db_input);
  
  // read humming sequence
  read_stream (humming_input1, seq);

  
  // read song sequence
  read_stream (humming_input2, reference_seq);
  // initialize process
  verbmsg (" analizando...\n");
  matching (seq, reference_seq, 0, rank);
  verbmsg ("..fin de la cancion\n\n");
  
  return 0;
}

