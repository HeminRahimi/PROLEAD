#define CATCH_CONFIG_ENABLE_BENCHMARKING
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch.hpp>

#include "Hardware/Execute.hpp"

TEST_CASE(
    "Test the full verification of a first-order secure DOM-indep multiplier with faulted randomness",
    "[dom_indep_d1_faulty_rand]") {
  CommandLineParameterStruct command_line_parameters;
  command_line_parameters.LibraryFileName = "library.lib";
  command_line_parameters.LibraryName = "NANG45";
  command_line_parameters.DesignFileName =
      "ut/full/dom_indep_d1_faulty_rand/dom_indep_d1_faulty_rand.v";
  command_line_parameters.MainModuleName = "dom_indep_d1_faulty_rand";
  command_line_parameters.SettingsFileName =
      "ut/full/dom_indep_d1_faulty_rand/dom_indep_d1_faulty_rand.set";

  Hardware::CircuitStruct circuit;
  Hardware::LibraryStruct library;
  Hardware::SettingsStruct settings;
  Hardware::ProbesStruct probes;
  Hardware::SimulationStruct simulation;
  Hardware::SharedDataStruct* shared_data = NULL;
  Hardware::Prepare::All(command_line_parameters, circuit, library, settings,
                         probes, simulation, shared_data);
  settings.ModuleName =
      new char[command_line_parameters.MainModuleName.length() + 1];
  strcpy(settings.ModuleName, command_line_parameters.MainModuleName.c_str());
  Hardware::Adversaries<Hardware::probing::GlitchExtendedProbe>
      glitch_extended_adversary(library, circuit, settings);
  double maximum_leakage =
      glitch_extended_adversary.EvaluateRobustProbingSecurity(
          library, circuit, settings, shared_data, simulation);

  REQUIRE(glitch_extended_adversary.GetNumberOfProbeExtensions() == 4);
  REQUIRE(glitch_extended_adversary.GetNumberOfStandardProbes() == 4);
  REQUIRE(glitch_extended_adversary.GetNumberOfExtendedProbes() == 14);
  REQUIRE(glitch_extended_adversary.GetNumberOfUniqueProbes() == 0);
  REQUIRE(glitch_extended_adversary.GetNumberOfFaultTargets() == 1);
  REQUIRE(maximum_leakage > 5.0);
}