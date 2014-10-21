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


#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cmath>
#include <cstring>
#include <vector>
#include <map>
#include <algorithm>
#include <getopt.h>
#include <unistd.h>
#include <aubio/aubio.h>

#ifdef HAVE_DEBUG
#define debug(...)                fprintf (stderr, format , **args)
#else
#define debug(...)
#endif
#define verbmsg(format, args...)  if (verbose) fprintf(stderr, format , ##args)
#define errmsg(format, args...)   fprintf(stderr, format , ##args)
#define outmsg(format, args...)   fprintf(stdout, format , ##args)

using namespace std;


/* Global Variables */

int verbose = 0;
// input / output
char_t * source_uri = NULL;
char_t * sink_uri = NULL;
// general stuff
uint_t samplerate = 0;
uint_t buffer_size = 2048;
uint_t hop_size = 256;
// onset stuff
char_t * onset_method = "default";
smpl_t onset_threshold = 0.1;
// pitch stuff
char_t * pitch_unit = "default";
char_t * pitch_method = "default";
smpl_t pitch_tolerance = 0.85;
smpl_t silence_threshold = -45.0;
smpl_t min_f = 55.0;
smpl_t max_f = 1000.0;
// smoothing stuff
char_t * smoothing_method = "default";
uint_t window_size = 32;
// internal stuff
const char *prog_name;


/* Functions */

/* Statistics functions */

/**
 * @brief Calculates the arithmetic average of the signal pitch values.
 *
 * Calculate the arithmetic average. Discriminate frequencies 0 Hz value.
 *
 * @param values Pitch values.
 */
double avg (vector<double> values)
{
  int n = 0;
  double avg = 0.0;
  
  for (int i = 0; i < values.size (); i++) {
    if (values[i] > 0.0) {
      avg += values[i];
      n++;
    }
  }
  
  return avg / n;
}

/**
 * @brief Calculates the variance of the signal pitch values.
 *
 * Calculate the weighted variance of the signal pitch values using pitch confidence. Discriminate
 * frequencies 0 Hz value, the higher frequencies to a humbral (> 1000 Hz) and low confidence (< 0.5).
 *
 * @param pitch Pitch values.
 * @param pitch_conf Confidence of the correspondent pitch values.
 * @param var_p Pointer to double object where variance value of pitch is stored.
 */
double var (vector<double> values)
{
  int n = 0;
  double a = avg (values), var = 0.0;
  
  for (int i = 0; i < values.size (); i++) {
    if (values[i] > 0.0) {
      var += pow (values[i] - a, 2.0);
      n++;
    }
  }
  
  return var / (2*n);
}

/* Smoothing functions */

/**
 * @brief Smoothing
 *
 *
 *
 * @param pitch
 */
void mode_smth (vector<double> &pitch)
{
  int size = pitch.size();
  // window stuff
  int begin = window_size / 2;
  int end = size - begin;
  // pitch smoothing
  vector<double> new_pitch;
  
  for (int i = 0; i < size; i++) {
    if (pitch[i] > 0.0) {
      int pos = (i < begin) ? 0 : ((i > end) ? end - begin : i - begin);
      map<int,int> freq;
      map<int,int>::iterator it;
      
      for (int j = 0 ; j < window_size; j++) {
        if (pitch[pos+j] > 0.0) {
          int note = floor (aubio_freqtomidi (pitch[pos+j]) + .5);
          it = freq.find (note);
          if (it == freq.end ()) freq.insert (make_pair (note, 1));
          else it->second++;
        }
      }
      
      int max = 0, mode_note = 0;
      for (it = freq.begin (); it != freq.end (); it++) {
        if(it->second > max) {
          max = it->second;
          mode_note = it->first;
        }
      }
      
      new_pitch.push_back (aubio_miditofreq (mode_note));
    }
    else new_pitch.push_back (0.0);
  }
  
  // save the result
  pitch.swap (new_pitch);
}


/**
 * @brief Smoothing
 *
 *
 *
 * @param pitch
 */
