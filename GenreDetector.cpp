#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

#define CSV_COLUMN_DELIMITER ','
typedef unsigned long long Long; // This type is for making sure that keyword Id and word count never overflow.
typedef long long SLong; // signed long
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
string ConvertToLowerCase(string str);
string ConvertToCapitalizedCase(string str);
/*** Structs & Models: ***/
struct Word
{
	string w;
	Long count;
	bool is_captial_case;
	bool is_at_sentence_start;
	Word(string _word, bool _is_at_sentence_start = false)
	{
		count = 1;
		is_at_sentence_start = _is_at_sentence_start;
		if (_word[0] >= 'A' && _word[0] <= 'Z')
			is_captial_case = true; // Special names always start with a Capital
		w = ConvertToLowerCase(_word);
		CleanSigns();
	}

	void CleanSigns();
};

struct Keyword
{
	Long id;
	string w;
	Word *story_word; // for Stories, this will connect to their matching words
	int weight; // int is used (instead of unsigned int) for negative weights (if its considered!)
	// for example, a word with negative weight can be used for decreasing the possibility of a genre

	Keyword(string _w, int _weight)
	{
		w = ConvertToLowerCase(_w);
		weight = _weight;
		story_word = nullptr;
	}
};

struct Genre
{
	static short number_of_genres; // really we dont have more than 65536 genres in anything!
	vector<Keyword> keywords;
	Int id;
	string title;

	Genre(string _title)
	{
		title = _title;
		id = ++Genre::number_of_genres; // id starts from 1, 
	}

	Genre(string _title, vector<Keyword> _keywords)
	{
		title = _title;
		keywords = _keywords;
	}

	static Genre *CreateGenreFromCSV(string filename);
	Int NumberOfLinkedKeywords()
	{
		int n = 0;
		for (const auto &k : keywords) if (k.story_word != nullptr) n++;
		return n;
	}
};

short Genre::number_of_genres = 0;

struct Prediction
{
	Genre *genre;
	float confidence; // The accuracy of the prediction
	vector<Keyword *> common_words;
	SLong m_i; // sum of weights * counts
	Long number_of_keywords; // absolute number of keywords in the story, considering their counts.
	Prediction(Genre *_genre)
	{
		genre = _genre;
		m_i = 0;
		number_of_keywords = 0;
	}

	void ComputeConfidence(int sum_of_all_genre_weights);
	void ComputeGenreWeight(); // compute m_i
	string ToCSVColumn();
	void FindCommonWords();
};


struct Story
{
	// Story and its analysis are linked two ways.
	struct Analysis
	{
		static Long number_of_analysis;
		Int id;
		Story *story;
		vector<Prediction *> predictions; // an array of predictions
		vector<Long> genre_keywords_total_counts; // number of keywords in the storys, by the order of imported genres
		int sum_of_all_genre_weights;

		Analysis(Story *_story)
		{
			story = _story;
			id = ++number_of_analysis;
			sum_of_all_genre_weights = 0;
		}

		string ToCSVColumn();
		void ComputeConfidences();
		string ToString();

