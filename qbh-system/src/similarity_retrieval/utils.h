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
#include <algorithm>
#include <getopt.h>
#include <unistd.h>
#include <dirent.h>
#include "tinyxml2.h"

#ifdef HAVE_DEBUG
#define debug(...)                fprintf (stderr, format , **args)
#else
#define debug(...)
#endif
#define verbmsg(format, args...)  if (verbose) fprintf(stderr, format , ##args)
#define errmsg(format, args...)   fprintf(stderr, format , ##args)
#define outmsg(format, args...)   fprintf(stdout, format , ##args)

#define pos_min(a,b,c)            (a < b && a < c) ? 1 : ((b < a && b < c) ? 2 : 3)

using namespace std;
using namespace tinyxml2;


/* Global Variables */

int verbose = 0;
// input / output
char * db_input = "../../db/db.xml";
char * rank_output = NULL;
char * humming_input = NULL;
// matching method stuff
char * matching_method = "default";
int sim_threshold = 0;
// general stuff
vector<char> reference_secuence;
vector<char> secuence;
// internal stuff
const char *prog_name;
// stuff
struct Cost {
  int ini, fin;
  int score, height;
  Cost (int i, int f, int s, int h) : ini(i), fin(f), score(s) , height(h) {}
  bool operator<(const Cost &c) const
  {
    //return this->score < c.score;
    return ((((double)this->score) / ((double)(this->fin - this->ini))) < (((double)c.score) / ((double)(c.fin - c.ini))));
  }
};

int distance_ij(int a, int b)
{
  int d = abs (a - b);
  return (d > 12) ? 12 : d;
}

int min(Cost &c1, Cost &c2, Cost &c3, int d)
{
  int a1 = abs (c1.height - d), a2 = abs (c2.height - d), a3 = abs (c3.height - d);
  if (a1 < a2 && a1 < a3) return 1;
  else if (a2 < a1 && a2 < a3) return 2;
  else if (a3 < a1 && a3 < a2) return 3;
  else {
    if (a1 == a2 && a1 == a3)  return pos_min(c1.score, c2.score, c3.score);
    else if (a1 == a2) return pos_min(c1.score, c2.score, 1000);
    else if (a1 == a3) return pos_min(c1.score, 1000, c3.score);
    else /*if (a2 == a3)*/ return pos_min(1000, c2.score, c3.score);
  }
}

/* Functions */

void convert_to_UDS (FILE *stream, vector<int> &seq)
{
  double onset, duration, note;
  double last_duration = 0, last_note;
  
  while (fscanf (stream, "%lf %lf %lf", &onset, &duration, &note) == 3) {
    if (note) {
      if (last_duration) {
        seq.push_back ((last_note > note) ? 'D' : ((last_note < note) ? 'U' : 'S'));
        seq.push_back ((last_duration > duration) ? 'S' : ((last_duration < duration)? 'L' : 'E'));
      }
      last_duration = duration;
      last_note = note;
    }
  }
}

void convert_to_MIDI (FILE *stream, vector<int> &seq)
{
  double onset, duration, note;
  double last_duration = 0, last_note;
  
  while (fscanf (stream, "%lf %lf %lf", &onset, &duration, &note) == 3) {
    if (note) seq.push_back (note);
  }
}

void read_stream (char *hmg, vector<int> &seq)
{
  debug ("Opening files ...\n");
  FILE *this_sec;
  this_sec = fopen (hmg, "r");
  if (this_sec == NULL) {
    errmsg ("Error: could not open humming input file '%s'\n", hmg);
    exit (1);
  }
  
  seq.clear ();
  
  if (strcmp (matching_method, "uds") == 0)
    convert_to_UDS (this_sec, seq);
  else if (strcmp (matching_method, "dtw") == 0)
    convert_to_MIDI (this_sec, seq);
    
  fclose (this_sec);
}