void mode_smth2 (vector<double> &pitch)
{
  int size = pitch.size();
  int max, mode;
  // pitch smoothing
  vector<double> new_pitch;
  
  for (int i = 0; i < size; i++) {
    if (pitch[i] > 0.0) {
      int count;
      int windows = window_size;
      do {
        // window stuff
        int begin = windows / 2;
        int end = size - begin;
        // internal stuff
        count = 0;
        int pos = (i < begin) ? 0 : ((i > end) ? end - begin : i - begin);
        map<int,int> freq;
        map<int,int>::iterator it;
        
        for (int j = 0 ; j < windows; j++) {
          if (pitch[pos+j] > 0.0) {
            count++;
            int note = floor (aubio_freqtomidi (pitch[pos+j]) + .5);
            it = freq.find (note);
            if (it == freq.end ()) freq.insert (make_pair (note, 1));
            else it->second++;
          }
        }
        
        max = 0;
        mode = 0;
        for (it = freq.begin (); it != freq.end (); it++) {
          if(it->second > max) {
            max = it->second;
            mode = it->first;
          }
        }
        
        windows += window_size;
      } while (max > count*0.6);
      
      new_pitch.push_back (aubio_miditofreq (mode));
    }
    else new_pitch.push_back (0.0);
  }
  
  // save the result
  pitch.swap (new_pitch);
}


/**
 * @brief Smoothing
 *
 *
 *
 * @param pitch
 */
void silence_smth (vector<double> &pitch)
{
  int size = pitch.size();
  // pitch smoothing
  vector<double> new_pitch;
  
  for (int i = 0; i < size; i++) {
    if (pitch[i] > 0.0) {
      
      int j;
      int zeros = 0, nzeros = 0;
      double note = pitch[i];
      bool other_note = false;
      
      for (j = 0; zeros < 16 && i+j < size && !other_note; j++) {
        if (pitch[i+j] == 0.0) zeros++;
        else if (pitch[i+j] == note) { zeros = 0; nzeros++; }
        else other_note = true;
      }
      
      if (!other_note) j -= zeros;
      i += j-1;
      
      if (nzeros > 8) while (j--) new_pitch.push_back (note);
      else while (j--) new_pitch.push_back (new_pitch.back ());
    }
    else new_pitch.push_back (0.0);
  }
  
  // save the result
  pitch.swap (new_pitch);
}


void mode_smth3 (vector<double> pitch, vector<double> &onsets, vector<double> &duration, vector<int> &notes)
{
  int zeros_allowed = 4;
  int min_windows = 24;
  
  int begin = 0, end = 0, note;
  bool push;
  
  onsets.push_back (begin);
  for (int i = begin; i < pitch.size(); i++) {
    push = false;
    if (pitch[i] > 0.0) {
    
      int j, z = 0;
      int zeros = 0, max, mode_note, count = 0;
      double pmode = 0;
      
      map<int,int> freq;
      map<int,int>::iterator it;
      
      for (j = i; zeros < zeros_allowed && j < pitch.size(); j++) {
        
        if (pitch[j] > 0.0) {
          
          zeros = 0;
          count++;
          
          int n = floor (aubio_freqtomidi (pitch[j]) + .5);
          it = freq.find (n);
          if (it == freq.end ()) freq.insert (make_pair (n, 1));
          else it->second++;
          
          if (j-i > min_windows) {
            
            max = 0;
            mode_note = 0;
            
            for (it = freq.begin (); it != freq.end (); it++) {
              if(it->second > max) {
                max = it->second;
                mode_note = it->first;
              }
            }
            
            pmode = ((double) max) / ((double) j-i);
            if (pmode < 0.7) break;
            z = j;
          }
        }
        else zeros++;
      }
      
      if (j-i < min_windows || pmode < 0.3 || count < 0.6*min_windows) {
        i = j;
        end = j;
      } else {
        i = (z==0) ? j : z;
        push = true;
        note = mode_note;
      }
    } else end++;
    
    if (push) {
      if (begin != end) {
        duration.push_back ((end-begin)*hop_size/(float)samplerate);
        notes.push_back (0.0);
        onsets.push_back (end*hop_size/(float)samplerate);
      }
      duration.push_back ((i-end)*hop_size/(float)samplerate);
      notes.push_back (note);
      onsets.push_back (i*hop_size/(float)samplerate);
      begin = i;
      end = i;
    }
  }
  
}


/**
 * @brief Smoothing
 *
 *
 *
 * @param mean
 * @param pitch
 *
 * @return
 */
void smoothing_pitch (vector<double> &pitch, vector<double> pitch_conf)
{
  // smoothing methods
  mode_smth2 (pitch);
  silence_smth (pitch);
}


