#include<string>
#include<vector>
#include<fstream>
#include<iostream>
#include<algorithm>

using namespace std;

class CSV_Reader {
 public:
  CSV_Reader();
  ~CSV_Reader();
  
  const bool open(const string& filepath);
  const bool close();
  const bool eof();

  void reset();

  vector<string> get();

 private:
  fstream* stream_in;
};

//I should probably build a string utilty header and put this in it...
vector<string> split(const string& str, const string& delimiters);
