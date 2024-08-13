#include "Util/Settings.hpp"

void Settings::ParseArrayOfTriStateBitVectors(
    const boost::json::object& json_object, const std::string& identifier,
    uint64_t number_of_values, bool required, bool check_number_of_values,
    std::vector<std::vector<TriStateBit>>& values) {
  boost::json::array json_array;
  std::string error_context = "Error while parsing \"" + identifier + "\": ";

  if (SetValue(json_object, identifier, json_array)) {
    if ((json_array.size() == number_of_values) || !check_number_of_values) {
      std::string value_string;
      uint64_t bit_width = 0;

      for (auto& array_value : json_array) {
        if (array_value.is_string()) {
          value_string = array_value.as_string().c_str();
          VerilogBitstringGrammar grammar(value_string);
          values.push_back(grammar.Parse(value_string));

          if (bit_width == 0 || bit_width == values.back().size()) {
            bit_width = values.back().size();
          } else {
            throw std::invalid_argument(error_context + "Size mismatch in \"" +
                                        value_string + "\"!");
          }
        } else {
          throw std::invalid_argument(error_context + "Invalid format!");
        }
      }
    } else {
      throw std::invalid_argument(error_context +
                                  "Invalid number of array elements!");
    }
  } else {
    if (required) {
      throw std::runtime_error(error_context + "Key \"" + identifier +
                               "\" not found!");
    }
  }
}

void Settings::ParseFiniteField(const boost::json::object& json_object,
                                const std::string& identifier,
                                FiniteFieldSettings& settings) {
  boost::json::object finite_field;
  std::string error_context = "Error while parsing \"" + identifier + "\": ";

  settings.base = 2;
  settings.exponent = 1;
  settings.is_additive = true;
  settings.excluded_elements.clear();
  settings.irreducible_polynomial.clear();

  if (SetValue(json_object, identifier, finite_field)) {
    if (!SetValue(finite_field, SettingNames::BASE, settings.base)) {
      throw std::runtime_error(error_context + "\"" + SettingNames::BASE +
                               "\" not found!");
    }

    if (!SetValue(finite_field, SettingNames::EXPONENT, settings.base)) {
      throw std::runtime_error(error_context + "\"" + SettingNames::EXPONENT +
                               "\" not found!");
    }

    SetValue(finite_field, SettingNames::IS_ADDITIVE, settings.is_additive);
    ParseArrayOfTriStateBitVectors(finite_field,
                                   SettingNames::EXCLUDED_ELEMENTS, 0, false,
                                   false, settings.excluded_elements);

    if (finite_field.contains(SettingNames::IRREDUCIBLE_POLYNOMIAL)) {
      std::string polynomial_string;
      SetValue(finite_field, SettingNames::IRREDUCIBLE_POLYNOMIAL,
               polynomial_string);
      IrreduciblePolynomialGrammar grammar;
      settings.irreducible_polynomial =
          grammar.Parse(polynomial_string, settings.base, settings.exponent);
    }
  }
}

void Settings::ParsePerformanceSettings(const boost::json::object& json_object,
                                        const std::string& identifier,
                                        PerformanceSettings& settings) {
  std::string error_context = "Error while parsing \"" + identifier + "\": ";
  std::string parsed_value;
  CpuCoreSelector cpu_core_selector;
  boost::json::object performance_object;

  settings.number_of_threads = 1;
  settings.minimize = Minimization::none;
  settings.compact_distributions = false;
  settings.remove_full_probing_sets = false;
  settings.number_of_entries_in_report = 0;
  settings.number_of_probing_sets_per_step = UINT64_MAX;

  if (SetValue(json_object, identifier, performance_object)) {
    if (SetValue(performance_object, SettingNames::NUMBER_OF_THREADS,
                 parsed_value)) {
      settings.number_of_threads =
          cpu_core_selector.GetOptimalNumberOfThreads(parsed_value);
    }

    if (SetValue(performance_object, SettingNames::MINIMIZE_PROBING_SETS, parsed_value)) {
      if (is_target_hardware_) {
        if (parsed_value == "no") {
          settings.minimize = Minimization::none;
        } else if (parsed_value == "trivial") {
          settings.minimize = Minimization::trivial;
        } else if (parsed_value == "aggressive") {
          settings.minimize = Minimization::aggressive;
        } else {
          throw std::invalid_argument(error_context + "Invalid argument for \"" +
                                      SettingNames::MINIMIZE_PROBING_SETS + "\"!");
        }
      } else {
        throw std::runtime_error(error_context + "Unsupported key \"" +
                                 SettingNames::MINIMIZE_PROBING_SETS + "\"!");
      }
    }

    SetValue(performance_object, SettingNames::COMPACT_DISTRIBUTIONS,
             settings.compact_distributions);
    SetValue(performance_object, SettingNames::REMOVE_FULL_PROBING_SETS,
             settings.remove_full_probing_sets);
    SetValue(performance_object, SettingNames::NUMBER_OF_ENTRIES_IN_REPORT,
             settings.number_of_entries_in_report);
    SetValue(performance_object, SettingNames::NUMBER_OF_PROBING_SETS_PER_STEP,
             settings.number_of_probing_sets_per_step);
  }
}

void Settings::ParseHardwareSettings(const boost::json::object& json_object,
                                     const std::string& identifier,
                                     HardwareSettings& settings) {
  std::string error_context = "Error while parsing \"" + identifier + "\": ";
  boost::json::object hardware_object;

  if (is_target_hardware_) {
    if (SetValue(json_object, identifier, hardware_object)) {
      if (!SetValue(hardware_object, SettingNames::CLOCK_SIGNAL_NAME,
                    settings.clock_signal_name)) {
        throw std::runtime_error(error_context + "Key \"" +
                                 SettingNames::CLOCK_SIGNAL_NAME +
                                 "\" not found!");
      }
    } else {
      throw std::runtime_error(error_context + "Key \"" + identifier +
                               "\" not found!");
    }
  } else {
    if (SetValue(json_object, identifier, hardware_object)) {
      throw std::runtime_error(error_context + "Unsupported key \"" +
                               identifier + "\"!");
    }
  }
}

void Settings::ParseSoftwareSettings(const boost::json::object& json_object,
                                     const std::string& identifier,
                                     SoftwareSettings& settings) {
  std::string error_context = "Error while parsing \"" + identifier + "\": ";
  boost::json::object software_object;
  settings.number_of_pipeline_stages = 3;
  settings.compiler_flags = "";

  if (is_target_hardware_) {
    if (SetValue(json_object, identifier, software_object)) {
      throw std::runtime_error(error_context + "Unsupported key \"" +
                               identifier + "\"!");
    }
  } else {
    if (SetValue(json_object, identifier, software_object)) {
      SetValue(software_object, SettingNames::COMPILER_FLAGS, settings.compiler_flags);

      if (!SetValue(software_object, SettingNames::LOCATION_OF_CIPHER,
                    settings.location_of_cipher)) {
        throw std::runtime_error(error_context + "Key \"" +
                                 SettingNames::LOCATION_OF_CIPHER +
                                 "\" not found!");
      }

      SetValue(software_object, SettingNames::NUMBER_OF_PIPELINE_STAGES,
               settings.number_of_pipeline_stages);

      if (settings.number_of_pipeline_stages > 6) {
        throw std::out_of_range(
            error_context + "The maximum supported value for key \"" +
            SettingNames::NUMBER_OF_PIPELINE_STAGES +
            "\" is 6 (for the Cortex M7). If you require a higher number of "
            "pipeline stages please contact the Jannik Zeitschner "
            "(jannik.zeitschner@rub.de)!");
      }
    } else {
      throw std::runtime_error(error_context + "Key \"" + identifier +
                               "\" not found!");
    }
  }
}

void Settings::ParseGroups(const boost::json::object& json_object,
                           std::vector<std::vector<TriStateBit>>& groups) {
  std::string error_context =
      "Error while parsing \"" + SettingNames::GROUPS + "\": ";
  boost::json::array groups_array;

  if (SetValue(json_object, SettingNames::GROUPS, groups_array)) {
    std::string value_string;
    uint64_t element_size = std::ceil(std::log2l(input_finite_field.base)) *
                            input_finite_field.exponent;

    for (auto& group : groups_array) {
      if (group.is_string()) {
        value_string = group.as_string().c_str();
        VerilogBitstringGrammar grammar(value_string);
        groups.push_back(grammar.Parse(value_string));

        if (groups.back().size() % element_size) {
          throw std::invalid_argument(error_context + "Size mismatch in \"" +
                                      value_string + "\"!");
        }
      } else {
        throw std::invalid_argument(error_context + "Invalid format!");
      }
    }
  } else {
    throw std::runtime_error(error_context + "Key \"" + SettingNames::GROUPS +
                             "\" not found!");
  }
}

void Settings::ParseAlwaysRandomInputs(
    const boost::json::object& json_object,
    std::vector<std::vector<std::string>>& always_random_input_signals) {
  std::string error_context =
      "Error while parsing \"" + SettingNames::ALWAYS_RANDOM_INPUTS + "\": ";
  boost::json::array json_array;

  if (is_target_hardware_) {
    if (SetValue(json_object, SettingNames::ALWAYS_RANDOM_INPUTS, json_array)) {
      SignalNameGrammar grammar;
      std::vector<std::string> signal_names, parsed;
      std::string signal_name;
      uint64_t element_size = std::ceil(std::log2l(input_finite_field.base)) *
                              input_finite_field.exponent;
      uint64_t number_of_elements;

      for (auto& input_signal : json_array) {
        if (input_signal.is_string()) {
          signal_name = input_signal.as_string().c_str();
          parsed = grammar.Parse(signal_name);
          signal_names.insert(signal_names.end(), parsed.begin(), parsed.end());

          if (signal_names.size() % element_size) {
            throw std::invalid_argument(error_context +
                                        "Size mismatch after parsing \"" +
                                        signal_name + "\"!");
          }
        } else {
          throw std::invalid_argument(error_context + "Invalid format!");
        }
      }

      number_of_elements = signal_names.size() / element_size;
      always_random_input_signals.resize(number_of_elements);

      for (uint64_t index = 0; index < number_of_elements; ++index) {
        always_random_input_signals[index] = std::vector<std::string>(
            signal_names.begin() + index * element_size,
            signal_names.begin() + (index + 1) * element_size);
      }
    }
  } else {
    if (SetValue(json_object, SettingNames::ALWAYS_RANDOM_INPUTS, json_array)) {
      throw std::runtime_error(error_context + "Unsupported key \"" +
                               SettingNames::ALWAYS_RANDOM_INPUTS + "\"!");
    }
  }
}

void Settings::ParseSignalNameValuePair(
    const boost::json::object& json_object, const std::string& identifier,
    std::vector<std::pair<std::string, bool>>& name_value_pairs) {
  std::string error_context = "Error while parsing \"" + identifier + "\": ";
  boost::json::array json_array;

  if (SetValue(json_object, identifier, json_array)) {
    std::string signal_name, signal_value;
    SignalNameGrammar name_grammar;
    std::vector<TriStateBit> values;
    std::vector<std::string> names;

    for (auto& name_value_pair : json_array) {
      if (name_value_pair.is_object()) {
        if (name_value_pair.as_object().contains("name") &&
            name_value_pair.as_object().contains("value")) {
          SetValue(name_value_pair.as_object(), "name", signal_name);
          SetValue(name_value_pair.as_object(), "value", signal_value);
          VerilogBitstringGrammar value_grammar(signal_value);
          names = name_grammar.Parse(signal_name);
          values = value_grammar.Parse(signal_value);

          for (uint64_t index = 0; index < names.size(); ++index) {
            if (values[index] == TriStateBit::zero_value) {
              name_value_pairs.push_back(std::make_pair(names[index], false));
            } else if (values[index] == TriStateBit::one_value) {
              name_value_pairs.push_back(std::make_pair(names[index], true));
            } else {
              throw std::invalid_argument(error_context +
                                          "Invalid argument for \"value\"!");
            }
          }
        } else {
          if (name_value_pair.as_object().contains("name")) {
            throw std::invalid_argument(error_context +
                                        "Key \"value\" not found!");
          } else {
            throw std::invalid_argument(error_context +
                                        "Key \"name\" not found!");
          }
        }
      } else {
        throw std::invalid_argument(error_context + "Invalid format!");
      }
    }
  }
}

void Settings::ParseEndCondition(
    const boost::json::object& json_object,
    uint64_t& end_condition_clock_cycles,
    std::vector<std::pair<std::string, bool>>& end_condition_signals) {
  std::string error_context =
      "Error while parsing \"" + SettingNames::END_CONDITION + "\": ";
  boost::json::object end_condition;

  if (SetValue(json_object, SettingNames::END_CONDITION, end_condition)) {
    if (end_condition.contains("clock_cycles") &&
        end_condition.contains("signals")) {
      throw std::invalid_argument(
          error_context + "Redundant keys \"clock_cycles\" and \"signals\"!");
    } else if (end_condition.contains("clock_cycles") &&
               !end_condition.contains("signals")) {
      SetValue(end_condition, "clock_cycles", end_condition_clock_cycles);
    } else if (!end_condition.contains("clock_cycles") &&
               end_condition.contains("signals")) {
      ParseSignalNameValuePair(end_condition, "signals", end_condition_signals);
    }
  }
}

void Settings::ParseOutputShares(
    const boost::json::object& json_object,
    std::vector<std::vector<std::string>>& output_shares) {
  std::string error_context =
      "Error while parsing \"" + SettingNames::OUTPUT_SHARES + "\": ";
  boost::json::array json_array;

  if (SetValue(json_object, SettingNames::OUTPUT_SHARES, json_array)) {
    SignalNameGrammar grammar;
    std::string signal_name;
    uint64_t bit_width = 0;

    for (auto& value : json_array) {
      if (value.is_string()) {
        signal_name = value.as_string().c_str();
        output_shares.push_back(grammar.Parse(signal_name));
        if (bit_width == 0 || bit_width == output_shares.back().size()) {
          bit_width = output_shares.back().size();
        } else {
          throw std::invalid_argument(error_context + "Size mismatch in \"" +
                                      signal_name + "\"!");
        }
      } else {
        throw std::invalid_argument(error_context + "Invalid format!");
      }
    }
  }
}

void Settings::ParseInputSequence(
    const boost::json::object& json_object,
    std::vector<std::vector<InputAssignment>>& input_sequence) {
  std::string error_context =
      "Error while parsing \"" + SettingNames::INPUT_SEQUENCE + "\": ";
  boost::json::array input_sequence_json, signals_json;

  if (SetValue(json_object, SettingNames::INPUT_SEQUENCE,
               input_sequence_json)) {
    boost::json::object clock_cycle_obj, signal_obj;
    std::string signal_name, type_name, value_name;
    InputAssignmentGrammar value_grammar;
    SignalNameGrammar name_grammar;
    InputAssignment input_assignment;
    std::vector<InputAssignment> inputs_per_cycle;

    for (auto& clock_cycle : input_sequence_json) {
      if (clock_cycle.is_object()) {
        clock_cycle_obj = clock_cycle.as_object();

        if (SetValue(clock_cycle_obj, "signals", signals_json)) {
          for (auto& signal : signals_json) {
            if (signal.is_object()) {
              signal_obj = signal.as_object();

              if (signal_obj.contains("name") && signal_obj.contains("value")) {
                if (signal_obj.at("name").is_string() &&
                    signal_obj.at("value").is_string()) {
                  value_name = signal_obj.at("value").as_string().c_str();
                  input_assignment = value_grammar.Parse(value_name);
                  signal_name = signal_obj.at("name").as_string().c_str();
                  input_assignment.signal_names_ =
                      name_grammar.Parse(signal_name);

                  if (input_assignment.signal_names_.size() ==
                          input_assignment.group_indices_.size() ||
                      input_assignment.signal_names_.size() ==
                          input_assignment.signal_values_.size()) {
                    if (signal_obj.contains("type")) {
                      if (signal_obj.at("type").is_string()) {
                        type_name = signal_obj.at("type").as_string().c_str();

                        if (type_name == "var") {
                          input_assignment.type_ = ValueType::variable;
                        } else if (type_name == "arr") {
                          input_assignment.type_ = ValueType::arary;
                        } else {
                          throw std::runtime_error(
                              error_context +
                              "Invalid argument for \"type\" for \"signal\" " +
                              signal_name + " and \"value\" " + value_name +
                              "!");
                        }

                      } else {
                        throw std::runtime_error(
                            error_context +
                            "Invalid format in \"type\" for \"signal\" " +
                            signal_name + " and \"value\" " + value_name + "!");
                      }
                    } else {
                      if (is_target_hardware_) {
                        input_assignment.type_ = ValueType::standard;
                      } else {
                        throw std::runtime_error(error_context + "Key \"type\" not found!");
                      }
                    }

                    inputs_per_cycle.push_back(input_assignment);
                  } else {
                    throw std::runtime_error(
                        error_context + "Size mismatch between \"signal\" " +
                        signal_name + " and \"value\" " + value_name + "!");
                  }
                } else if (signal_obj.at("name").is_string()) {
                  throw std::runtime_error(error_context +
                                           "Invalid format in \"value\"!");
                } else if (signal_obj.at("value").is_string()) {
                  throw std::runtime_error(error_context +
                                           "Invalid format in \"name\"!");
                } else {
                  throw std::runtime_error(
                      error_context +
                      "Invalid format in \"name\" and \"value\"!");
                }
              } else if (signal_obj.contains("name")) {
                throw std::runtime_error(error_context +
                                         "Key \"value\" not found!");
              } else if (signal_obj.contains("value")) {
                throw std::runtime_error(error_context +
                                         "Key \"name\" not found!");
              } else {
                throw std::runtime_error(
                    error_context + "Key \"name\" and \"value\" not found!");
              }
            } else {
              throw std::runtime_error(error_context + "Invalid format!");
            }
          }
        } else {
          throw std::runtime_error(error_context +
                                   "Key \"signals\" not found!");
        }

        input_sequence.push_back(inputs_per_cycle);
        inputs_per_cycle.clear();

        if (clock_cycle_obj.contains("hold_for_cycles")) {
          if (clock_cycle_obj.at("hold_for_cycles").is_int64()) {
            if (clock_cycle_obj.at("hold_for_cycles").as_int64() > 0) {
              for (int64_t index = 1;
                   index < clock_cycle_obj.at("hold_for_cycles").as_int64();
                   ++index) {
                input_sequence.push_back(inputs_per_cycle);
              }
            } else {
              throw std::runtime_error(
                  error_context + "Invalid argument for \"hold_for_cycles\"!");
            }
          } else {
            throw std::runtime_error(error_context +
                                     "Invalid format in \"hold_for_cycles\"!");
          }
        }
      } else {
        throw std::runtime_error(error_context + "Invalid format!");
      }
    }

    if ((input_sequence.size() != 1) && !is_target_hardware_) {
      throw std::invalid_argument(error_context + "Invalid number of arguments!");
    }
  } else {
    throw std::runtime_error(error_context + "Key \"" +
                             SettingNames::INPUT_SEQUENCE + "\" not found!");
  }
}

