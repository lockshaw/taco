#include "taco/io/mtx_file_format.h"

#include <iostream>
#include <sstream>
#include <cstdlib>

#include "taco/tensor_base.h"
#include "taco/util/error.h"

using namespace std;

namespace taco {
namespace io {
namespace mtx {

void readFile(std::ifstream &mtxfile, int blockSize,
              int* nrow, int* ncol, int* nnzero,
              TensorBase* tensor) {

  std::string line;
  int rowind,colind;
  double value;
  std::string val;
  while(std::getline(mtxfile,line)) {
    std::stringstream iss(line);
    char firstChar;
    iss >> firstChar;
    // Skip comments
    if (firstChar != '%') {
      iss.clear();
      iss.str(line);
      iss >> *nrow >> *ncol >> *nnzero;
      break;
    }
  }

  if (blockSize == 1) {
    while(std::getline(mtxfile,line)) {
      std::stringstream iss(line);
      iss >> rowind >> colind >> val;
      value = std::stod(val);
      if (value != 0.0)
        tensor->insert({rowind-1,colind-1},value);
    }
  }
  else {
    while(std::getline(mtxfile,line)) {
      std::stringstream iss(line);
      iss >> rowind >> colind >> val;
      value = std::stod(val);
      if (value != 0.0)
        tensor->insert({(rowind-1)/blockSize, (colind-1)/blockSize,
                        (rowind-1)%blockSize, (colind-1)%blockSize},value);
    }
  }

  tensor->pack();
}

void writeFile(std::ofstream &mtxfile, std::string name,
               const std::vector<int> dimensions, int nnzero) {
  mtxfile << "%-----------------------------------" << std::endl;
  mtxfile << "% MTX matrix file generated by taco " << std::endl;
  mtxfile << "% name: " << name << std::endl;
  mtxfile << "%-----------------------------------" << std::endl;
  for (size_t i=0; i<dimensions.size(); i++) {
    mtxfile << dimensions[i] << " " ;
  }
  mtxfile << " " << nnzero << std::endl;
}

TensorBase readFile(std::ifstream& file, std::string name) {

  string line;
  if (!std::getline(file, line)) {
    return TensorBase();
  }

  // Skip comments at the top of the file
  string token;
  do {
    std::stringstream lineStream(line);
    lineStream >> token;
    if (token[0] != '%') {
      break;
    }
  } while (std::getline(file, line));

  // The first non-comment line is the header with dimension sizes and nnz
  int rows, cols;
  size_t nnz;
  std::stringstream lineStream(line);
  lineStream >> rows;
  lineStream >> cols;
  lineStream >> nnz;

  constexpr size_t order = 2;
  constexpr size_t coordinateSize = order*sizeof(int) + sizeof(double);
  vector<char> coordinates_(nnz * coordinateSize);
  char* coordinatesPtr = coordinates_.data();
  while (std::getline(file, line)) {
    int rowIdx, colIdx;
    double val;
    std::stringstream lineStream(line);
    lineStream >> rowIdx;
    lineStream >> colIdx;
    lineStream >> val;

    int* coordinateLoc = (int*)coordinatesPtr;
    *coordinateLoc++ = rowIdx;
    *coordinateLoc++ = colIdx;
    double* valueLoc = (double*)coordinateLoc;
    *valueLoc = val;
    coordinatesPtr = (char*)(valueLoc+1);
  }

  TensorBase tensor(name, ComponentType::Double, {rows,cols});
  tensor.insert(coordinates_);

  return tensor;
}


}}}
