// pivot_compressors.h 
// By Ken Burchfiel
// Released under the MIT License

// Documentation on these functions is available
// within pivot_compressors.cpp.

#include "csv.hpp"
#include <functional>
#include <map>
#include <string>
#include <array>

using namespace csv;

  struct Pivot_Vals {
    double pivot_sum{0.0};
    long pivot_count{0};
    double pivot_mean{0.0};
  };

void scan_to_pivot(std::string &data_file_path, std::vector<
  std::string>& value_fields,
                   std::string index_headers, long &rows_to_scan,
                   std::string &pivot_file_path,
                   std::function<std::string(CSVRow)> index_gen,
                   std::map<std::string, std::vector<std::string>>
                       &include_map,
                  std::map<std::string, std::vector<std::string>>
                       &exclude_map);

std::map<std::string, std::map<std::string, Pivot_Vals>> in_memory_pivot(
    std::vector<std::map<std::string, 
    std::variant<std::string, double>>>
        &table_rows,
    std::vector<std::string> &index_fields,
    std::vector<std::string> &value_fields,
    bool save_to_csv,
    std::string &pivot_file_path,
    std::map<std::string, std::vector<std::string>> &string_include_map,
    std::map<std::string, std::vector<std::string>> &string_exclude_map,
    std::map<std::string, std::vector<double>> &double_include_map,
    std::map<std::string, std::vector<double>> &double_exclude_map);