void Settings::ParseSimulationSettings(const boost::json::object& json_object,
                                       const std::string& identifier,
                                       SimulationSettings& settings) {
  std::string error_context = "Error while parsing \"" + identifier + "\": ";
  boost::json::object simulation_object;

  settings.waveform_simulation = false;
  settings.number_of_clock_cycles = 0;
  settings.number_of_wait_cycles = 0;
  settings.end_condition_clock_cycles = 0;
  settings.number_of_simulations = 64;
  settings.number_of_simulations_per_step = 64;
  settings.number_of_simulations_per_write = 0;

  if (SetValue(json_object, identifier, simulation_object)) {
    if (!SetValue(simulation_object, SettingNames::NUMBER_OF_CLOCK_CYCLES,
                  settings.number_of_clock_cycles)) {
      throw std::runtime_error(error_context + "Key \"" +
                               SettingNames::NUMBER_OF_CLOCK_CYCLES +
                               "\" not found!");
    } else {
      settings.end_condition_clock_cycles = settings.number_of_clock_cycles;
    }

    SetValue(simulation_object, SettingNames::WAVEFORM_SIMULATION,
             settings.waveform_simulation);
    SetValue(simulation_object, SettingNames::END_WAIT_CYCLES,
             settings.number_of_wait_cycles);
    ParseAlwaysRandomInputs(simulation_object,
                            settings.always_random_input_signals);
    ParseEndCondition(simulation_object, settings.end_condition_clock_cycles,
                      settings.end_condition_signals);
    ParseGroups(simulation_object, settings.groups);
    ParseOutputShares(simulation_object, settings.output_shares);
    ParseArrayOfTriStateBitVectors(
        simulation_object, SettingNames::EXPECTED_OUTPUT,
        settings.groups.size(), settings.output_shares.size(), true,
        settings.expected_outputs);
    ParseInputSequence(simulation_object, settings.input_sequence);
    ParseSignalNameValuePair(simulation_object,
                             SettingNames::FAULT_DETECTION_FLAGS,
                             settings.fault_detection_flags);

    if (SetValue(simulation_object,
                 SettingNames::NUMBER_OF_SIMULATIONS_PER_STEP,
                 settings.number_of_simulations_per_step)) {
      if ((settings.number_of_simulations_per_step % 64 != 0) ||
          (settings.number_of_simulations_per_step == 0)) {
        throw std::invalid_argument(
            error_context + "Invalid argument for \"" +
            SettingNames::NUMBER_OF_SIMULATIONS_PER_STEP + "\"!");
      }
    }

    if (SetValue(simulation_object,
                 SettingNames::NUMBER_OF_SIMULATIONS_PER_WRITE,
                 settings.number_of_simulations_per_write)) {
      if (settings.number_of_simulations_per_write %
          settings.number_of_simulations_per_step) {
        throw std::invalid_argument(
            error_context + "Invalid argument for \"" +
            SettingNames::NUMBER_OF_SIMULATIONS_PER_WRITE + "\"!");
      }
    }

    if (SetValue(simulation_object, SettingNames::NUMBER_OF_SIMULATIONS,
                 settings.number_of_simulations)) {
      if (settings.number_of_simulations %
          settings.number_of_simulations_per_step) {
        throw std::invalid_argument(error_context + "Invalid argument for \"" +
                                    SettingNames::NUMBER_OF_SIMULATIONS +
                                    "\"!");
      }
    }
  } else {
    throw std::runtime_error(error_context + "Key \"" + identifier +
                             "\" not found!");
  }
}