/* Features extraction functions */

/**
 * @brief Extract features of an audio signal.
 *
 * Extract features (pitch, pitch confidence) of an audio signal.
 *
 * @param source C string containing the name of the file to be opened.
 *               Its value shall follow the file name specifications of the running environment
 *               and can include a path (if supported by the system).
 * @param pitch Pointer to a std::vector<double> object where pitch signal is stored (Hz).
 * @param pitch_conf Pointer to a std::vector<double> object where pitch confidence is stored ([0,1]).
 */
void aubio_pitch (char_t *source, vector<double> &pitch, vector<double> &pitch_conf)
{
  // opening audio file
  aubio_source_t *this_source = new_aubio_source ((char_t*)source, samplerate, hop_size);
  if (this_source == NULL) {
    errmsg ("Error: could not open input file %s\n", source);
    exit (1);
  }
  
  samplerate = aubio_source_get_samplerate(this_source);
  
  // creation of the pitch detection object
  aubio_pitch_t *o = new_aubio_pitch (pitch_method, buffer_size, hop_size, samplerate);
  if (pitch_tolerance != 0.)
    aubio_pitch_set_tolerance (o, pitch_tolerance);
  if (silence_threshold != -90.)
    aubio_pitch_set_silence (o, silence_threshold);
  if (pitch_unit != NULL)
    aubio_pitch_set_unit (o, pitch_unit);
  
  // internal memory stuff
  int blocks = 0;
  uint_t read = 0;
  uint_t total_read = 0;
  fvec_t *note = new_fvec (1);
  fvec_t *ibuf = new_fvec (hop_size);
  
  // process to analize audio file
  do {
    aubio_source_do (this_source, ibuf, &read);
    aubio_pitch_do (o, ibuf, note);
    
    // store features
    smpl_t n = fvec_get_sample (note, 0);
    pitch.push_back ((n > min_f && n < max_f) ? n : 0.0);
    pitch_conf.push_back (aubio_pitch_get_confidence (o));
    
    blocks++;
    total_read += read;
  } while (read == hop_size);
  
  // clean all aubio objects
  del_fvec (note);
  del_aubio_source (this_source);
  del_aubio_pitch (o);
  del_fvec (ibuf);
  aubio_cleanup ();
}

void save_notes (vector<double> pitch, vector<double> pitch_conf, vector<double> &onsets, vector<double> &duration, vector<int> &notes)
{
  int size = pitch.size(), count = 1;
  double note = pitch[0];
  
  onsets.push_back (0.0);
  for (int i = 1; i < size; i++)
  {
    if (pitch[i] == note) count++;
    else {
      if (note != 0.0 && count < 8) continue;
      duration.push_back (count*hop_size/(float)samplerate);
      notes.push_back (floor (aubio_freqtomidi (note) + .5));
      onsets.push_back (i*hop_size/(float)samplerate);
      note = pitch[i];
      count = 1;
    }
  }
  duration.push_back (count*hop_size/(float)samplerate);
  notes.push_back (floor (aubio_freqtomidi (note) + .5));
}

void my_audio_notes (char_t *source, vector<double> &onsets, vector<double> &duration, vector<int> &notes)
{
  // variables
  vector<double> pitch;
  vector<double> pitch_confidence;
  
  // method to obtain audio features
  aubio_pitch (source_uri, pitch, pitch_confidence);
  
  // smoothing the pitch values
  smoothing_pitch (pitch, pitch_confidence);
  
  // store the result
  save_notes (pitch, pitch_confidence, onsets, duration, notes);
  //mode_smth3 (pitch, onsets, duration, notes);
}

/**
 * @brief Extract features of an audio signal.
 *
 * Extract features (pitch, pitch confidence) of an audio signal.
 *
 * @param source C string containing the name of the file to be opened.
 *               Its value shall follow the file name specifications of the running environment
 *               and can include a path (if supported by the system).
 * @param pitch Pointer to a std::vector<double> object where pitch signal is stored (Hz).
 * @param pitch_conf Pointer to a std::vector<double> object where pitch confidence is stored ([0,1]).
 */
