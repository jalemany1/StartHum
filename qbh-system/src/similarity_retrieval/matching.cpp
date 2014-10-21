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
  fprintf (stream, "usage: %s humming_input [ options ] \n", prog_name);
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
  
  humming_input = argv[1];
  
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

bool cmp (pair<int,double> p1, pair<int,double> p2)
{
  return p1.second < p2.second;
}

struct Song {
  const char* author;
  const char* title;
  const char* genre;
  const char* url;
  Song(const char* a, const char* t, const char* g, const char* u): author(a), title(t), genre(g), url(u) {}
};


/* Main program */

int main(int argc, char **argv)
{
  // variables
  vector<int> seq;
  vector<int> reference_seq;
  vector<pair<int,double> > rank;
  vector<Song> songs;
  bool used[50] = {0};
 
  
  // parse command line arguments
  parse_args (argc, argv);
  
  // read db.xml file
  XMLDocument doc;
  doc.LoadFile (db_input);
  
  // read humming sequence
  read_stream (humming_input, seq);

  XMLElement *song = doc.RootElement()->FirstChildElement("song");
  // loop for each song
  do
  {
    verbmsg ("%s %s\n", song->Name (), song->Attribute("id"));
    XMLElement *sample = song->FirstChildElement("samples")->FirstChildElement("sample");
    songs.push_back (Song(song->FirstChildElement("author")->GetText(),
                          song->FirstChildElement("title")->GetText(),
                          song->FirstChildElement("genre")->GetText(),
                          song->FirstChildElement("thumb_url")->GetText()));
    // loop for each sample
    do
    {
      verbmsg ("%s %s\n", sample->Name (), sample->Attribute("path"));
      char path[80] = "../../";
      strcat(path, sample->Attribute("path"));
      
      // read song sequence
      read_stream (path, reference_seq);
      // initialize process
      verbmsg ("'%s' analizando...\n", path);
      matching (seq, reference_seq, atoi (song->Attribute("id")), rank);
      verbmsg ("..fin de la cancion\n\n");
    } while ((sample=sample->NextSiblingElement("sample")) != NULL);
  } while ((song=song->NextSiblingElement("song")) != NULL);
  
  
  sort (rank.begin (), rank.end (), cmp);
  
  // save the result
  XMLDocument xmlDoc;
  XMLNode *pRoot = xmlDoc.NewElement("rank");
  
  int max = 5, n = 0;
  for (int i = 0; n < max && i < rank.size (); i++) {
    int id = rank[i].first - 1;
    if (!used[id]) {
      //verbmsg ("%d: %lf\n", id, rank[i].second);
      used[id] = true;
      n++;
      XMLNode *so = xmlDoc.NewElement("song");
      XMLElement *auth = xmlDoc.NewElement("author");
      auth->SetText(songs[rank[i].first - 1].author);
      so->InsertEndChild(auth);
      XMLElement *tit = xmlDoc.NewElement("title");
      tit->SetText(songs[rank[i].first - 1].title);
      so->InsertEndChild(tit);
      XMLElement *gen = xmlDoc.NewElement("genre");
      gen->SetText(songs[rank[i].first - 1].genre);
      so->InsertEndChild(gen);
      XMLElement *u = xmlDoc.NewElement("thumb_url");
      u->SetText(songs[rank[i].first - 1].url);
      so->InsertEndChild(u);
      pRoot->InsertEndChild(so);
      XMLElement *sim = xmlDoc.NewElement("similarity");
      sim->SetText(rank[i].second);
      so->InsertEndChild(sim);
    }
  }
  
  xmlDoc.InsertFirstChild(pRoot);
  if (rank_output == NULL) xmlDoc.SaveFile(stdout);
  else xmlDoc.SaveFile(rank_output);
  
  return 0;
}