void Settings::ParseClockCycles(const boost::json::object& json_object,
                                const std::string& identifier,
                                std::vector<uint64_t>& clock_cycles) {
  std::string error_context = "Error while parsing \"" + identifier + "\": ";
  boost::json::array json_array;

  if (SetValue(json_object, SettingNames::CLOCK_CYCLES, json_array)) {
    std::string range_string;
    std::pair<uint64_t, uint64_t> borders;
    IntegerRangeGrammar grammar(GetNumberOfClockCycles());

    for (auto& range : json_array) {
      if (range.is_string()) {
        range_string = range.as_string().c_str();
        borders = grammar.Parse(range_string);

        for (uint64_t index = borders.first; index <= borders.second; ++index) {
          clock_cycles.push_back(index);
        }

      } else {
        throw std::invalid_argument(error_context + "Invalid format!");
      }
    }

    std::sort(clock_cycles.begin(), clock_cycles.end());
    clock_cycles.erase(std::unique(clock_cycles.begin(), clock_cycles.end()),
                       clock_cycles.end());
  } else {
    for (uint64_t index = 1; index <= GetNumberOfClockCycles();
         ++index) {
      clock_cycles.push_back(index);
    }
  }
}

void Settings::ParseIncludeSettings(const boost::json::object& json_object,
                                    const std::string& identifier,
                                    bool default_include,
                                    IncludeSettings& settings) {
  std::string error_context = "Error while parsing \"" + identifier + "\": ";
  std::string included_elements = "(?!)", excluded_elements = "(?!)";
  std::string included_paths = "(?!)", excluded_paths = "(?!)";
  bool include_found = false, exclude_found = false;
  boost::json::object include, regexes;

  if (SetValue(json_object, identifier, include)) {
    std::string first_key;

    if (include.size() == 2) {
      first_key = include.begin()->key_c_str();
      if (first_key == "include") {
        settings.first_include = true;
      } else if (first_key == "exclude") {
        settings.first_include = false;
      } else {
        throw std::runtime_error(error_context + "Invalid key!");
      }
    } else {
      throw std::runtime_error(error_context + "Invalid size of elements!");
    }

    for (auto& [key, value] : include) {
      regexes = value.as_object();
      if (key == "include") {
        SetValue(regexes, "signals", included_elements);
        settings.included_elements = std::regex(included_elements);
        SetValue(regexes, "paths", included_paths);
        settings.included_paths = std::regex(included_paths);
        include_found = true;
      } else if (key == "exclude") {
        SetValue(regexes, "signals", excluded_elements);
        settings.excluded_elements = std::regex(excluded_elements);
        SetValue(regexes, "paths", excluded_paths);
        settings.excluded_paths = std::regex(excluded_paths);
        exclude_found = true;
      } else {
        throw std::runtime_error(error_context + "Invalid key!");
      }
    }

    if (!include_found) {
      throw std::runtime_error(error_context + "Missing key \"include\"!");
    } else if (!exclude_found) {
      throw std::runtime_error(error_context + "Missing key \"exclude\"!");
    }
  } else {
    if (default_include) {
      included_elements = "(.*)";
      settings.first_include = true;
    } else {
      excluded_elements = "(.*)";
      settings.first_include = false;
    }

    settings.included_elements = std::regex(included_elements);
    settings.excluded_elements = std::regex(excluded_elements);
    settings.included_paths = std::regex(included_paths);
    settings.excluded_paths = std::regex(excluded_paths);
  }
}

void Settings::ParseSideChannelAnalysisSettings(
    const boost::json::object& json_object, const std::string& identifier,
    SideChannelAnalysisSettings& settings) {
  std::string error_context = "Error while parsing \"" + identifier + "\": ";
  boost::json::object sca;
  settings.order = 1;
  settings.relaxed_model = false;
  settings.transitional_leakage = true;
  settings.effect_size = 0.1;
  settings.alpha_threshold = 5.0;
  settings.beta_threshold = 0.00001;
  settings.variate = Analysis::univariate;
  settings.distance = 10;

  if (SetValue(json_object, identifier, sca)) {
    SetValue(sca, SettingNames::ORDER_OF_TEST, settings.order);
    SetValue(sca, SettingNames::RELAXED_MODEL, settings.relaxed_model);

    SetValue(sca, SettingNames::TRANSITIONAL_LEAKAGE,
             settings.transitional_leakage);
    if (!settings.transitional_leakage && settings.relaxed_model) {
      throw std::invalid_argument(error_context + "Invalid combination of \"" +
                                  SettingNames::TRANSITIONAL_LEAKAGE +
                                  "\" and \"" + SettingNames::RELAXED_MODEL +
                                  "\"!");
    }

    SetValue(sca, SettingNames::EFFECT_SIZE, settings.effect_size);
    if ((settings.effect_size < 0) || (settings.effect_size > 1)) {
      throw std::invalid_argument(error_context + "Invalid argument for \"" +
                                  SettingNames::EFFECT_SIZE + "\"!");
    }

    SetValue(sca, SettingNames::ALPHA_THRESHOLD, settings.alpha_threshold);
    if ((settings.alpha_threshold < 0)) {
      throw std::invalid_argument(error_context + "Invalid argument for \"" +
                                  SettingNames::ALPHA_THRESHOLD + "\"!");
    }

    SetValue(sca, SettingNames::BETA_THRESHOLD, settings.beta_threshold);
    if ((settings.beta_threshold < 0)) {
      throw std::invalid_argument(error_context + "Invalid argument for \"" +
                                  SettingNames::BETA_THRESHOLD + "\"!");
    }

    std::string variate_string;
    if (SetValue(sca, SettingNames::VARIATE, variate_string)) {
      if (variate_string == "univariate") {
        settings.variate = Analysis::univariate;
      } else if (variate_string == "multivariate") {
        settings.variate = Analysis::multivariate;
      } else if (variate_string == "exclusive_multivariate") {
        settings.variate = Analysis::exclusive_multivariate;
      } else {
        throw std::invalid_argument(error_context + "Invalid argument for \"" +
                                    SettingNames::VARIATE + "\"!");
      }
    }

    ParseClockCycles(sca, SettingNames::CLOCK_CYCLES, settings.clock_cycles);
    SetValue(sca, SettingNames::DISTANCE, settings.distance);
    ParseIncludeSettings(sca, SettingNames::PROBE_PLACEMENT, true,
                         settings.locations);
    ParseIncludeSettings(sca, SettingNames::EXTENSION_ROUTES, true,
                         settings.extension);
    ParseIncludeSettings(sca, SettingNames::OBSERVED_EXTENSIONS, true,
                         settings.analysis);
  } else {
    throw std::runtime_error(error_context + "Key \"" + identifier +
                             "\" not found!");
  }
}

