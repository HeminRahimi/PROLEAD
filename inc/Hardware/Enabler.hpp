#pragma once

#include <memory>

#include "Hardware/Operation.hpp"
#include "Hardware/Library.hpp"

namespace Hardware {
template <typename OperationContainer>
class Enabler {
 public:


  Enabler(Cell cell, uint64_t function_index, unsigned int extended_probe_index,
          std::vector<std::unique_ptr<uint64_t[]>*> input_addresses);


  uint64_t EvaluateGlitch(std::vector<uint64_t>& input_values);
  uint64_t EvaluatePropagation(std::vector<uint64_t>& input_values);
  unsigned int GetExtendedProbeIndex();

  void SetInputAddress(unsigned int index, std::unique_ptr<uint64_t[]>* ptr);

  std::vector<std::unique_ptr<uint64_t[]>*> GetInputSignalIndices() {
    return input_addresses_;
  }

  bool operator==(const Enabler<CustomOperation>& other) const;
  bool operator<(const Enabler<CustomOperation>& other) const;

  bool CheckFunctions() {
    return cell_.GetNumberOfProbeExtensions() > 0;
  }

 private:
  Cell cell_;
  uint64_t output_index_;
  unsigned int extended_probe_index_;
  std::vector<std::unique_ptr<uint64_t[]>*> input_addresses_;
};

size_t SearchEnabler(unsigned int signal_index,
                     std::vector<Enabler<CustomOperation>>& enabler);

}  // namespace Hardware
