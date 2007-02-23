#include"CSV_Reader.h"

CSV_Reader::CSV_Reader() {
  stream_in = new fstream();
}

CSV_Reader::~CSV_Reader() {
  if(stream_in->is_open())
    stream_in->close();
  delete stream_in;
}

const bool CSV_Reader::open(const string& filepath) {
  stream_in->open(filepath.c_str(),fstream::in);
  if(stream_in->fail() or !(stream_in->is_open()))
    return false;
  else
    return true;
}

const bool CSV_Reader::close() {
  stream_in->close();
  if(stream_in->bad() || stream_in->is_open())
    return false;
  else
    return true;
}

const bool CSV_Reader::eof() {
  return stream_in->eof();
}

void CSV_Reader::reset() {
  stream_in->seekg(ios::beg);
}

vector<string> CSV_Reader::get() {
  vector<string> foo;
  string bar;

  getline(*stream_in, bar);
  foo = split(bar,",");
  return foo;
}

vector<string> split(const string& str, const string& delimiters) {
  vector<string> tokens;
  string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  string::size_type pos = str.find_first_not_of(delimiters, lastPos);

  while(string::npos != pos || string::npos != lastPos) {
    if(lastPos != pos)
      tokens.push_back(str.substr(lastPos, pos - lastPos));
    lastPos = str.find_first_not_of(delimiters, pos);
    pos = str.find_first_of(delimiters, lastPos);
  }

  return tokens;
}