void Settings::ParseFaultInjectionSettings(
    const boost::json::object& json_object, const std::string& identifier,
    FaultInjectionSettings& settings) {
  std::string error_context = "Error while parsing \"" + identifier + "\": ";
  boost::json::object fi;
  settings.type = FaultType::none;
  settings.analysis = FaultAnalysis::both;
  settings.maximum_per_run = 0;
  settings.minimum_per_run = 0;
  settings.maximum_per_cycle = 0;
  settings.minimum_per_cycle = 0;

  if (SetValue(json_object, identifier, fi)) {
    std::string parsed_value;

    if (SetValue(fi, SettingNames::FAULT_TYPE, parsed_value)) {
      if (parsed_value == "StuckAt0") {
        settings.type = FaultType::stuck_at_0;
      } else if (parsed_value == "StuckAt1") {
        settings.type = FaultType::stuck_at_1;
      } else if (parsed_value == "Toggle") {
        settings.type = FaultType::toggle;
      } else {
        throw std::invalid_argument(error_context + "Invalid argument for \"" +
                                    SettingNames::FAULT_TYPE + "\"!");
      }
    } else {
      throw std::runtime_error(error_context + "Key \"" +
                               SettingNames::FAULT_TYPE + "\" not found!");
    }

    if (SetValue(fi, SettingNames::FAULT_ANALYSIS, parsed_value)) {
      if (parsed_value == "All") {
        settings.analysis = FaultAnalysis::both;
      } else if (parsed_value == "OnlyFaulty") {
        settings.analysis = FaultAnalysis::only_faulty_simulations;
      } else if (parsed_value == "OnlyFaultFree") {
        settings.analysis = FaultAnalysis::only_fault_free_simulations;
      } else {
        throw std::invalid_argument(error_context + "Invalid argument for \"" +
                                    SettingNames::FAULT_ANALYSIS + "\"!");
      }
    } 
    
    SetValue(fi, SettingNames::MAXIMUM_NUMBER_OF_FAULTS_PER_RUN,
             settings.maximum_per_run);
    SetValue(fi, SettingNames::MINIMUM_NUMBER_OF_FAULTS_PER_RUN,
             settings.minimum_per_run);
    SetValue(fi, SettingNames::MAXIMUM_NUMBER_OF_FAULTS_PER_CYCLE,
             settings.maximum_per_cycle);
    SetValue(fi, SettingNames::MINIMUM_NUMBER_OF_FAULTS_PER_CYCLE,
             settings.minimum_per_cycle);
    ParseClockCycles(fi, SettingNames::FAULTED_CLOCK_CYCLES,
                     settings.clock_cycles);
    ParseIncludeSettings(fi, SettingNames::FAULT_LOCATIONS, false,
                         settings.locations);
  }
}

bool Settings::IsCompactDistribution() const {
  return performance.compact_distributions;
}

void Settings::SetCompactDistribution(bool compact) {
  performance.compact_distributions = compact;
}

bool Settings::IsRemoveFullProbingSets() const {
  return performance.remove_full_probing_sets && IsCompactDistribution();
}

uint64_t Settings::GetNumberOfThreads() const {
  return performance.number_of_threads;
}

uint64_t Settings::GetNumberOfEntriesInReport() const {
  return performance.number_of_entries_in_report;
}

uint64_t Settings::GetNumberOfProbingSetsPerStep() const {
  return performance.number_of_probing_sets_per_step;
}

Minimization Settings::GetMinimization() const { return performance.minimize; }

void Settings::SetMinimization(const Minimization& minimization) { 
  performance.minimize = minimization;
}

std::string Settings::GetClockSignalName() const {
  return hardware.clock_signal_name;
}

uint64_t Settings::GetNumberOfPipelineStages() const {
  return software.number_of_pipeline_stages;
}

std::string Settings::GetCompilerFlags() const {
  return software.compiler_flags;
}

std::string Settings::GetLocationOfCipher() const {
  return software.location_of_cipher;
}

bool Settings::IsWaveformSimulation() const {
  return simulation.waveform_simulation;
}

uint64_t Settings::GetNumberOfClockCycles() const {
  return simulation.number_of_clock_cycles;
}

uint64_t Settings::GetNumberOfWaitCycles() const {
  return simulation.number_of_wait_cycles;
}

uint64_t Settings::GetNumberOfSimulations() const {
  return simulation.number_of_simulations;
}

uint64_t Settings::GetNumberOfSimulationsPerStep() const {
  return simulation.number_of_simulations_per_step;
}

uint64_t Settings::GetNumberOfSimulationsPerWrite() const {
  return simulation.number_of_simulations_per_write;
}

uint64_t Settings::GetEndConditionClockCycles() const {
  return simulation.end_condition_clock_cycles;
}

uint64_t Settings::GetNumberOfGroups() const {
  return simulation.groups.size();
}

uint64_t Settings::GetNumberOfBitsPerGroup() const {
  return simulation.groups[0].size();
}

TriStateBit Settings::GetGroupBit(uint64_t group_index,
                                  uint64_t bit_index) const {
  return simulation.groups[group_index][bit_index];
}

uint64_t Settings::GetNumberOfOutputShares() const {
  return simulation.output_shares.size();
}

