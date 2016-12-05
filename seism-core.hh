
#include <string>
#include <iomanip>
#include <sstream>
#include <iostream>

// helper function to generate group names
std::string padIntWithZeros(int value, int target_length)
{
  std::ostringstream ss;
  ss << std::setw(target_length) << std::setfill('0') << value;
  std::string retval = ss.str();
  return retval;
}
