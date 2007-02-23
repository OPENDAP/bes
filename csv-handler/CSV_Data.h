
#include<string>

#include"CSV_Header.h"

using namespace std;

static const char STRING[]  = "String";
static const char BYTE[]    = "Byte";
static const char INT32[]   = "Int32";
static const char INT16[]   = "Int16";
static const char FLOAT64[] = "Float64";
static const char FLOAT32[] = "Float32";

class CSV_Data {
 public:
  CSV_Data();
  ~CSV_Data();

  void insert(CSV_Field* field, void* value);

  void* getData();
  string getType();

 private:
  void* data;
  string type;
  bool initialized;
};