uint64_t Settings::GetNumberOfBitsPerOutputShare() const {
  return simulation.output_shares[0].size();
}

std::string Settings::GetOutputShareName(uint64_t share_index, uint64_t bit_index) const {
  return simulation.output_shares[share_index][bit_index];
}

uint64_t Settings::GetNumberOfExpectedOutputs() const {
  return simulation.expected_outputs.size();
}

TriStateBit Settings::GetExpectedOutputBit(uint64_t group_index, uint64_t bit_index) const {
  return simulation.expected_outputs[group_index][bit_index];
}

uint64_t Settings::GetNumberOfEndConditionSignalValuePairs() const {
  return simulation.end_condition_signals.size();
}

const std::vector<std::pair<std::string, bool>>& Settings::GetEndConditionSignalValuePairs() const {
  return simulation.end_condition_signals;
}

std::pair<std::string, bool> Settings::GetEndConditionSignalValuePair(uint64_t index) const {
  return simulation.end_condition_signals[index];
}

uint64_t Settings::GetNumberOfFaultDetectionSignalValuePairs() const {
  return simulation.fault_detection_flags.size();
}

const std::vector<std::pair<std::string, bool>>& Settings::GetFaultDetectionSignalValuePairs() const {
  return simulation.fault_detection_flags;
}

std::pair<std::string, bool> Settings::GetFaultDetectionSignalValuePair(uint64_t index) const {
  return simulation.fault_detection_flags[index];
}

uint64_t Settings::GetNumberOfAlwaysRandomInputSignals() const {
  return simulation.always_random_input_signals.size();
}

const std::vector<std::string>& Settings::GetAlwaysRandomInputElement(uint64_t index) const {
  return simulation.always_random_input_signals[index];
}

const std::vector<InputAssignment>& Settings::GetInputSequenceOfOneCycle(uint64_t index) const {
  return simulation.input_sequence[index];
}

uint64_t Settings::GetCyclesForInputSequence() const {
  return simulation.input_sequence.size();
}

bool Settings::IsRelaxedModel() const {
  return side_channel_analysis.relaxed_model;
}

void Settings::SetRelaxedModel(bool model) {
  side_channel_analysis.relaxed_model = model;
}

bool Settings::IsTransitionalLeakage() const {
  return side_channel_analysis.transitional_leakage;
}

uint64_t Settings::GetTestOrder() const { return side_channel_analysis.order; }

void Settings::SetTestOrder(uint64_t order) {
  side_channel_analysis.order = order;
}

Analysis Settings::GetVariate() const {
  return side_channel_analysis.variate;
}

uint64_t Settings::GetDistance() const {
  return side_channel_analysis.distance;
}














uint64_t Settings::GetNumberOfTestClockCycles() const {
  return side_channel_analysis.clock_cycles.size();
}

uint64_t Settings::GetTestClockCycle(uint64_t index) const {
  return side_channel_analysis.clock_cycles[index];
}

uint64_t Settings::GetNumberOfSignalValuePairs(uint64_t index) const {
  return simulation.input_sequence[index].size();
}

uint64_t Settings::GetSignalLengthPerAssignment(uint64_t clock_index, uint64_t assignment_index) const {
  return simulation.input_sequence[clock_index][assignment_index].signal_names_.size();
}

uint64_t Settings::GetGroupIndexOfAssignment(uint64_t clock_index, uint64_t assignment_index, uint64_t group_index) const {
  return simulation.input_sequence[clock_index][assignment_index].group_indices_[group_index];
}

uint64_t Settings::GetShareIndexOfAssignment(uint64_t clock_index, uint64_t assignment_index) const {
  return simulation.input_sequence[clock_index][assignment_index].share_index_;
}

ValueType Settings::GetAssignmentType(uint64_t clock_index, uint64_t assignment_index) const {
  return simulation.input_sequence[clock_index][assignment_index].type_;
}

std::string Settings::GetSignalVectorName(uint64_t clock_index, uint64_t assignment_index) const {
  std::string result = simulation.input_sequence[clock_index][assignment_index].signal_names_[0];
  uint64_t index = result.find('[');
  if (index != std::string::npos) {
    result = result.substr(0, index);
  }
  return result;
}

bool Settings::IsAssignedToConstant(uint64_t clock_index, uint64_t assignment_index) const {
  return !simulation.input_sequence[clock_index][assignment_index].signal_values_.empty();
}

TriStateBit Settings::GetAssignedConstantBit(uint64_t clock_index, uint64_t assignment_index, uint64_t bit_index) const {
  return simulation.input_sequence[clock_index][assignment_index].signal_values_[bit_index];
}

bool Settings::EndConditionIsBasedOnClockCycles() const {
  return simulation.end_condition_signals.empty();
}


std::string Settings::GetEndConditionVectorName() const {
  std::string result = simulation.end_condition_signals[0].first;
  uint64_t index = result.find('[');
  if (index != std::string::npos) {
    result = result.substr(0, index);
  }
  return result;
}






bool Settings::IsMultivariateEvaluationRequired() const {
  return (GetVariate() != Analysis::univariate) &&
         (side_channel_analysis.clock_cycles.size() > 1) &&
         (GetTestOrder() != 1);
}

bool Settings::IsDistanceSmallEnough(uint64_t distance) const {
  bool is_distance_small_enough = distance <= GetDistance();
  bool is_no_unallowed_univariate_set =
      (GetVariate() != Analysis::exclusive_multivariate) ||
      distance != 0;
  return is_distance_small_enough && is_no_unallowed_univariate_set;
}

