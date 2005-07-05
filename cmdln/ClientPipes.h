#ifndef ClientPipes_h_
#define ClientPipes_h_ 1

class DAS;
class DDS;

class ClientPipes
{
public:
  static int das_magic_stdout(DAS &das);
  static int dds_magic_stdout(DDS *pdds);
};

#endif // ClientPipes_h_