int get_note (vector<double> note_buffer)
{
  double v = var (note_buffer);
  verbmsg ("%lf", sqrt(v));
  if (sqrt(v) < 1000.0) {
    map<int,int> freq;
    map<int,int>::iterator it;
    for (int i = 0; i < note_buffer.size(); i++) {
      if (note_buffer[i] > 0.0) {
        int n = floor (aubio_freqtomidi (note_buffer[i]) + .5);
        it = freq.find (n);
        if (it == freq.end ()) freq.insert (make_pair (n, 1));
        else it->second++;
      }
    }
  
    int max = 0;
    int mode_note = 0;
    for (it = freq.begin (); it != freq.end (); it++) {
      if(it->second > max) {
        max = it->second;
        mode_note = it->first;
      }
    }
    verbmsg ("\t%d", max);
    if (max > 0.3*note_buffer.size() || max > 10) { verbmsg ("\tSi\n"); return mode_note; }
    else { verbmsg ("\tNo\n"); return floor (aubio_freqtomidi (note_buffer[note_buffer.size() / 2]) + .5); }
  }
  else { verbmsg ("\n"); return 0;}
}

void aubio_notes (char_t *source, vector<double> &onsets, vector<double> &duration, vector<int> &notes)
{
  // opening audio file
  aubio_source_t *this_source = new_aubio_source ((char_t*)source, samplerate, hop_size);
  if (this_source == NULL) {
    errmsg ("Error: could not open input file %s\n", source);
    exit (1);
  }
  
  samplerate = aubio_source_get_samplerate(this_source);
  
  // creation of the onset detection object
  aubio_onset_t *o = new_aubio_onset (onset_method, buffer_size/4, hop_size, samplerate);
  aubio_onset_set_threshold (o, onset_threshold);
  aubio_onset_set_minioi_s (o, 0.15);
  
  // creation of the pitch detection object
  aubio_pitch_t *p = new_aubio_pitch (pitch_method, buffer_size, hop_size, samplerate);
  aubio_pitch_set_tolerance (p, pitch_tolerance);
  aubio_pitch_set_silence (p, silence_threshold);
  if (pitch_unit != NULL) aubio_pitch_set_unit (p, pitch_unit);
  
  // internal memory stuff
  int blocks = 0;
  int last_note = 0;
  uint_t read = 0;
  uint_t total_read = 0;
  fvec_t *onset = new_fvec (1);
  fvec_t *note = new_fvec (1);
  fvec_t *ibuf = new_fvec (hop_size);
  vector<double> note_buffer;
  
  onsets.push_back (0.0);
  // process to analize audio file
  do {
    smpl_t new_pitch, curlevel;
    aubio_source_do (this_source, ibuf, &read);
    aubio_onset_do (o, ibuf, onset);
    aubio_pitch_do (p, ibuf, note);
    
    // get note frecuency
    note_buffer.push_back (fvec_get_sample (note, 0));
    
    smpl_t os = fvec_get_sample(onset, 0);
    if (os && blocks > 0) {
      duration.push_back ((blocks * hop_size / (float) samplerate) - (onsets.size() ? onsets.back() : 0));
      int n = get_note(note_buffer);
      notes.push_back ((n != 0 && last_note && abs (last_note - n) > 20) ? 0 : n);
      note_buffer.clear();
      onsets.push_back ((blocks * hop_size / (float) samplerate));
      if (notes.size() && notes.back() != 0) last_note = notes.back();
    }
    
    blocks++;
    total_read += read;
  } while (read == hop_size);
  
  // last note
  duration.push_back ((blocks * hop_size / (float) samplerate) - (onsets.size() ? onsets.back() : 0));
  notes.push_back (get_note(note_buffer));
  
  // clean all aubio objects
  del_fvec (note);
  del_fvec (onset);
  del_aubio_source (this_source);
  del_aubio_pitch (p);
  del_aubio_onset (o);
  del_fvec (ibuf);
  aubio_cleanup ();
}


/* Output functions */
/**
 * @brief Print the result in an output stream.
 *
 * @param output Pointer to a FILE object that identifies an output stream.
 * @param args Parameters to print.
 */
void print_notes (char_t *source, vector<double> onsets, vector<double> duration, vector<int> notes)
{
  FILE *pFile = stdout;
  if (sink_uri != NULL) pFile = fopen (sink_uri, "w");
  
  // write the information
  for (int i = 0; i < notes.size(); i++) {
    fprintf (pFile, "%lf\t%lf\t%d\n", onsets[i], duration[i], notes[i]);
  }
  
  // close file
  fclose (pFile);
}