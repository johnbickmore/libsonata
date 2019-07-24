#include "report.hpp"

class SpikeReport: public Report {

  public:
    SpikeReport(const std::string& reportName, double tstart, double tend, double dt);

    size_t get_total_compartments() override;
    int add_variable(int cell_number, double* pointer) override;

    // We dont need this on spike reports
    int recData(double timestep, int ncells, int* cellids) {}
    int end_iteration(double timestep) {}
    int flush(double time) {}
};