void dtw_matching (vector<int> seq, vector<int> r_seq, int id_song, vector<pair<int,double> > &rank)
{
  verbmsg ("%lu %lu\n", seq.size(), r_seq.size());
  vector<Cost> prev, curr;
  
  // DTW Matching Algorithm
  for (int j = 0; j < r_seq.size (); j++)
    prev.push_back (Cost (j, j, 0, (seq[0] - r_seq[j])));
  for (int i = 1; i < seq.size (); i++) {
    int dist_ij = (seq[i] - r_seq[0]);
    curr.push_back (Cost (0, 0, prev[0].score + abs (prev[0].height - dist_ij), prev[0].height));
    for (int j = 1; j < r_seq.size (); j++) {
      int dist_ij = (seq[i] - r_seq[j]);
      int m = min (curr[j-1], prev[j], prev[j-1], dist_ij);
      if (m == 1) {
        curr.push_back (Cost (curr[j-1].ini, j, curr[j-1].score + abs (curr[j-1].height - dist_ij), curr[j-1].height));
      } else if (m == 2) {
        curr.push_back (Cost (prev[j].ini, j, prev[j].score + abs (prev[j].height - dist_ij), prev[j].height));
      } else {
        curr.push_back (Cost (prev[j-1].ini, j, prev[j-1].score + abs (prev[j-1].height - dist_ij), prev[j-1].height));
      }
    }
    prev.swap (curr);
    curr.clear ();
  }
  
  // Transform the result <(ini, score), fin> in <ini, fin> ordered ascending by score
  sort (prev.begin (), prev.end ());
  
  double s = 0.0;
  double p = 0.6;
  for (int i = 0; i < prev.size (); i++) {
    //verbmsg ("ini: %d\tfin: %d\tscore: %d\tnorm_score: %lf\n", prev[i].ini, prev[i].fin, prev[i].score, ((double)prev[i].score) / ((double)(prev[i].fin-prev[i].ini)));
    bool ok = true;
    for (int j = 0; j < curr.size (); j++) {
      if ((prev[i].ini >= curr[j].ini && prev[i].ini <= curr[j].fin) ||
          (prev[i].fin >= curr[j].ini && prev[i].fin <= curr[j].fin)) {
        ok = false;
      }
    }
    double norm_score = ((double)prev[i].score) / ((double)(prev[i].fin-prev[i].ini));
    if (ok && (norm_score < 3 && ((double)(prev[i].fin-prev[i].ini)) > p*seq.size() && ((double)(prev[i].fin-prev[i].ini)) < (3-p)*seq.size()) && curr.size() < 1) {
      curr.push_back (prev[i]);
      s += norm_score;
      verbmsg ("ini: %d\tfin: %d\tscore: %d\tnorm_score: %lf\n", prev[i].ini, prev[i].fin, prev[i].score, ((double)prev[i].score) / ((double)(prev[i].fin-prev[i].ini)));
    }
  }
  
  if (s) {
    s = s / curr.size();
    rank.push_back (make_pair(id_song, s - ((curr.size() - 1)*0.15) ));
  }
}


void dp_matching (vector<int> seq, vector<int> r_seq, int id_song, vector<pair<int,double> > &rank)
{
  verbmsg ("%lu %lu\n", seq.size(), r_seq.size());
  vector<Cost> prev, curr;
  
  // DP Matching Algorithm
  for (int j = 0; j < r_seq.size (); j++)
    prev.push_back (Cost (j, j, ((seq[0] == r_seq[j]) ? 0 : 4), 0));
  for (int i = 1; i < seq.size (); i++) {
    curr.push_back (Cost (0, 0, prev[0].score + ((seq[i] == r_seq[0]) ? 0 : 4), 0));
    for (int j = 1; j < r_seq.size (); j++) {
      int dist_ij = ((seq[i] == r_seq[j]) ? 0 : 2);
      int m = pos_min (curr[j-1].score + dist_ij/2,
                   prev[j].score + dist_ij,
                   prev[j-1].score + (3*dist_ij)/2);
      if (m == 1) {
        curr.push_back (Cost (curr[j-1].ini, j, curr[j-1].score + dist_ij/2, 0));
      } else if (m == 2) {
        curr.push_back (Cost (prev[j].ini, j, prev[j].score + dist_ij, 0));
      } else {
        curr.push_back (Cost (prev[j-1].ini, j, prev[j-1].score + (3*dist_ij)/2, 0));
      }
    }
    prev.swap (curr);
    curr.clear ();
  }
  
  // Transfor the result <(ini, score), fin> in <ini, fin> ordered ascending by score
  sort (prev.begin (), prev.end ());
  
  double s = 0.0;
  double p = 0.6;
  for (int i = 0; i < prev.size (); i++) {
    //verbmsg ("ini: %d\tfin: %d\tscore: %d\n", prev[i].ini, prev[i].fin, prev[i].score);
    bool ok = true;
    for (int j = 0; j < curr.size (); j++) {
      if ((prev[i].ini >= curr[j].ini && prev[i].ini <= curr[j].fin) ||
          (prev[i].fin >= curr[j].ini && prev[i].fin <= curr[j].fin)) {
        ok = false;
      }
    }
    double norm_score = ((double)prev[i].score) / ((double)(prev[i].fin-prev[i].ini));
    if (ok && (/*norm_score < 0.35 && */((double)(prev[i].fin-prev[i].ini)) > p*seq.size() && ((double)(prev[i].fin-prev[i].ini)) < (3-p)*seq.size()) && curr.size() < 1) {
      curr.push_back (prev[i]);
      s += norm_score;
      verbmsg ("ini: %d\tfin: %d\tscore: %d\tnorm_score: %lf\n", prev[i].ini, prev[i].fin, prev[i].score, ((double)prev[i].score) / ((double)(prev[i].fin-prev[i].ini)));
    }
  }
  
  if (s) {
    s = s / curr.size();
    rank.push_back (make_pair(id_song, s - ((curr.size() - 1)*0.1) ));
  }
}


// main process
void matching (vector<int> seq, vector<int> r_seq, int id_song, vector<pair<int,double> > &rank)
{
  if (strcmp (matching_method, "uds") == 0)
    dp_matching (seq, r_seq, id_song, rank);
  else if (strcmp (matching_method, "dtw") == 0)
    dtw_matching (seq, r_seq, id_song, rank);
    
}