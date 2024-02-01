#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>


#define CSV_COLUMN_DELIMITER ','
typedef unsigned long long Long; // This type is for making sure that keyword Id and word count never overflow.
typedef unsigned int Int; // For story indexes and genre Ids and other stuffs

using namespace std;

// Check file existence in different Oses:
#ifdef _WIN32
	#include <io.h> // windows
#else
	#include <unistd.h>  // linux and mac
#endif

bool FileExists(const std::string& filename) 
{
	#ifdef _WIN32
		return _access(filename.c_str(), 0) == 0;
	#else
		return access(filename.c_str(), F_OK) == 0;
	#endif
}


/*** Global Function Prototypes: **/
	string ExtractNameFromFilename(string filename);

/*** Structs & Models: ***/
	struct Word
	{
		string w;
		Long count;

		Word(string _word)
		{
			count = 1;
			w = _word;
			CleanSigns();
		}

		void CleanSigns();
	};

	struct Keyword
	{
		Long id;
		string value;
		Word *story_word; // for Stories, this will connect to their matching words
		int weight; // int is used (instead of unsigned int) for negative weights (if its considered!)
		// for example, a word with negative weight can be used for decreasing the possibility of a genre

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
		Int id;
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
		// Story and its analysis are linked two ways.
		struct Analysis
		{
			static Int number_of_analysis;
			Int id;
			Story *story;
			vector<Prediction *> predictions; // an array of predictions
			vector<Keyword *> common_words;

			Analysis(Story *_story)
			{
				story = _story;
				id = ++number_of_analysis;
			}

			void Predict(vector<Genre> genres);

			void Dump();
		};

		Int index;
		vector<Word> words;
		string name, filename;
		Analysis *analysis;

		Story(string _filename, Int _index)
		{
			filename = _filename;
			// remove file extension from filename and set it on name
			name = ExtractNameFromFilename(filename);
			analysis = nullptr;
		}

		bool LoadStory();
		void AddWord(Word &);
		void PrintWords();
		bool Analyze(vector<Genre>, string);
	};

	Int Story::Analysis::number_of_analysis = 0;

/*** UI Structures & enums ***/
	enum Commands {
		CMD_EXIT = (char) 0,
		CMD_IMPORT_STORY,
		CMD_ANALYZE_STORY
	};

// Main:
int main()
{
	/*** Initial Data **/
	const short NUMBER_OF_GENRES = 4;
	const string GENRE_FILENAMES[] = { "Fantasy.csv", "Mystery.csv", "Romance.csv", "SciFi.csv" }; // Genre file names:

	const string PROGRAM_COMMANDS[] = { // List of program commands:
		"exit",
		"import_story",
		"analyze_story"
	};

	/*** Data Preprocessing: ***/
	vector<Genre> common_genres;
	vector<Story *> stories;

	// Read genres data
	for (int i = 0; i < NUMBER_OF_GENRES; i++)
	{
		Genre *next_genre = Genre::CreateGenreFromCSV(GENRE_FILENAMES[i]); // CreateGenreFromCSV return a pointer
		if (next_genre == nullptr) // For this if check, so if the pointer doesnt point to a new genre, means loading the genre was not successfull.
		{
			cout << "Error importing genre keywords. Please check keyword files." << endl;
			return 1;
		}
		common_genres.push_back(*next_genre); // common_genres will be passed as copy, because we doesnt want its original objects to be modified.
		// each story analysis will get a copy of these genres 
	}

	/*** Menu & Main App Loop:  ***/
	string line = "", command = "";
	do
	{
		getline(cin, line);
		// Use stringstream to split string in a more effective way
		stringstream iss_line(line);

		// Split line by space:
		iss_line >> command;

		// Command Check:
		if (command == PROGRAM_COMMANDS[CMD_IMPORT_STORY])
		{
			string filename;
			iss_line >> filename;
			if (!filename[0] || filename.empty())
			{
				cout << PROGRAM_COMMANDS[CMD_IMPORT_STORY] << " {filename.txt}"<< endl;
				continue; // Go to getting next command in next loop.
			}
			Story *new_story = new Story(filename, stories.size() + 1);
			if (!FileExists(filename))
			{
				cout << "File not found." << endl;
				continue;
			}

			if (!new_story->LoadStory())
			{
				cout << "Error importing the story." << endl;
				continue;
			}
			stories.push_back(new_story);
			cout << new_story->name << " imported successfully." << endl;
		}
		else if (command == PROGRAM_COMMANDS[CMD_ANALYZE_STORY])
		{
			Int index;
			iss_line >> index;
			if (index < 0 || index >= stories.size())
			{
				cout << "Invalid story index." << endl;
				continue;
			}
			string output_filename;
			iss_line >> output_filename;
			if (!output_filename[0] || output_filename.empty())
			{
				cout << PROGRAM_COMMANDS[CMD_ANALYZE_STORY] << " {story_index} {output_file_name.txt}\n";
				continue;
			}

		}
	} while (command != PROGRAM_COMMANDS[CMD_EXIT]);

	return 0;
}


/*** Gobal Functions  ***/
	string ExtractNameFromFilename(string filename)
	{
		// remove file extension from filename and set it on name
		string name = filename;
		Int index_of_dot = name.length();
		while (name[--index_of_dot] != '.' && index_of_dot > 0) // find the position of dot
			;
		if (index_of_dot > 0)          // if filename has a .extension part :
			name.erase(index_of_dot); // remove .extension part of the filenbame

		return name;
	}


/*** struct Word Methods: ***/
	void Word::CleanSigns()
	{
		// Remove every not alphanumeric characters from the word.
		while (!isalnum(w[0]))
			w.erase(0, 1);
		while (!isalnum(w[w.length() - 1]))
			w.erase(w.length() - 1);
	}


/*** struct Genre Methods: ***/
	Genre *Genre::CreateGenreFromCSV(string filename)
	{
		Genre *genre = new Genre(ExtractNameFromFilename(filename));
		std::ifstream genre_file(filename);

		// Check if the file is opened successfully
		if (!genre_file.is_open() || !genre_file.good() || genre_file.fail()) 
			return nullptr;

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


/*** struct Story Methods:  ***/
	bool Story::LoadStory() // read story from file name;
	{
		// read file word by word, and count senmtences
		ifstream story_file(filename);
		if (!story_file.is_open() || story_file.fail() || !story_file.good())
			return false;
		string str_word;

		while (story_file >> str_word) // Read the file word by word
		{
			Word word(str_word);
			AddWord(word); // Add word to the words vector, or increase its count if it exists already.
		}

		story_file.close();
		return true;
	}

	void Story::AddWord(Word &word)
	{
		// Add a word to words vector or if it exists, increase the count;
		// search for the word.
		Long i;
		for (i = 0; i < words.size() && words[i].w != word.w; i++);
		if (i >= words.size())
			words.push_back(word); // If words is not in the vector, add it.
		else
		{
			// If word is added to the vector previously, just increase its count.
			words[i].count++;
		}

	}

	void Story::PrintWords()
	{
		cout << "Word\t\tCount\n";
		for (const auto &word: words)
			cout << word.w << "\t\t" << word.count << endl;
	}

	bool Story::Analyze(vector<Genre> genres, string output_filename)
	{
		Analysis *analysis = new Analysis(this);
		for (const auto &genre : genres)
		{
			for (const auto &word : words)
			{
				
			}
		}
		return true;
	}