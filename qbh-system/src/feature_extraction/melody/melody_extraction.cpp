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

#define AUBIO_UNSTABLE 1
#include "utils.h"

using namespace std;


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
  fprintf (stream, "usage: %s [ options ] \n", prog_name);
  fprintf (stream,
           "Input / Output options:\n"
           "       -i      --input                 input file\n"
           "       -o      --output                output file\n"
//           "General options:\n"
//           "       -r      --samplerate            select samplerate\n"
//           "       -B      --bufsize               set buffer size\n"
//           "       -H      --hopsize               set hopsize\n"
//           "Pitch algorithm options:\n"
           "       -p      --pitch                 select pitch detection algorithm\n"
//           "       -u      --pitch-unit            select pitch output unit\n"
//           "       -l      --pitch-tolerance       select pitch tolerance\n"
//           "       -s      --silence               select silence threshold\n"
           "Smoothing algorithm options:\n"
           "       -S      --smoothing             select smoothing algorithm\n"
           "       -T      --smoothing-threshold   set smoothing threshold\n"
           "       -w      --windowsize            set window size\n"
           "General options:\n"
           "       -v      --verbose               be verbose\n"
           "       -h      --help                  display this message\n"
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
void parse_args (int argc, char **argv)
{
  const char *options = "hvi:r:B:H:o:p:u:l:s:S:w:";
  int next_option;
  struct option long_options[] = {
    {"help",                  0, NULL, 'h'},
    {"verbose",               0, NULL, 'v'},
    {"input",                 1, NULL, 'i'},
    {"samplerate",            1, NULL, 'r'},
    {"bufsize",               1, NULL, 'B'},
    {"hopsize",               1, NULL, 'H'},
    {"output",                1, NULL, 'o'},
    {"pitch",                 1, NULL, 'p'},
    {"pitch-unit",            1, NULL, 'u'},
    {"pitch-tolerance",       1, NULL, 'l'},
    {"silence",               1, NULL, 's'},
    {"smoothing",             1, NULL, 'S'},
    {"windowsize",            1, NULL, 'w'},
    {NULL,                    0, NULL, 0}
  };
  
  prog_name = argv[0];
  
  if (argc < 1) usage (stderr, 1);
  
  do {
    next_option = getopt_long (argc, argv, options, long_options, NULL);
    switch (next_option) {
      case 'h':                // help
        usage (stdout, 0);
        return;
      case 'v':                // verbose
        verbose = 1;
        break;
      case 'i':
        source_uri = optarg;
        break;
      case 'o':
        sink_uri = optarg;
        break;
      case 'r':
        samplerate = atoi (optarg);
        break;
      case 'B':
        buffer_size = atoi (optarg);
        break;
      case 'H':
        hop_size = atoi (optarg);
        break;
      case 'p':
        pitch_method = optarg;
        break;
      case 'u':
        pitch_unit = optarg;
        break;
      case 'l':
        pitch_tolerance = (smpl_t) atof (optarg);
        break;
      case 's':                // silence threshold
        silence_threshold = (smpl_t) atof (optarg);
        break;
      case 'S':                // smoothing method
        smoothing_method = optarg;
        break;
      case 'w':
        window_size = atoi (optarg);
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
  }
  while (next_option != -1);
  
  // if unique, use the non option argument as the source
  if ( source_uri == NULL ) {
    if (argc - optind == 1) {
      source_uri = argv[optind];
    } else if ( argc - optind > 1 ) {
      errmsg ("Error: too many non-option arguments `%s'\n", argv[argc - 1]);
      usage ( stderr, 1 );
    }
  } else if ( argc - optind > 0 ) {
    errmsg ("Error: extra non-option argument %s\n", argv[optind]);
    usage ( stderr, 1 );
  }
  
  // if no source, show a message
  if (source_uri == NULL) {
    errmsg("Error: no arguments given\n");
    usage ( stderr, 1 );
  }
  
  if ((sint_t)hop_size < 1) {
    errmsg("Error: got hop_size %d, but can not be < 1\n", hop_size);
    usage ( stderr, 1 );
  } else if ((sint_t)buffer_size < 2) {
    errmsg("Error: got buffer_size %d, but can not be < 2\n", buffer_size);
    usage ( stderr, 1 );
  } else if ((sint_t)buffer_size < (sint_t)hop_size) {
    errmsg("Error: hop size (%d) is larger than win size (%d)\n",
           hop_size, buffer_size);
    usage ( stderr, 1 );
  }
  
  if ((sint_t)samplerate < 0) {
    errmsg("Error: got samplerate %d, but can not be < 0\n", samplerate);
    usage ( stderr, 1 );
  }
  
}


/* Main program */

int main(int argc, char **argv)
{
  int opt = 1;
  // variables
  vector<double> onsets;
  vector<double> duration;
  vector<int> notes;
  
  
  // parse command line arguments
  parse_args (argc, argv);
  
  // method to obtain audio features
  if (opt == 0) my_audio_notes (source_uri, onsets, duration, notes);
  else aubio_notes (source_uri, onsets, duration, notes);
  
  // print the result
  print_notes (sink_uri, onsets, duration, notes);
  
  return 0;
}