using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "imdb.h"

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";

struct key
{
	const void *value;
	const void *file;
};

imdb::imdb(const string& directory)
{
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;
  
  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const
{
  return !( (actorInfo.fd == -1) || 
	    (movieInfo.fd == -1) ); 
}


int compareActors(const void * a, const void * b)
{
	key* data = (key*)a;
	int bytesToOffset = *(int *)b;

	char * actorName = (char *)((char *)data->file + bytesToOffset);

	return strcmp((char *)data->value, actorName );
	
}

// you should be implementing these two methods right here... 
bool imdb::getCredits(const string& player, vector<film>& films) const 
{	
	int nActors = *(int *)actorFile;
	
	key data;
	data.value = player.c_str();
	data.file = actorFile;

	int * pointerToOffset = (int *)bsearch(&data, (char *)actorFile + sizeof(int), nActors, 
											sizeof(int), compareActors);
	if(pointerToOffset == NULL)
		return false; //there's no actor named player.

	char * playerPos = (char *)actorFile + *pointerToOffset;

	int len = strlen(playerPos) % 2 == 1 ? strlen(playerPos) + 1 : strlen(playerPos) + 2;
	short *nMovies = (short *)(playerPos + len);

	len += sizeof(short);

	int *firstMovie;
	if(len % 4 == 0)
		firstMovie= (int *)(nMovies + 1);
	else
		firstMovie = (int *)((char *)nMovies + sizeof(short) + 2);

	for(int i = 0; i < *nMovies; i++)
	{
		int currMoviePos = *(firstMovie + i);
		char * currMovie = (char *)movieFile + currMoviePos;
		
		int currYear = *(char *)(currMovie + strlen(currMovie) + 1);
		
		film currFilm;
		currFilm.title = currMovie;
		currFilm.year = currYear + 1900;

		films.push_back(currFilm);
	}

	return true;
}

int compareMovies(const void * a, const void * b)
{
	key *data = (key*)a;
	int bytesToOffset = *(int *)b;

	char * movieName = (char *)((char *)data->file + bytesToOffset);
	int movieYear = *(char *)(movieName + strlen(movieName) + 1);

	film film2;
	film2.title = movieName;
	film2.year = movieYear + 1900;

	if(*(film *)data->value == film2)
		return 0;
	else if(*(film *)data->value < film2)
		return -1;

	return 1;
}

bool imdb::getCast(const film& movie, vector<string>& players) const
{
	int nMovies = *(int *)movieFile;

	key data;
	data.value = &movie;
	data.file = movieFile;
	int *pointerToOffset = (int *)bsearch(&data, (char *)movieFile + sizeof(int), nMovies,
											sizeof(int), compareMovies);
	if(pointerToOffset == NULL)
		return false;

	char *moviePos = (char *)movieFile + *pointerToOffset;

	int len = (strlen(moviePos) + 1) % 2 == 1 ? strlen(moviePos) + 2 : strlen(moviePos) + 3;
	short *nPlayers = (short *)(moviePos + len);
	
	len += sizeof(short);

	int *firstPlayer;
	if(len % 4 == 0)
		firstPlayer = (int *)(nPlayers + 1);
	else
		firstPlayer = (int *)((char *)nPlayers + sizeof(short) + 2);

	for(int i = 0; i < *nPlayers; i++)
	{
		int currPlayerPos = *(firstPlayer + i); 
		char * currPlayer = (char *)actorFile + currPlayerPos;

		players.push_back(currPlayer);
	}

	return true;
}

imdb::~imdb()
{
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

// ignore everything below... it's all UNIXy stuff in place to make a file look like
// an array of bytes in RAM.. 
const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info)
{
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info)
{
  if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
  if (info.fd != -1) close(info.fd);
}
