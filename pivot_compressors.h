// pivot_compressors.h 
// By Ken Burchfiel
// Released under the MIT License

#include "csv.hpp"
#include <functional>
#include <map>
#include <string>
#include <array>

using namespace csv;

void scan_to_pivot(std::string &data_file_path, std::vector<
  std::string>& value_fields,
                   std::string index_headers, long &rows_to_scan,
                   std::string &pivot_file_path,
                   std::function<std::string(CSVRow)> index_gen,
                   std::map<std::string, std::vector<std::string>>
                       &include_map,
                  std::map<std::string, std::vector<std::string>>
                       &exclude_map);

// Documentation on this function is available
// within pivot_compressors.cpp.