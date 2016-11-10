#include "parse_input.h"
#include <hdf5.h>
#include <cassert>
#include <iostream>

extern unsigned int simulation_time, processor[3], chunk[3], domain[3];
extern bool coll_flg; // flag for collective I/O
extern int mpi_collective_io_int;
extern int mpi_size, mpi_rank;

using namespace std;

void parse_input()
{
  // rank 0 reads the input file, then broadcasts
  string parameter;
  string rest_of_line;
  while (true){
    cin >> parameter;
    if (parameter.at(0) == '#') continue; // ignore as comment
    if (parameter.at(0) == 0) continue; // ignore empty line
    if (!parameter.compare("DONE")) break; // exit
    if (!parameter.compare("processor"))
      cin >> processor[0] >> processor[1] >> processor[2];
    if (!parameter.compare("chunk"))
      cin >> chunk[0] >> chunk[1] >> chunk[2];
    if (!parameter.compare("domain"))
      cin >> domain[0] >> domain[1] >> domain[2];
    if (!parameter.compare("time"))
      cin >> simulation_time;
    if (!parameter.compare("use_collective"))
      mpi_collective_io_int = true;
    getline(cin, rest_of_line); // read the rest of the line
  }

  // echo the args back
  cout << "\nNumber of processes:\t" << mpi_size << endl;
  cout << "Process layout:\t\t" << processor[0] << " x " <<
    processor[1] << " x " << processor[2] << endl;
  cout << "Per process grid:\t" << domain[0] << " x " << domain[1] <<
    " x " << domain[2] << endl;
  cout << "Chunk dimensions:\t" << chunk[0] << " x " << chunk[1] <<
    " x " << chunk[2] << endl;
  cout << "Number of time steps:\t" << simulation_time << endl;
  cout << "Collective I/O:\t\t" << coll_flg << endl;
      
  // check the arguments
  assert(simulation_time > 0);
  assert(processor[0]*processor[1]*processor[2] == (hsize_t) mpi_size);
  assert(processor[0] > 1 && processor[1] > 1 && processor[2] > 1);
  assert(chunk[0] > 1 && chunk[1] > 1 && chunk[2] > 1);
  assert(domain[0] > 1 && domain[1] > 1 && domain[2] > 1);
}