Settings::Settings(const std::string& config_file_path, bool is_target_hardware)
    : is_target_hardware_(is_target_hardware) {
  boost::json::object json_object = ParseJsonFile(config_file_path);
  std::cout << "Successfully parsed the settings file at \"" << config_file_path
            << "\"." << std::endl;
  ParseFiniteField(json_object, SettingNames::INPUT_FINITE_FIELD,
                   input_finite_field);
  ParseFiniteField(json_object, SettingNames::OUTPUT_FINITE_FIELD,
                   output_finite_field);
  ParsePerformanceSettings(json_object, SettingNames::PERFORMANCE, performance);
  ParseHardwareSettings(json_object, SettingNames::HARDWARE, hardware);
  ParseSoftwareSettings(json_object, SettingNames::SOFTWARE, software);
  ParseSimulationSettings(json_object, SettingNames::SIMULATION, simulation);
  ParseSideChannelAnalysisSettings(
      json_object, SettingNames::SIDE_CHANNEL_ANALYSIS, side_channel_analysis);
  ParseFaultInjectionSettings(json_object, SettingNames::FAULT_INJECTION,
                              fault_injection);
}

const std::string SettingNames::BASE = "base";
const std::string SettingNames::EXPONENT = "exponent";
const std::string SettingNames::IS_ADDITIVE = "is_additive";
const std::string SettingNames::EXCLUDED_ELEMENTS = "excluded_elements";
const std::string SettingNames::IRREDUCIBLE_POLYNOMIAL =
    "irreducible_polynomial";
const std::string SettingNames::INPUT_FINITE_FIELD = "input_finite_field";
const std::string SettingNames::OUTPUT_FINITE_FIELD = "output_finite_field";

const std::string SettingNames::NUMBER_OF_THREADS = "max_number_of_threads";
const std::string SettingNames::MINIMIZE_PROBING_SETS = "minimize_probing_sets";
const std::string SettingNames::COMPACT_DISTRIBUTIONS = "compact_distributions";
const std::string SettingNames::REMOVE_FULL_PROBING_SETS =
    "remove_full_probing_sets";
const std::string SettingNames::NUMBER_OF_ENTRIES_IN_REPORT =
    "number_of_entries_in_report";
const std::string SettingNames::NUMBER_OF_PROBING_SETS_PER_STEP =
    "number_of_probing_sets_per_step";
const std::string SettingNames::PERFORMANCE = "performance";

const std::string SettingNames::CLOCK_SIGNAL_NAME = "clock_signal_name";
const std::string SettingNames::HARDWARE = "hardware";

const std::string SettingNames::COMPILER_FLAGS = "compiler_flags";
const std::string SettingNames::LOCATION_OF_CIPHER = "location_of_cipher";
const std::string SettingNames::NUMBER_OF_PIPELINE_STAGES =
    "number_of_pipeline_stages";
const std::string SettingNames::SOFTWARE = "software";

const std::string SettingNames::WAVEFORM_SIMULATION = "waveform_simulation";
const std::string SettingNames::NUMBER_OF_CLOCK_CYCLES =
    "number_of_clock_cycles";
const std::string SettingNames::END_WAIT_CYCLES = "end_wait_cycles";
const std::string SettingNames::END_CONDITION = "end_condition";
const std::string SettingNames::GROUPS = "groups";
const std::string SettingNames::ALWAYS_RANDOM_INPUTS = "always_random_inputs";
const std::string SettingNames::OUTPUT_SHARES = "output_shares";
const std::string SettingNames::EXPECTED_OUTPUT = "expected_output";
const std::string SettingNames::INPUT_SEQUENCE = "input_sequence";
const std::string SettingNames::FAULT_DETECTION_FLAGS = "fault_detection_flags";
const std::string SettingNames::NUMBER_OF_SIMULATIONS = "number_of_simulations";
const std::string SettingNames::NUMBER_OF_SIMULATIONS_PER_STEP =
    "number_of_simulations_per_step";
const std::string SettingNames::NUMBER_OF_SIMULATIONS_PER_WRITE =
    "number_of_simulations_per_write";
const std::string SettingNames::SIMULATION = "simulation";

const std::string SettingNames::EFFECT_SIZE = "effect_size";
const std::string SettingNames::ALPHA_THRESHOLD = "alpha_threshold";
const std::string SettingNames::BETA_THRESHOLD = "beta_threshold";
const std::string SettingNames::ORDER_OF_TEST = "order";
const std::string SettingNames::RELAXED_MODEL = "relaxed_model";
const std::string SettingNames::TRANSITIONAL_LEAKAGE = "transitional_leakage";
const std::string SettingNames::VARIATE = "variate";
const std::string SettingNames::DISTANCE = "distance";
const std::string SettingNames::CLOCK_CYCLES = "clock_cycles";
const std::string SettingNames::PROBE_PLACEMENT = "probe_placement";
const std::string SettingNames::EXTENSION_ROUTES = "extension_routes";
const std::string SettingNames::OBSERVED_EXTENSIONS = "observed_extensions";
const std::string SettingNames::SIDE_CHANNEL_ANALYSIS = "side_channel_analysis";

const std::string SettingNames::FAULTED_CLOCK_CYCLES = "faulted_clock_cycles";
const std::string SettingNames::FAULT_TYPE = "type";
const std::string SettingNames::FAULT_LOCATIONS = "fault_locations";
const std::string SettingNames::MAXIMUM_NUMBER_OF_FAULTS_PER_RUN =
    "maximum_number_of_faults_per_run";
const std::string SettingNames::MINIMUM_NUMBER_OF_FAULTS_PER_RUN =
    "minimum_number_of_faults_per_run";
const std::string SettingNames::MAXIMUM_NUMBER_OF_FAULTS_PER_CYCLE =
    "maximum_number_of_faults_per_cycle";
const std::string SettingNames::MINIMUM_NUMBER_OF_FAULTS_PER_CYCLE =
    "minimum_number_of_faults_per_cycle";
const std::string SettingNames::FAULT_INJECTION = "fault_injection";
const std::string SettingNames::FAULT_ANALYSIS = "simulations_to_analyze";