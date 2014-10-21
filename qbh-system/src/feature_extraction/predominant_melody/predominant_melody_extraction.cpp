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

#include <iostream>
#include <dirent.h>
#include <getopt.h>
#include <cmath>
#include <essentia/algorithmfactory.h>
#include <essentia/scheduler/network.h>
#include <essentia/streaming/algorithms/poolstorage.h>


using namespace std;
using namespace essentia;
using namespace essentia::streaming;
using namespace scheduler;

// input / output
const char *source;
const char *sink;
// algorithm parameters
int frame_size = 2048;
int hop_size = 128;
int sample_rate = 44100;
Real voice_tolerance = 1;
// general stuffs
const char *prog_name;

// functions
void usage (FILE * stream, int exit_code)
{
  fprintf (stream, "usage: %s audio_input file_output [ options ] \n", prog_name);
  fprintf (stream,
           "       -r      --samplerate       set samplerate\n"
           "       -f      --framesize        set frame size\n"
           "       -p      --hopsize          set hopsize\n"
           "       -t      --voice-tolerance  set voice tolerance\n"
           "       -h      --help             display this message\n");
  exit (exit_code);
}

int parse_args (int argc, char **argv)
{
  const char *options = "hr:f:p:t";
  int next_option;
  struct option long_options[] = {
    {"help",                  0, NULL, 'h'},
    {"samplerate",            1, NULL, 'r'},
    {"framesize",             1, NULL, 'f'},
    {"hopsize",               1, NULL, 'p'},
    {"voice-tolerance",       1, NULL, 't'},
    {NULL,                    0, NULL, 0}
  };
  
  prog_name = argv[0];
  
  // if required parameters are not received
  if (argc < 3) usage (stderr, 1);
  
  source = argv[1];
  sink = argv[2];
  
  do {
    next_option = getopt_long (argc, argv, options, long_options, NULL);
    switch (next_option) {
      case 'h':                // help
        usage (stdout, 0);
      case 'r':
        sample_rate = atoi (optarg);
        break;
      case 'f':
        frame_size = atoi (optarg);
        break;
      case 'p':
        hop_size = atoi (optarg);
        break;
      case 't':
        voice_tolerance = stod (optarg);
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
  
  return 0;
}

int freq2midi(Real freq)
{
  return (69 + 12 * log2(freq / 440));
}

int main(int argc, char *argv[])
{
  parse_args (argc, argv);
  
  // register the algorithms in the factory(ies)
  essentia::init();
  
  Pool pool;

  // instantiate factory and create algorithms:
  streaming::AlgorithmFactory& factory = streaming::AlgorithmFactory::instance();
  
  Algorithm* audioload = factory.create("MonoLoader",
                                        "filename", source,
                                        "sampleRate", sample_rate,
                                        "downmix", "mix");
  
  Algorithm* equalLoudness = factory.create("EqualLoudness");
  Algorithm* predominantMelody = factory.create("PredominantMelody",
                                                "frameSize", frame_size,
                                                "hopSize", hop_size,
                                                "sampleRate", sample_rate,
                                                "voicingTolerance", voice_tolerance);
                                                //"minFrequency", 80.0,
                                                //"maxFrequency", 900.0);
  
  Algorithm* onsetrate  = factory.create("OnsetRate");
  
  /////////// CONNECTING THE ALGORITHMS ////////////////
  // audio -> equal loudness && onsetrate && pool
  audioload->output("audio") >> equalLoudness->input("signal");
  audioload->output("audio") >> onsetrate->input("signal");
  audioload->output("audio") >> PC(pool, "io.audio");
  // onsetrate -> pool
  onsetrate->output("onsetTimes") >>  PC(pool, "rhythm.onsetTimes");
  onsetrate->output("onsetRate")  >>  NOWHERE;
  // equal loundness -> predominantMelody
  equalLoudness->output("signal") >> predominantMelody->input("signal");
  // predominantMelody -> pool
  predominantMelody->output("pitch") >> PC(pool, "tonal.predominant_melody.pitch");
  predominantMelody->output("pitchConfidence") >> PC(pool, "tonal.predominant_melody.pitchConfidence");
  
  /////////// STARTING THE ALGORITHMS //////////////////
  Network network(audioload);
  network.run();
  
  /////////// SAVING RESULTS //////////////////
  Real audiosize = (Real) pool.value<vector<Real> >("io.audio").size();
  vector<Real> onsets = pool.value<vector<Real> >("rhythm.onsetTimes");
  vector<Real> pitch = pool.value<vector<Real> >("tonal.predominant_melody.pitch");
  vector<Real> pitchConfidence = pool.value<vector<Real> >("tonal.predominant_melody.pitchConfidence");
  Real inc = (((Real) audiosize) / ((Real) sample_rate)) / ((Real) pitch.size());
  onsets.push_back(audiosize / sample_rate);
  
  
  FILE * pFile;
  pFile = fopen (sink,"w");
  
  int n;
  Real time, accumpitch, accumconf;
  for (int i = 1; i < onsets.size(); i++) {
    time = onsets[i-1];
    accumpitch = 0.0;
    accumconf = 0.0;
    n = 0;
    for (int j = (int)(time / inc); j < pitch.size() && time < onsets[i]; j++) {
      time += inc;
      if (pitch[j] > 0.0) {
        accumpitch += pitch[j]*pitchConfidence[j];
        accumconf += pitchConfidence[j];
        n++;
      }
    }
    if (accumpitch > 0.0) fprintf(pFile, "%f\t%f\t%d\n", onsets[i-1], onsets[i] - onsets[i-1], freq2midi((accumpitch / (accumconf / (Real)n)) / (Real)n));
    else fprintf(pFile, "%f\t%f\t%d\n", onsets[i-1], onsets[i] - onsets[i-1], 0);
  }
  fclose (pFile);
  
  // clean up:
  essentia::shutdown();

  return 0;
}

