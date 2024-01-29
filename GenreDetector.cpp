#include <iostream>
#include <vector>

using namespace std;

// Structs & Models:
struct Word
{
    string value;
    int count;

    Word(string _word)
    {
        count = 0;
        value = _word;
    }
};

struct Keyword
{
    string word;
    int weight;

    Keyword(string _word, int _weight)
    {
        word = _word;
        weight = _weight;
    }
};
struct Genre
{
    vector<Word> keywords;
    string title;
};

struct Prediction
{
    Genre *genre;
    float confidence; // The accuracy of the prediction
};

struct Story
{
    vector<Word> words;
    string name, filename;
    int index;
    int number_of_sentences;

    Story(string _filename, unsigned int _index)
    {
        filename = _filename;
        SetName();
        index = _index;
        LoadWords(); // load words vector
        number_of_sentences = 0;
    }

    void SetName()
    {
        // remove file extension from filename and set it on name
        name = filename;
        int index_of_dot = name.length();
        while (name[--index_of_dot] != '.' && index_of_dot > 0) // find the position of dot
            ;
        if (index_of_dot > 0)          // if filename has a .extension part :
            name[index_of_dot] = '\0'; // remove .extension part of the filenbame
    }
    void LoadWords() // read story from file name;
    {
        // read file word by word, and count senmtences
    }
};

struct Analysis
{
    Story *story;
    Prediction *prediction;
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
// Function Prototypes:

// Main:
int main()
{

    // Data Preprocessing:

    string command = "";

    // Menu & Main App Loop
    while (command != "exit")
    {

        // Command Check:
    }
    return 0;
}

// Function Declerations:
