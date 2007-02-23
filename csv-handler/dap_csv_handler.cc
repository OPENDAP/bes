#include<string>

#include"DODSFilter.h"
#include"DAS.h"
#include"DDS.h"
#include"DataDDS.h"

#include"ObjectType.h"
#include"cgi_util.h"
#include"ConstraintEvaluator.h"

#include"CSVDDS.h"
#include"CSVDAS.h"

using namespace std;

const string cgi_version = PACKAGE_VERSION;

int main(int argc, char* argv[]) {

  try {
    DODSFilter df(argc, argv);
    //...

    //is this only for server3?

  } catch(Error& e) {
    set_mime_text(stdout, dods_error, cgi_version);
    e.print(stdout);
    return EXIT_ERROR;
  }

  return EXIT_SUCCESS;
}