		void SortPredictions();
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

Long Story::Analysis::number_of_analysis = 0;

/*** UI Structures & enums ***/
enum Commands {
	CMD_IMPORT_STORY = (char)0,
	CMD_SHOW_THE_LIST_OF_STORIES,
	CMD_ANALYZE_STORY,
	CMD_ANALYZED_SROTIES_LIST,
	CMD_DUMP_ANALYZED_STORIES,
	CMD_EXIT,
	CMD_SHOW_STORY_ANALYSIS,
	CMD_SHOW_COMMANDS,
};

// Main:
int main()
{
	/*** Initial Data **/
	const short NUMBER_OF_GENRES = 4;
	const string GENRE_FILENAMES[] = { 
		"Romance.csv",
		"Mystery.csv",
		"Fantasy.csv",
		"SciFi.csv"
	}; // Genre file names:
	const short NUMBER_OF_PROGRAM_COMMANDS = 8;
	const string PROGRAM_COMMANDS[] = { // List of program commands:
		"import_story",
		"show_the_list_of_stories",
		"analyze_story",
		"analyzed_stories_list",
		"dump_analyzed_stories",
		"exit",
		"show_story_analysis",
		"show_the_list_of_commands"
	};
	const string PROGRAM_COMMANDS_PARAM_LIST[] = {
		"{filename.txt}",
		"",
		"{story_index} {output_file_name.txt}",
		"",
		"{output_file_name.csv}",
		"",
		"{story_index}",
		""
	};
	/*** Data Preprocessing: ***/
	vector<Genre> common_genres;
	vector<Story *> stories;
	vector<Story::Analysis *> story_analyzes;
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
				cout << PROGRAM_COMMANDS[CMD_IMPORT_STORY] << " " << PROGRAM_COMMANDS_PARAM_LIST[CMD_IMPORT_STORY] << endl;
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
			SLong index = -1;
			iss_line >> index;

			string output_filename;
			iss_line >> output_filename;
			if (index == -1 || !output_filename[0] || output_filename.empty())
			{
				cout << PROGRAM_COMMANDS[CMD_ANALYZE_STORY] << " " << PROGRAM_COMMANDS_PARAM_LIST[CMD_ANALYZE_STORY] << endl;
				continue;
			}
			if (index < 1 || index > stories.size())
			{
				cout << "Invalid story index." << endl;
				continue;
			}
			stories[--index]->Analyze(common_genres, output_filename);
			story_analyzes.push_back(stories[index]->analysis);
			string str_analysis = stories[index]->analysis->ToString();
			cout << str_analysis;
			ofstream output_file(output_filename);
			if (!output_file.is_open() || !output_file.good() || output_file.fail())
			{
				cout << "Error openning file." << endl;
				continue;
			}
			output_file << str_analysis;
			output_file.close();
		}
		else if (command == PROGRAM_COMMANDS[CMD_SHOW_STORY_ANALYSIS])
		{
			SLong index = -1;
			iss_line >> index;
			if (index == -1)
			{
				cout << index << PROGRAM_COMMANDS[CMD_SHOW_STORY_ANALYSIS] << " " << PROGRAM_COMMANDS_PARAM_LIST[CMD_SHOW_STORY_ANALYSIS] << endl;
				continue;
			}
			if (index < 1 || index > stories.size())
			{
				cout << "Invalid story index." << endl;
				continue;
			}
			if (stories[--index]->analysis == nullptr)
			{
				cout << "This story has not been analyzed yet. Please use the " << PROGRAM_COMMANDS[CMD_ANALYZE_STORY] << " command." << endl;
				continue;
			}
			cout << stories[index]->analysis->ToString();
		}
		else if (command == PROGRAM_COMMANDS[CMD_ANALYZED_SROTIES_LIST])
		{
			Long number_of_analysis = story_analyzes.size();
			if (!number_of_analysis)
			{
				cout << "No analyzed stories." << endl;
				continue;
			}

			cout << "The analyzed stories are: ";
			cout << ConvertToCapitalizedCase(story_analyzes[0]->story->name);
			for (Int i = 1; i < number_of_analysis - 1; i++)
				cout << ", " << ConvertToLowerCase(story_analyzes[i]->story->name);
			if (number_of_analysis > 1)
				cout << " and " << ConvertToLowerCase(story_analyzes[number_of_analysis - 1]->story->name);
			cout << "." << endl;
		}
		else if (command == PROGRAM_COMMANDS[CMD_SHOW_THE_LIST_OF_STORIES])
		{
			if (!stories.size())
			{
				cout << "No imported stories." << endl; // Not mentioned in the project description!
				continue;
			}
			cout << "List of all imported stories are:" << endl;
			for (Long i = 0; i < stories.size(); i++)
				cout << i + 1 << ". " << ConvertToCapitalizedCase(stories[i]->name) << endl;
			
		}
		else if (command == PROGRAM_COMMANDS[CMD_SHOW_COMMANDS])
		{
			for (short i = 0; i < NUMBER_OF_PROGRAM_COMMANDS; i++)
			{
				cout << PROGRAM_COMMANDS[i];
				if (!PROGRAM_COMMANDS_PARAM_LIST[i].empty())
					cout << " " << PROGRAM_COMMANDS_PARAM_LIST[i];
				cout << endl;
			}
		}
		else if (command == PROGRAM_COMMANDS[CMD_DUMP_ANALYZED_STORIES])
		{
			string output_filename;
			iss_line >> output_filename;
			if (!output_filename[0] || output_filename.empty())
			{
				cout << PROGRAM_COMMANDS[CMD_DUMP_ANALYZED_STORIES] << " " << PROGRAM_COMMANDS_PARAM_LIST[CMD_DUMP_ANALYZED_STORIES] << endl;
				continue;
			}
			if (!story_analyzes.size())
			{
				cout << "No analyzed stories to dump." << endl;
				continue;
			}
			ofstream output_file(output_filename);
			if (!output_file.is_open() || !output_file.good() || output_file.fail())
			{
				cout << "Error openning file." << endl;
				continue;
			}
			// Create csv header :
			output_file << "Story, Genre, Confidence, ";
			for (const auto &genre : common_genres)
				output_file << genre.title << " Words, ";

			output_file << "Common Keyword 1, Common Keyword 2, Common Keyword 3, Common Keyword 4\n";
			for (const auto &analysis : story_analyzes)
			{
				output_file << analysis->ToCSVColumn();
			}

			output_file.close();
		}
		else if (command != PROGRAM_COMMANDS[CMD_EXIT])
			// all command list checked. command is invalid!
			cout << "Command not found. See the list of commands with " << PROGRAM_COMMANDS[CMD_SHOW_COMMANDS] << "." << endl;
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

string ConvertToLowerCase(string str)
{
	string result = "";
	for (char c : str)
	{
		result += tolower(c);
	}
	return result;
}

string ConvertToCapitalizedCase(string str)
{
	string str_cap = ConvertToLowerCase(str);
	str_cap[0] = toupper(str_cap[0]);
	return str_cap;
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
		bool previous_sentence_ended = true;
		char last_char;
		while (story_file >> str_word) // Read the file word by word
		{
			last_char = str_word[str_word.length() - 1];
			Word word(str_word, previous_sentence_ended);
			AddWord(word); // Add word to the words vector, or increase its count if it exists already.
			previous_sentence_ended = last_char == '.' || last_char == '?'
				|| last_char == '!' || last_char == '\n' || last_char == ':' || last_char == ';';
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
			if (!word.is_captial_case) // if it was a Person name, it must be always captial!
				words[i].is_captial_case = false;
		}

	}

