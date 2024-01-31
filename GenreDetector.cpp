#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#define CSV_COLUMN_DELIMITER ','
using namespace std;

// Structs & Models:
struct Word
{
	string _w;
	int count;

	Word(string _word)
	{
		count = 0;
		_w = _word;
	}
};

struct Keyword
{
	long id;
	string value;
	Word *story_word; // for Stories, this will connect to their matching words
	int weight;

	Keyword(string _value, int _weight)
	{
		value = _value;
		weight = _weight;
		story_word = nullptr;
	}
};
struct Genre
{
	static short number_of_genres;
	vector<Keyword> keywords;
	int id;
	string title;

	Genre(string _title)
	{
		title = _title;
		id = ++Genre::number_of_genres; // id starts from 1, 
	}

	static Genre *CreateGenreFromCSV(string filename);

};

short Genre::number_of_genres = 0;

struct Prediction
{
	Genre *genre;
	float confidence; // The accuracy of the prediction
	vector<Keyword> keywords;
};

struct Story
{
	long index;
	vector<Word> words;
	string name, filename;
	int number_of_sentences;

	Story(string _filename, unsigned int _index)
	{
		filename = _filename;
		SetName();
		index = _index;
		LoadWords(); // load words vector
		number_of_sentences = 0;
	}

	void SetName();

	void LoadWords() // read story from file name;
	{
		// read file word by word, and count senmtences
	}
};

struct Analysis
{
	long id;
	Story *story;
	Prediction *predictions; // an array
	vector<Word> common_words;
	bool analyzed;
	Analysis(Story *_story)
	{
		story = _story;
		analyzed = false;
	}

	void Predict(vector<Genre> genres)
	{

		analyzed = true;
	}

	void Dump()
	{
	}
};

// UI Structures

enum Commands {
	CMD_EXIT = 0,

};


// Function Prototypes:
string ExtractNameFromFilename(string filename);

// Main:
int main()
{
	/*** Initial Data **/
	const short NUMBER_OF_GENRES = 4;
	const string GENRE_FILENAMES[] = { "Fantasy.csv", "Mystery.csv", "Romance.csv", "SciFi.csv" }; // Genre file names:
	
	const string PROGRAM_COMMANDS[] = { // List of program commands:
		"exit",

	};


	/*** Data Preprocessing: ***/

	Genre *common_genres[4];
	// Read genres data
	for (int i = 0; i < 4; i++)
		common_genres[i] = Genre::CreateGenreFromCSV(GENRE_FILENAMES[i]);
	
	/*** Menu & Main App Loop:  ***/
	string command = "";
	while (command != PROGRAM_COMMANDS[CMD_EXIT])
	{

		// Command Check:
	}
	cin >> command;
	return 0;
}

// Function Declerations:

// struct Genre Methods:
Genre *Genre::CreateGenreFromCSV(string filename)
{
	Genre *genre = new Genre(ExtractNameFromFilename(filename));
	std::ifstream genre_file(filename);

	// Check if the file is opened successfully
	if (!genre_file.is_open()) {
		return false;
	}

	string line;
	getline(genre_file, line);// skip first row
	// Read data line by line
	while (getline(genre_file, line)) {
		stringstream ss_line(line);
		string word, weight;
		getline(ss_line, word, CSV_COLUMN_DELIMITER); // first column is the word
		getline(ss_line, weight, CSV_COLUMN_DELIMITER);

		genre->keywords.push_back(Keyword(word, stoi(weight)));

	}

	// Close the file
	genre_file.close();
	return genre;
}

// struct Story Methods:
void Story::SetName()
{
	// remove file extension from filename and set it on name
	name = ExtractNameFromFilename(filename);
}


// global functions
string ExtractNameFromFilename(string filename)
{
	// remove file extension from filename and set it on name
	string name = filename;
	int index_of_dot = name.length();
	while (name[--index_of_dot] != '.' && index_of_dot > 0) // find the position of dot
		;
	if (index_of_dot > 0)          // if filename has a .extension part :
		name.erase(index_of_dot); // remove .extension part of the filenbame
	
	return name;
}