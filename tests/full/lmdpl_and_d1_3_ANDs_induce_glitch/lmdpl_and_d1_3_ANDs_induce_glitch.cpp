#include <catch2/catch.hpp>

#include "Hardware/Adversaries.hpp"
#include "Hardware/Prepare.hpp"

class Test {
 public:
  Test(double maximum_leakage, uint64_t number_of_propagations,
       uint64_t number_of_standard_probes, uint64_t number_of_probe_extensions,
       uint64_t number_of_probing_sets)
      : maximum_leakage_(maximum_leakage),
        number_of_propagations_(number_of_propagations),
        number_of_standard_probes_(number_of_standard_probes),
        number_of_probe_extensions_(number_of_probe_extensions),
        number_of_probing_sets_(number_of_probing_sets) {}

  bool Require(bool leakage_expected, uint64_t number_of_propagations,
               uint64_t number_of_standard_probes,
               uint64_t number_of_probe_extensions,
               uint64_t number_of_probing_sets) {
    if (leakage_expected && maximum_leakage_ < 5.0) {
      return false;
    } else if (!leakage_expected && maximum_leakage_ >= 5.0) {
      return false;
    } else if (number_of_propagations != number_of_propagations_) {
      return false;
    } else if (number_of_standard_probes != number_of_standard_probes_) {
      return false;
    } else if (number_of_probe_extensions != number_of_probe_extensions_) {
      return false;
    } else if (number_of_probing_sets != number_of_probing_sets_) {
      return false;
    } else {
      return true;
    }
  }

 private:
  double maximum_leakage_;
  uint64_t number_of_propagations_;
  uint64_t number_of_standard_probes_;
  uint64_t number_of_probe_extensions_;
  uint64_t number_of_probing_sets_;
};

Test TestLmdplAndd13AndsInduceGlitch(Settings& settings) {
  const std::string library_file_name = "library.json";
  const std::string library_name = "NANG45";
  const std::string design_file_name =
      "tests/full/lmdpl_and_d1_3_ANDs_induce_glitch/"
      "lmdpl_and_d1_3_ANDs_induce_glitch.v";
  const std::string topmodule_name = "lmdpl_and_d1_3_ANDs_induce_glitch";
  const std::string result_folder_name =
      "tests/full/lmdpl_and_d1_3_ANDs_induce_glitch";

  Hardware::CircuitStruct circuit;
  Hardware::Library library(library_file_name, library_name,
                            settings.side_channel_analysis.relaxed_model);
  Hardware::Read::DesignFile(design_file_name, topmodule_name, settings,
                             library, &circuit, 0, 0, 0, NULL);
  circuit.SetIsProbeAllowed(library, settings);
  circuit.SetIsExtensionAllowed(library, settings);
  circuit.SetIsAnalysisAllowed(settings);
  circuit.SetIsFaultAllowed(settings);
  std::cout << "done" << std::endl;

  Hardware::Prepare::MakeCircuitDepth(library, &circuit);

  std::vector<SharedData> shared_data(settings.GetNumberOfThreads(),
                                      SharedData(circuit, settings));
  Simulation simulation(circuit, settings);
  simulation.topmodule_name_ = topmodule_name;
  simulation.result_folder_name_ = result_folder_name;

  if (settings.side_channel_analysis.relaxed_model) {
    Hardware::Adversaries<Hardware::RelaxedProbe> adversary(
        library, circuit, settings, simulation);
    double maximum_leakage =
        adversary.EvaluateRobustProbingSecurity(shared_data);
    return Test(maximum_leakage, adversary.GetNumberOfPropagations(),
                adversary.GetNumberOfStandardProbes(),
                adversary.GetNumberOfExtendedProbes(),
                adversary.GetNumberOfProbingSets());
  } else {
    Hardware::Adversaries<Hardware::RobustProbe> adversary(
        library, circuit, settings, simulation);
    double maximum_leakage =
        adversary.EvaluateRobustProbingSecurity(shared_data);
    return Test(maximum_leakage, adversary.GetNumberOfPropagations(),
                adversary.GetNumberOfStandardProbes(),
                adversary.GetNumberOfExtendedProbes(),
                adversary.GetNumberOfProbingSets());
  }
}

TEST_CASE("Test full verification (lmdpl_and_d1_3_ANDs_induce_glitch)",
          "[lmdpl_and_d1_3_ANDs_induce_glitch]") {
  const std::string config_file_name =
      "tests/full/lmdpl_and_d1_3_ANDs_induce_glitch/"
      "lmdpl_and_d1_3_ANDs_induce_glitch.json";

  Settings settings(config_file_name, true);

  SECTION("Robust but Relaxed Probing Model with no minimization") {
    settings.SetMinimization(Minimization::none);
    Test test = TestLmdplAndd13AndsInduceGlitch(settings);

    // We expect leakage in the first order as the fault induces a glitch.
    REQUIRE(test.Require(true, 354, 234, 414, 234));
  }

  SECTION("Robust but Relaxed Probing Model with trivial minimization") {
    settings.SetMinimization(Minimization::trivial);
    Test test = TestLmdplAndd13AndsInduceGlitch(settings);

    // We expect leakage in the first order as the fault induces a glitch.
    REQUIRE(test.Require(true, 354, 78, 414, 78));
  }

  SECTION("Robust but Relaxed Probing Model with aggressive minimization") {
    settings.SetMinimization(Minimization::aggressive);
    Test test = TestLmdplAndd13AndsInduceGlitch(settings);

    // We expect leakage in the first order as the fault induces a glitch.
    REQUIRE(test.Require(true, 354, 78, 414, 78));
  }
}