	void Story::PrintWords()
	{
		cout << "Word\t\tCount\n";
		for (const auto &word : words)
			cout << word.w << "\t\t" << word.count << endl;
	}

	bool Story::Analyze(vector<Genre> genres, string output_filename)
	{
		analysis = new Analysis(this);
		analysis->sum_of_all_genre_weights = 0;
		for (auto &genre : genres)
		{
			Prediction *prediction = new Prediction(new Genre(genre));
			for (auto &keyword : prediction->genre->keywords)
			{
				keyword.story_word = nullptr;
				for (auto &word : words) // Check all the words of story for KeyWord match:
				{
					if (keyword.w == word.w)
					{

						keyword.story_word = &word; // Connect the word to its matched keyword; remember word contains the .count variable which is computed before wile loading,
						// So there is no need to count anything in the story.
						break;
					}
				}
			}

			prediction->ComputeGenreWeight(); // m_i calculation
			analysis->sum_of_all_genre_weights += prediction->m_i;
			prediction->FindCommonWords();
			analysis->predictions.push_back(prediction);
			analysis->genre_keywords_total_counts.push_back(prediction->number_of_keywords);
		}

		analysis->ComputeConfidences(); // sompute each prediction confidence
		analysis->SortPredictions();
		return true;
	}

/*** struct Story::Analysis ***/
	void Story::Analysis::ComputeConfidences()
	{
		for (auto &prediction : predictions)
			prediction->ComputeConfidence(sum_of_all_genre_weights);
	}

	string Story::Analysis::ToString()
	{
		ostringstream oss;
		oss << "Story Name: " << ConvertToCapitalizedCase(story->name) << endl;
		oss << "Predicted Genre: " << predictions[0]->genre->title << endl;
		oss << "Genre, Number of Keywords, Confidence\n";
		for (const auto &prediction : predictions)
			oss << prediction->ToCSVColumn();
	
		return oss.str();
	}

	void Story::Analysis::SortPredictions()
	{ // sort predictions order by confidence in a descending order
		for (short i = 0; i < predictions.size(); i++)
		{
			for (short j = i + 1; j < predictions.size(); j++)
			{
				if (predictions[i]->confidence < predictions[j]->confidence)
				{
					// swap their positions
					Prediction *temp = predictions[i];
					predictions[i] = predictions[j]; // Change predictions[i] reference to the Prediction with ahigher value of confidence.
					predictions[j] = temp; // Send lesser-in-value confidence Predeictions to the right.
				}
			}
		}
	}

	string Story::Analysis::ToCSVColumn()
	{
		ostringstream oss;
		oss << story->name << ", " << predictions[0]->genre->title << ", " << fixed << setprecision(6) << predictions[0]->confidence << ", ";

		// Genre Keywords
		for (const auto &genre_keywords_count : genre_keywords_total_counts)
		{
			oss << genre_keywords_count << ", ";
		}
		Long number_of_common_words = predictions[0]->common_words.size();
		if (number_of_common_words > 4)
			number_of_common_words = 4;
		for (Long i = 0; i < number_of_common_words - 1; i++)
			oss << predictions[0]->common_words[i]->w << ", ";
		if (number_of_common_words > 0)
			oss << predictions[0]->common_words[number_of_common_words - 1]->w << "\n";
		return oss.str();
	}


/***struct Prediction ***/
	void Prediction::ComputeGenreWeight()
	{
		m_i = 0;
		for (const auto &keyword : genre->keywords)
			if (keyword.story_word != nullptr)
			{
				m_i += keyword.weight * keyword.story_word->count;
				number_of_keywords += keyword.story_word->count;
			}
	}

	void Prediction::ComputeConfidence(int sum_of_all_genre_weights)
	{
		confidence = static_cast<float>(m_i) / static_cast<float>(sum_of_all_genre_weights);
	}

	string Prediction::ToCSVColumn()
	{
		ostringstream oss;
		oss << genre->title << ", " << number_of_keywords << ", " << fixed << setprecision(6) << confidence * 100 << "%\n";
		return oss.str();
	}

	void Prediction::FindCommonWords()
	{
		common_words.clear();
		for (auto &keyword : genre->keywords)
			if (keyword.story_word != nullptr)
				common_words.push_back(&keyword);
		// Sort keywords by counts in the story
		for (int i = 0; i < common_words.size(); i++)
		{
			for (int j = i + 1; j < common_words.size(); j++)
			{
				if (common_words[i]->story_word->count < common_words[j]->story_word->count)
				{
					// Swap references:
					Keyword *temp = common_words[i];
					common_words[i] = common_words[j];
					common_words[j] = temp;
				}
			}
		}

	}